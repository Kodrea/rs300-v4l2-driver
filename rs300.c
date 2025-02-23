// SPDX-License-Identifier: GPL-2.0
/*
 * rs300 CMOS Image Sensor driver
 *
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd.
 * V0.0X01.0X01 add enum_frame_interval function.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/media.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/videodev2.h>
#include <linux/version.h>
//#include <linux/rk-camera-module.h>	//remove
#include <media/media-entity.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-image-sizes.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <linux/pinctrl/consumer.h>
#define DRIVER_VERSION			KERNEL_VERSION(0, 0x01, 0x1)
#define DRIVER_NAME "rs300-mipi"
//80M (clk)* 2(double ) *2 (lan) /8
#define rs300_LINK_RATE (80 * 1000 * 1000)
#define rs300_PIXEL_RATE		(40 * 1000 * 1000)

/*
 * Module parameters that can be passed during module loading:
 * - mode: Camera resolution mode (0=1280p, 1=640p)
 * - fps: Frames per second
 * - width/height: Image dimensions
 * - type: Image format type
 */
static int mode = 0; 
static int fps = 30;
static int width = 0; 
static int height = 0;
static int type = 16; // 16 for YUYV, 18 for UYVY
module_param(mode, int, 0644);
module_param(fps, int, 0644);
module_param(width, int, 0644);
module_param(height, int, 0644);
module_param(type, int, 0644);

/*
 * rs300 register definitions
 */
//get命令不止是读还有写，所有要_IOWR
//_IOWR/_IOR 会自动做一层浅拷贝到用户空间，如果有参数类型包含指针需要自己再调用copy_to_user。
//_IOW 会自动把用户空间参数拷贝到内核并且指针也会拷贝，不需要调用copy_from_user。

#define CMD_MAGIC 0xEF //定义幻数
#define CMD_MAX_NR 3 //定义命令的最大序数
#define CMD_GET _IOWR(CMD_MAGIC, 1,struct ioctl_data)
#define CMD_SET _IOW(CMD_MAGIC, 2,struct ioctl_data)
#define CMD_KBUF _IO(CMD_MAGIC, 3)
//这个是v4l2标准推荐的私有命令配置，这里直接使用自定义的命令也是可以的
//#define CMD_GET _IOWR('V', BASE_VIDIOC_PRIVATE + 11,struct ioctl_data)
//#define CMD_SET _IOW('V', BASE_VIDIOC_PRIVATE + 12,struct ioctl_data)

//结构体与usb-i2c保持一致，有效位为 寄存器地址：wIndex  数据指针:data  数据长度:wLength
struct ioctl_data{
	unsigned char bRequestType;
	unsigned char bRequest;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned char* data;
	unsigned short wLength;
	unsigned int timeout;		///< unit:ms
};
#define REG_NULL			0xFFFF	/* Array end token */

/*
 * I2C Communication Constants:
 * Defines buffer sizes, status bits, and command codes for 
 * communicating with the camera over I2C bus
 */
#define I2C_VD_BUFFER_RW			0x1D00
#define I2C_VD_BUFFER_HLD			0x9D00
#define I2C_VD_CHECK_ACCESS			0x8000
#define I2C_VD_BUFFER_DATA_LEN		256
#define I2C_OUT_BUFFER_MAX			64 // IN buffer set equal to I2C_VD_BUFFER_DATA_LEN(256)
#define I2C_TRANSFER_WAIT_TIME_2S	2000

#define I2C_VD_BUFFER_STATUS			0x0200
#define VCMD_BUSY_STS_BIT				0x01
#define VCMD_RST_STS_BIT				0x02
#define VCMD_ERR_STS_BIT				0xFC

#define VCMD_BUSY_STS_IDLE				0x00
#define VCMD_BUSY_STS_BUSY				0x01
#define VCMD_RST_STS_PASS				0x00
#define VCMD_RST_STS_FAIL				0x01

#define VCMD_ERR_STS_SUCCESS				0x00
#define VCMD_ERR_STS_LEN_ERR				0x04
#define VCMD_ERR_STS_UNKNOWN_CMD_ERR		0x08
#define VCMD_ERR_STS_HW_ERR					0x0C
#define VCMD_ERR_STS_UNKNOWN_SUBCMD_ERR		0x10
#define VCMD_ERR_STS_PARAM_ERR				0x14

