/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2013 Bert Vermeulen <bert@biot.com>
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

#include <sys/time.h>

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
 * use and \#include. There are other header files which get installed with
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
	SR_ERR_NA         = -6, /**< Not applicable. */
	SR_ERR_DEV_CLOSED = -7, /**< Device is closed, but needs to be open. */

	/*
	 * Note: When adding entries here, don't forget to also update the
	 * sr_strerror() and sr_strerror_name() functions in error.c.
	 */
};

/* Handy little macros */
#define SR_HZ(n)  (n)
#define SR_KHZ(n) ((n) * (uint64_t)(1000ULL))
#define SR_MHZ(n) ((n) * (uint64_t)(1000000ULL))
#define SR_GHZ(n) ((n) * (uint64_t)(1000000000ULL))

#define SR_HZ_TO_NS(n) ((uint64_t)(1000000000ULL) / (n))

#define SR_NS(n)   (n)
#define SR_US(n)   ((n) * (uint64_t)(1000ULL))
#define SR_MS(n)   ((n) * (uint64_t)(1000000ULL))
#define SR_SEC(n)  ((n) * (uint64_t)(1000000000ULL))
#define SR_MIN(n)  ((n) * (uint64_t)(60000000000ULL))
#define SR_HOUR(n) ((n) * (uint64_t)(3600000000000ULL))
#define SR_DAY(n)  ((n) * (uint64_t)(86400000000000ULL))

#define SR_n(n)  (n)
#define SR_Kn(n) ((n) * (uint64_t)(1000ULL))
#define SR_Mn(n) ((n) * (uint64_t)(1000000ULL))
#define SR_Gn(n) ((n) * (uint64_t)(1000000000ULL))

#define SR_B(n)  (n)
#define SR_KB(n) ((n) * (uint64_t)(1024ULL))
#define SR_MB(n) ((n) * (uint64_t)(1048576ULL))
#define SR_GB(n) ((n) * (uint64_t)(1073741824ULL))

#define SR_mV(n) (n)
#define SR_V(n)  ((n) * (uint64_t)(1000ULL))
#define SR_KV(n) ((n) * (uint64_t)(1000000ULL))
#define SR_MV(n) ((n) * (uint64_t)(1000000000ULL))

#define SR_MAX_PROBENAME_LEN 32
#define DS_MAX_ANALOG_PROBES_NUM 4
#define DS_MAX_DSO_PROBES_NUM 2
#define TriggerStages 16
#define TriggerProbes 16
#define TriggerCountBits 16
#define STriggerDataStage 3

#define DS_CONF_DSO_HDIVS 10
#define DS_CONF_DSO_VDIVS 10

#define DS_MAX_TRIG_PERCENT 90
/*
 * Oscilloscope
 */
#define MAX_TIMEBASE SR_SEC(10)

extern char DS_RES_PATH[256];

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

/** Data types used by sr_config_info(). */
enum {
	SR_T_UINT64 = 10000,
    SR_T_UINT8,
    SR_T_CHAR,
	SR_T_BOOL,
	SR_T_FLOAT,
	SR_T_RATIONAL_PERIOD,
	SR_T_RATIONAL_VOLT,
	SR_T_KEYVALUE,
};

