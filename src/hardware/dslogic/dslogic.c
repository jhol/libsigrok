/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2013 Bert Vermeulen <bert@biot.com>
 * Copyright (C) 2012 Joel Holdsworth <joel@airwebreathe.org.uk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <math.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "protocol.h"
#include "dslogic.h"

/*
 * This should be larger than the FPGA bitstream image so that it'll get
 * uploaded in one big operation. There seem to be issues when uploading
 * it in chunks.
 */
#define FW_BUFSIZE (1024 * 1024)

#define FPGA_UPLOAD_DELAY (10 * 1000)

#define USB_TIMEOUT (3 * 1000)

SR_PRIV int dslogic_set_vth(const struct sr_dev_inst *sdi, double vth)
{
	struct sr_usb_dev_inst *usb;
	int ret;
	const uint8_t value = (vth / 5.0) * 255;
	const uint16_t cmd = value | (DS_ADDR_VTH << 8);

	usb = sdi->conn;

	/* Send the control command. */
	ret = libusb_control_transfer(usb->devhdl,
			LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT,
			DS_CMD_WR_REG, 0x0000, 0x0000,
			(unsigned char *)&cmd, sizeof(cmd), 3000);
	if (ret < 0) {
		sr_err("Unable to send VTH command: %s.",
		libusb_error_name(ret));
		return SR_ERR;
	}

	return SR_OK;
}

SR_PRIV int dslogic_fpga_firmware_upload(const struct sr_dev_inst *sdi)
{
	const char *name = NULL;
	uint64_t sum;
	struct sr_resource bitstream;
	struct drv_context *drvc;
	struct dev_context *devc;
	struct sr_usb_dev_inst *usb;
	unsigned char *buf;
	ssize_t chunksize;
	int transferred;
	int result, ret;
	const uint8_t cmd[3] = {0, 0, 0};

	drvc = sdi->driver->context;
	devc = sdi->priv;
	usb = sdi->conn;

	if (!strcmp(devc->profile->model, "DSLogic")) {
		if (devc->voltage_threshold == DS_VOLTAGE_RANGE_18_33_V)
			name = DSLOGIC_FPGA_FIRMWARE_3V3;
		else
			name = DSLOGIC_FPGA_FIRMWARE_5V;
	} else if (!strcmp(devc->profile->model, "DSLogic Pro")){
		name = DSLOGIC_PRO_FPGA_FIRMWARE;
	} else if (!strcmp(devc->profile->model, "DSLogic Plus")){
		name = DSLOGIC_PLUS_FPGA_FIRMWARE;
	} else if (!strcmp(devc->profile->model, "DSLogic Basic")){
		name = DSLOGIC_BASIC_FPGA_FIRMWARE;
	} else if (!strcmp(devc->profile->model, "DSCope")) {
		name = DSCOPE_FPGA_FIRMWARE;
	} else {
		sr_err("Failed to select FPGA firmware.");
		return SR_ERR;
	}

	sr_dbg("Uploading FPGA firmware '%s'.", name);

	result = sr_resource_open(drvc->sr_ctx, &bitstream,
			SR_RESOURCE_FIRMWARE, name);
	if (result != SR_OK)
		return result;

	/* Tell the device firmware is coming. */
	if ((ret = libusb_control_transfer(usb->devhdl, LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_ENDPOINT_OUT, DS_CMD_CONFIG, 0x0000, 0x0000,
			(unsigned char *)&cmd, sizeof(cmd), USB_TIMEOUT)) < 0) {
		sr_err("Failed to upload FPGA firmware: %s.", libusb_error_name(ret));
		sr_resource_close(drvc->sr_ctx, &bitstream);
		return SR_ERR;
	}

	/* Give the FX2 time to get ready for FPGA firmware upload. */
	g_usleep(FPGA_UPLOAD_DELAY);

	buf = g_malloc(FW_BUFSIZE);
	sum = 0;
	result = SR_OK;
	while (1) {
		chunksize = sr_resource_read(drvc->sr_ctx, &bitstream,
				buf, FW_BUFSIZE);
		if (chunksize < 0)
			result = SR_ERR;
		if (chunksize <= 0)
			break;

		if ((ret = libusb_bulk_transfer(usb->devhdl, 2 | LIBUSB_ENDPOINT_OUT,
				buf, chunksize, &transferred, USB_TIMEOUT)) < 0) {
			sr_err("Unable to configure FPGA firmware: %s.",
					libusb_error_name(ret));
			result = SR_ERR;
			break;
		}
		sum += transferred;
		sr_spew("Uploaded %" PRIu64 "/%" PRIu64 " bytes.",
			sum, bitstream.size);

		if (transferred != chunksize) {
			sr_err("Short transfer while uploading FPGA firmware.");
			result = SR_ERR;
			break;
		}
	}
	g_free(buf);
	sr_resource_close(drvc->sr_ctx, &bitstream);

	if (result == SR_OK)
		sr_dbg("FPGA firmware upload done.");

	return result;
}