/*
 * CRC calculation helper function
 * Used to validate data integrity in camera communications
 */
static unsigned short do_crc(unsigned char *ptr, int len)
{
    unsigned int i;
    unsigned short crc = 0x0000;
    
    while(len--)
    {
        crc ^= (unsigned short)(*ptr++) << 8;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    
    return crc;
}

/*
 * Camera initialization registers
 * Contains the register values needed to start/stop the camera stream
 */
static u8 start_regs[] = {
		0x01,  
    0x30,
    0xc1,
    0x00,
    
    0x00,
    0x00,
    0x00,
    0x00,
      
    0x00,
    0x00,
    0x00,
    0x00,  
    
    0x0a,
    0x00,
    
    0x00,//crc [16]
    0x00,
    
    0x2F,//crc [18]
    0x0D,
    
    
    0x00,//path
    0x16,//src
    0x03,//dst
    0x1e,//fps
    
    0x80,//width&0xff;
    0x02, //width>>8; 
    0x00, //height&0xff;
    0x02,//height>>8;
    
    0x00,
    0x00
};

static  u8 stop_regs[]={
		0x01,
    0x30,
    0xc2,
    0x00,
    
    0x00,
    0x00,
    0x00,
    0x00,
      
    0x00,
    0x00,
    0x00,
    0x00,  
    
    0x0a,
    0x00,
    
    0x00,//crc [16]
    0x00,
    
    0x2F,//crc [18]
    0x0D,
    
    
    0x01,//path
    0x16,//src
    0x00,//dst
    0x0e,//fps
    
    0x80,//width&0xff;
    0x02, //width>>8; 
    0x00, //height&0xff;
    0x02,//height>>8;
    
    0x00,
    0x00
    
};
/*
 * Core I2C read/write functions
 * Handles low-level communication with the camera sensor
 */
static int read_regs(struct i2c_client *client, u16 reg, u8 *val, int len)
{
    // Create array for two I2C messages (write + read sequence)
    struct i2c_msg msg[2];
    // Buffer to hold the register address we want to read from
    unsigned char data[4] = { 0, 0, 0, 0 };
    int ret;
    dev_dbg(&client->dev, "Reading %d bytes from reg 0x%04x\n", len, reg);

    // First message: Write the register address we want to read from
    msg[0].addr = client->addr;  // 7-bit I2C address (0x3c)
    msg[0].flags = 0;           // 0 = Write operation
    msg[0].len = 2;            // Send 2 bytes (16-bit register address)
    msg[0].buf = data;         // Points to our data buffer containing register address

    // Second message: Read data from the previously specified register
    msg[1].addr = client->addr;  // Same 7-bit I2C address (0x3c)
    msg[1].flags = 1;           // 1 = Read operation
    msg[1].len = len;           // Number of bytes to read (passed as parameter)
    msg[1].buf = val;           // Buffer where read data will be stored

    // Break down 16-bit register address into two bytes
    // Example: reg = 0x1D00
    data[0] = reg>>8;           // High byte = 0x1D
    data[1] = reg&0xff;         // Low byte = 0x00


    // Send both messages to the I2C bus
    // This performs:
    // 1. START condition
    // 2. Send slave address (0x3c) with write bit (0) = 0x78
    // 3. Send register address (2 bytes)
    // 4. REPEATED START condition
    // 5. Send slave address (0x3c) with read bit (1) = 0x79
    // 6. Read 'len' bytes of data
    // 7. STOP condition

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret < 0) {
        dev_err(&client->dev, "i2c read failed at reg 0x%04x: %d\n", reg, ret);
        return ret;
    }
    return 0;
}