/** Value for sr_datafeed_packet.type. */
enum {
	SR_DF_HEADER = 10000,
	SR_DF_END,
	SR_DF_META,
	SR_DF_TRIGGER,
	SR_DF_LOGIC,
    SR_DF_DSO,
	SR_DF_ANALOG,
	SR_DF_FRAME_BEGIN,
	SR_DF_FRAME_END,
    SR_DF_OVERFLOW,
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

enum DSO_MEASURE_TYPE {
    DSO_MS_BEGIN = 0,
    DSO_MS_FREQ,
    DSO_MS_PERD,
    DSO_MS_VMAX,
    DSO_MS_VMIN,
    DSO_MS_VRMS,
    DSO_MS_VMEA,
    DSO_MS_VP2P,
    DSO_MS_END,
};

enum {
    SR_PKT_OK,
    SR_PKT_SOURCE_ERROR,
    SR_PKT_DATA_ERROR,
};

struct sr_context;

struct sr_datafeed_packet {
	uint16_t type;
    uint16_t status;
	const void *payload;
};

struct sr_datafeed_header {
	int feed_version;
	struct timeval starttime;
};

struct sr_datafeed_meta {
	GSList *config;
};

enum LA_DATA_FORMAT {
    LA_CROSS_DATA,
    LA_SPLIT_DATA,
};

struct sr_datafeed_logic {
	uint64_t length;
    /** data format */
    int format;
    /** for LA_SPLIT_DATA, indicate the channel index */
    uint16_t index;
    uint16_t order;
	uint16_t unitsize;
    uint16_t data_error;
    uint64_t error_pattern;
	void *data;
};

struct sr_datafeed_dso {
    /** The probes for which data is included in this packet. */
    GSList *probes;
    int num_samples;
    /** Measured quantity (voltage, current, temperature, and so on). */
    int mq;
    /** Unit in which the MQ is measured. */
    int unit;
    /** Bitmap with extra information about the MQ. */
    uint64_t mqflags;
    /** samplerate different from last packet */
    gboolean samplerate_tog;
    /** trig flag */
    gboolean trig_flag;
    /** The analog value(s). The data is interleaved according to
     * the probes list. */
    void *data;
};

struct sr_datafeed_analog {
	/** The probes for which data is included in this packet. */
	GSList *probes;
	int num_samples;
    /** How many bits for each sample */
    uint8_t unit_bits;
    /** Interval between two valid samples */
    uint16_t unit_pitch;
	/** Measured quantity (voltage, current, temperature, and so on). */
	int mq;
	/** Unit in which the MQ is measured. */
	int unit;
	/** Bitmap with extra information about the MQ. */
	uint64_t mqflags;
	/** The analog value(s). The data is interleaved according to
	 * the probes list. */
    void *data;
};

/** Input (file) format struct. */
struct sr_input {
	/**
	 * A pointer to this input format's 'struct sr_input_format'.
	 * The frontend can use this to call the module's callbacks.
	 */
	struct sr_input_format *format;

	GHashTable *param;

	struct sr_dev_inst *sdi;

	void *internal;
};

struct sr_input_format {
	/** The unique ID for this input format. Must not be NULL. */
	char *id;

	/**
	 * A short description of the input format, which can (for example)
	 * be displayed to the user by frontends. Must not be NULL.
	 */
	char *description;

	/**
	 * Check if this input module can load and parse the specified file.
	 *
	 * @param filename The name (and path) of the file to check.
	 *
	 * @return TRUE if this module knows the format, FALSE if it doesn't.
	 */
	int (*format_match) (const char *filename);

	/**
	 * Initialize the input module.
	 *
	 * @param in A pointer to a valid 'struct sr_input' that the caller
	 *           has to allocate and provide to this function. It is also
	 *           the responsibility of the caller to free it later.
	 * @param filename The name (and path) of the file to use.
	 *
	 * @return SR_OK upon success, a negative error code upon failure.
	 */
	int (*init) (struct sr_input *in, const char *filename);

	/**
	 * Load a file, parsing the input according to the file's format.
     *
	 * This function will send datafeed packets to the session bus, so
	 * the calling frontend must have registered its session callbacks
	 * beforehand.
	 *
	 * The packet types sent across the session bus by this function must
	 * include at least SR_DF_HEADER, SR_DF_END, and an appropriate data
	 * type such as SR_DF_LOGIC. It may also send a SR_DF_TRIGGER packet
	 * if appropriate.
	 *
	 * @param in A pointer to a valid 'struct sr_input' that the caller
	 *           has to allocate and provide to this function. It is also
	 *           the responsibility of the caller to free it later.
	 * @param filename The name (and path) of the file to use.
	 *
     * @return SR_OK upon succcess, a negative error code upon failure.
	 */
	int (*loadfile) (struct sr_input *in, const char *filename);
};

/** Output (file) format struct. */
struct sr_output {
	/**
	 * A pointer to this output format's 'struct sr_output_format'.
	 * The frontend can use this to call the module's callbacks.
	 */
    const struct sr_output_module *module;

