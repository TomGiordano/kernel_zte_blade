
#ifndef __ASM__ARCH_CAMERA_H
#define __ASM__ARCH_CAMERA_H

#include <linux/list.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include "linux/types.h"

#include <mach/board.h>
#include <media/msm_camera.h>

#undef CCRT
#undef CINF
#undef CDBG
#ifdef CONFIG_MSM_CAMERA_DEBUG
#define CPREFIX "[jia@msm_camera]"
#define CCRT(fmt, args...) printk(KERN_CRIT CPREFIX": " fmt, ##args)
#define CINF(fmt, args...) printk(KERN_CRIT CPREFIX": " fmt, ##args)
#define CDBG(fmt, args...) printk(KERN_CRIT CPREFIX": " fmt, ##args)
#else
#define CCRT(fmt, args...) do { } while (0)
#define CINF(fmt, args...) do { } while (0)
#define CDBG(fmt, args...) do { } while (0)
#endif

#define MSM_CAMERA_MSG 0
#define MSM_CAMERA_EVT 1
#define NUM_WB_EXP_NEUTRAL_REGION_LINES 4
#define NUM_WB_EXP_STAT_OUTPUT_BUFFERS  3
#define NUM_AUTOFOCUS_MULTI_WINDOW_GRIDS 16
#define NUM_STAT_OUTPUT_BUFFERS      3
#define NUM_AF_STAT_OUTPUT_BUFFERS      3
#define max_control_command_size 150

enum msm_queue {
	MSM_CAM_Q_CTRL,     /* control command or control command status */
	MSM_CAM_Q_VFE_EVT,  /* adsp event */
	MSM_CAM_Q_VFE_MSG,  /* adsp message */
	MSM_CAM_Q_V4L2_REQ, /* v4l2 request */
};

enum vfe_resp_msg {
	VFE_EVENT,
	VFE_MSG_GENERAL,
	VFE_MSG_SNAPSHOT,
	VFE_MSG_OUTPUT_P,   /* preview (continuous mode ) */
	VFE_MSG_OUTPUT_T,   /* thumbnail (snapshot mode )*/
	VFE_MSG_OUTPUT_S,   /* main image (snapshot mode )*/
	VFE_MSG_OUTPUT_V,   /* video   (continuous mode ) */
	VFE_MSG_STATS_AEC,
	VFE_MSG_STATS_AF,
	VFE_MSG_STATS_AWB,
	VFE_MSG_STATS_RS,
	VFE_MSG_STATS_CS,
	VFE_MSG_STATS_IHIST,
	VFE_MSG_STATS_SKIN,
	VFE_MSG_STATS_WE, /* AEC + AWB */
};

#define VFE31_OUTPUT_MODE_PT (0x1 << 0)
#define VFE31_OUTPUT_MODE_S (0x1 << 1)
#define VFE31_OUTPUT_MODE_V (0x1 << 2)

struct msm_vfe_phy_info {
	uint32_t sbuf_phy;
	uint32_t y_phy;
	uint32_t cbcr_phy;
	uint8_t  output_id; /* VFE31_OUTPUT_MODE_PT/S/V */
};

struct msm_vfe_resp {
	enum vfe_resp_msg type;
	struct msm_vfe_evt_msg evt_msg;
	struct msm_vfe_phy_info phy;
	void    *extdata;
	int32_t extlen;
};

struct msm_vfe_callback {
	void (*vfe_resp)(struct msm_vfe_resp *,
		enum msm_queue, void *syncdata,
		gfp_t gfp);
	void* (*vfe_alloc)(int, void *syncdata, gfp_t gfp);
	void (*vfe_free)(void *ptr);
};

struct msm_camvfe_fn {
	int (*vfe_init)(struct msm_vfe_callback *, struct platform_device *);
	int (*vfe_enable)(struct camera_enable_cmd *);
	int (*vfe_config)(struct msm_vfe_cfg_cmd *, void *);
	int (*vfe_disable)(struct camera_enable_cmd *,
		struct platform_device *dev);
	void (*vfe_release)(struct platform_device *);
};

