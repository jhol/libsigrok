/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2010-2012 Bert Vermeulen <bert@biot.com>
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

#ifndef LIBSIGROK_SIGROK_H
#define LIBSIGROK_SIGROK_H

#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <inttypes.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * The public libsigrok header file to be used by frontends.
 *
 * This is the only file that libsigrok users (frontends) are supposed to
 * use and #include. There are other header files which get installed with
 * libsigrok, but those are not meant to be used directly by frontends.
 *
 * The correct way to get/use the libsigrok API functions is:
 *
 * @code{.c}
 *   #include <libsigrok/libsigrok.h>
 * @endcode
 */

/*
 * All possible return codes of libsigrok functions must be listed here.
 * Functions should never return hardcoded numbers as status, but rather
 * use these enum values. All error codes are negative numbers.
 *
 * The error codes are globally unique in libsigrok, i.e. if one of the
 * libsigrok functions returns a "malloc error" it must be exactly the same
 * return value as used by all other functions to indicate "malloc error".
 * There must be no functions which indicate two different errors via the
 * same return code.
 *
 * Also, for compatibility reasons, no defined return codes are ever removed
 * or reused for different errors later. You can only add new entries and
 * return codes, but never remove or redefine existing ones.
 */

/** Status/error codes returned by libsigrok functions. */
enum {
	SR_OK             =  0, /**< No error. */
	SR_ERR            = -1, /**< Generic/unspecified error. */
	SR_ERR_MALLOC     = -2, /**< Malloc/calloc/realloc error. */
	SR_ERR_ARG        = -3, /**< Function argument error. */
	SR_ERR_BUG        = -4, /**< Errors hinting at internal bugs. */
	SR_ERR_SAMPLERATE = -5, /**< Incorrect samplerate. */
};

#define SR_MAX_NUM_PROBES    64 /* Limited by uint64_t. */
#define SR_MAX_PROBENAME_LEN 32

/* Handy little macros */
#define SR_HZ(n)  (n)
#define SR_KHZ(n) ((n) * 1000)
#define SR_MHZ(n) ((n) * 1000000)
#define SR_GHZ(n) ((n) * 1000000000)

#define SR_HZ_TO_NS(n) (1000000000 / (n))

/** libsigrok loglevels. */
enum {
	SR_LOG_NONE = 0, /**< Output no messages at all. */
	SR_LOG_ERR  = 1, /**< Output error messages. */
	SR_LOG_WARN = 2, /**< Output warnings. */
	SR_LOG_INFO = 3, /**< Output informational messages. */
	SR_LOG_DBG  = 4, /**< Output debug messages. */
	SR_LOG_SPEW = 5, /**< Output very noisy debug messages. */
};

/*
 * Use SR_API to mark public API symbols, and SR_PRIV for private symbols.
 *
 * Variables and functions marked 'static' are private already and don't
 * need SR_PRIV. However, functions which are not static (because they need
 * to be used in other libsigrok-internal files) but are also not meant to
 * be part of the public libsigrok API, must use SR_PRIV.
 *
 * This uses the 'visibility' feature of gcc (requires gcc >= 4.0).
 *
 * This feature is not available on MinGW/Windows, as it is a feature of
 * ELF files and MinGW/Windows uses PE files.
 *
 * Details: http://gcc.gnu.org/wiki/Visibility
 */

/* Marks public libsigrok API symbols. */
#ifndef _WIN32
#define SR_API __attribute__((visibility("default")))
#else
#define SR_API
#endif

/* Marks private, non-public libsigrok symbols (not part of the API). */
#ifndef _WIN32
#define SR_PRIV __attribute__((visibility("hidden")))
#else
#define SR_PRIV
#endif

typedef int (*sr_receive_data_callback_t)(int fd, int revents, void *cb_data);
typedef ssize_t (*sr_save_data_callback_t)(uint16_t type,
	void *data, size_t len, void *cb_data);

/** Data types used by hardware drivers for dev_config_set(). */
enum {
	SR_T_UINT64 = 10000,
	SR_T_CHAR,
	SR_T_BOOL,
	SR_T_FLOAT,
	SR_T_RATIONAL_PERIOD,
	SR_T_RATIONAL_VOLT,
	SR_T_KEYVALUE,
};

