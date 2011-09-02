#ifndef _LINUX_ATMEL_H
#define _LINUX_ATMEL_H

#define ATMEL_QT602240_NAME "atmel_qt602240"

#define RESERVED_T0                               0u
#define RESERVED_T1                               1u
#define DEBUG_DELTAS_T2                           2u
#define DEBUG_REFERENCES_T3                       3u
#define DEBUG_SIGNALS_T4                          4u
#define GEN_MESSAGEPROCESSOR_T5                   5u
#define GEN_COMMANDPROCESSOR_T6                   6u
#define GEN_POWERCONFIG_T7                        7u
#define GEN_ACQUISITIONCONFIG_T8                  8u
#define TOUCH_MULTITOUCHSCREEN_T9                 9u
#define TOUCH_SINGLETOUCHSCREEN_T10               10u
#define TOUCH_XSLIDER_T11                         11u
#define TOUCH_YSLIDER_T12                         12u
#define TOUCH_XWHEEL_T13                          13u
#define TOUCH_YWHEEL_T14                          14u
#define TOUCH_KEYARRAY_T15                        15u
#define PROCG_SIGNALFILTER_T16                    16u
#define PROCI_LINEARIZATIONTABLE_T17              17u
#define SPT_COMCONFIG_T18                         18u
#define SPT_GPIOPWM_T19                           19u
#define PROCI_GRIPFACESUPPRESSION_T20             20u
#define RESERVED_T21                              21u
#define PROCG_NOISESUPPRESSION_T22                22u
#define TOUCH_PROXIMITY_T23	                  23u
#define PROCI_ONETOUCHGESTUREPROCESSOR_T24        24u
#define SPT_SELFTEST_T25                          25u
#define DEBUG_CTERANGE_T26                        26u
#define PROCI_TWOTOUCHGESTUREPROCESSOR_T27        27u
#define SPT_CTECONFIG_T28                         28u
#define SPT_GPI_T29                               29u
#define SPT_GATE_T30                              30u
#define TOUCH_KEYSET_T31                          31u
#define TOUCH_XSLIDERSET_T32                      32u
#define DIAGNOSTIC_T37			37u
struct info_id_t {
	uint8_t family_id;
	uint8_t variant_id;
	uint8_t version;
	uint8_t build;
	uint8_t matrix_x_size;
	uint8_t matrix_y_size;
	uint8_t num_declared_objects;
};

struct object_t {
	uint8_t object_type;
	uint16_t i2c_address;
	uint8_t size;
	uint8_t instances;
	uint8_t num_report_ids;
	uint8_t report_ids;
};

struct atmel_virtual_key {
	int keycode;
	int range_min;
	int range_max;
};

struct atmel_finger_data {
	int x;
	int y;
	int w;
	int z;
	int report_enable;
};

struct atmel_i2c_platform_data {
	uint16_t version;
	uint16_t source;
	uint16_t abs_x_min;
	uint16_t abs_x_max;
	uint16_t abs_y_min;
	uint16_t abs_y_max;
	uint8_t abs_pressure_min;
	uint8_t abs_pressure_max;
	uint8_t abs_width_min;
	uint8_t abs_width_max;
	int gpio_irq;
	int (*power)(int on);
	int8_t config_T6[6];
	int8_t config_T38[8];
	int8_t config_T7[3];
	int8_t config_T8[10];//adjust the frameware 2.0 huangjinyu changed @20110707
	int8_t config_T9[32];//adjust the frameware 2.0 huangjinyu changed @20110707
	int8_t config_T15[11];
	int8_t config_T18[2];
	int8_t config_T19[16];
	int8_t config_T20[12];
	int8_t config_T22[17];
	int8_t config_T23[15];
	int8_t config_T24[19];
	int8_t config_T25[14];
	int8_t config_T27[7];
	int8_t config_T28[6];
	uint8_t object_crc[3];
	int8_t cable_config[4];
	int8_t cable_config_T7[3];
	int8_t cable_config_T8[8];
	int8_t cable_config_T9[31];
	int8_t cable_config_T22[17];
	int8_t cable_config_T28[6];
	uint16_t filter_level[4];
	uint8_t GCAF_level[5];
	int display_width;	/* display width in pixel */
	int display_height;	/* display height in pixel */
};

struct atmel_config_data {
	int8_t config[4];
	int8_t *config_T7;
	int8_t *config_T8;
	int8_t *config_T9;
	int8_t *config_T22;
	int8_t *config_T28;
};

#endif