struct msm_sensor_ctrl {
	int (*s_init)(const struct msm_camera_sensor_info *);
	int (*s_release)(void);
	int (*s_config)(void __user *);
};

struct msm_queue_cmd {
	struct list_head list_config;
	struct list_head list_control;
	struct list_head list_frame;
	struct list_head list_pict;
	enum msm_queue type;
	void *command;
	int on_heap;
	struct timespec ts;
};

struct msm_device_queue {
	struct list_head list;
	spinlock_t lock;
	wait_queue_head_t wait;
	int max;
	int len;
	const char *name;
};

struct msm_sync {
	struct hlist_head pmem_frames;
	struct hlist_head pmem_stats;

	struct msm_device_queue event_q;

	struct msm_device_queue frame_q;
	int unblock_poll_frame;

	struct msm_device_queue pict_q;
	int get_pic_abort;

	struct msm_camera_sensor_info *sdata;
	struct msm_camvfe_fn vfefn;
	struct msm_sensor_ctrl sctrl;
	struct wake_lock wake_suspend_lock;
	struct wake_lock wake_lock;
	struct platform_device *pdev;
	uint8_t opencnt;
	void *cropinfo;
	int  croplen;

	uint32_t pp_mask;
	struct msm_queue_cmd *pp_prev;
	struct msm_queue_cmd *pp_snap;

	const char *apps_id;

	struct mutex lock;
	struct list_head list;
};

#define MSM_APPS_ID_V4L2 "msm_v4l2"
#define MSM_APPS_ID_PROP "msm_qct"

struct msm_device {
	struct msm_sync *sync; /* most-frequently accessed */
	struct device *device;
	struct cdev cdev;
	atomic_t opened;
};

struct msm_control_device {
	struct msm_device *pmsm;

	uint8_t ctrl_data[max_control_command_size];
	struct msm_ctrl_cmd ctrl;
	struct msm_queue_cmd qcmd;
	struct msm_device_queue ctrl_q;
};

struct register_address_value_pair {
	uint16_t register_address;
	uint16_t register_value;
};

struct msm_pmem_region {
	struct hlist_node list;
	unsigned long paddr;
	unsigned long len;
	struct file *file;
	struct msm_pmem_info info;
};

struct axidata {
	uint32_t bufnum1;
	uint32_t bufnum2;
	uint32_t bufnum3;
	struct msm_pmem_region *region;
};

#ifdef CONFIG_MSM_CAMERA_FLASH
	int msm_camera_flash_set_led_state(
		struct msm_camera_sensor_flash_data *fdata,
		unsigned led_state);
int32_t msm_camera_flash_led_enable(void);
int32_t msm_camera_flash_led_disable(void);
#else
	static inline int msm_camera_flash_set_led_state(
		struct msm_camera_sensor_flash_data *fdata,
		unsigned led_state)
	{
		return -ENOTSUPP;
	}
static inline int32_t msm_camera_flash_led_enable(void)
{
    return -ENOTSUPP;
}

static inline int32_t msm_camera_flash_led_disable(void)
{
    return -ENOTSUPP;
}
#endif

struct msm_v4l2_driver {
	struct msm_sync *sync;
	int (*open)(struct msm_sync *, const char *apps_id);
	int (*release)(struct msm_sync *);
	int (*ctrl)(struct msm_sync *, struct msm_ctrl_cmd *);
	int (*reg_pmem)(struct msm_sync *, struct msm_pmem_info *);
	int (*get_frame) (struct msm_sync *, struct msm_frame *);
	int (*put_frame) (struct msm_sync *, struct msm_frame *);
	int (*get_pict) (struct msm_sync *, struct msm_ctrl_cmd *);
	unsigned int (*drv_poll) (struct msm_sync *, struct file *,
				struct poll_table_struct *);
};

int msm_v4l2_register(struct msm_v4l2_driver *);
int msm_v4l2_unregister(struct msm_v4l2_driver *);

void msm_camvfe_init(void);
int msm_camvfe_check(void *);
void msm_camvfe_fn_init(struct msm_camvfe_fn *, void *);
int msm_camera_drv_start(struct platform_device *dev,
		int (*sensor_probe)(const struct msm_camera_sensor_info *,
					struct msm_sensor_ctrl *));