/*
static int check_access_done(struct i2c_client *client,int timeout){
	u8 ret=-1;
	do{
		read_regs(client,I2C_VD_BUFFER_STATUS,&ret,1);
		if ((ret & (VCMD_RST_STS_BIT | VCMD_BUSY_STS_BIT)) == \
					(VCMD_BUSY_STS_IDLE | VCMD_RST_STS_PASS))
			{
				return 0;
			}

		msleep(1);
		timeout--;
	}while (timeout);
	v4l_err(client,"timeout ret=%d\n",ret);
	return -1;
}
*/
static int write_regs(struct i2c_client *client, u16 reg, u8 *val, int len)
{
    struct i2c_msg msg[1];
    unsigned char *outbuf;
    int ret;

    dev_dbg(&client->dev, "Writing %d bytes to reg 0x%04x\n", len, reg);

    outbuf = kmalloc(len + 2, GFP_KERNEL);
    if (!outbuf) {
        dev_err(&client->dev, "Memory allocation failed\n");
        return -ENOMEM;
    }

    msg->addr = client->addr;
    msg->flags = 0;
    msg->len = len + 2;
    msg->buf = outbuf;

    outbuf[0] = reg>>8;
    outbuf[1] = reg&0xff;
    memcpy(outbuf + 2, val, len);

    ret = i2c_transfer(client->adapter, msg, 1);
    if (ret < 0) {
        dev_err(&client->dev, "i2c write failed at reg 0x%04x: %d\n", reg, ret);
        kfree(outbuf);
        return ret;
    }

    dev_dbg(&client->dev, "Wrote %d bytes to reg 0x%04x, first byte: 0x%02x\n", 
            len, reg, val[0]);

    kfree(outbuf);
    return ret;
	// if (reg & I2C_VD_CHECK_ACCESS){
	// 	return ret;
	// }else
	// {
	// 	return check_access_done(client,2000);//命令超时控制，由于应用层已经控制这里不需要了
	// }
}

/* Frame size configuration structure
 * To add a new resolution:
 * 1. Add a new entry to rs300_framesizes array
 * 2. Specify width, height, max_fps and pixel format code
 * 3. Update rs300_enum_frame_sizes() if needed
 */
struct rs300_framesize {
	u16 width;
	u16 height;
	struct v4l2_fract max_fps;  // Framerate specified as fraction (e.g. 30/1 for 30fps)
	u32 code;  // V4L2 media bus format code (e.g. MEDIA_BUS_FMT_YUYV8_1X16)
};

/*
 * Power supply configuration
 * Camera sensors typically need multiple power rails:
 * - dovdd: Digital I/O voltage
 * - avdd: Analog voltage
 * - dvdd: Digital core voltage
 */
static const char * const rs300_supply_names[] = {
	"dovdd",    /* Digital I/O power */
	"avdd",     /* Analog power */
	"dvdd",     /* Digital core voltage */
};

#define rs300_NUM_SUPPLIES ARRAY_SIZE(rs300_supply_names)

/*
 * Main camera sensor structure
 * Contains all state information and V4L2 subdev structures needed
 * to represent this camera in the Linux media framework
 */
struct rs300 {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt format;
	unsigned int xvclk_frequency;
	struct clk *xvclk;
	struct gpio_desc *pwdn_gpio;
	struct regulator_bulk_data supplies[rs300_NUM_SUPPLIES];
	struct mutex lock;
	struct i2c_client *client;
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *link_frequency;
	struct v4l2_ctrl *pixel_rate;
	const struct rs300_framesize *frame_size;
	int streaming;
	u32 module_index;
	const char *module_facing;
	const char *module_name;
	const char *len_name;
};

static struct rs300_framesize rs300_framesizes[] = {
	{
		.width = 256,
		.height = 192,
		.max_fps = {
			.numerator = 1,
			.denominator = 25,  // 25fps
		},
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
	},
};

/*
 * Helper function to convert V4L2 subdev pointer to rs300 structure
 * Uses container_of macro which is a common Linux kernel pattern for
 * accessing the parent structure from a member pointer
 */
static inline struct rs300 *to_rs300(struct v4l2_subdev *sd)
{
	return container_of(sd, struct rs300, sd);
}

/*
 * rs300_get_default_format - Sets up default video format
 * Configures resolution and pixel format based on camera mode
 * Uses module parameters if provided, otherwise uses defaults
 */