/** Rational number data type, containing numerator and denominator values. */
struct sr_rational {
	/** Numerator of the rational number. */
	uint64_t p;
	/** Denominator of the rational number. */
	uint64_t q;
};

/** Value for sr_datafeed_packet.type. */
enum {
	SR_DF_HEADER = 10000,
	SR_DF_END,
	SR_DF_TRIGGER,
	SR_DF_LOGIC,
	SR_DF_META_LOGIC,
	SR_DF_ANALOG,
	SR_DF_META_ANALOG,
	SR_DF_FRAME_BEGIN,
	SR_DF_FRAME_END,
};

/* sr_save_data_callback_t type values */
enum {
	SR_DS_BEGIN,
	SR_DS_READ,
	SR_DS_END,
	SR_DS_ERROR
};

/** Values for sr_datafeed_analog.mq. */
enum {
	SR_MQ_VOLTAGE = 10000,
	SR_MQ_CURRENT,
	SR_MQ_RESISTANCE,
	SR_MQ_CAPACITANCE,
	SR_MQ_TEMPERATURE,
	SR_MQ_FREQUENCY,
	SR_MQ_DUTY_CYCLE,
	SR_MQ_CONTINUITY,
	SR_MQ_PULSE_WIDTH,
	SR_MQ_CONDUCTANCE,
	/** Electrical power, usually in W, or dBm. */
	SR_MQ_POWER,
	/** Gain (a transistor's gain, or hFE, for example). */
	SR_MQ_GAIN,
	/** Logarithmic representation of sound pressure relative to a
	 * reference value. */
	SR_MQ_SOUND_PRESSURE_LEVEL,
	SR_MQ_CARBON_MONOXIDE,
	SR_MQ_RELATIVE_HUMIDITY,
};

/** Values for sr_datafeed_analog.unit. */
enum {
	SR_UNIT_VOLT = 10000,
	SR_UNIT_AMPERE,
	SR_UNIT_OHM,
	SR_UNIT_FARAD,
	SR_UNIT_KELVIN,
	SR_UNIT_CELSIUS,
	SR_UNIT_FAHRENHEIT,
	SR_UNIT_HERTZ,
	SR_UNIT_PERCENTAGE,
	SR_UNIT_BOOLEAN,
	SR_UNIT_SECOND,
	/** Unit of conductance, the inverse of resistance. */
	SR_UNIT_SIEMENS,
	/**
	 * An absolute measurement of power, in decibels, referenced to
	 * 1 milliwatt (dBu).
	 */
	SR_UNIT_DECIBEL_MW,
	/** Voltage in decibel, referenced to 1 volt (dBV). */
	SR_UNIT_DECIBEL_VOLT,
	/**
	 * Measurements that intrinsically do not have units attached, such
	 * as ratios, gains, etc. Specifically, a transistor's gain (hFE) is
	 * a unitless quantity, for example.
	 */
	SR_UNIT_UNITLESS,
	/** Sound pressure level relative so 20 micropascals. */
	SR_UNIT_DECIBEL_SPL,
	/**
	 * Normalized (0 to 1) concentration of a substance or compound with 0
	 * representing a concentration of 0%, and 1 being 100%. This is
	 * represented as the fraction of number of particles of the substance.
	 */
	SR_UNIT_CONCENTRATION,
};