	/**
	 * The device for which this output module is creating output. This
	 * can be used by the module to find out probe names and numbers.
	 */
    const struct sr_dev_inst *sdi;

	/**
	 * An optional parameter which the frontend can pass in to the
	 * output module. How the string is interpreted is entirely up to
	 * the module.
	 */
	char *param;

	/**
	 * A generic pointer which can be used by the module to keep internal
	 * state between calls into its callback functions.
	 *
	 * For example, the module might store a pointer to a chunk of output
	 * there, and only flush it when it reaches a certain size.
	 */
	void *priv;
};

/** Generic option struct used by various subsystems. */
struct sr_option {
	/* Short name suitable for commandline usage, [a-z0-9-]. */
	char *id;
	/* Short name suitable for GUI usage, can contain UTF-8. */
	char *name;
	/* Description of the option, in a sentence. */
	char *desc;
	/* Default value for this option. */
	GVariant *def;
	/* List of possible values, if this is an option with few values. */
	GSList *values;
};

/** Output module driver. */
struct sr_output_module {
	/**
	 * A unique ID for this output module, suitable for use in command-line
	 * clients, [a-z0-9-]. Must not be NULL.
	 */
	char *id;

	/**
	 * A unique name for this output module, suitable for use in GUI
	 * clients, can contain UTF-8. Must not be NULL.
	 */
	const char *name;

	/**
	 * A short description of the output module. Must not be NULL.
	 *
	 * This can be displayed by frontends, e.g. when selecting the output
	 * module for saving a file.
	 */
	char *desc;

	/**
	 * A NULL terminated array of strings containing a list of file name
	 * extensions typical for the input file format, or NULL if there is
	 * no typical extension for this file format.
	 */
	const char *const *exts;

	/**
	 * Returns a NULL-terminated list of options this module can take.
	 * Can be NULL, if the module has no options.
	 */
	const struct sr_option *(*options) (void);

	/**
	 * This function is called once, at the beginning of an output stream.
	 *
	 * The device struct will be available in the output struct passed in,
	 * as well as the param field -- which may be NULL or an empty string,
	 * if no parameter was passed.
	 *
	 * The module can use this to initialize itself, create a struct for
	 * keeping state and storing it in the <code>internal</code> field.
	 *
	 * @param o Pointer to the respective 'struct sr_output'.
	 *
	 * @retval SR_OK Success
	 * @retval other Negative error code.
	 */
	int (*init) (struct sr_output *o, GHashTable *options);

	/**
	 * This function is passed a copy of every packed in the data feed.
	 * Any output generated by the output module in response to the
	 * packet should be returned in a newly allocated GString
	 * <code>out</code>, which will be freed by the caller.
	 *
	 * Packets not of interest to the output module can just be ignored,
	 * and the <code>out</code> parameter set to NULL.
	 *
	 * @param o Pointer to the respective 'struct sr_output'.
	 * @param sdi The device instance that generated the packet.
	 * @param packet The complete packet.
	 * @param out A pointer where a GString * should be stored if
	 * the module generates output, or NULL if not.
	 *
	 * @retval SR_OK Success
	 * @retval other Negative error code.
	 */
	int (*receive) (const struct sr_output *o,
			const struct sr_datafeed_packet *packet, GString **out);