SR_PRIV int dslogic_start_acquisition(const struct sr_dev_inst *sdi)
{
	struct sr_usb_dev_inst *usb;
	struct dslogic_mode mode;
	int ret;

	mode.flags = DS_START_FLAGS_MODE_LA | DS_START_FLAGS_SAMPLE_WIDE;
	mode.sample_delay_h = mode.sample_delay_l = 0;

	usb = sdi->conn;
	ret = libusb_control_transfer(usb->devhdl, LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_ENDPOINT_OUT, DS_CMD_START, 0x0000, 0x0000,
			(unsigned char *)&mode, sizeof(mode), USB_TIMEOUT);
	if (ret < 0) {
		sr_err("Failed to send start command: %s.", libusb_error_name(ret));
		return SR_ERR;
	}

	return SR_OK;
}

SR_PRIV int dslogic_stop_acquisition(const struct sr_dev_inst *sdi)
{
	struct sr_usb_dev_inst *usb;
	struct dslogic_mode mode;
	int ret;

	mode.flags = DS_START_FLAGS_STOP;
	mode.sample_delay_h = mode.sample_delay_l = 0;

	usb = sdi->conn;
	ret = libusb_control_transfer(usb->devhdl, LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_ENDPOINT_OUT, DS_CMD_START, 0x0000, 0x0000,
			(unsigned char *)&mode, sizeof(struct dslogic_mode), USB_TIMEOUT);
	if (ret < 0) {
		sr_err("Failed to send stop command: %s.", libusb_error_name(ret));
		return SR_ERR;
	}

	return SR_OK;
}

/*
 * Get the session trigger and configure the FPGA structure
 * accordingly.
 */
static int dslogic_set_trigger(const struct sr_dev_inst *sdi,
	struct dslogic_fpga_config *cfg)
{
	struct sr_trigger *trigger;
	struct sr_trigger_stage *stage;
	struct sr_trigger_match *match;
	struct dev_context *devc;
	const GSList *l, *m;
	int channelbit, i = 0;
	uint16_t v16;

	devc = sdi->priv;

	cfg->ch_en = 0;
	for (l = sdi->channels; l; l = l->next) {
		const struct sr_channel *const probe = (struct sr_channel *)l->data;
		cfg->ch_en |= probe->enabled << probe->index;
	}

	cfg->trig_mask0[0] = 0xffff;
	cfg->trig_mask1[0] = 0xffff;

	cfg->trig_value0[0] = 0;
	cfg->trig_value1[0] = 0;

	cfg->trig_edge0[0] = 0;
	cfg->trig_edge1[0] = 0;

	cfg->trig_logic0[0] = 0;
	cfg->trig_logic1[0] = 0;

	cfg->trig_count[0] = 0;

	cfg->trig_glb = 0;

	for (i = 1; i < DS_NUM_TRIGGER_STAGES; i++) {
		cfg->trig_mask0[i] = 0xff;
		cfg->trig_mask1[i] = 0xff;
		cfg->trig_value0[i] = 0;
		cfg->trig_value1[i] = 0;
		cfg->trig_edge0[i] = 0;
		cfg->trig_edge1[i] = 0;
		cfg->trig_logic0[i] = 2;
		cfg->trig_logic1[i] = 2;
		cfg->trig_count[i] = 0;
	}

	cfg->trig_pos = (uint32_t)(devc->capture_ratio / 100.0 * devc->limit_samples);
	sr_dbg("pos: %d", cfg->trig_pos);

	sr_dbg("configuring trigger");

	if (!(trigger = sr_session_trigger_get(sdi->session))) {
		sr_dbg("No session trigger found");
		return SR_OK;
	}

	for (l = trigger->stages; l; l = l->next) {
		stage = l->data;
		for (m = stage->matches; m; m = m->next) {
			match = m->data;
			if (!match->channel->enabled)
				/* Ignore disabled channels with a trigger. */
				continue;
			channelbit = 1 << (match->channel->index);
			/* Simple trigger support (event). */
			if (match->match == SR_TRIGGER_ONE) {
				cfg->trig_mask0[0] &= ~channelbit;
				cfg->trig_mask1[0] &= ~channelbit;
				cfg->trig_value0[0] |= channelbit;
				cfg->trig_value1[0] |= channelbit;
			} else if (match->match == SR_TRIGGER_ZERO) {
				cfg->trig_mask0[0] &= ~channelbit;
				cfg->trig_mask1[0] &= ~channelbit;
			} else if (match->match == SR_TRIGGER_FALLING) {
				cfg->trig_mask0[0] &= ~channelbit;
				cfg->trig_mask1[0] &= ~channelbit;
				cfg->trig_edge0[0] |= channelbit;
				cfg->trig_edge1[0] |= channelbit;
			} else if (match->match == SR_TRIGGER_RISING) {
				cfg->trig_mask0[0] &= ~channelbit;
				cfg->trig_mask1[0] &= ~channelbit;
				cfg->trig_value0[0] |= channelbit;
				cfg->trig_value1[0] |= channelbit;
				cfg->trig_edge0[0] |= channelbit;
				cfg->trig_edge1[0] |= channelbit;
			} else if (match->match == SR_TRIGGER_EDGE) {
				cfg->trig_edge0[0] |= channelbit;
				cfg->trig_edge1[0] |= channelbit;
			}
		}
	}