/** Values for sr_datafeed_analog.flags. */
enum {
	/** Voltage measurement is alternating current (AC). */
	SR_MQFLAG_AC = 0x01,
	/** Voltage measurement is direct current (DC). */
	SR_MQFLAG_DC = 0x02,
	/** This is a true RMS measurement. */
	SR_MQFLAG_RMS = 0x04,
	/** Value is voltage drop across a diode, or NAN. */
	SR_MQFLAG_DIODE = 0x08,
	/** Device is in "hold" mode (repeating the last measurement). */
	SR_MQFLAG_HOLD = 0x10,
	/** Device is in "max" mode, only updating upon a new max value. */
	SR_MQFLAG_MAX = 0x20,
	/** Device is in "min" mode, only updating upon a new min value. */
	SR_MQFLAG_MIN = 0x40,
	/** Device is in autoranging mode. */
	SR_MQFLAG_AUTORANGE = 0x80,
	/** Device is in relative mode. */
	SR_MQFLAG_RELATIVE = 0x100,
	/** Sound pressure level is A-weighted in the frequency domain,
	 * according to IEC 61672:2003. */
	SR_MQFLAG_SPL_FREQ_WEIGHT_A = 0x200,
	/** Sound pressure level is C-weighted in the frequency domain,
	 * according to IEC 61672:2003. */
	SR_MQFLAG_SPL_FREQ_WEIGHT_C = 0x400,
	/** Sound pressure level is Z-weighted (i.e. not at all) in the
	 * frequency domain, according to IEC 61672:2003. */
	SR_MQFLAG_SPL_FREQ_WEIGHT_Z = 0x800,
	/** Sound pressure level is not weighted in the frequency domain,
	 * albeit without standards-defined low and high frequency limits. */
	SR_MQFLAG_SPL_FREQ_WEIGHT_FLAT = 0x1000,
	/** Sound pressure level measurement is S-weighted (1s) in the
	 * time domain. */
	SR_MQFLAG_SPL_TIME_WEIGHT_S = 0x2000,
	/** Sound pressure level measurement is F-weighted (125ms) in the
	 * time domain. */
	SR_MQFLAG_SPL_TIME_WEIGHT_F = 0x4000,
	/** Sound pressure level is time-averaged (LAT), also known as
	 * Equivalent Continuous A-weighted Sound Level (LEQ). */
	SR_MQFLAG_SPL_LAT = 0x8000,
	/** Sound pressure level represented as a percentage of measurements
	 * that were over a preset alarm level. */
	SR_MQFLAG_SPL_PCT_OVER_ALARM = 0x10000,
};

struct sr_context;

struct sr_datafeed_packet {
	uint16_t type;
	void *payload;
};

struct sr_datafeed_header {
	int feed_version;
	struct timeval starttime;
};

struct sr_datafeed_meta_logic {
	int num_probes;
	uint64_t samplerate;
};

struct sr_datafeed_logic {
	uint64_t length;
	uint16_t unitsize;
	void *data;
};

struct sr_datafeed_meta_analog {
	int num_probes;
};

struct sr_datafeed_analog {
	int num_samples;
	/** Measured quantity (voltage, current, temperature, and so on). */
	int mq;
	/** Unit in which the MQ is measured. */
	int unit;
	/** Bitmap with extra information about the MQ. */
	uint64_t mqflags;
	/** The analog value. */
	float *data;
};

struct sr_input {
	struct sr_input_format *format;
	GHashTable *param;
	struct sr_dev_inst *sdi;
	void *internal;
};

struct sr_input_format {
	char *id;
	char *description;
	int (*format_match) (const char *filename);
	int (*init) (struct sr_input *in);
	int (*loadfile) (struct sr_input *in, const char *filename);
};

struct sr_output {
	struct sr_output_format *format;
	struct sr_dev_inst *sdi;
	char *param;
	void *internal;
};

struct sr_output_format {
	char *id;
	char *description;
	int df_type;
	int (*init) (struct sr_output *o);
	int (*data) (struct sr_output *o, const uint8_t *data_in,
		     uint64_t length_in, uint8_t **data_out,
		     uint64_t *length_out);
	int (*event) (struct sr_output *o, int event_type, uint8_t **data_out,
		      uint64_t *length_out);
	GString *(*recv) (struct sr_output *o, const struct sr_dev_inst *sdi,
			struct sr_datafeed_packet *packet);
	int (*cleanup) (struct sr_output *o);
};

/*
 * This represents a generic device connected to the system.
 * For device-specific information, ask the driver. The driver_index refers
 * to the device index within that driver; it may be handling more than one
 * device. All relevant driver calls take a dev_index parameter for this.
 */
struct sr_dev {
	/** Which driver handles this device. */
	struct sr_dev_driver *driver;
	/** A driver may handle multiple devices of the same type. */
	int driver_index;
	/** List of struct sr_probe pointers. */
	GSList *probes;
	/** Data acquired by this device, if any. */
	struct sr_datastore *datastore;
};

enum {
	SR_PROBE_LOGIC = 10000,
	SR_PROBE_ANALOG,
};

struct sr_probe {
	int index;
	int type;
	gboolean enabled;
	char *name;
	char *trigger;
};

struct sr_hwopt {
	int hwopt;
	const void *value;
};