static void rs300_get_default_format(struct v4l2_mbus_framefmt *format, int index)
{
	// Set native resolution and format
	format->width = 256;
	format->height = 192;
	format->code = MEDIA_BUS_FMT_YUYV8_2X8;
	format->field = V4L2_FIELD_NONE;
	format->colorspace = V4L2_COLORSPACE_SRGB;
	format->ycbcr_enc = V4L2_YCBCR_ENC_601;
	format->quantization = V4L2_QUANTIZATION_LIM_RANGE;
	format->xfer_func = V4L2_XFER_FUNC_SRGB;
}

/*
 * rs300_enum_mbus_code - Lists supported pixel formats
 * Part of V4L2 subdev pad ops - allows userspace to query
 * supported format codes (like YUYV, RGB, etc)
 */
static int rs300_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct rs300 *rs300 = to_rs300(sd);
	if (code->index >= 1)
		return -EINVAL;
	code->code = rs300_framesizes[rs300->module_index].code;

	return 0;
}

/*
 * rs300_enum_frame_sizes - Lists supported frame sizes
 * Allows userspace to query available resolutions for each format
 */
static int rs300_enum_frame_sizes(struct v4l2_subdev *sd,
				   struct v4l2_subdev_state *state,
				   struct v4l2_subdev_frame_size_enum *fse)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct rs300 *rs300 = to_rs300(sd);
	if (fse->index >= 1)
		return -EINVAL;

	fse->code = rs300_framesizes[rs300->module_index].code;
	fse->min_width  = rs300_framesizes[rs300->module_index].width;
	fse->max_width  = fse->min_width;
	fse->max_height = rs300_framesizes[rs300->module_index].height;
	fse->min_height = fse->max_height;

	return 0;
}

static int rs300_get_fmt(struct v4l2_subdev *sd,
                        struct v4l2_subdev_state *state,
                        struct v4l2_subdev_format *fmt)
{
    struct rs300 *rs300 = to_rs300(sd);

    fmt->format.width = rs300->format.width;
    fmt->format.height = rs300->format.height;
    fmt->format.code = rs300->format.code;
    fmt->format.field = V4L2_FIELD_NONE;
    fmt->format.colorspace = V4L2_COLORSPACE_SRGB;
    fmt->format.ycbcr_enc = V4L2_YCBCR_ENC_601;
    fmt->format.quantization = V4L2_QUANTIZATION_LIM_RANGE;
    fmt->format.xfer_func = V4L2_XFER_FUNC_SRGB;

    return 0;
}

static int rs300_set_fmt(struct v4l2_subdev *sd,
                        struct v4l2_subdev_state *state,
                        struct v4l2_subdev_format *fmt)
{
    struct rs300 *rs300 = to_rs300(sd);
    struct v4l2_mbus_framefmt *format;

    // Question: Should we allow format changes while streaming?
    if (rs300->streaming)
        return -EBUSY;

    format = v4l2_subdev_get_try_format(sd, state, fmt->pad);

    // Force native format - but should we be more flexible?
    fmt->format.width = 256;
    fmt->format.height = 192;
    fmt->format.code = MEDIA_BUS_FMT_YUYV8_2X8;
    fmt->format.field = V4L2_FIELD_NONE;
    fmt->format.colorspace = V4L2_COLORSPACE_SRGB;
    fmt->format.ycbcr_enc = V4L2_YCBCR_ENC_601;
    fmt->format.quantization = V4L2_QUANTIZATION_LIM_RANGE;
    fmt->format.xfer_func = V4L2_XFER_FUNC_SRGB;

    if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
        *format = fmt->format;
    } else {
        rs300->format = fmt->format;
        // Should we configure sensor registers here?
    }

    return 0;
}

/* Rockchip module info structure 
*  can be removed for Raspberry Pi, find where it's used throughout the code
*/
/*
static void rs300_get_module_inf(struct rs300 *rs300,
				  struct rkmodule_inf *inf)
{
	memset(inf, 0, sizeof(*inf));
	strlcpy(inf->base.sensor, DRIVER_NAME, sizeof(inf->base.sensor));
	strlcpy(inf->base.module, rs300->module_name,
		sizeof(inf->base.module));
	strlcpy(inf->base.lens, rs300->len_name, sizeof(inf->base.lens));
}
*/