	/**
	 * This function is called after the caller is finished using
	 * the output module, and can be used to free any internal
	 * resources the module may keep.
	 *
	 * @retval SR_OK Success
	 * @retval other Negative error code.
	 */
	int (*cleanup) (struct sr_output *o);
};


enum CHANNEL_TYPE {
    SR_CHANNEL_LOGIC = 10000,
    SR_CHANNEL_DSO,
    SR_CHANNEL_ANALOG,
    SR_CHANNEL_GROUP,
    SR_CHANNEL_DECODER,
    SR_CHANNEL_FFT,
};

enum OPERATION_MODE {
    LOGIC = 0,
    DSO = 1,
    ANALOG = 2,
};

struct sr_channel {
    /* The index field will go: use g_slist_length(sdi->channels) instead. */
    uint16_t index;
    int type;
	gboolean enabled;
	char *name;
	char *trigger;
    uint64_t vdiv;
    uint16_t vfactor;
    double vpos;
    uint16_t vpos_trans;
    uint8_t coupling;
    uint8_t trig_value;
    int8_t comb_diff_top;
    int8_t comb_diff_bom;
    gboolean ms_show;
    gboolean ms_en[DSO_MS_END - DSO_MS_BEGIN];
    const char *map_unit;
    double map_min;
    double map_max;
    struct DSL_vga *vga_ptr;
};

/** Structure for groups of channels that have common properties. */
struct sr_channel_group {
    /** Name of the channel group. */
    char *name;
    /** List of sr_channel structs of the channels belonging to this group. */
    GSList *channels;
    /** Private data for driver use. */
    void *priv;
};

struct sr_config {
	int key;
	GVariant *data;
};

struct sr_config_info {
	int key;
	int datatype;
	char *id;
	char *name;
    char *label;
	char *description;
};

enum {
    SR_STATUS_TRIG_BEGIN = 0,
    SR_STATUS_TRIG_END = 4,
    SR_STATUS_CH0_BEGIN = 5,
    SR_STATUS_CH0_END = 14,
    SR_STATUS_CH1_BEGIN = 15,
    SR_STATUS_CH1_END = 24,
    SR_STATUS_ZERO_BEGIN = 128,
    SR_STATUS_ZERO_END = 135,
};

struct sr_status {
    uint8_t trig_hit;
    uint8_t captured_cnt3;
    uint8_t captured_cnt2;
    uint8_t captured_cnt1;
    uint8_t captured_cnt0;

    uint8_t ch0_max;
    uint8_t ch0_min;
    uint64_t ch0_period;
    uint32_t ch0_pcnt;
    uint8_t ch1_max;
    uint8_t ch1_min;
    uint64_t ch1_period;
    uint32_t ch1_pcnt;

    uint32_t vlen;
    gboolean stream_mode;
    uint32_t sample_divider;
    gboolean sample_divider_tog;
    gboolean trig_flag;

    uint16_t pkt_id;
};

enum {
	/*--- Device classes ------------------------------------------------*/

	/** The device can act as logic analyzer. */
	SR_CONF_LOGIC_ANALYZER = 10000,

	/** The device can act as an oscilloscope. */
	SR_CONF_OSCILLOSCOPE,

	/** The device can act as a multimeter. */
	SR_CONF_MULTIMETER,

	/** The device is a demo device. */
	SR_CONF_DEMO_DEV,

	/** The device can act as a sound level meter. */
	SR_CONF_SOUNDLEVELMETER,

	/** The device can measure temperature. */
	SR_CONF_THERMOMETER,

	/** The device can measure humidity. */
	SR_CONF_HYGROMETER,

	/*--- Driver scan options -------------------------------------------*/

	/**
	 * Specification on how to connect to a device.
	 *
	 * In combination with SR_CONF_SERIALCOMM, this is a serial port in
	 * the form which makes sense to the OS (e.g., /dev/ttyS0).
	 * Otherwise this specifies a USB device, either in the form of
	 * @verbatim <bus>.<address> @endverbatim (decimal, e.g. 1.65) or
	 * @verbatim <vendorid>.<productid> @endverbatim
	 * (hexadecimal, e.g. 1d6b.0001).
	 */
	SR_CONF_CONN = 20000,

	/**
	 * Serial communication specification, in the form:
	 *
	 *   @verbatim <baudrate>/<databits><parity><stopbits> @endverbatim
	 *
	 * Example: 9600/8n1
	 *
	 * The string may also be followed by one or more special settings,
	 * in the form "/key=value". Supported keys and their values are:
	 *
	 * rts    0,1    set the port's RTS pin to low or high
	 * dtr    0,1    set the port's DTR pin to low or high
	 * flow   0      no flow control
	 *        1      hardware-based (RTS/CTS) flow control
	 *        2      software-based (XON/XOFF) flow control
	 * 
	 * This is always an optional parameter, since a driver typically
	 * knows the speed at which the device wants to communicate.
	 */
	SR_CONF_SERIALCOMM,