/** Hardware driver options. */
enum {
	/**
	 * Some drivers cannot detect the exact model they're talking to
	 * (may be phased out).
	 */
	SR_HWOPT_MODEL = 10000,

	/**
	 * Specification on how to connect to a device.
	 *
	 * In combination with SR_HWOPT_SERIALCOMM, this is a serial port in
	 * the form which makes sense to the OS (e.g., /dev/ttyS0).
	 * Otherwise this specifies a USB device, either in the form of
	 * @verbatim <bus>.<address> @endverbatim (decimal, e.g. 1.65) or
	 * @verbatim <vendorid>.<productid> @endverbatim
	 * (hexadecimal, e.g. 1d6b.0001).
	 */
	SR_HWOPT_CONN,

	/**
	 * Serial communication specification, in the form:
	 *
	 *   @verbatim <baudrate>/<databits><parity><stopbits> @endverbatim
	 *
	 * Example: 9600/8n1
	 *
	 * This is always an optional parameter, since a driver typically
	 * knows the speed at which the device wants to communicate.
	 */
	SR_HWOPT_SERIALCOMM,
};

/** Hardware device capabilities. */
enum {
	/*--- Device classes ------------------------------------------------*/

	/** The device can act as logic analyzer. */
	SR_HWCAP_LOGIC_ANALYZER = 10000,

	/** The device can act as an oscilloscope. */
	SR_HWCAP_OSCILLOSCOPE,

	/** The device can act as a multimeter. */
	SR_HWCAP_MULTIMETER,

	/** The device is a demo device. */
	SR_HWCAP_DEMO_DEV,

	/** The device can act as a sound level meter. */
	SR_HWCAP_SOUNDLEVELMETER,

	/** The device can measure temperature. */
	SR_HWCAP_THERMOMETER,

	/** The device can measure humidity. */
	SR_HWCAP_HYGROMETER,

	/*--- Device configuration ------------------------------------------*/

	/** The device supports setting/changing its samplerate. */
	SR_HWCAP_SAMPLERATE = 20000,

	/** The device supports setting a pre/post-trigger capture ratio. */
	SR_HWCAP_CAPTURE_RATIO,

	/* TODO? */
	/** The device supports setting a pattern (pattern generator mode). */
	SR_HWCAP_PATTERN_MODE,

	/** The device supports Run Length Encoding. */
	SR_HWCAP_RLE,

	/** The device supports setting trigger slope. */
	SR_HWCAP_TRIGGER_SLOPE,

	/** Trigger source. */
	SR_HWCAP_TRIGGER_SOURCE,

	/** Horizontal trigger position. */
	SR_HWCAP_HORIZ_TRIGGERPOS,

	/** Buffer size. */
	SR_HWCAP_BUFFERSIZE,

	/** Time base. */
	SR_HWCAP_TIMEBASE,

	/** Filter. */
	SR_HWCAP_FILTER,

	/** Volts/div. */
	SR_HWCAP_VDIV,

	/** Coupling. */
	SR_HWCAP_COUPLING,

	/*--- Special stuff -------------------------------------------------*/

	/** Session filename. */
	SR_HWCAP_SESSIONFILE = 30000,

	/* TODO: Better description. */
	/** The device supports specifying a capturefile to inject. */
	SR_HWCAP_CAPTUREFILE,

	/* TODO: Better description. */
	/** The device supports specifying the capturefile unit size. */
	SR_HWCAP_CAPTURE_UNITSIZE,

	/* TODO: Better description. */
	/** The device supports setting the number of probes. */
	SR_HWCAP_CAPTURE_NUM_PROBES,

	/*--- Acquisition modes ---------------------------------------------*/

	/**
	 * The device supports setting a sample time limit (how long
	 * the sample acquisition should run, in ms).
	 */
	SR_HWCAP_LIMIT_MSEC = 40000,

	/**
	 * The device supports setting a sample number limit (how many
	 * samples should be acquired).
	 */
	SR_HWCAP_LIMIT_SAMPLES,

	/**
	 * The device supports setting a frame limit (how many
	 * frames should be acquired).
	 */
	SR_HWCAP_LIMIT_FRAMES,

	/**
	 * The device supports continuous sampling. Neither a time limit
	 * nor a sample number limit has to be supplied, it will just acquire
	 * samples continuously, until explicitly stopped by a certain command.
	 */
	SR_HWCAP_CONTINUOUS,
};

