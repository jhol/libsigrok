/*
 * This file is part of the sigrok project.
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

#ifndef LIBSIGROK_HARDWARE_FX2LAFW_FX2LAFW_H
#define LIBSIGROK_HARDWARE_FX2LAFW_FX2LAFW_H

#include <glib.h>

/* Message logging helpers with driver-specific prefix string. */
#define DRIVER_LOG_DOMAIN "fx2lafw: "
#define sr_log(l, s, args...) sr_log(l, DRIVER_LOG_DOMAIN s, ## args)
#define sr_spew(s, args...) sr_spew(DRIVER_LOG_DOMAIN s, ## args)
#define sr_dbg(s, args...) sr_dbg(DRIVER_LOG_DOMAIN s, ## args)
#define sr_info(s, args...) sr_info(DRIVER_LOG_DOMAIN s, ## args)
#define sr_warn(s, args...) sr_warn(DRIVER_LOG_DOMAIN s, ## args)
#define sr_err(s, args...) sr_err(DRIVER_LOG_DOMAIN s, ## args)

#define USB_INTERFACE		0
#define USB_CONFIGURATION	1
#define NUM_TRIGGER_STAGES	4
#define TRIGGER_TYPE 		"01"

#define MAX_RENUM_DELAY_MS	3000
#define NUM_SIMUL_TRANSFERS	32
#define MAX_EMPTY_TRANSFERS	(NUM_SIMUL_TRANSFERS * 2)

#define FX2LAFW_REQUIRED_VERSION_MAJOR	1

#define MAX_8BIT_SAMPLE_RATE	SR_MHZ(24)
#define MAX_16BIT_SAMPLE_RATE	SR_MHZ(12)

/* 6 delay states of up to 256 clock ticks */
#define MAX_SAMPLE_DELAY	(6 * 256)

/* Software trigger implementation: positive values indicate trigger stage. */
#define TRIGGER_FIRED          -1

#define DEV_CAPS_16BIT_POS		0
#define DEV_CAPS_AX_ANALOG_POS	1

#define DEV_CAPS_16BIT			(1 << DEV_CAPS_16BIT_POS)
#define DEV_CAPS_AX_ANALOG		(1 << DEV_CAPS_AX_ANALOG_POS)

struct fx2lafw_profile {
	uint16_t vid;
	uint16_t pid;

	const char *vendor;
	const char *model;
	const char *model_version;

	const char *firmware;

	uint32_t dev_caps;
};

struct dev_context {
	const struct fx2lafw_profile *profile;

	/*
	 * Since we can't keep track of an fx2lafw device after upgrading
	 * the firmware (it renumerates into a different device address
	 * after the upgrade) this is like a global lock. No device will open
	 * until a proper delay after the last device was upgraded.
	 */
	int64_t fw_updated;

	/* Device/capture settings */
	uint64_t cur_samplerate;
	uint64_t limit_samples;

	gboolean sample_wide;

	uint16_t trigger_mask[NUM_TRIGGER_STAGES];
	uint16_t trigger_value[NUM_TRIGGER_STAGES];
	int trigger_stage;
	uint16_t trigger_buffer[NUM_TRIGGER_STAGES];

	int num_samples;
	int submitted_transfers;
	int empty_transfer_count;

	void *cb_data;

	unsigned int num_transfers;
	struct libusb_transfer **transfers;

	void (*send_data_proc)(struct libusb_transfer *transfer,
		uint8_t *data, size_t length, size_t sample_width);
};

#endif