	/*--- Device configuration ------------------------------------------*/

	/** The device supports setting its samplerate, in Hz. */
	SR_CONF_SAMPLERATE = 30000,

	/** The device supports setting a pre/post-trigger capture ratio. */
	SR_CONF_CAPTURE_RATIO,

    /** */
    SR_CONF_DEVICE_MODE,
    SR_CONF_INSTANT,
    SR_CONF_STATUS,

	/** The device supports setting a pattern (pattern generator mode). */
	SR_CONF_PATTERN_MODE,

	/** The device supports Run Length Encoding. */
	SR_CONF_RLE,

    /** Need wait to uplad captured data */
    SR_CONF_WAIT_UPLOAD,

	/** The device supports setting trigger slope. */
	SR_CONF_TRIGGER_SLOPE,

	/** Trigger source. */
	SR_CONF_TRIGGER_SOURCE,

    /** Trigger channel */
    SR_CONF_TRIGGER_CHANNEL,

    /** Trigger Value. */
    SR_CONF_TRIGGER_VALUE,

	/** Horizontal trigger position. */
	SR_CONF_HORIZ_TRIGGERPOS,

    /** Trigger hold off time */
    SR_CONF_TRIGGER_HOLDOFF,

    /** Trigger Margin */
    SR_CONF_TRIGGER_MARGIN,

	/** Buffer size. */
	SR_CONF_BUFFERSIZE,

	/** Time base. */
    SR_CONF_MAX_TIMEBASE,
	SR_CONF_TIMEBASE,

	/** Filter. */
	SR_CONF_FILTER,

    /** DSO configure sync */
    SR_CONF_DSO_SYNC,

    /** How many bits for each sample */
    SR_CONF_UNIT_BITS,

    /** Valid channel number */
    SR_CONF_VLD_CH_NUM,

    /** Zero */
    SR_CONF_HAVE_ZERO,
    SR_CONF_ZERO,
    SR_CONF_ZERO_SET,
    SR_CONF_ZERO_LOAD,
    SR_CONF_VOCM,
    SR_CONF_CALI,

    /** status for dso channel */
    SR_CONF_STATUS_PERIOD,
    SR_CONF_STATUS_PCNT,
    SR_CONF_STATUS_MAX,
    SR_CONF_STATUS_MIN,

    /** Stream */
    SR_CONF_STREAM,

    /** DSO Roll */
    SR_CONF_ROLL,

    /** Test */
    SR_CONF_TEST,
    SR_CONF_EEPROM,

	/** The device supports setting its sample interval, in ms. */
	SR_CONF_SAMPLE_INTERVAL,

	/** Number of timebases, as related to SR_CONF_TIMEBASE.  */
	SR_CONF_NUM_TIMEBASE,

    /** Number of vertical divisions, as related to SR_CONF_PROBE_VDIV.  */
	SR_CONF_NUM_VDIV,

    /** clock type (internal/external) */
    SR_CONF_CLOCK_TYPE,

    /** clock edge (posedge/negedge) */
    SR_CONF_CLOCK_EDGE,

    /** Device operation mode */
    SR_CONF_OPERATION_MODE,

    /** Device buffer options */
    SR_CONF_BUFFER_OPTIONS,

    /** Device channel mode */
    SR_CONF_CHANNEL_MODE,

    /** RLE compress support */
    SR_CONF_RLE_SUPPORT,

    /** Signal max height **/
    SR_CONF_MAX_HEIGHT,
    SR_CONF_MAX_HEIGHT_VALUE,

    /** Device sample threshold */
    SR_CONF_THRESHOLD,
    SR_CONF_VTH,

    /** Hardware capacity **/
    SR_CONF_MAX_DSO_SAMPLERATE,
    SR_CONF_MAX_DSO_SAMPLELIMITS,
    SR_CONF_HW_DEPTH,

    /*--- Probe configuration -------------------------------------------*/
    /** Probe options */
    SR_CONF_PROBE_CONFIGS,

    /** Probe options */
    SR_CONF_PROBE_SESSIONS,