struct sr_hwcap_option {
	int hwcap;
	int type;
	char *description;
	char *shortname;
};

struct sr_dev_inst {
	struct sr_dev_driver *driver;
	int index;
	int status;
	int inst_type;
	char *vendor;
	char *model;
	char *version;
	GSList *probes;
	void *priv;
};

/** Types of device instances (sr_dev_inst). */
enum {
	/** Device instance type for USB devices. */
	SR_INST_USB = 10000,
	/** Device instance type for serial port devices. */
	SR_INST_SERIAL,
};

/** Device instance status. */
enum {
	/** The device instance was not found. */
	SR_ST_NOT_FOUND = 10000,
	/** The device instance was found, but is still booting. */
	SR_ST_INITIALIZING,
	/** The device instance is live, but not in use. */
	SR_ST_INACTIVE,
	/** The device instance is actively in use in a session. */
	SR_ST_ACTIVE,
	/** The device is winding down its session. */
	SR_ST_STOPPING,
};

/*
 * TODO: This sucks, you just kinda have to "know" the returned type.
 * TODO: Need a DI to return the number of trigger stages supported.
 */

/** Device info IDs. */
enum {
	/** A list of options supported by the driver. */
	SR_DI_HWOPTS = 10000,
	/** A list of capabilities supported by the device. */
	SR_DI_HWCAPS,
	/** The number of probes connected to this device. */
	SR_DI_NUM_PROBES,
	/** The probe names on this device. */
	SR_DI_PROBE_NAMES,
	/** Samplerates supported by this device (struct sr_samplerates). */
	SR_DI_SAMPLERATES,
	/** Types of logic trigger supported, out of "01crf" (char *). */
	SR_DI_TRIGGER_TYPES,
	/** The currently set samplerate in Hz (uint64_t). */
	SR_DI_CUR_SAMPLERATE,
	/** Supported patterns (in pattern generator mode). */
	SR_DI_PATTERNS,
	/** Supported buffer sizes. */
	SR_DI_BUFFERSIZES,
	/** Supported time bases. */
	SR_DI_TIMEBASES,
	/** Supported trigger sources. */
	SR_DI_TRIGGER_SOURCES,
	/** Supported filter targets. */
	SR_DI_FILTERS,
	/** Valid volts/div values. */
	SR_DI_VDIVS,
	/** Coupling options. */
	SR_DI_COUPLING,
};

/*
 * A device supports either a range of samplerates with steps of a given
 * granularity, or is limited to a set of defined samplerates. Use either
 * step or list, but not both.
 */
struct sr_samplerates {
	uint64_t low;
	uint64_t high;
	uint64_t step;
	const uint64_t *list;
};

struct sr_dev_driver {
	/* Driver-specific */
	char *name;
	char *longname;
	int api_version;
	int (*init) (struct sr_context *sr_ctx);
	int (*cleanup) (void);
	GSList *(*scan) (GSList *options);
	GSList *(*dev_list) (void);
	int (*dev_clear) (void);

	/* Device-specific */
	int (*dev_open) (struct sr_dev_inst *sdi);
	int (*dev_close) (struct sr_dev_inst *sdi);
	int (*info_get) (int info_id, const void **data,
			const struct sr_dev_inst *sdi);
	int (*dev_config_set) (const struct sr_dev_inst *sdi, int hwcap,
			const void *value);
	int (*dev_acquisition_start) (const struct sr_dev_inst *sdi,
			void *cb_data);
	int (*dev_acquisition_stop) (struct sr_dev_inst *sdi,
			void *cb_data);

	/* Dynamic */
	void *priv;
};

struct sr_session {
	/** List of struct sr_dev pointers. */
	GSList *devs;
	/** List of sr_receive_data_callback_t items. */
	GSList *datafeed_callbacks;
	GTimeVal starttime;

	unsigned int num_sources;

	/*
	 * Both "sources" and "pollfds" are of the same size and contain pairs
	 * of descriptor and callback function. We can not embed the GPollFD
	 * into the source struct since we want to be able to pass the array
	 * of all poll descriptors to g_poll().
	 */
	struct source *sources;
	GPollFD *pollfds;
	int source_timeout;
};

#include "proto.h"
#include "version.h"

#ifdef __cplusplus
}
#endif

#endif