/* rs300_ioctl - Handles custom V4L2 commands for camera control
 * @sd: V4L2 subdevice pointer
 * @cmd: IOCTL command code
 * @arg: Command argument/data pointer
 *
 * Supports three main operations:
 * 1. RKMODULE_GET_MODULE_INFO: Get camera module information
 * 2. CMD_GET: Read camera registers via I2C
 * 3. CMD_SET: Write to camera registers via I2C
 *
 * Return: 0 on success, error code on failure
 */
static long rs300_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
    struct rs300 *rs300 = to_rs300(sd);
    struct i2c_client *client = rs300->client;
    long ret = 0;

    dev_dbg(&client->dev, "IOCTL cmd: 0x%x\n", cmd);

    // Add mutex protection
    mutex_lock(&rs300->lock);

    switch (cmd) {
        case VIDIOC_QUERYCAP: {
            struct v4l2_capability *cap = arg;
            // But should we handle this at subdev level? Maybe this belongs in the bridge driver?
            strlcpy(cap->driver, DRIVER_NAME, sizeof(cap->driver));
            strlcpy(cap->card, "rs300-mipi", sizeof(cap->card));
            cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
            break;
        }

        case VIDIOC_S_PARM: {
            struct v4l2_streamparm *parm = arg;
            // Question: Should we allow dynamic FPS changes?
            if (parm->parm.capture.timeperframe.numerator != 1 ||
                parm->parm.capture.timeperframe.denominator != 25) {
                ret = -EINVAL;
            }
            break;
        }

        // Handle custom commands from before
        case CMD_GET: {
            struct ioctl_data *valp = arg;
            unsigned char *data = kmalloc(valp->wLength, GFP_KERNEL);
            if (!data) {
                ret = -ENOMEM;
                break;
            }
            ret = read_regs(client, valp->wIndex, data, valp->wLength);
            if (ret >= 0 && copy_to_user(valp->data, data, valp->wLength))
                ret = -EFAULT;
            kfree(data);
            break;
        }

        case CMD_SET: {
            struct ioctl_data *valp = arg;
            ret = write_regs(client, valp->wIndex, valp->data, valp->wLength);
            break;
        }

        default:
            // Should we pass unknown commands to parent?
            ret = -ENOTTY;
            break;
    }

    mutex_unlock(&rs300->lock);
    return ret;
}

// comment out for Raspberry Pi	
/*	
#ifdef CONFIG_COMPAT
static long rs300_compat_ioctl32(struct v4l2_subdev *sd,
				  unsigned int cmd, unsigned long arg)
{
	void __user *up = compat_ptr(arg);
	struct rkmodule_inf *inf;
	struct rkmodule_awb_cfg *cfg;
	long ret;
	printk("rs300 ioctl %d\n",cmd);
	switch (cmd) {
	case RKMODULE_GET_MODULE_INFO:
		inf = kzalloc(sizeof(*inf), GFP_KERNEL);
		if (!inf) {
			ret = -ENOMEM;
			return ret;
		}

		ret = rs300_ioctl(sd, cmd, inf);
		if (!ret)
			ret = copy_to_user(up, inf, sizeof(*inf));
		kfree(inf);
		break;
	case RKMODULE_AWB_CFG:
		cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);
		if (!cfg) {
			ret = -ENOMEM;
			return ret;
		}

		ret = copy_from_user(cfg, up, sizeof(*cfg));
		if (!ret)
			ret = rs300_ioctl(sd, cmd, cfg);
		kfree(cfg);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
#endif
*/


/* rs300_s_stream - Start/Stop video streaming
 * @sd: V4L2 subdev
 * @enable: Stream state (1=on, 0=off)
 *
 * Key steps:
 * 1. Validates streaming state to avoid duplicate starts/stops
 * 2. Updates sensor registers via I2C for streaming control
 * 3. Calculates CRC for command validation
 * 4. Handles error conditions and recovery
 *
 * To modify framerates:
 * 1. Update start_regs/stop_regs timing values
 * 2. Modify rs300_framesize max_fps values
 */