#if defined(CONFIG_SENSOR_ADAPTER)
int msm_camera_dev_start(struct platform_device *dev,
                                 int (*i2c_dev_probe_on)(void),
                                 void (*i2c_dev_probe_off)(void),
                                 int (*sensor_dev_probe)(const struct msm_camera_sensor_info *));
#endif

#if defined(CONFIG_SENSOR_INFO)
void msm_sensorinfo_set_sensor_id(uint16_t id);
#endif

enum msm_camio_clk_type {
	CAMIO_VFE_MDC_CLK,
	CAMIO_MDC_CLK,
	CAMIO_VFE_CLK,
	CAMIO_VFE_AXI_CLK,

	CAMIO_VFE_CAMIF_CLK,
	CAMIO_VFE_PBDG_CLK,
	CAMIO_CAM_MCLK_CLK,
	CAMIO_CAMIF_PAD_PBDG_CLK,

	CAMIO_CSI0_VFE_CLK,
	CAMIO_CSI1_VFE_CLK,
	CAMIO_VFE_PCLK,

	CAMIO_CSI_SRC_CLK,
	CAMIO_CSI0_CLK,
	CAMIO_CSI1_CLK,
	CAMIO_CSI0_PCLK,
	CAMIO_CSI1_PCLK,
	CAMIO_MAX_CLK
};

enum msm_camio_clk_src_type {
	MSM_CAMIO_CLK_SRC_INTERNAL,
	MSM_CAMIO_CLK_SRC_EXTERNAL,
	MSM_CAMIO_CLK_SRC_MAX
};

enum msm_s_test_mode {
	S_TEST_OFF,
	S_TEST_1,
	S_TEST_2,
	S_TEST_3
};

enum msm_s_resolution {
	S_QTR_SIZE,
	S_FULL_SIZE,
	S_INVALID_SIZE
};

enum msm_s_reg_update {
	/* Sensor egisters that need to be updated during initialization */
	S_REG_INIT,
	/* Sensor egisters that needs periodic I2C writes */
	S_UPDATE_PERIODIC,
	/* All the sensor Registers will be updated */
	S_UPDATE_ALL,
	/* Not valid update */
	S_UPDATE_INVALID
};

enum msm_s_setting {
	S_RES_PREVIEW,
	S_RES_CAPTURE
};

enum msm_camera_pwr_mode_t {
    MSM_CAMERA_PWRUP_MODE = 0,
    MSM_CAMERA_STANDBY_MODE,
    MSM_CAMERA_NORMAL_MODE,
    MSM_CAMERA_PWRDWN_MODE,
    MSM_CAMERA_PWR_MODE_MAX
};

int msm_camio_enable(struct platform_device *dev);

int  msm_camio_clk_enable(enum msm_camio_clk_type clk);
int  msm_camio_clk_disable(enum msm_camio_clk_type clk);
int  msm_camio_clk_config(uint32_t freq);
void msm_camio_clk_rate_set(int rate);
void msm_camio_clk_rate_set_2(struct clk *clk, int rate);
void msm_camio_clk_axi_rate_set(int rate);
void msm_disable_io_gpio_clk(struct platform_device *);

void msm_camio_camif_pad_reg_reset(void);
void msm_camio_camif_pad_reg_reset_2(void);

void msm_camio_vfe_blk_reset(void);

void msm_camio_clk_sel(enum msm_camio_clk_src_type);
void msm_camio_disable(struct platform_device *);
int msm_camio_probe_on(struct platform_device *);
int msm_camio_probe_off(struct platform_device *);
int msm_camio_csi_config(struct msm_camera_csi_params *csi_params);
int add_axi_qos(void);
int update_axi_qos(uint32_t freq);
void release_axi_qos(void);
void msm_io_w(u32 data, void __iomem *addr);
void msm_io_w_mb(u32 data, void __iomem *addr);
u32 msm_io_r(void __iomem *addr);
u32 msm_io_r_mb(void __iomem *addr);
void msm_io_dump(void __iomem *addr, int size);
void msm_io_memcpy(void __iomem *dest_addr, void __iomem *src_addr, u32 len);

#endif