    /** Enable */
    SR_CONF_PROBE_EN,

    /** Coupling */
    SR_CONF_PROBE_COUPLING,

    /** Volts/div */
    SR_CONF_PROBE_VDIV,

    /** Factor */
    SR_CONF_PROBE_FACTOR,

    /** Vertical position */
    SR_CONF_PROBE_VPOS,

    /** Mapping */
    SR_CONF_PROBE_MAP_UNIT,
    SR_CONF_PROBE_MAP_MIN,
    SR_CONF_PROBE_MAP_MAX,

    /** Vertical offset */
    SR_CONF_PROBE_VOFF,
    SR_CONF_PROBE_VOFF_DEFAULT,
    SR_CONF_PROBE_VOFF_RANGE,

    /** VGain */
    SR_CONF_PROBE_VGAIN,
    SR_CONF_PROBE_VGAIN_DEFAULT,
    SR_CONF_PROBE_VGAIN_RANGE,

	/*--- Special stuff -------------------------------------------------*/

	/** Device options for a particular device. */
	SR_CONF_DEVICE_OPTIONS,

    /** Sessions */
    SR_CONF_DEVICE_SESSIONS,

	/** Session filename. */
	SR_CONF_SESSIONFILE,

	/** The device supports specifying a capturefile to inject. */
	SR_CONF_CAPTUREFILE,

    /** Session file version */
    SR_CONF_FILE_VERSION,

	/** The device supports setting the number of probes. */
	SR_CONF_CAPTURE_NUM_PROBES,

    /** The device supports setting the number of data blocks. */
    SR_CONF_NUM_BLOCKS,

	/*--- Acquisition modes ---------------------------------------------*/

	/**
	 * The device supports setting a sample time limit (how long
	 * the sample acquisition should run, in ms).
	 */
	SR_CONF_LIMIT_MSEC = 50000,

	/**
	 * The device supports setting a sample number limit (how many
	 * samples should be acquired).
	 */
	SR_CONF_LIMIT_SAMPLES,

    /**
     * Absolute time record for session driver
     */
    SR_CONF_TRIGGER_TIME,

    /**
     * Trigger position for session driver
     */
    SR_CONF_TRIGGER_POS,

    /**
     * The actual sample count received
     */
    SR_CONF_ACTUAL_SAMPLES,

	/**
	 * The device supports setting a frame limit (how many
	 * frames should be acquired).
	 */
	SR_CONF_LIMIT_FRAMES,

	/**
	 * The device supports continuous sampling. Neither a time limit
	 * nor a sample number limit has to be supplied, it will just acquire
	 * samples continuously, until explicitly stopped by a certain command.
	 */
	SR_CONF_CONTINUOUS,

	/** The device has internal storage, into which data is logged. This
	 * starts or stops the internal logging. */
	SR_CONF_DATALOG,
};

struct sr_dev_inst {
    /** Device driver. */
    struct sr_dev_driver *driver;
    /** Index of device in driver. */
    int index;
    /** Device instance status. SR_ST_NOT_FOUND, etc. */
    int status;
    /** Device instance type. SR_INST_USB, etc. */
    int inst_type;
    /** Device mode. LA/DAQ/OSC, etc. */
    int mode;
    /** Device vendor. */
    char *vendor;
    /** Device model. */
    char *model;
    /** Device version. */
    char *version;
    /** List of channels. */
    GSList *channels;
    /** List of sr_channel_group structs */
    GSList *channel_groups;
    /** Device instance connection data (used?) */
    void *conn;
    /** Device instance private data (used?) */
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
    /** The device instance has an imcompatible firmware */
    SR_ST_INCOMPATIBLE,
	/** The device instance is actively in use in a session. */
	SR_ST_ACTIVE,
	/** The device is winding down its session. */
	SR_ST_STOPPING,
};

/** Device test modes. */
enum {
    /** No test mode */
    SR_TEST_NONE,
    /** Internal pattern test mode */
    SR_TEST_INTERNAL,
    /** External pattern test mode */
    SR_TEST_EXTERNAL,
    /** SDRAM loopback test mode */
    SR_TEST_LOOPBACK,
};

/** Device buffer mode */
enum {
    /** Stop immediately */
    SR_BUF_STOP = 0,
    /** Upload captured data */
    SR_BUF_UPLOAD = 1,
};

/** Device threshold level. */
enum {
    /** 1.8/2.5/3.3 level */
    SR_TH_3V3 = 0,
    /** 5.0 level */
    SR_TH_5V0 = 1,
};

/** Device input filter. */
enum {
    /** None */
    SR_FILTER_NONE = 0,
    /** One clock cycle */
    SR_FILTER_1T = 1,
};

/** Coupling. */
enum {
    /** DC */
    SR_DC_COUPLING = 0,
    /** AC */
    SR_AC_COUPLING = 1,
    /** Ground */
    SR_GND_COUPLING = 2,
};

struct sr_dev_mode {
    char *name;
    int mode;
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
    const GSList *(*dev_mode_list) (const struct sr_dev_inst *sdi);
    int (*dev_clear) (void);