static int rs300_s_stream(struct v4l2_subdev *sd, int enable)
{
    struct rs300 *rs300 = to_rs300(sd);
    struct i2c_client *client = rs300->client;
    int ret = 0;

    mutex_lock(&rs300->lock);

    if (enable) {
        if (rs300->streaming) {
            mutex_unlock(&rs300->lock);
            return 0;
        }

        // Question: Should we verify format before starting stream?
        if (rs300->format.width != 256 || rs300->format.height != 192) {
            dev_err(&client->dev, "Invalid format: %dx%d\n", 
                    rs300->format.width, rs300->format.height);
            mutex_unlock(&rs300->lock);
            return -EINVAL;
        }

        // Initialize sensor with specific settings
        // But what if these settings are wrong for this sensor variant?
        ret = write_regs(client, I2C_VD_BUFFER_RW, start_regs, sizeof(start_regs));
        if (ret < 0) {
            dev_err(&client->dev, "Failed to start streaming: %d\n", ret);
            mutex_unlock(&rs300->lock);
            return ret;
        }

        // Add delay for sensor to stabilize - but is 100ms enough?
        msleep(100);
        rs300->streaming = 1;
    } else {
        if (!rs300->streaming) {
            mutex_unlock(&rs300->lock);
            return 0;
        }

        ret = write_regs(client, I2C_VD_BUFFER_RW, stop_regs, sizeof(stop_regs));
        if (ret < 0) {
            dev_err(&client->dev, "Failed to stop streaming: %d\n", ret);
            // Should we force streaming off despite error?
            rs300->streaming = 0;
            mutex_unlock(&rs300->lock);
            return ret;
        }

        rs300->streaming = 0;
        msleep(50);
    }

    mutex_unlock(&rs300->lock);
    return ret;
}

static int rs300_set_test_pattern(struct rs300 *rs300, int value)
{
	return 0;
}

static int rs300_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct rs300 *rs300 = container_of(ctrl->handler, struct rs300, ctrls);

	switch (ctrl->id) {
	case V4L2_CID_TEST_PATTERN:
		return rs300_set_test_pattern(rs300, ctrl->val);
	}

	return 0;
}

/* Fix control ops structure definition */
static const struct v4l2_ctrl_ops rs300_ctrl_ops = {
	.s_ctrl = rs300_s_ctrl,
};

static const s64 link_freq_menu_items[] = {
	rs300_LINK_RATE,//80m
};
/* -----------------------------------------------------------------------------
 * V4L2 subdev internal operations
 */

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static int rs300_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct rs300 *rs300 = to_rs300(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format =
				v4l2_subdev_get_try_format(sd, fh->state, 0);

	dev_dbg(&client->dev, "%s:\n", __func__);

	rs300_get_default_format(format,rs300->module_index);

	return 0;
}
#endif

/*
 * MIPI CSI-2 Configuration
 * Sets up the MIPI serial interface parameters used by the camera
 * - Configures number of data lanes
 * - Sets up channel allocation
 * - Defines clock behavior
 */
static int rs300_g_mbus_config(struct v4l2_subdev *sd,
                              unsigned int pad,
                              struct v4l2_mbus_config *config)
{
    /* Use the correct structure members for newer kernels */
    config->type = V4L2_MBUS_CSI2_DPHY;
    config->bus.mipi_csi2.num_data_lanes = 2;
    config->bus.mipi_csi2.flags = V4L2_MBUS_CSI2_NONCONTINUOUS_CLOCK;
    return 0;
}

/* rs300_enum_frame_interval - List supported frame intervals
 *
 * Returns supported framerates for each resolution
 * To add new framerates:
 * 1. Update rs300_framesizes max_fps values
 * 2. Modify sensor timing registers if needed
 * 3. Update fie->interval validation
 */
static int rs300_enum_frame_interval(struct v4l2_subdev *sd,
				       struct v4l2_subdev_state *state,
				       struct v4l2_subdev_frame_interval_enum *fie)
{
	struct rs300 *rs300 = to_rs300(sd);
	if (fie->index >= 1)
		return -EINVAL;

	fie->width = rs300_framesizes[rs300->module_index].width;
	fie->height = rs300_framesizes[rs300->module_index].height;
	fie->interval = rs300_framesizes[rs300->module_index].max_fps;
	return 0;
}

// Forward declarations for power management functions
static int rs300_power_on(struct rs300 *rs300);
static void rs300_power_off(struct rs300 *rs300);