	v16 = RL16(&cfg->mode);
	v16 |= 1 << 0;
	WL16(&cfg->mode, v16);

	return SR_OK;
}

SR_PRIV int dslogic_fpga_configure(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc;
	struct sr_usb_dev_inst *usb;
	uint8_t c[3];
	struct dslogic_fpga_config cfg;
	uint16_t v16;
	uint32_t v32;
	int transferred, len, ret;

	sr_dbg("Configuring FPGA.");

	usb = sdi->conn;
	devc = sdi->priv;

	WL32(&cfg.sync, DS_CFG_START);
	WL16(&cfg.mode_header, DS_CFG_MODE);
	WL16(&cfg.divider_header, DS_CFG_DIVIDER);
	WL16(&cfg.count_header, DS_CFG_COUNT);
	WL16(&cfg.trig_pos_header, DS_CFG_TRIG_POS);
	WL16(&cfg.trig_glb_header, DS_CFG_TRIG_GLB);
	WL16(&cfg.ch_en_header, DS_CFG_CH_EN);
	WL16(&cfg.trig_header, DS_CFG_TRIG);
	WL32(&cfg.end_sync, DS_CFG_END);

	/* Pass in the length of a fixed-size struct. Really. */
	len = sizeof(struct dslogic_fpga_config) / 2;
	c[0] = len & 0xff;
	c[1] = (len >> 8) & 0xff;
	c[2] = (len >> 16) & 0xff;

	ret = libusb_control_transfer(usb->devhdl, LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_ENDPOINT_OUT, DS_CMD_SETTING, 0x0000, 0x0000,
			c, sizeof(c), USB_TIMEOUT);
	if (ret < 0) {
		sr_err("Failed to send FPGA configure command: %s.",
			libusb_error_name(ret));
		return SR_ERR;
	}

	v16 = 0x0000;

	if (devc->mode == DS_OP_INTERNAL_TEST)
		v16 = DS_MODE_INT_TEST;
	else if (devc->mode == DS_OP_EXTERNAL_TEST)
		v16 = DS_MODE_EXT_TEST;
	else if (devc->mode == DS_OP_LOOPBACK_TEST)
		v16 = DS_MODE_LPB_TEST;

	if (devc->cur_samplerate == DS_MAX_LOGIC_SAMPLERATE * 2)
		v16 |= DS_MODE_HALF_MODE;
	else if (devc->cur_samplerate == DS_MAX_LOGIC_SAMPLERATE * 4)
		v16 |= DS_MODE_QUAR_MODE;

	if (devc->continuous_mode)
		v16 |= DS_MODE_STREAM_MODE;
	if (devc->external_clock) {
		v16 |= DS_MODE_CLK_TYPE;
		if (devc->clock_edge == DS_EDGE_FALLING)
			v16 |= DS_MODE_CLK_EDGE;
	}
	if (devc->limit_samples > DS_MAX_LOGIC_DEPTH *
		ceil(devc->cur_samplerate * 1.0 / DS_MAX_LOGIC_SAMPLERATE)
		&& !devc->continuous_mode) {
		/* Enable RLE for long captures.
		 * Without this, captured data present errors.
		 */
		v16 |= DS_MODE_RLE_MODE;
	}

	WL16(&cfg.mode, v16);
	v32 = ceil(DS_MAX_LOGIC_SAMPLERATE * 1.0 / devc->cur_samplerate);
	WL32(&cfg.divider, v32);
	WL32(&cfg.count, devc->limit_samples);

	dslogic_set_trigger(sdi, &cfg);

	len = sizeof(struct dslogic_fpga_config);
	ret = libusb_bulk_transfer(usb->devhdl, 2 | LIBUSB_ENDPOINT_OUT,
			(unsigned char *)&cfg, len, &transferred, USB_TIMEOUT);
	if (ret < 0 || transferred != len) {
		sr_err("Failed to send FPGA configuration: %s.", libusb_error_name(ret));
		return SR_ERR;
	}

	return SR_OK;
}

static int to_bytes_per_ms(struct dev_context *devc)
{
	if (devc->cur_samplerate > SR_MHZ(100))
		return SR_MHZ(100) / 1000 * 2;
	return devc->cur_samplerate / 1000 * 2;
}

static size_t get_buffer_size(struct dev_context *devc)
{
	size_t s;

	/*
	 * The buffer should be large enough to hold 10ms of data and
	 * a multiple of 512.
	 */
	s = 10 * to_bytes_per_ms(devc);
	// s = to_bytes_per_ms(devc->cur_samplerate);
	return (s + 511) & ~511;
}

SR_PRIV int dslogic_get_number_of_transfers(struct dev_context *devc)
{
	unsigned int n;

	/* Total buffer size should be able to hold about 100ms of data. */
	n = (100 * to_bytes_per_ms(devc) / get_buffer_size(devc));
	sr_info("New calculation: %d", n);

	if (n > NUM_SIMUL_TRANSFERS)
		return NUM_SIMUL_TRANSFERS;

	return n;
}