    int (*config_get) (int id, GVariant **data,
                       const struct sr_dev_inst *sdi,
                       const struct sr_channel *ch,
                       const struct sr_channel_group *cg);
    int (*config_set) (int id, GVariant *data,
                       struct sr_dev_inst *sdi,
                       struct sr_channel *ch,
                       struct sr_channel_group *cg);
    int (*config_list) (int info_id, GVariant **data,
                        const struct sr_dev_inst *sdi,
                        const struct sr_channel_group *cg);

	/* Device-specific */
	int (*dev_open) (struct sr_dev_inst *sdi);
	int (*dev_close) (struct sr_dev_inst *sdi);
    int (*dev_status_get) (const struct sr_dev_inst *sdi,
                           struct sr_status *status,
                           gboolean prg, int begin, int end);
    int (*dev_acquisition_start) (struct sr_dev_inst *sdi,
			void *cb_data);
    int (*dev_acquisition_stop) (const struct sr_dev_inst *sdi,
			void *cb_data);

	/* Dynamic */
	void *priv;
};

struct sr_session {
	/** List of struct sr_dev pointers. */
	GSList *devs;
	/** List of struct datafeed_callback pointers. */
	GSList *datafeed_callbacks;
	GTimeVal starttime;
	gboolean running;

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

	/*
	 * These are our synchronization primitives for stopping the session in
	 * an async fashion. We need to make sure the session is stopped from
	 * within the session thread itself.
	 */
    GMutex stop_mutex;
	gboolean abort_session;
};

enum {
    SIMPLE_TRIGGER = 0,
    ADV_TRIGGER,
    SERIAL_TRIGGER,
};

enum {
    DSO_TRIGGER_AUTO = 0,
    DSO_TRIGGER_CH0,
    DSO_TRIGGER_CH1,
    DSO_TRIGGER_CH0A1,
    DSO_TRIGGER_CH0O1,
};
enum {
    DSO_TRIGGER_RISING = 0,
    DSO_TRIGGER_FALLING,
};

struct ds_trigger {
    uint16_t trigger_en;
    uint16_t trigger_mode;
    uint16_t trigger_pos;
    uint16_t trigger_stages;
    unsigned char trigger_logic[TriggerStages+1];
    unsigned char trigger0_inv[TriggerStages+1];
    unsigned char trigger1_inv[TriggerStages+1];
    char trigger0[TriggerStages+1][TriggerProbes];
    char trigger1[TriggerStages+1][TriggerProbes];
    uint32_t trigger0_count[TriggerStages+1];
    uint32_t trigger1_count[TriggerStages+1];
};

struct ds_trigger_pos {
    uint32_t check_id;
    uint32_t real_pos;
    uint32_t ram_saddr;
    uint32_t remain_cnt_l;
    uint32_t remain_cnt_h;
    uint32_t status;
    unsigned char first_block[488];
};

typedef int (*sr_receive_data_callback_t)(int fd, int revents, const struct sr_dev_inst *sdi);

#include "proto.h"
#include "version.h"

#ifdef __cplusplus
}
#endif

#endif