// Remove duplicate g_frame_interval function and keep only one implementation
static int rs300_g_frame_interval(struct v4l2_subdev *sd,
                                struct v4l2_subdev_frame_interval *fi)
{
    // Return fixed 25fps
    fi->interval.numerator = 1;
    fi->interval.denominator = 25;
    return 0;
}

static int rs300_s_frame_interval(struct v4l2_subdev *sd,
                                struct v4l2_subdev_frame_interval *fi)
{
    // Force 25fps
    fi->interval.numerator = 1;
    fi->interval.denominator = 25;
    return 0;
}

static const struct v4l2_subdev_video_ops rs300_subdev_video_ops = {
    .s_stream = rs300_s_stream,
    .g_frame_interval = rs300_g_frame_interval,
    .s_frame_interval = rs300_s_frame_interval,
};

// Move power management implementations before probe function
static int rs300_power_on(struct rs300 *rs300)
{
    struct i2c_client *client = rs300->client;
    int ret;

    // Question: Is this power sequence correct for this specific sensor?
    ret = regulator_bulk_enable(rs300_NUM_SUPPLIES, rs300->supplies);
    if (ret) {
        dev_err(&client->dev, "Failed to enable regulators\n");
        return ret;
    }

    // Toggle reset pin - but what's the correct timing?
    if (rs300->pwdn_gpio) {
        gpiod_set_value_cansleep(rs300->pwdn_gpio, 1);
        msleep(20); // Is 20ms enough?
        gpiod_set_value_cansleep(rs300->pwdn_gpio, 0);
        msleep(20);
    }

    // Should we add sensor-specific initialization here?
    ret = write_regs(client, I2C_VD_BUFFER_RW, start_regs, sizeof(start_regs));
    if (ret) {
        dev_err(&client->dev, "Sensor init failed\n");
        regulator_bulk_disable(rs300_NUM_SUPPLIES, rs300->supplies);
        return ret;
    }

    return 0;
}

static void rs300_power_off(struct rs300 *rs300)
{
    if (rs300->pwdn_gpio)
        gpiod_set_value_cansleep(rs300->pwdn_gpio, 1);
    regulator_bulk_disable(rs300_NUM_SUPPLIES, rs300->supplies);
}

// First, define the subdev ops structure that combines all operation types
static const struct v4l2_subdev_core_ops rs300_core_ops = {
    .log_status = v4l2_ctrl_subdev_log_status,
    .subscribe_event = v4l2_ctrl_subdev_subscribe_event,
    .unsubscribe_event = v4l2_event_subdev_unsubscribe,
    .ioctl = rs300_ioctl,
};

static const struct v4l2_subdev_video_ops rs300_video_ops = {
    .s_stream = rs300_s_stream,
    .g_frame_interval = rs300_g_frame_interval,
    .s_frame_interval = rs300_s_frame_interval,
};

static const struct v4l2_subdev_pad_ops rs300_pad_ops = {
    .enum_mbus_code = rs300_enum_mbus_code,
    .enum_frame_size = rs300_enum_frame_sizes,
    .enum_frame_interval = rs300_enum_frame_interval,
    .get_fmt = rs300_get_fmt,
    .set_fmt = rs300_set_fmt,
    .get_mbus_config = rs300_g_mbus_config,
};

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static const struct v4l2_subdev_internal_ops rs300_internal_ops = {
    .open = rs300_open,
};
#endif

// Main subdev ops structure that was missing
static const struct v4l2_subdev_ops rs300_subdev_ops = {
    .core = &rs300_core_ops,
    .video = &rs300_video_ops,
    .pad = &rs300_pad_ops,
};

// Update probe function to properly initialize subdev
static int rs300_probe(struct i2c_client *client)
{
    struct rs300 *rs300;
    int ret;
    int i;  // Add missing variable declaration

    dev_info(&client->dev, "Probing rs300 camera\n");

    rs300 = devm_kzalloc(&client->dev, sizeof(*rs300), GFP_KERNEL);
    if (!rs300)
        return -ENOMEM;

    rs300->client = client;

    // Initialize power supplies
    for (i = 0; i < rs300_NUM_SUPPLIES; i++)
        rs300->supplies[i].supply = rs300_supply_names[i];

    ret = devm_regulator_bulk_get(&client->dev,
                                 rs300_NUM_SUPPLIES,
                                 rs300->supplies);
    if (ret)
        return ret;

    // Get GPIO descriptors
    rs300->pwdn_gpio = devm_gpiod_get_optional(&client->dev, "pwdn",
                                              GPIOD_OUT_HIGH);
    if (IS_ERR(rs300->pwdn_gpio))
        return PTR_ERR(rs300->pwdn_gpio);

    // Initialize mutex
    mutex_init(&rs300->lock);

    // Initialize V4L2 subdev with the ops structure
    v4l2_i2c_subdev_init(&rs300->sd, client, &rs300_subdev_ops);

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
    rs300->sd.internal_ops = &rs300_internal_ops;
    rs300->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
#endif

    // Initialize controls
    ret = v4l2_ctrl_handler_init(&rs300->ctrls, 2);
    if (ret)
        goto error_mutex_destroy;

    rs300->link_frequency = v4l2_ctrl_new_int_menu(&rs300->ctrls, NULL,
                                                  V4L2_CID_LINK_FREQ,
                                                  0, 0, link_freq_menu_items);

    rs300->pixel_rate = v4l2_ctrl_new_std(&rs300->ctrls, &rs300_ctrl_ops,
                                         V4L2_CID_PIXEL_RATE, 0,
                                         rs300_PIXEL_RATE, 1,
                                         rs300_PIXEL_RATE);

    rs300->sd.ctrl_handler = &rs300->ctrls;
    if (rs300->ctrls.error) {
        ret = rs300->ctrls.error;
        goto error_free_ctrl;
    }

    // Initialize format
    rs300_get_default_format(&rs300->format, rs300->module_index);

    // Initialize media entity
    rs300->pad.flags = MEDIA_PAD_FL_SOURCE;
    rs300->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
    ret = media_entity_pads_init(&rs300->sd.entity, 1, &rs300->pad);
    if (ret)
        goto error_free_ctrl;

    ret = v4l2_async_register_subdev_sensor(&rs300->sd);
    if (ret)
        goto error_clean_entity;

    dev_info(&client->dev, "rs300 camera probed successfully\n");
    return 0;

error_clean_entity:
    media_entity_cleanup(&rs300->sd.entity);
error_free_ctrl:
    v4l2_ctrl_handler_free(&rs300->ctrls);
error_power_off:
    rs300_power_off(rs300);
error_mutex_destroy:
    mutex_destroy(&rs300->lock);
    return ret;
}

/* rs300_remove - Driver cleanup
 *
 * Cleanup steps:
 * 1. Free V4L2 controls
 * 2. Unregister V4L2 subdevice
 * 3. Cleanup media controller resources
 * 4. Free mutex
 */
static void rs300_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct rs300 *rs300 = to_rs300(sd);

	v4l2_ctrl_handler_free(&rs300->ctrls);
	v4l2_async_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif
	mutex_destroy(&rs300->lock);
}

static const struct i2c_device_id rs300_id[] = {
	{ "rs300", 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(i2c, rs300_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id rs300_of_match[] = {
	{ .compatible = "infisense,rs300-mipi"  },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, rs300_of_match);
#endif

static struct i2c_driver rs300_i2c_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.of_match_table = of_match_ptr(rs300_of_match),
	},
	.probe = rs300_probe,
	.remove		= rs300_remove,
	.id_table	= rs300_id,
};

/*
 * sensor_mod_init - Module initialization
 *
 * Called during driver loading:
 * 1. Registers I2C driver with kernel
 * 2. Sets up module parameters
 * 3. Initializes driver resources
 */
static int __init sensor_mod_init(void)
{
	return i2c_add_driver(&rs300_i2c_driver);
}

/*
 * sensor_mod_exit - Module cleanup function
 * Called when the driver is unloaded
 * Unregisters the I2C driver
 */
static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&rs300_i2c_driver);
}

device_initcall_sync(sensor_mod_init);
module_exit(sensor_mod_exit);

/* 
 * Module metadata and license information
 * Required for all kernel modules
 */
MODULE_AUTHOR("infisense");
MODULE_DESCRIPTION("rs300 ir camera driver");
MODULE_LICENSE("GPL v2");
