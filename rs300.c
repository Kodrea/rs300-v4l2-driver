// SPDX-License-Identifier: GPL-2.0
/*
 * Mini2 / WN2 Uncooled Microbolometer Thermal Camera driver
 *
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd.
 * Copyright (C) 2026 Kodrea
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/media.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/videodev2.h>
#include <linux/version.h>
#include <media/media-entity.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>


#define DRIVER_VERSION			KERNEL_VERSION(0, 0x01, 0x1)
#define DRIVER_NAME "rs300"
//80M (clk)* 2(double ) *2 (lan) /8

#define RS300_LINK_RATE (80 * 1000 * 1000)       /* 80MHz link rate matching device tree */
#define RS300_PIXEL_RATE	(200 * 1000 * 1000)  /* 8-bit: conservative, validated at 384x288@60fps */
#define RS300_PIXEL_RATE_16BIT	(400 * 1000 * 1000)  /* 16-bit: conservative, validated at 384x288@60fps */
#define RS300_BRIGHTNESS_MIN 0
#define RS300_BRIGHTNESS_MAX 100
#define RS300_BRIGHTNESS_STEP 10
#define RS300_BRIGHTNESS_DEFAULT 50
#define V4L2_CID_CUSTOM_BASE (V4L2_CID_USER_BASE + 1000 )

/* Define colormap menu items with the actual names */
static const char * const colormap_menu[] = {
    "White Hot",           /* 0 */
    "Reserved",            /* 1 */
    "Sepia",               /* 2 */
    "Ironbow",             /* 3 */
    "Rainbow",             /* 4 */
    "Night",               /* 5 */
    "Aurora",              /* 6 */
    "Red Hot",             /* 7 */
    "Jungle",              /* 8 */
    "Medical",             /* 9 */
    "Black Hot",           /* 10 */
    "Golden Red Glory_Hot", /* 11 */
    NULL
};

/* Define scene mode menu items */
static const char * const scene_mode_menu[] = {
    "Low",                /* 0 */
    "Linear Stretch",     /* 1 */
    "Low Contrast",       /* 2 */
    "General Mode",       /* 3 */
    "High Contrast",      /* 4 */
    "Highlight",          /* 5 */
    "Reserved 1",         /* 6 */
    "Reserved 2",         /* 7 */
    "Reserved 3",         /* 8 */
    "Outline Mode",       /* 9 */
    NULL
};

/* Define output mode menu items */
static const char * const output_mode_menu[] = {
    "YUV Output",         /* 0 - 8-bit YUV (default) */
    "Y16 Output",         /* 1 - raw 16-bit thermal */
    NULL
};

#define NUM_COLORMAP_ITEMS (ARRAY_SIZE(colormap_menu) - 1) // Account for NULL terminator

// Mode must be set before running setup.sh
// TODO: Make mode adjustable during runtime
static int mode = 2; // 0-640; 1-256; 2-384
static int fps = 60;
static int type = 16;
module_param(mode, int, 0644);
module_param(fps, int, 0644);
module_param(type, int, 0644);

/*
 * rs300 register definitions
 */
//The get command is not only for reading but also for writing, all of which require _IOWR
//_IOWR/_IOR will automatically make a shallow copy to user space. If any parameter type contains a pointer, you need to call copy_to_user yourself.
//_IOW will automatically copy user space parameters to the kernel and pointers will also be copied, without calling copy_from_user.

#define CMD_MAGIC 0xEF //Define magic number
#define CMD_MAX_NR 3 //Defines the maximum ordinal number of commands
#define CMD_GET _IOWR(CMD_MAGIC, 1,struct ioctl_data)
#define CMD_SET _IOW(CMD_MAGIC, 2,struct ioctl_data)
#define CMD_KBUF _IO(CMD_MAGIC, 3)
//This is the private command configuration recommended by the v4l2 standard. You can also use custom commands directly here.
//#define CMD_GET _IOWR('V', BASE_VIDIOC_PRIVATE + 11,struct ioctl_data)
//#define CMD_SET _IOW('V', BASE_VIDIOC_PRIVATE + 12,struct ioctl_data)

//The structure is consistent with usb-i2c, and the valid bits are Register address: wIndex Data pointer: data Data length: wLength
//(from original driver)
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

#define I2C_VD_BUFFER_RW			0x1D00
#define I2C_VD_BUFFER_HLD			0x9D00
#define I2C_VD_CHECK_ACCESS			0x8000
#define I2C_VD_BUFFER_DATA_LEN		256
#define I2C_OUT_BUFFER_MAX			64 // IN buffer set equal to I2C_VD_BUFFER_DATA_LEN(256)
#define I2C_TRANSFER_WAIT_TIME_2S	2000
#define MAX_I2C_TRANSFER_SIZE		256  /* Maximum I2C transfer size (security limit) */

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

/* Forward declarations */
static int read_regs(struct i2c_client *client, u32 reg, u8 *val, int len);
static int write_regs(struct i2c_client *client, u32 reg, u8 *val, int len);

/* Data structures - must be defined before rs300_send_command */
enum pad_types {
	IMAGE_PAD,
	NUM_PADS
};

struct rs300_mode {
	unsigned int width;
	unsigned int height;
	struct v4l2_fract max_fps;
	u32 code;
};

struct pll_ctrl_reg {
	unsigned int div;
	unsigned char reg;
};

static const char * const rs300_supply_names[] = {
	"VANA",	/* Digital I/O power */
	"VDIG",		/* Analog power */
	"VDDL",		/* Digital core power */
};

#define rs300_NUM_SUPPLIES ARRAY_SIZE(rs300_supply_names)

static const u32 codes[] = {
	/* Y16 monochrome format - MUST be first for libcamera MONO sensor detection */
	MEDIA_BUS_FMT_Y16_1X16,    /* 16-bit grayscale - thermal camera native format */
	MEDIA_BUS_FMT_YUYV8_1X16,  /* 16-bit packed - preferred for RP1-CFE */
	/* Additional formats disabled - not needed */
	// MEDIA_BUS_FMT_UYVY8_1X16,  /* 16-bit packed - alternative */
	// MEDIA_BUS_FMT_YUYV8_2X8,   /* 8-bit dual lane - legacy */
	// MEDIA_BUS_FMT_UYVY8_2X8,   /* 8-bit dual lane - legacy */
};

struct rs300 {
	struct v4l2_subdev sd;
	struct media_pad pad[NUM_PADS];

	struct v4l2_mbus_framefmt fmt;

	unsigned int xvclk_frequency;
	struct clk *xvclk;

	struct gpio_desc *reset_gpio;
	struct regulator_bulk_data supplies[rs300_NUM_SUPPLIES];

	struct v4l2_ctrl_handler ctrl_handler;
	/* V4L2 Controls */
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *link_frequency;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *analogue_gain;
	struct v4l2_ctrl *brightness;
	struct v4l2_ctrl *shutter_cal;  /* Shutter calibration button */
	struct v4l2_ctrl *colormap;  /* Colormap selection control */
	struct v4l2_ctrl *zoom;  // Custom zoom control
	struct v4l2_ctrl *scene_mode;  /* Scene mode selection control */
	struct v4l2_ctrl *dde;
	struct v4l2_ctrl *contrast;
	struct v4l2_ctrl *spatial_nr;
	struct v4l2_ctrl *temporal_nr;
	struct v4l2_ctrl *autoshutter;  /* Auto shutter enable/disable */
	struct v4l2_ctrl *autoshutter_temp;  /* Auto shutter temperature threshold */
	struct v4l2_ctrl *autoshutter_min_interval;  /* Auto shutter minimum interval */
	struct v4l2_ctrl *autoshutter_max_interval;  /* Auto shutter maximum interval */
	struct v4l2_ctrl *output_mode;  /* Output mode selection control */
	struct v4l2_ctrl *camera_sleep;  /* Camera sleep/wake control */
	struct v4l2_ctrl *antiburn;  /* Anti-burn protection enable/disable */
	struct v4l2_ctrl *shutter;  /* Shutter open/close control */
	struct v4l2_ctrl *hook_edge;  /* Hook edge position control */
	struct v4l2_ctrl *frame_rate;  /* Detector frame rate control */
	struct v4l2_ctrl *analog_output_fmt;  /* Digital-Analog output format control */

	/* Current mode */
	const struct rs300_mode *mode;

	/* Mode filtering - only advertise modes supported by physical hardware */
	const struct rs300_mode *available_modes;  /* Pointer to single supported mode */
	unsigned int num_modes;  /* Always 1 - only one resolution per physical module */

	/*
	 * Mutex for serialized access:
	 * Protect sensor module set pad format and start/stop streaming safely.
	 */
	struct mutex mutex;

	/* Streaming on/off */
	bool streaming;

	/* Deferred YUV format configuration (set on first stream start) */
	bool yuv_format_configured;
};

/* Autoshutter function prototypes (after struct rs300 definition) */
static int rs300_set_autoshutter(struct rs300 *rs300, int enable);
static int rs300_get_autoshutter(struct rs300 *rs300, int *value);
static int rs300_set_autoshutter_params(struct rs300 *rs300, int param_type, int value);

/**
 * rs300_send_command - Send I2C command to RS300 camera
 * @rs300: RS300 device structure
 * @class: Command class (typically 0x10, 0x01 for zoom)
 * @module: Module code (0x02-0x31)
 * @subcmd: Sub-command code
 * @params: Parameter buffer (12 bytes max, or NULL)
 * @param_len: Length of parameter data (0-12)
 * @timeout_ms: Maximum wait time for completion (default 500ms)
 *
 * Builds 18-byte command packet, calculates CRC, sends command,
 * and polls for completion status.
 *
 * Returns: 0 on success, negative error code on failure
 */
static int rs300_send_command(struct rs300 *rs300,
                              u8 class,
                              u8 module,
                              u8 subcmd,
                              const u8 *params,
                              size_t param_len,
                              unsigned int timeout_ms)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    unsigned char buf[18];
    unsigned char status[2];
    unsigned short crc;
    int ret, i;
    int max_polls;

    if (param_len > 12) {
        dev_err(&client->dev, "Parameter length %zu exceeds maximum 12\n",
                param_len);
        return -EINVAL;
    }

    /* Build 18-byte command packet */
    memset(buf, 0, sizeof(buf));
    buf[0] = class;     // Command class
    buf[1] = module;    // Module code
    buf[2] = subcmd;    // Sub-command code
    buf[3] = 0x00;      // Reserved

    /* Copy parameters (buf[4-15]) */
    if (params && param_len > 0)
        memcpy(&buf[4], params, param_len);

    /* Calculate and append CRC-16 */
    crc = do_crc(buf, 16);
    buf[16] = crc & 0xff;        // CRC low byte
    buf[17] = (crc >> 8) & 0xff; // CRC high byte

    dev_dbg(&client->dev, "Command buffer: %*ph", (int)sizeof(buf), buf);

    /* Send command to camera */
    ret = write_regs(client, 0x1d00, buf, 18);
    if (ret < 0) {
        dev_err(&client->dev, "I2C write failed: %d\n", ret);
        return ret;
    }

    /* Initial delay for camera processing */
    msleep(20);

    /* Poll for completion status */
    max_polls = timeout_ms / 10;
    for (i = 0; i < max_polls; i++) {
        ret = read_regs(client, 0x0200, status, 2);
        if (ret < 0) {
            dev_err(&client->dev, "Status read failed: %d\n", ret);
            return ret;
        }

        /* Extract status components */
        bool is_busy = (status[0] & 0x01) != 0;
        bool has_failed = (status[0] & 0x02) != 0;
        u8 error_code = (status[0] >> 2) & 0x3F;

        /* Check if command is still busy */
        if (is_busy) {
            msleep(10);
            continue;
        }

        /* Check for errors */
        if (has_failed || error_code != 0) {
            dev_err(&client->dev,
                    "Command 0x%02x:0x%02x failed: status=0x%02x error_code=0x%02x\n",
                    module, subcmd, status[0], error_code);

            /* Interpret error code */
            switch (error_code) {
            case 0x01:
                dev_err(&client->dev, "Error: Length error\n");
                break;
            case 0x02:
                dev_err(&client->dev, "Error: Unknown command\n");
                break;
            case 0x03:
                dev_err(&client->dev, "Error: Hardware error\n");
                break;
            case 0x04:
                dev_err(&client->dev, "Error: Command not enabled\n");
                break;
            case 0x05:
            case 0x06:
            case 0x07:
                dev_err(&client->dev, "Error: CRC check error\n");
                break;
            default:
                if (error_code != 0)
                    dev_err(&client->dev, "Error: Unknown error code\n");
                break;
            }

            return -EIO;
        }

        /* Command executed successfully */
        dev_dbg(&client->dev,
                "Command 0x%02x:0x%02x succeeded after %dms\n",
                module, subcmd, (i + 2) * 10);
        return 0;
    }

    /* Timeout */
    dev_err(&client->dev,
            "Command 0x%02x:0x%02x timeout after %dms\n",
            module, subcmd, timeout_ms);
    return -ETIMEDOUT;
}

/*
 * SECURITY FIX: Removed static global buffers to prevent race conditions
 * These are now allocated as local variables in rs300_set_stream()
 * to ensure thread-safety in multi-camera scenarios.
 *
 * Previous vulnerable code (CRITICAL-001):
 * static u8 start_regs[] = { ... };
 * static u8 stop_regs[] = { ... };
 *
 * Issue: Multiple camera instances would modify the same global arrays,
 * causing data corruption in multi-camera setups.
 */

static int read_regs(struct i2c_client *client,  u32 reg, u8 *val ,int len )
{
	struct i2c_msg msg[2];
	unsigned char data[4] = { 0, 0, 0, 0 };
    int ret;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = data;

	msg[1].addr = client->addr;
	msg[1].flags = 1;
	msg[1].len = len;
	msg[1].buf = val;
	/* High byte goes out first */
	data[0] = reg>>8;
	data[1] = reg&0xff;
    
    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret != 2) {
        dev_err(&client->dev, "i2c read error at reg 0x%04x: %d\n", reg, ret);
        return ret < 0 ? ret : -EIO;
    }
    
    return 0;
}

static int write_regs(struct i2c_client *client,  u32 reg, u8 *val,int len)
{
	struct i2c_msg msg[1];
	unsigned char *outbuf = (unsigned char *)kmalloc(sizeof(unsigned char)*(len+2), GFP_KERNEL);
    int ret;

    if (!outbuf) {
        dev_err(&client->dev, "Failed to allocate memory for I2C write\n");
        return -ENOMEM;
    }

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = len+2;
	msg->buf = outbuf;
	outbuf[0] = reg>>8;
    outbuf[1] = reg&0xff;
	memcpy(outbuf+2, val, len);
    
    ret = i2c_transfer(client->adapter, msg, 1);
    if (ret != 1) {
        dev_err(&client->dev, "i2c write error at reg 0x%04x: %d\n", reg, ret);
        kfree(outbuf);
        return ret < 0 ? ret : -EIO;
    }
    
    kfree(outbuf);
    return 0;
	// if (reg & I2C_VD_CHECK_ACCESS){
	// 	return ret;
	// }else
	// {
	// 	return check_access_done(client,2000);//命令超时控制，由于应用层已经控制这里不需要了
	// }
}

/* Duplicate struct definitions removed - now defined earlier in file */

static struct rs300_mode supported_modes[] = {
    { /* 640 - Primary mode for Pi 5 */
        .width      = 640,
        .height     = 512,
        .max_fps = {
            .numerator = 60,
            .denominator = 1,
        },
        .code = MEDIA_BUS_FMT_YUYV8_2X8,  /* YUYV8_2X8 for bcm2835_unicam_legacy (Zero 2W) */
    },
    {
        .width      = 256,
        .height     = 192,
        .max_fps = {
            .numerator = 25,
            .denominator = 1,
        },
        .code = MEDIA_BUS_FMT_YUYV8_2X8,  /* YUYV8_2X8 for bcm2835_unicam_legacy (Zero 2W) */
    },
        { /* 384*/
        .width      = 384,
        .height     = 288,
        .max_fps = {
            .numerator = 60,
            .denominator = 1,
        },
        .code = MEDIA_BUS_FMT_YUYV8_2X8,  /* YUYV8_2X8 for bcm2835_unicam_legacy (Zero 2W) */
    }

};

static inline struct rs300 *to_rs300(struct v4l2_subdev *sd)
{
	return container_of(sd, struct rs300, sd);
}

static u32 rs300_get_format_code(struct rs300 *rs300, u32 code)
{
	unsigned int i;
	struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);

	lockdep_assert_held(&rs300->mutex);	

	dev_dbg(&client->dev, "rs300_get_format_code: input code=0x%x", code);

	for (i = 0; i < ARRAY_SIZE(codes); i++) {
		dev_dbg(&client->dev, "  Checking supported code[%d]=0x%x", i, codes[i]);
		if (codes[i] == code)
			break;
	}

	if (i >= ARRAY_SIZE(codes)) {
		dev_warn(&client->dev, "Format code 0x%x not found, defaulting to 0x%x", code, codes[0]);
		i = 0; /* Default to first supported code (YUYV8_1X16) */
	}

	dev_dbg(&client->dev, "rs300_get_format_code: returning code=0x%x", codes[i]);
	return codes[i];
}

static long rs300_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ioctl_data ioctl_data_kernel;  /* Kernel copy - SECURITY CRITICAL */
	unsigned char *data = NULL;
	unsigned char *user_data_ptr;
	long ret = 0;

	/*
	 * IMPORTANT: Validate command BEFORE copying from userspace.
	 * When V4L2 tools probe for supported ioctls, arg may not point to
	 * a valid ioctl_data structure. Return silently for unsupported commands.
	 */
	if ((cmd != CMD_GET) && (cmd != CMD_SET)) {
		return -ENOIOCTLCMD;  /* Translated to -ENOTTY by V4L2 core */
	}

	/* SECURITY FIX: Copy from userspace - prevents direct pointer dereference */
	if (copy_from_user(&ioctl_data_kernel, (struct ioctl_data __user *)arg,
			   sizeof(struct ioctl_data))) {
		dev_err(&client->dev, "Failed to copy ioctl data from userspace\n");
		return -EFAULT;
	}

	/* SECURITY FIX: Validate data pointer */
	if (ioctl_data_kernel.data == NULL) {
		dev_err(&client->dev, "NULL data pointer in ioctl\n");
		return -EINVAL;
	}

	/* SECURITY FIX: Validate transfer length to prevent integer overflow */
	if (ioctl_data_kernel.wLength == 0 ||
	    ioctl_data_kernel.wLength > MAX_I2C_TRANSFER_SIZE) {
		dev_err(&client->dev,
			"Invalid I2C transfer length: %u (max %d)\n",
			ioctl_data_kernel.wLength, MAX_I2C_TRANSFER_SIZE);
		return -EINVAL;
	}

	/* SECURITY FIX: Validate register address */
	if (ioctl_data_kernel.wIndex > 0xFFFF) {
		dev_err(&client->dev, "Invalid I2C register address: 0x%x\n",
			ioctl_data_kernel.wIndex);
		return -EINVAL;
	}

	dev_dbg(&client->dev, "rs300 ioctl: cmd=%d reg=0x%x len=%u\n",
		 cmd, ioctl_data_kernel.wIndex, ioctl_data_kernel.wLength);

	switch (cmd) {
	case CMD_GET:
		/* SECURITY FIX: Allocate kernel buffer with NULL check */
		data = kmalloc(ioctl_data_kernel.wLength, GFP_KERNEL);
		if (!data) {
			dev_err(&client->dev,
				"Failed to allocate %u byte transfer buffer\n",
				ioctl_data_kernel.wLength);
			return -ENOMEM;
		}

		/* Read I2C registers into kernel buffer */
		ret = read_regs(client, ioctl_data_kernel.wIndex, data,
				ioctl_data_kernel.wLength);
		if (ret) {
			dev_err(&client->dev, "I2C read failed: %ld\n", ret);
			kfree(data);
			return ret;
		}

		/* Copy kernel buffer to userspace */
		if (copy_to_user(ioctl_data_kernel.data, data,
				 ioctl_data_kernel.wLength)) {
			dev_err(&client->dev, "Failed to copy data to userspace\n");
			kfree(data);
			return -EFAULT;
		}

		kfree(data);
		break;

	case CMD_SET:
		/* SECURITY FIX: Allocate kernel buffer and copy from userspace */
		data = kmalloc(ioctl_data_kernel.wLength, GFP_KERNEL);
		if (!data) {
			dev_err(&client->dev,
				"Failed to allocate %u byte transfer buffer\n",
				ioctl_data_kernel.wLength);
			return -ENOMEM;
		}

		/* Save userspace pointer before copying */
		user_data_ptr = ioctl_data_kernel.data;

		/* Copy data from userspace to kernel buffer */
		if (copy_from_user(data, user_data_ptr, ioctl_data_kernel.wLength)) {
			dev_err(&client->dev, "Failed to copy data from userspace\n");
			kfree(data);
			return -EFAULT;
		}

		/* Write kernel buffer to I2C registers */
		ret = write_regs(client, ioctl_data_kernel.wIndex, data,
				 ioctl_data_kernel.wLength);
		if (ret) {
			dev_err(&client->dev, "I2C write failed: %ld\n", ret);
		}

		kfree(data);
		break;

	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
static void rs300_reset_colorspace(struct v4l2_mbus_framefmt *fmt)
{
	switch (fmt->code) {
	case MEDIA_BUS_FMT_Y16_1X16:
		/* Monochrome format - raw colorspace for thermal data */
		fmt->colorspace = V4L2_COLORSPACE_RAW;
		fmt->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
		fmt->quantization = V4L2_QUANTIZATION_FULL_RANGE;
		fmt->xfer_func = V4L2_XFER_FUNC_NONE;
		break;
	case MEDIA_BUS_FMT_YUYV8_1X16:
	case MEDIA_BUS_FMT_UYVY8_1X16:
	case MEDIA_BUS_FMT_YUYV8_2X8:
	case MEDIA_BUS_FMT_UYVY8_2X8:
		/* YUV formats - video colorspace for ISP processing */
		fmt->colorspace = V4L2_COLORSPACE_SMPTE170M;
		fmt->ycbcr_enc = V4L2_YCBCR_ENC_601;
		fmt->quantization = V4L2_QUANTIZATION_LIM_RANGE;
		fmt->xfer_func = V4L2_XFER_FUNC_709;
		break;
	default:
		/* Default to raw colorspace */
		fmt->colorspace = V4L2_COLORSPACE_RAW;
		fmt->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
		fmt->quantization = V4L2_QUANTIZATION_FULL_RANGE;
		fmt->xfer_func = V4L2_XFER_FUNC_NONE;
		break;
	}
}

/* Calculate pixel rate based on format */
static u64 rs300_get_pixel_rate(u32 format_code)
{
	switch (format_code) {
	case MEDIA_BUS_FMT_Y16_1X16:
		/* 16-bit monochrome format */
		return RS300_PIXEL_RATE_16BIT;
	case MEDIA_BUS_FMT_YUYV8_1X16:
	case MEDIA_BUS_FMT_UYVY8_1X16:
		/* 16-bit packed YUV formats require higher pixel rate */
		return RS300_PIXEL_RATE_16BIT;
	case MEDIA_BUS_FMT_YUYV8_2X8:
	case MEDIA_BUS_FMT_UYVY8_2X8:
		/* 8-bit dual lane formats use base pixel rate */
		return RS300_PIXEL_RATE;
	default:
		/* Default to 16-bit rate for unknown formats */
		return RS300_PIXEL_RATE_16BIT;
	}
}

static void rs300_set_default_format(struct rs300 *rs300)
{
    struct v4l2_mbus_framefmt *fmt;
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    
    dev_dbg(&client->dev, "rs300_set_default_format");
    
    /* Initialize the default format */
    fmt = &rs300->fmt;
    fmt->code = supported_modes[mode].code;
    fmt->width = supported_modes[mode].width;
    fmt->height = supported_modes[mode].height;
    fmt->field = V4L2_FIELD_NONE;
    rs300_reset_colorspace(fmt);
    
    /* Set the default mode */
    rs300->mode = &supported_modes[mode];
    
    dev_dbg(&client->dev, "Default format set: code=0x%x, %dx%d",
        fmt->code, fmt->width, fmt->height);
}	

/*
 * V4L2 subdev video and pad level operations
 */
static int rs300_set_test_pattern(struct rs300 *rs300, int value)
{
	return 0;
}

/*
 * NOTE: Code duplication in command functions (2025-10-24)
 *
 * The functions below contain ~500 lines of repetitive I2C command execution code.
 * Consolidation was attempted but ABANDONED due to mysterious kernel crashes when
 * modifying rs300_send_command() signature (ARM64 ABI issue with 8+ parameters).
 *
 * RECOMMENDATION: Leave as-is. Working code > Pretty code in kernel drivers.
 */

/* Function to get the current brightness value from the camera */
static int rs300_get_brightness(struct rs300 *rs300, int *brightness_value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    u8 result_buffer[18];  /* Buffer to hold the result data */
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    
    dev_dbg(&client->dev, "Getting current brightness value from camera");
    
    /* Construct the command buffer for GET brightness based on the example */
    cmd_buffer[0] = 0x10;  /* Command Class */
    cmd_buffer[1] = 0x04;  /* Module Command Index */
    cmd_buffer[2] = 0x87;  /* SubCmd - 0x87 for GET brightness */
    cmd_buffer[3] = 0x00;  /* Reserved */
    cmd_buffer[4] = 0x01;  /* Parameter 1 - Based on your example */
    cmd_buffer[5] = 0x00;
    cmd_buffer[6] = 0x00;
    cmd_buffer[7] = 0x00;
    cmd_buffer[8] = 0x00;
    cmd_buffer[9] = 0x00;
    cmd_buffer[10] = 0x00;
    cmd_buffer[11] = 0x00;
    cmd_buffer[12] = 0x01;  /* Parameter 9 - Based on your example */
    cmd_buffer[13] = 0x00;
    cmd_buffer[14] = 0x00;
    cmd_buffer[15] = 0x00;
    
    /* Calculate CRC/checksum for the command */
    unsigned short crc = do_crc(cmd_buffer, 16);
    /* Swap byte order to match the expected format */
    cmd_buffer[16] = crc & 0xFF;         /* Low byte of CRC first */
    cmd_buffer[17] = (crc >> 8) & 0xFF;  /* High byte of CRC second */
    
    dev_dbg(&client->dev, "Get brightness command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* STEP 1: Write command to command buffer register (0x1d00) */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write get brightness command: %d", ret);
        return ret;
    }
    
    /* STEP 2: Read command status and wait for completion */
    while (retry_count < max_retries) {
        /* Wait for command processing */
        msleep(200);
        
        /* Read command status from status register (0x0200) */
        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read command status: %d", ret);
            return ret;
        }
        
        /* Check command status */
        dev_dbg(&client->dev, "Get brightness command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  /* Bits 2-7 */
        
        /* Check if command is still busy */
        if (is_busy) {
            dev_dbg(&client->dev, "Get brightness command is busy, retrying...");
            retry_count++;
            continue;
        }
        
        /* Check for errors */
        if (has_failed) {
            /* Bit 1 set: Command execution failed */
            dev_err(&client->dev, "Get brightness command execution failed with error code: 0x%02X", error_code);
            
            /* Interpret error code */
            switch (error_code) {
            case 0x00:
                dev_dbg(&client->dev, "Error: Correct");
                break;
            case 0x01:
                dev_err(&client->dev, "Error: Length");
                break;
            case 0x02:
                dev_err(&client->dev, "Error: Unknown instruction");
                break;
            case 0x03:
                dev_err(&client->dev, "Error: Hardware error");
                break;
            case 0x04:
                dev_err(&client->dev, "Error: Unknown instruction (not yet enabled)");
                break;
            case 0x05:
            case 0x06:
            case 0x07:
                dev_err(&client->dev, "Error: CRC check error");
                break;
            default:
                dev_err(&client->dev, "Error: Unknown error code");
                break;
            }
            
            return -EIO;
        }
        
        /* Even if not failed, check if there's an error code */
        if (error_code != 0) {
            dev_warn(&client->dev, "Get brightness command completed but with error code: 0x%02X", error_code);
        }
        
        /* STEP 3: Command executed successfully, now read the result from 0x1d00 */
        ret = read_regs(client, 0x1d00, result_buffer, sizeof(result_buffer));
        if (ret) {
            dev_err(&client->dev, "Failed to read brightness result: %d", ret);
            return ret;
        }
        
        dev_dbg(&client->dev, "Brightness result buffer: %*ph", (int)sizeof(result_buffer), result_buffer);

        dev_dbg(&client->dev, "Result breakdown:");
        dev_dbg(&client->dev, "  [0-3] Cmd header: %02X %02X %02X %02X",
                 result_buffer[0], result_buffer[1], result_buffer[2], result_buffer[3]);
        dev_dbg(&client->dev, "  [4-7] P1-P4:     %02X %02X %02X %02X (P1=%d)",
                 result_buffer[4], result_buffer[5], result_buffer[6], result_buffer[7], result_buffer[4]);
        dev_dbg(&client->dev, "  [8-11] P5-P8:    %02X %02X %02X %02X",
                 result_buffer[8], result_buffer[9], result_buffer[10], result_buffer[11]);
        dev_dbg(&client->dev, "  [12-15] P9-P12:  %02X %02X %02X %02X (P12=%d)",
                 result_buffer[12], result_buffer[13], result_buffer[14], result_buffer[15], result_buffer[15]);
        dev_dbg(&client->dev, "  [16-17] CRC:     %02X %02X",
                 result_buffer[16], result_buffer[17]);

        dev_dbg(&client->dev, "Byte position candidates:");
        dev_dbg(&client->dev, "  byte[4]  (P1)  = %d (0x%02X) - current assumption",
                 result_buffer[4], result_buffer[4]);
        dev_dbg(&client->dev, "  byte[5]  (P2)  = %d (0x%02X)",
                 result_buffer[5], result_buffer[5]);
        dev_dbg(&client->dev, "  byte[12] (P9)  = %d (0x%02X)",
                 result_buffer[12], result_buffer[12]);
        dev_dbg(&client->dev, "  byte[13] (P10) = %d (0x%02X)",
                 result_buffer[13], result_buffer[13]);
        dev_dbg(&client->dev, "  byte[14] (P11) = %d (0x%02X)",
                 result_buffer[14], result_buffer[14]);
        dev_dbg(&client->dev, "  byte[15] (P12) = %d (0x%02X)",
                 result_buffer[15], result_buffer[15]);

        /* Based on the command structure, the brightness value should be in byte 4 */
        *brightness_value = result_buffer[4];

        dev_dbg(&client->dev, "Current brightness value: %d (0x%02X)", *brightness_value, *brightness_value);
        return 0;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "Get brightness command timed out after %d retries", max_retries);
    return -ETIMEDOUT;
}

static int rs300_set_dde(struct rs300 *rs300, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    dev_dbg(&client->dev, "Setting DDE to %d", value);

    /* Validate value range */
    if (value < 0 || value > 100) {
        dev_err(&client->dev, "Invalid DDE value: %d (valid range: 0-100)", value);
        return -EINVAL;
    }

    /* Pack parameters */
    params[0] = value;

    return rs300_send_command(rs300, 0x10, 0x04, 0x45, params, 1, 500);
}

static int rs300_set_output_mode(struct rs300 *rs300, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];  /* Standard 18-byte I2C command format */
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;

    dev_dbg(&client->dev, "Setting output mode to %d", value);

    /* Validate value range (0=YUV, 1=Y16; values 2-5 reserved) */
    if (value < 0 || value > 5) {
        dev_err(&client->dev, "Invalid output mode value: %d (valid: 0-5)", value);
        return -EINVAL;
    }

    /* Construct the command buffer for output mode selection (standard 18-byte format) */
    cmd_buffer[0] = 0x10;  /* Command Class: Camera control */
    cmd_buffer[1] = 0x10;  /* Module Index: MIPI interface */
    cmd_buffer[2] = 0x45;  /* SubCmd: Output source selection */
    cmd_buffer[3] = 0x00;  /* Reserved - MUST be 0x00 per I2C protocol */
    cmd_buffer[4] = value; /* Output mode: 0=YUV (8-bit), 1=Y16 (raw 16-bit) */

    /* Fill remaining parameters with zeros (bytes 5-15) */
    memset(&cmd_buffer[5], 0, 11);

    /* Use pre-calculated CRC values for each mode (hardcoded per I2C_QUICK_REFERENCE.md) */
    /* CRC lookup table: [mode][low_byte, high_byte] */
    static const u8 mode_crc[6][2] = {
        {0xFB, 0xC0},  /* Mode 0: YUV (8-bit processed) */
        {0x8E, 0xC3},  /* Mode 1: Y16 (raw 16-bit thermal) */
        {0x11, 0xC6},  /* Mode 2: reserved */
        {0x64, 0xC5},  /* Mode 3: reserved */
        {0x2F, 0xCD},  /* Mode 4: reserved */
        {0x5A, 0xCE},  /* Mode 5: reserved */
    };

    cmd_buffer[16] = mode_crc[value][0];  /* CRC low byte */
    cmd_buffer[17] = mode_crc[value][1];  /* CRC high byte */

    dev_dbg(&client->dev, "Output mode command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);

    /* Write command */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write output mode command: %d", ret);
        return ret;
    }

    /* Wait for completion */
    while (retry_count < max_retries) {
        msleep(50);

        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read status: %d", ret);
            return ret;
        }

        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;

        if (!is_busy) {
            if (has_failed) {
                dev_err(&client->dev, "Output mode command failed with error code: 0x%02X", error_code);
                return -EIO;
            }
            dev_dbg(&client->dev, "Output mode set successfully to %d", value);
            return 0;
        }

        retry_count++;
    }

    dev_err(&client->dev, "Output mode command timed out");
    return -ETIMEDOUT;
}

static int rs300_set_yuv_format(struct rs300 *rs300, int format)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    
    dev_dbg(&client->dev, "Setting YUV format to %d (0=UYVY, 1=VYUY, 2=YUYV, 3=YVYU)", format);
    
    /* Validate format range */
    if (format < 0 || format > 3) {
        dev_err(&client->dev, "Invalid YUV format: %d (valid range: 0-3)", format);
        return -EINVAL;
    }
    
    /* Construct the YUV format command buffer based on CSV data */
    cmd_buffer[0] = 0x10;  /* Command Class */
    cmd_buffer[1] = 0x03;  /* Module Command Index */
    cmd_buffer[2] = 0x4D;  /* SubCmd - YUV format setting */
    cmd_buffer[3] = 0x00;  /* Reserved */
    cmd_buffer[4] = format; /* Parameter: 0=UYVY, 1=VYUY, 2=YUYV, 3=YVYU */
    
    /* Fill remaining parameters with zeros */
    memset(&cmd_buffer[5], 0, 11);
    
    /* Calculate CRC */
    unsigned short crc = do_crc(cmd_buffer, 16);
    cmd_buffer[16] = crc & 0xFF;
    cmd_buffer[17] = (crc >> 8) & 0xFF;
    
    dev_dbg(&client->dev, "YUV format command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* Write command */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write YUV format command: %d", ret);
        return ret;
    }
    
    /* Wait for completion with timeout and retry */
    while (retry_count < max_retries) {
        msleep(50);
        
        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read status: %d", ret);
            return ret;
        }
        
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;
        
        if (!is_busy) {
            if (has_failed) {
                dev_err(&client->dev, "YUV format command failed with error code: 0x%02X", error_code);
                return -EIO;
            }
            dev_dbg(&client->dev, "YUV format set to %d successfully", format);
            return 0;
        }
        
        retry_count++;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "YUV format command timed out after %d retries", max_retries);
    return -ETIMEDOUT;
}

/* Anti-burn Protection SET command (0x10/0x03/0x4B with hardcoded CRC) */
static int rs300_set_antiburn(struct rs300 *rs300, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;

    dev_dbg(&client->dev, "Setting anti-burn protection: %s", enable ? "ON" : "OFF");

    if (enable != 0 && enable != 1) {
        dev_err(&client->dev, "Invalid anti-burn value: %d (valid: 0 or 1)", enable);
        return -EINVAL;
    }

    /* Construct anti-burn protection command packet */
    cmd_buffer[0] = 0x10;  /* Class: Camera control */
    cmd_buffer[1] = 0x03;  /* Module: Image quality */
    cmd_buffer[2] = 0x4B;  /* SubCmd: Anti-burn protection SET */
    cmd_buffer[3] = 0x00;  /* Reserved */
    cmd_buffer[4] = enable;  /* P1: 0=OFF, 1=ON */

    memset(&cmd_buffer[5], 0, 11);  /* Fill P2-P12 with zeros */

    /* Hardcoded CRC lookup table (from CSV rows 42-43) */
    static const u8 antiburn_crc[2][2] = {
        {0x58, 0x8D},  /* OFF (enable=0) CRC LSB, MSB */
        {0x2D, 0x8E},  /* ON  (enable=1) CRC LSB, MSB */
    };

    cmd_buffer[16] = antiburn_crc[enable][0];
    cmd_buffer[17] = antiburn_crc[enable][1];

    dev_dbg(&client->dev, "Anti-burn command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);

    /* Write command */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write anti-burn command: %d", ret);
        return ret;
    }

    /* Poll status register until ready */
    while (retry_count < max_retries) {
        msleep(50);

        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read status: %d", ret);
            return ret;
        }

        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;

        if (!is_busy) {
            if (has_failed) {
                dev_err(&client->dev, "Anti-burn command failed with error code: 0x%02X", error_code);
                return -EIO;
            }
            dev_dbg(&client->dev, "Anti-burn protection set successfully to %s", enable ? "ON" : "OFF");
            return 0;
        }

        retry_count++;
    }

    dev_err(&client->dev, "Anti-burn command timed out");
    return -ETIMEDOUT;
}

/* Shutter Control SET command (0x01/0x0F/0x45 with hardcoded CRC) */
static int rs300_set_shutter(struct rs300 *rs300, int state)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;

    dev_dbg(&client->dev, "Setting shutter to %s", state ? "OPEN" : "CLOSED");

    if (state != 0 && state != 1) {
        dev_err(&client->dev, "Invalid shutter state: %d (valid: 0=close, 1=open)", state);
        return -EINVAL;
    }

    /* Construct shutter control command packet */
    cmd_buffer[0] = 0x01;  /* Class: Device control (not standard 0x10) */
    cmd_buffer[1] = 0x0F;  /* Module: Shutter control */
    cmd_buffer[2] = 0x45;  /* SubCmd: Shutter open/close */
    cmd_buffer[3] = 0x00;  /* Reserved */
    cmd_buffer[4] = state;  /* P1: 0=CLOSE, 1=OPEN */

    memset(&cmd_buffer[5], 0, 11);  /* Fill P2-P12 with zeros */

    /* Hardcoded CRC lookup table (from CSV rows 4-5) */
    static const u8 shutter_crc[2][2] = {
        {0x8D, 0x5A},  /* CLOSE (state=0) CRC LSB, MSB */
        {0xF8, 0x59},  /* OPEN  (state=1) CRC LSB, MSB */
    };

    cmd_buffer[16] = shutter_crc[state][0];
    cmd_buffer[17] = shutter_crc[state][1];

    dev_dbg(&client->dev, "Shutter command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);

    /* Write command */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write shutter command: %d", ret);
        return ret;
    }

    /* Poll status register until ready */
    while (retry_count < max_retries) {
        msleep(50);

        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read status: %d", ret);
            return ret;
        }

        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;

        if (!is_busy) {
            if (has_failed) {
                dev_err(&client->dev, "Shutter command failed with error code: 0x%02X", error_code);
                return -EIO;
            }
            dev_dbg(&client->dev, "Shutter set successfully to %s", state ? "OPEN" : "CLOSED");
            return 0;
        }

        retry_count++;
    }

    dev_err(&client->dev, "Shutter command timed out");
    return -ETIMEDOUT;
}

/* Hook Edge Position SET command (0x10/0x04/0x4E with dynamic CRC) */
static int rs300_set_hook_edge(struct rs300 *rs300, int position)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    dev_dbg(&client->dev, "Setting hook edge position to %d (0=No Hook, 1=1st Gear, 2=2 Levels)", position);

    if (position < 0 || position > 2) {
        dev_err(&client->dev, "Invalid hook edge position: %d (valid: 0-2)", position);
        return -EINVAL;
    }

    params[0] = position;

    /* Use dynamic CRC calculation via rs300_send_command */
    return rs300_send_command(rs300, 0x10, 0x04, 0x4E, params, 1, 500);
}

/* Detector Frame Rate SET command (0x10/0x10/0x44 with dynamic CRC) */
static int rs300_set_frame_rate(struct rs300 *rs300, int rate_index)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};
    u8 rate_value;

    /* Map menu index to frame rate parameter value */
    static const u8 frame_rate_values[] = {
        0x19,  /* Index 0: 25Hz */
        0x1E,  /* Index 1: 30Hz */
        0x32,  /* Index 2: 50Hz */
        0x3C,  /* Index 3: 60Hz */
    };

    if (rate_index < 0 || rate_index > 3) {
        dev_err(&client->dev, "Invalid frame rate index: %d (valid: 0-3)", rate_index);
        return -EINVAL;
    }

    rate_value = frame_rate_values[rate_index];
    dev_dbg(&client->dev, "Setting detector frame rate to index %d (value: 0x%02X)", rate_index, rate_value);

    params[0] = rate_value;

    /* Use dynamic CRC calculation via rs300_send_command with extended timeout for frame rate switching */
    return rs300_send_command(rs300, 0x10, 0x10, 0x44, params, 1, 2500);
}

/* Digital-Analog Output Format SET command (0x10/0x10/0x49 with hardcoded CRC) */
static int rs300_set_analog_output_fmt(struct rs300 *rs300, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;

    dev_dbg(&client->dev, "Setting digital-analog output format (enable: %d)", enable);

    if (enable != 0 && enable != 1) {
        dev_err(&client->dev, "Invalid analog output format value: %d", enable);
        return -EINVAL;
    }

    /* Construct digital-analog output format command packet */
    cmd_buffer[0] = 0x10;  /* Class: Camera control */
    cmd_buffer[1] = 0x10;  /* Module: MIPI interface */
    cmd_buffer[2] = 0x49;  /* SubCmd: Digital-analog output format */
    cmd_buffer[3] = 0x00;  /* Reserved */

    /* All parameters are 0x00 for this command (single fixed config) */
    memset(&cmd_buffer[4], 0, 12);

    /* Hardcoded CRC for this command (from CSV row 89) */
    cmd_buffer[16] = 0x35;  /* CRC LSB */
    cmd_buffer[17] = 0xD6;  /* CRC MSB */

    dev_dbg(&client->dev, "Analog output format command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);

    /* Write command */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write analog output format command: %d", ret);
        return ret;
    }

    /* Poll status register until ready */
    while (retry_count < max_retries) {
        msleep(50);

        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read status: %d", ret);
            return ret;
        }

        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;

        if (!is_busy) {
            if (has_failed) {
                dev_err(&client->dev, "Analog output format command failed with error code: 0x%02X", error_code);
                return -EIO;
            }
            dev_dbg(&client->dev, "Analog output format set successfully");
            return 0;
        }

        retry_count++;
    }

    dev_err(&client->dev, "Analog output format command timed out");
    return -ETIMEDOUT;
}

static int rs300_set_contrast(struct rs300 *rs300, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    dev_dbg(&client->dev, "Setting contrast to %d", value);

    /* Validate value range */
    if (value < 0 || value > 100) {
        dev_err(&client->dev, "Invalid contrast value: %d (valid range: 0-100)", value);
        return -EINVAL;
    }

    /* Pack parameters */
    params[0] = value;

    return rs300_send_command(rs300, 0x10, 0x04, 0x4A, params, 1, 500);
}

static int rs300_set_spatial_nr(struct rs300 *rs300, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    dev_dbg(&client->dev, "Setting spatial noise reduction to %d", value);

    /* Validate value range */
    if (value < 0 || value > 100) {
        dev_err(&client->dev, "Invalid spatial NR value: %d (valid range: 0-100)", value);
        return -EINVAL;
    }

    /* Pack parameters */
    params[0] = value;

    return rs300_send_command(rs300, 0x10, 0x04, 0x4B, params, 1, 500);
}

static int rs300_set_temporal_nr(struct rs300 *rs300, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    dev_dbg(&client->dev, "Setting temporal noise reduction to %d", value);

    /* Validate value range */
    if (value < 0 || value > 100) {
        dev_err(&client->dev, "Invalid temporal NR value: %d (valid range: 0-100)", value);
        return -EINVAL;
    }

    /* Pack parameters */
    params[0] = value;

    return rs300_send_command(rs300, 0x10, 0x04, 0x4C, params, 1, 500);
}

static int rs300_get_colormap(struct rs300 *rs300, int *colormap_value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    u8 result_buffer[18];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    int current_colormap;
    
    /* Construct the command buffer */
    cmd_buffer[0] = 0x10;             /* Command Class */
    cmd_buffer[1] = 0x03;             /* Module Command Index */
    cmd_buffer[2] = 0x85;             /* SubCmd */
    // 9 bytes of 0x00
    memset(&cmd_buffer[3], 0x00, 9);
    cmd_buffer[12] = 0x01;
    cmd_buffer[13] = 0x00;
    cmd_buffer[14] = 0x00;
    cmd_buffer[15] = 0x00;

    /* Calculate CRC/checksum for the command */
    unsigned short crc = do_crc(cmd_buffer, 16);
    /* Swap byte order to match the expected format */
    cmd_buffer[16] = crc & 0xFF;         /* Low byte of CRC first */
    cmd_buffer[17] = (crc >> 8) & 0xFF;  /* High byte of CRC second */
    
    dev_dbg(&client->dev, "Command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* STEP 1: Write command to command buffer register (0x1d00) */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write command: %d", ret);
        return ret;
    }

    /* STEP 2: Read command status and wait for completion */
    while (retry_count < max_retries) {
        /* Wait for command processing */
        msleep(50);
        
        /* Read command status from status register (0x0200) */
        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read command status: %d", ret);
            return ret;
        }
        
        /* Check command status */
        dev_dbg(&client->dev, "Command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  // Bits 2-7

        /* Check if command is still busy */
        if (is_busy) {
            dev_dbg(&client->dev, "Command is busy, retrying...");
            retry_count++;
            continue;
        }

        /* Check for errors */
        if (has_failed) {
            dev_err(&client->dev, "Command execution failed with error code: 0x%02X", error_code);
            return -EIO;
        }
        
        /* STEP 3: Command executed successfully, now read the result from 0x1d00 */
        ret = read_regs(client, 0x1d00, result_buffer, sizeof(result_buffer));
        if (ret) {
            dev_err(&client->dev, "Failed to read colormap result: %d", ret);
            return ret;
        }
        
        /* Based on the command structure, the colormap value should be in byte 4 */
        *colormap_value = result_buffer[4];
        
        dev_dbg(&client->dev, "Current colormap value: %d (0x%02X)", *colormap_value, *colormap_value);
        return 0;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "Get colormap command timed out after %d retries", max_retries);
    return -ETIMEDOUT;

}

static int rs300_set_colormap(struct rs300 *rs300, int colormap_value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};
    int ret;
    int current_colormap;

    dev_dbg(&client->dev, "Setting colormap to %d", colormap_value);

    /* Validate colormap value range */
    if (colormap_value < 0 || colormap_value > 11) {
        dev_err(&client->dev, "Invalid colormap value: %d (valid range: 0-11)",
                colormap_value);
        return -EINVAL;
    }

    /* Pack parameters */
    params[0] = 0x00;            /* Parameter 1 (0x00) */
    params[1] = colormap_value;  /* Parameter 2 (0-11) */

    /* Send command */
    ret = rs300_send_command(rs300, 0x10, 0x03, 0x45, params, 2, 500);
    if (ret)
        return ret;

    /* Verify colormap was set correctly */
    msleep(100);
    ret = rs300_get_colormap(rs300, &current_colormap);
    if (ret) {
        dev_warn(&client->dev, "Failed to get current colormap: %d", ret);
    } else {
        if (current_colormap == colormap_value) {
            dev_dbg(&client->dev, "Colormap successfully set and verified: %d", current_colormap);
        } else {
            dev_warn(&client->dev, "Colormap mismatch! Set: %d, Got: %d",
                     colormap_value, current_colormap);
        }
    }

    return 0;
}

static int rs300_shutter_cal(struct rs300 *rs300)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);

    dev_dbg(&client->dev, "Triggering shutter calibration (FFC)");

    /* FFC requires longer timeout due to physical shutter movement */
    return rs300_send_command(rs300, 0x10, 0x02, 0x43, NULL, 0, 5000);
}

static int rs300_brightness_correct(struct rs300 *rs300, int brightness_value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 brightness_param;
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    int current_brightness;
    
    /* Map 0-100 brightness to parameter values (simple linear mapping) */
    brightness_param = (brightness_value > 100) ? 0x64 : brightness_value;
    
    dev_dbg(&client->dev, "Setting brightness correctly to %d (param: 0x%02X)",
             brightness_value, brightness_param);
    
    /* Construct the command buffer */
    cmd_buffer[0] = 0x10;             /* Command Class */
    cmd_buffer[1] = 0x04;             /* Module Command Index */
    cmd_buffer[2] = 0x47;             /* SubCmd */
    cmd_buffer[3] = 0x00;             /* Reserved */
    cmd_buffer[4] = brightness_param; /* Parameter 1 (brightness) */
    
    /* Fill remaining parameters with zeros */
    memset(&cmd_buffer[5], 0, 11);
    
    /* Calculate CRC/checksum for the command */
    unsigned short crc = do_crc(cmd_buffer, 16);
    /* Swap byte order to match the expected format */
    cmd_buffer[16] = crc & 0xFF;         /* Low byte of CRC first */
    cmd_buffer[17] = (crc >> 8) & 0xFF;  /* High byte of CRC second */
    
    dev_dbg(&client->dev, "Command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* STEP 1: Write command to command buffer register (0x1d00) */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write command: %d", ret);
        return ret;
    }
    
    /* STEP 2: Read command status and wait for completion */
    while (retry_count < max_retries) {
        /* Wait for command processing */
        msleep(50);
        
        /* Read command status from status register (0x0200) */
        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read command status: %d", ret);
            return ret;
        }
        
        /* Check command status */
        dev_dbg(&client->dev, "Command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  // Bits 2-7
        
        /* Check if command is still busy */
        if (is_busy) {
            dev_dbg(&client->dev, "Command is busy, retrying...");
            retry_count++;
            continue;
        }
        
        /* Check for errors */
        if (has_failed) {
            /* Bit 1 set: Command execution failed */
            dev_err(&client->dev, "Command execution failed with error code: 0x%02X", error_code);
            
            /* Interpret error code */
            switch (error_code) {
            case 0x00:
                dev_dbg(&client->dev, "Error: Correct");
                break;
            case 0x01:
                dev_err(&client->dev, "Error: Length");
                break;
            case 0x02:
                dev_err(&client->dev, "Error: Unknown instruction");
                break;
            case 0x03:
                dev_err(&client->dev, "Error: Hardware error");
                break;
            case 0x04:
                dev_err(&client->dev, "Error: Unknown instruction (not yet enabled)");
                break;
            case 0x05:
            case 0x06:
            case 0x07:
                dev_err(&client->dev, "Error: CRC check error");
                break;
            default:
                dev_err(&client->dev, "Error: Unknown error code");
                break;
            }
            
            return -EIO;
        }
        
        /* After successful command execution */
        if (!has_failed && error_code == 0) {
            /* Wait a moment before getting the brightness */
            msleep(100);
            
            /* Get the current brightness to verify the change */
            ret = rs300_get_brightness(rs300, &current_brightness);
            if (ret) {
                dev_warn(&client->dev, "Failed to get current brightness: %d", ret);
            } else {
                if (current_brightness == brightness_param) {
                    dev_dbg(&client->dev, "Brightness successfully set and verified: %d", current_brightness);
                } else {
                    dev_warn(&client->dev, "Brightness mismatch! Set: 0x%02X, Got: 0x%02X", 
                             brightness_param, current_brightness);
                }
            }
        }
        
        return 0;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "Command timed out after %d retries", max_retries);
    return -ETIMEDOUT;
}

/* Add new function to handle zoom setting */
///TODO: Reduce fix zoom function. Trying to link to v4l2 zoom control but encountering issues.
static int rs300_set_zoom(struct rs300 *rs300, int zoom_level)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    dev_dbg(&client->dev, "Setting zoom to %dx", zoom_level);

    /* Validate zoom level */
    if (zoom_level < 1 || zoom_level > 8) {
        dev_err(&client->dev, "Invalid zoom level: %d (valid range: 1-8)", zoom_level);
        return -EINVAL;
    }

    /* Pack parameters */
    params[0] = 0x00;                /* Fixed value */
    params[1] = zoom_level * 10;     /* Convert zoom level to command value (10, 20, ... 80) */

    /* Note: Previously used hardcoded CRC values (0x06, 0x0A) - now properly calculated */
    /* Zoom uses class 0x01 instead of the standard 0x10 */
    return rs300_send_command(rs300, 0x01, 0x31, 0x42, params, 2, 500);
}

static int rs300_set_scene_mode(struct rs300 *rs300, int scene_mode_value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    dev_dbg(&client->dev, "Setting scene mode to %d", scene_mode_value);

    /* Validate scene mode value range */
    if (scene_mode_value < 0 || scene_mode_value > 9) {
        dev_err(&client->dev, "Invalid scene mode value: %d (valid range: 0-9)",
                scene_mode_value);
        return -EINVAL;
    }

    /* Pack parameters */
    params[0] = scene_mode_value;

    return rs300_send_command(rs300, 0x10, 0x04, 0x42, params, 1, 500);
}

/* Autoshutter control functions */
static int rs300_set_autoshutter(struct rs300 *rs300, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    dev_dbg(&client->dev, "Setting autoshutter: %s", enable ? "ON" : "OFF");

    /* Pack parameters: P1 = enable (0=off, 1=on) */
    params[0] = enable ? 0x01 : 0x00;

    /* Command: Class=0x10, Module=0x02, SubCmd=0x41 */
    return rs300_send_command(rs300, 0x10, 0x02, 0x41, params, 1, 500);
}

static int rs300_get_autoshutter(struct rs300 *rs300, int *value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};
    u8 result_buffer[18];
    int ret;

    dev_dbg(&client->dev, "Getting autoshutter state");

    /* Pack parameters: P9=0x01, Len=0x0001 */
    params[8] = 0x01;  /* P9 = 0x01 */
    /* Len field is in bytes 12-13, but rs300_send_command handles this */

    /* Command: Class=0x10, Module=0x02, SubCmd=0x81 */
    ret = rs300_send_command(rs300, 0x10, 0x02, 0x81, params, 9, 500);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to get autoshutter state: %d", ret);
        return ret;
    }

    /* Read result from camera (register 0x1d00 contains the result) */
    ret = read_regs(client, 0x1d00, result_buffer, 18);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read autoshutter result: %d", ret);
        return ret;
    }

    /* Extract result from buffer (P1 contains the state) */
    *value = result_buffer[4];  /* P1 is at byte 4 */

    dev_dbg(&client->dev, "Autoshutter state: %s", *value ? "ON" : "OFF");

    return 0;
}

static int rs300_set_autoshutter_params(struct rs300 *rs300, int param_type, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    dev_dbg(&client->dev, "Setting autoshutter param type %d to value %d",
             param_type, value);

    /* Validate parameter type */
    if (param_type < 0 || param_type > 2) {
        dev_err(&client->dev, "Invalid parameter type: %d (valid range: 0-2)",
                param_type);
        return -EINVAL;
    }

    /* Pack parameters:
     * P1[0] = param_type (0=temp threshold, 1=min interval, 2=max interval)
     * P1[2:1] = value (16-bit little-endian)
     */
    params[0] = param_type;
    params[1] = value & 0xFF;        /* Low byte */
    params[2] = (value >> 8) & 0xFF; /* High byte */

    /* Command: Class=0x10, Module=0x02, SubCmd=0x42 */
    return rs300_send_command(rs300, 0x10, 0x02, 0x42, params, 3, 500);
}

/* Camera sleep/wake control functions */
static int rs300_set_sleep(struct rs300 *rs300, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    dev_dbg(&client->dev, "Setting camera sleep: %s", enable ? "ON" : "OFF");

    /* Para1: 0x01 = sleep, 0x00 = wake */
    params[0] = enable ? 0x01 : 0x00;

    /* Command: Class=0x10, Module=0x10, SubCmd=0x48
     * After sleeping, video freezes and camera only responds to wake-up command.
     */
    return rs300_send_command(rs300, 0x10, 0x10, 0x48, params, 1, 500);
}

static int rs300_get_sleep(struct rs300 *rs300, int *value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    u8 result_buffer[18];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;

    dev_dbg(&client->dev, "Getting camera sleep state");

    /* Build GET command: Class=0x10, Module=0x10, SubCmd=0x88 */
    cmd_buffer[0] = 0x10;
    cmd_buffer[1] = 0x10;
    cmd_buffer[2] = 0x88;
    memset(&cmd_buffer[3], 0x00, 9);
    cmd_buffer[12] = 0x01;  /* Response length = 1 byte */
    cmd_buffer[13] = 0x00;
    cmd_buffer[14] = 0x00;
    cmd_buffer[15] = 0x00;

    /* Calculate CRC */
    unsigned short crc = do_crc(cmd_buffer, 16);
    cmd_buffer[16] = crc & 0xFF;
    cmd_buffer[17] = (crc >> 8) & 0xFF;

    /* Write command */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write get sleep command: %d", ret);
        return ret;
    }

    /* Poll for completion */
    while (retry_count < max_retries) {
        msleep(50);

        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read command status: %d", ret);
            return ret;
        }

        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;

        if (is_busy) {
            retry_count++;
            continue;
        }

        if (has_failed) {
            dev_err(&client->dev, "Get sleep command failed");
            return -EIO;
        }

        /* Read response from 0x1d00 */
        ret = read_regs(client, 0x1d00, result_buffer, sizeof(result_buffer));
        if (ret) {
            dev_err(&client->dev, "Failed to read sleep status result: %d", ret);
            return ret;
        }

        /* Sleep state is in byte 5 of response: 0x00=working, 0x01=sleeping */
        *value = result_buffer[5];
        dev_dbg(&client->dev, "Camera sleep state: %d (0=awake, 1=asleep)", *value);

        return 0;
    }

    dev_err(&client->dev, "Get sleep command timeout");
    return -ETIMEDOUT;
}

static int rs300_set_ctrl(struct v4l2_ctrl *ctrl)
{
    struct rs300 *rs300 =
        container_of(ctrl->handler, struct rs300, ctrl_handler);
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    int ret = 0;

    /* Add debug info */
    dev_dbg(&client->dev, "Setting control ID 0x%x to value %d\n",
            ctrl->id, ctrl->val);

    switch (ctrl->id) {
    case V4L2_CID_TEST_PATTERN:
        ret = rs300_set_test_pattern(rs300, ctrl->val);
        break;
    case V4L2_CID_BRIGHTNESS:
        ret = rs300_brightness_correct(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 1:
        /* This is our colormap selection control */
        ret = rs300_set_colormap(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 2:
        /* This is our FFC (Flat Field Correction) button */
        dev_dbg(&client->dev, "FFC trigger received\n");
        if (ctrl->val == 0) {
            ret = rs300_shutter_cal(rs300);
        }
        break;
    case V4L2_CID_ZOOM_ABSOLUTE:
        ret = rs300_set_zoom(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 3:
        /* This is our scene mode selection control */
        ret = rs300_set_scene_mode(rs300, ctrl->val);
        break;
    case V4L2_CID_CONTRAST:
        ret = rs300_set_contrast(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 4:  /* DDE */
        ret = rs300_set_dde(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 5:  /* Spatial NR */
        ret = rs300_set_spatial_nr(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 6:  /* Temporal NR */
        ret = rs300_set_temporal_nr(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 7:  /* Output Mode */
        ret = rs300_set_output_mode(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 8:  /* Autoshutter enable/disable */
        ret = rs300_set_autoshutter(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 9:  /* Autoshutter temperature threshold */
        ret = rs300_set_autoshutter_params(rs300, 0, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 10:  /* Autoshutter min interval */
        ret = rs300_set_autoshutter_params(rs300, 1, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 11:  /* Autoshutter max interval */
        ret = rs300_set_autoshutter_params(rs300, 2, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 12:  /* Camera sleep */
        dev_dbg(&client->dev, "Setting camera sleep: %s", ctrl->val ? "ON" : "OFF");
        ret = rs300_set_sleep(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 13:  /* Anti-burn Protection */
        ret = rs300_set_antiburn(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 14:  /* Shutter State */
        ret = rs300_set_shutter(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 15:  /* Hook Edge Position */
        ret = rs300_set_hook_edge(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 16:  /* Detector Frame Rate */
        ret = rs300_set_frame_rate(rs300, ctrl->val);
        break;
    case V4L2_CID_CUSTOM_BASE + 17:  /* Digital-Analog Output Format */
        ret = rs300_set_analog_output_fmt(rs300, ctrl->val);
        break;
    case V4L2_CID_EXPOSURE:
        /* Exposure control for libcamera compatibility */
        /* Thermal camera exposure is stored but not actively controlled via I2C */
        dev_dbg(&client->dev, "Exposure set to %d lines (stored for libcamera)", ctrl->val);
        ret = 0;  /* Success - value stored in control framework */
        break;
    default:
        dev_err(&client->dev, "Invalid control %d", ctrl->id);
        ret = -EINVAL;
    }

    return ret;
}

static const struct v4l2_ctrl_ops rs300_ctrl_ops = {
	.s_ctrl = rs300_set_ctrl,
};

static int rs300_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct rs300 *rs300 = to_rs300(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
	dev_dbg(&client->dev, "rs300_enum_mbus_code: pad=%d, index=%d", code->pad, code->index);
	
	if (code->pad >= NUM_PADS) {
		dev_err(&client->dev, "Invalid pad %d (max %d)", code->pad, NUM_PADS-1);
		return -EINVAL;
	}

	if (code->pad == IMAGE_PAD) {
		if (code->index >= ARRAY_SIZE(codes))
			return -EINVAL;

		mutex_lock(&rs300->mutex);
		code->code = codes[code->index];
		dev_dbg(&client->dev, "Returning format code[%d]: 0x%x (%s)",
			 code->index, code->code,
			 code->code == MEDIA_BUS_FMT_YUYV8_1X16 ? "YUYV8_1X16" :
			 code->code == MEDIA_BUS_FMT_UYVY8_1X16 ? "UYVY8_1X16" :
			 code->code == MEDIA_BUS_FMT_YUYV8_2X8 ? "YUYV8_2X8" :
			 code->code == MEDIA_BUS_FMT_UYVY8_2X8 ? "UYVY8_2X8" : "OTHER");
		mutex_unlock(&rs300->mutex);
	} else {
		dev_err(&client->dev, "Invalid pad %d", code->pad);
		return -EINVAL;
	}
	
	return 0;
}

static int rs300_enum_frame_sizes(struct v4l2_subdev *sd,
				   struct v4l2_subdev_state *sd_state,
				   struct v4l2_subdev_frame_size_enum *fse)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct rs300 *rs300 = to_rs300(sd);
	u32 code;

	if (fse->pad >= NUM_PADS)
		return -EINVAL;

	if (fse->pad == IMAGE_PAD) {
		/* Only enumerate modes supported by this physical hardware module */
		if (fse->index >= rs300->num_modes)
			return -EINVAL;

		mutex_lock(&rs300->mutex);
		code = rs300_get_format_code(rs300, fse->code);
		if (code != fse->code) {
			mutex_unlock(&rs300->mutex);
			return -EINVAL;
		}
		mutex_unlock(&rs300->mutex);

		/* Use filtered mode list - only the resolution this hardware supports */
		fse->min_width  = rs300->available_modes[fse->index].width;
		fse->max_width  = fse->min_width;
		fse->min_height = rs300->available_modes[fse->index].height;
		fse->max_height = fse->min_height;
	} else {
		return -EINVAL;
	}

	return 0;
}

static void rs300_update_image_pad_format(struct rs300 *rs300,
					   const struct rs300_mode *mode,
					   struct v4l2_subdev_format *fmt)
{
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.field = V4L2_FIELD_NONE;
	rs300_reset_colorspace(&fmt->format);
}

// Restore and fix __rs300_get_pad_fmt
static int __rs300_get_pad_fmt(struct rs300 *rs300,
                               struct v4l2_subdev_state *sd_state,
                               struct v4l2_subdev_format *fmt)
{
        struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
        struct v4l2_mbus_framefmt *try_fmt;
        if (fmt->pad >= NUM_PADS)
                return -EINVAL;

        dev_dbg(&client->dev, "rs300_get_pad_fmt: pad=%d, which=%d",
                fmt->pad, fmt->which);

        if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
                struct v4l2_subdev_format fmt_req = {
                        .which = V4L2_SUBDEV_FORMAT_TRY,
                        .pad = fmt->pad,
                };
                int ret = v4l2_subdev_get_fmt(&rs300->sd, sd_state, &fmt_req);
                if (ret < 0)
                        return ret;

                /* Copy the format from userspace (fmt->format) to that location (*try_fmt) */
                try_fmt = &fmt_req.format;
                *try_fmt = fmt->format;

		dev_dbg(&client->dev, "Get TRY format: code=0x%x, %dx%d",
			fmt->format.code, fmt->format.width, fmt->format.height);
	} else {
		/* Return the active format */
		if (fmt->pad == IMAGE_PAD) {
			fmt->format = rs300->fmt;

			/* Translate format based on output_mode for CSI2 compatibility
			 * output_mode 0 = YUYV8_1X16 (YUV 8-bit)
			 * output_mode 1 = Y16_1X16 (raw 16-bit thermal)
			 */
			if (rs300->output_mode && rs300->output_mode->cur.val == 0) {
				fmt->format.code = MEDIA_BUS_FMT_YUYV8_2X8;
				dev_dbg(&client->dev, "Get ACTIVE format: code=0x%x (YUYV8_2X8, output_mode=0), %dx%d",
					fmt->format.code, fmt->format.width, fmt->format.height);
			} else {
				fmt->format.code = MEDIA_BUS_FMT_Y16_1X16;
				dev_dbg(&client->dev, "Get ACTIVE format: code=0x%x (Y16_1X16, output_mode=%d), %dx%d",
					fmt->format.code, rs300->output_mode ? rs300->output_mode->cur.val : 1,
					fmt->format.width, fmt->format.height);
			}

			// Debug current active mode
			if (rs300->mode) {
				dev_dbg(&client->dev, "Current active mode: %dx%d @ %d/%d fps",
					rs300->mode->width, rs300->mode->height,
					rs300->mode->max_fps.denominator, rs300->mode->max_fps.numerator);
			} else {
				dev_dbg(&client->dev, "No active mode set yet");
			}
		} else {
			dev_err(&client->dev, "Invalid pad %d", fmt->pad);
			return -EINVAL;
		}
	}
	return 0;
}

// Fix rs300_get_pad_fmt to call __rs300_get_pad_fmt
static int rs300_get_pad_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *sd_state,
			  struct v4l2_subdev_format *fmt)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct rs300 *rs300 = to_rs300(sd);
	int ret;
	
	mutex_lock(&rs300->mutex);
	ret = __rs300_get_pad_fmt(rs300, sd_state, fmt);
	mutex_unlock(&rs300->mutex);
	return ret;
}

// Fix rs300_set_pad_fmt to use v4l2_subdev_get_fmt
static int rs300_set_pad_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *sd_state,
			  struct v4l2_subdev_format *fmt)
{
	struct rs300 *rs300 = to_rs300(sd);
	const struct rs300_mode *mode;
	struct v4l2_mbus_framefmt *framefmt;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int i;

	if (fmt->pad >= NUM_PADS)
		return -EINVAL;

	mutex_lock(&rs300->mutex);

	dev_dbg(&client->dev, "rs300_set_pad_fmt input: pad=%d, which=%d, code=0x%x, width=%d, height=%d",
		fmt->pad, fmt->which, fmt->format.code, fmt->format.width, fmt->format.height);

	if (fmt->pad == IMAGE_PAD) {
		/* Find the closest supported format code */
		for (i = 0; i < ARRAY_SIZE(codes); i++)
			if (codes[i] == fmt->format.code)
				break;
		if (i >= ARRAY_SIZE(codes))
			i = 0; /* Default to first supported code if not found */

		fmt->format.code = rs300_get_format_code(rs300, codes[i]);

		/* Find the closest supported resolution */
		dev_dbg(&client->dev, "rs300_set_pad_fmt searching for nearest mode to %dx%d",
			fmt->format.width, fmt->format.height);

		/* Print all supported modes for debugging (only shows hardware-supported mode) */
		for (i = 0; i < rs300->num_modes; i++) {
			dev_dbg(&client->dev, "Supported mode[%d]: %dx%d",
				i, rs300->available_modes[i].width, rs300->available_modes[i].height);
		}

		/*
		 * Use filtered mode list - only one mode for this hardware.
		 * Since num_modes=1, v4l2_find_nearest_size will always return the single mode.
		 * This prevents libcamera from attempting unsupported resolution changes.
		 */
		mode = v4l2_find_nearest_size(rs300->available_modes,
					      rs300->num_modes,
					      width, height,
					      fmt->format.width, fmt->format.height);

		/* Update the format with the selected mode */
		dev_dbg(&client->dev, "rs300_set_pad_fmt selected mode: width=%d, height=%d",
			mode->width, mode->height);

		rs300_update_image_pad_format(rs300, mode, fmt);

                if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
                        /* Just update the try format */
                        struct v4l2_subdev_format fmt_req = {
                                .which = V4L2_SUBDEV_FORMAT_TRY,
                                .pad = fmt->pad,
                        };
                        int ret = v4l2_subdev_get_fmt(sd, sd_state, &fmt_req);
                        if (ret < 0) {
                                mutex_unlock(&rs300->mutex);
                                return ret;
                        }
                        framefmt = &fmt_req.format;
                        *framefmt = fmt->format;
			dev_dbg(&client->dev, "Set TRY format: code=0x%x, %dx%d",
				framefmt->code, framefmt->width, framefmt->height);
		} else {
			/* Update the active format and mode */
			rs300->fmt = fmt->format;
			rs300->mode = mode;
			
			/* Update pixel rate control based on new format */
			if (rs300->pixel_rate) {
				u64 new_pixel_rate = rs300_get_pixel_rate(rs300->fmt.code);
				__v4l2_ctrl_s_ctrl_int64(rs300->pixel_rate, new_pixel_rate);
				dev_dbg(&client->dev, "Updated pixel rate to %llu for format 0x%x",
					 new_pixel_rate, rs300->fmt.code);
			}
			
			dev_info(&client->dev, "Set ACTIVE format: code=0x%x, %dx%d",
				rs300->fmt.code, rs300->fmt.width, rs300->fmt.height);
		}
	} else {
		dev_err(&client->dev, "Invalid pad %d", fmt->pad);
		mutex_unlock(&rs300->mutex);
		return -EINVAL;
	}

	mutex_unlock(&rs300->mutex);
	return 0;
}

static int rs300_set_framefmt(struct rs300 *rs300)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    
    dev_dbg(&client->dev, "Setting frame format: code=0x%x, %dx%d",
             rs300->fmt.code, rs300->fmt.width, rs300->fmt.height);
    
    switch (rs300->fmt.code) {
    case MEDIA_BUS_FMT_Y16_1X16:
        dev_dbg(&client->dev, "Using Y16_1X16 format (16-bit monochrome for thermal data)");
        /* Y16 monochrome format - raw 16-bit thermal data */
        return 0;
    case MEDIA_BUS_FMT_YUYV8_1X16:
        dev_dbg(&client->dev, "Using YUYV8_1X16 format (16-bit packed, preferred for RP1-CFE)");
        /* 16-bit packed format - should be compatible with Pi 5 RP1-CFE */
        return 0;
    case MEDIA_BUS_FMT_UYVY8_1X16:
        dev_dbg(&client->dev, "Using UYVY8_1X16 format (16-bit packed alternative)");
        return 0;
    case MEDIA_BUS_FMT_YUYV8_2X8:
        dev_dbg(&client->dev, "Using YUYV8_2X8 format (8-bit dual lane, legacy)");
        return 0;
    case MEDIA_BUS_FMT_UYVY8_2X8:
        dev_dbg(&client->dev, "Using UYVY8_2X8 format (8-bit dual lane, legacy)");
        return 0;
    default:
        dev_err(&client->dev, "Unsupported format code: 0x%x", rs300->fmt.code);
        dev_err(&client->dev, "Supported formats: Y16_1X16(0x%x), YUYV8_1X16(0x%x), UYVY8_1X16(0x%x), YUYV8_2X8(0x%x), UYVY8_2X8(0x%x)",
                MEDIA_BUS_FMT_Y16_1X16, MEDIA_BUS_FMT_YUYV8_1X16, MEDIA_BUS_FMT_UYVY8_1X16,
                MEDIA_BUS_FMT_YUYV8_2X8, MEDIA_BUS_FMT_UYVY8_2X8);
        return -EINVAL;
    }        
}

static void rs300_stop_streaming(struct rs300 *rs300)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);

    /* SECURITY FIX: Use local buffer instead of static global (prevents race condition) */
    u8 stop_regs[28] = {
        0x01, 0x30, 0xc2, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x0a, 0x00,
        0x00, 0x00, //crc [14]
        0x2F, 0x0D, //crc [16]
        0x01, //path [18]
        0x16, //src [19]
        0x00, //dst [20]
        0x3c, //fps [21]
        0x80, 0x02, //width&0xff, width>>8 [22-23]
        0x00, 0x02, //height&0xff, height>>8 [24-25]
        0x00, 0x00
    };

    dev_dbg(&client->dev, "Stopping streaming");

    /* Write stop registers */
    if (write_regs(client, I2C_VD_BUFFER_RW, stop_regs, sizeof(stop_regs)) < 0) {
        dev_err(&client->dev, "Error writing stop registers");
    }

    dev_dbg(&client->dev, "Streaming stopped");
}

static int rs300_set_fps(struct rs300 *rs300, int fps)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};
    int ret;

    /* Validate FPS value (25, 30, 50, or 60) */
    if (fps != 25 && fps != 30 && fps != 50 && fps != 60) {
        dev_warn(&client->dev, "Invalid FPS value: %d", fps);
        return 0;
    }

    dev_dbg(&client->dev, "Setting camera to %d fps", fps);

    /* Pack parameters */
    params[0] = 0x01;  /* Enable */
    params[1] = 0x03;  /* MIPI Progressive */
    params[2] = fps;   /* FPS value */

    /* FPS command needs longer timeout (4500ms total via 15 retries × 300ms) */
    ret = rs300_send_command(rs300, 0x10, 0x10, 0x46, params, 3, 4500);

    /* Note: FPS command always returns 0 even on error (legacy behavior) */
    if (ret)
        dev_warn(&client->dev, "FPS command failed: %d", ret);

    return 0;
}

static int rs300_set_stream(struct v4l2_subdev *sd, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct rs300 *rs300 = to_rs300(sd);
    unsigned short crcdata;
    u8 status_buffer[1];
    int ret = 0;

    dev_dbg(&client->dev, "rs300_set_stream: enable=%d, streaming=%d, fmt=0x%x %dx%d",
            enable, rs300->streaming, rs300->fmt.code, rs300->fmt.width, rs300->fmt.height);
    
    // Add detailed format info when streaming starts
    if (enable) {
        /*
         * IMPORTANT: Camera warm-up timing requirement
         *
         * The RS300 thermal camera requires approximately 2 seconds (60 frames at 30fps)
         * of warm-up time after stream start before outputting valid thermal data.
         *
         * - Frames 1-60: Initialization data (constant patterns like 0x36 0x80)
         * - Frame 60+: Valid thermal data (>1000 unique patterns)
         *
         * User-space applications should either:
         * 1. Capture 90+ frames and extract frame 60+ for processing
         * 2. Start stream, wait 2+ seconds, then begin capturing
         *
         * See documentation: ~/rs300-extra-documentation/test-reports/CAMERA_QUIRKS.txt
         */
        dev_dbg(&client->dev, "Stream start: fmt=0x%x %dx%d, mode=%dx%d @ %d/%d fps",
            rs300->fmt.code, rs300->fmt.width, rs300->fmt.height,
            rs300->mode->width, rs300->mode->height,
            rs300->mode->max_fps.denominator, rs300->mode->max_fps.numerator);
        dev_dbg(&client->dev, "Pixel rate: %llu Hz, colorspace: %d",
            rs300_get_pixel_rate(rs300->fmt.code), rs300->fmt.colorspace);
    } else {
        dev_dbg(&client->dev, "Stream stop requested");
    }

    mutex_lock(&rs300->mutex);
    if (rs300->streaming == enable) {
        dev_dbg(&client->dev, "Stream already in desired state");
        mutex_unlock(&rs300->mutex);
        return 0;
    }

    if (enable) {
        /* SECURITY FIX: Use local buffer instead of static global (prevents race condition) */
        u8 start_regs[28] = {
            0x01, 0x30, 0xc1, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x0a, 0x00,
            0x00, 0x00, //crc [14]
            0x00, 0x00, //crc [16]
            0x00, //path [18]
            0x16, //src [19]
            0x03, //dst [20]
            0x3c, // 60 fps [21]
            0x80, 0x02, //width&0xff, width>>8 [22-23]
            0x00, 0x02, //height&0xff, height>>8 [24-25]
            0x00, 0x00
        };

        /* Deferred YUV format config: set on first stream start when sensor is ready */
        if (!rs300->yuv_format_configured) {
            ret = rs300_set_yuv_format(rs300, 2); /* YUYV format */
            if (ret)
                dev_warn(&client->dev, "YUV format set failed: %d (continuing)", ret);
            else
                rs300->yuv_format_configured = true;
        }

        // Set FPS first
        ret = rs300_set_fps(rs300, fps);
        if (ret) {
            dev_err(&client->dev, "Failed to set camera to %d fps: %d", fps, ret);
            goto error_unlock;
        }
        dev_dbg(&client->dev, "FPS is set to %d", fps);

        /* SECURITY FIX: Don't set streaming flag yet - wait for hardware success */
        /* Removed: rs300->streaming = enable; (was set too early) */
        start_regs[19] = type;
        start_regs[21] = fps;  // Add this line to set the FPS from the module parameter
        start_regs[22] = rs300->mode->width & 0xff;
        start_regs[23] = rs300->mode->width >> 8;
        start_regs[24] = rs300->mode->height & 0xff;
        start_regs[25] = rs300->mode->height >> 8;

        dev_dbg(&client->dev, "Start registers before CRC: %*ph", (int)sizeof(start_regs), start_regs);

        //update crc
        crcdata = do_crc((uint8_t*)(start_regs+18), 10);
        start_regs[14] = crcdata & 0xff;
        start_regs[15] = crcdata >> 8;
        
        crcdata = do_crc((uint8_t*)(start_regs), 16);
        start_regs[16] = crcdata & 0xff;
        start_regs[17] = crcdata >> 8;
        
        dev_dbg(&client->dev, "Start registers after CRC: %*ph", (int)sizeof(start_regs), start_regs);
        dev_dbg(&client->dev, "Writing start registers to device");
        
        if (write_regs(client, I2C_VD_BUFFER_RW, start_regs, sizeof(start_regs)) < 0) {
            dev_err(&client->dev, "error start rs300\n");
            goto error_unlock;
        }

        // Read back the registers to verify they were written correctly
        u8 verify_regs[sizeof(start_regs)];
        if (read_regs(client, I2C_VD_BUFFER_RW, verify_regs, sizeof(verify_regs)) == 0) {
            dev_dbg(&client->dev, "Read back registers: %*ph", (int)sizeof(verify_regs), verify_regs);
            if (memcmp(start_regs, verify_regs, sizeof(start_regs)) != 0) {
                dev_err(&client->dev, "Register verification failed!");
            }
        }

        //check if device is ready
 

        ret = rs300_set_framefmt(rs300);
        if (ret) {
            dev_err(&client->dev, "error set framefmt\n");
            goto error_unlock;
        }
        
        dev_dbg(&client->dev, "Stream registers written successfully");

        // Retry loop to handle intermittent camera hardware errors
        // Poll timeouts escalate: 1.5s, 2.5s, 5.0s
        // 2s stabilization sleep ONLY on clean success
        // On final failure: warn but don't close stream (some modules
        // report errors even when the camera can still stream)
        #define STREAM_START_RETRIES 3
        static const int poll_timeout_ms[] = { 1500, 2500, 5000 };
        int stream_attempt;
        int stream_success = 0;

        for (stream_attempt = 0; stream_attempt < STREAM_START_RETRIES; stream_attempt++) {
            int retry = 0;
            int timeout_ms = poll_timeout_ms[stream_attempt];
            int max_retries = timeout_ms / 100;
            int got_error = 0;

            dev_dbg(&client->dev, "Attempt %d/%d - polling for %dms",
                     stream_attempt + 1, STREAM_START_RETRIES, timeout_ms);

            while (retry < max_retries) {
                ret = read_regs(client, I2C_VD_BUFFER_STATUS, status_buffer, 1);
                if (ret == 0) {
                    dev_dbg(&client->dev, "Attempt %d/%d - Status check %d: 0x%02x",
                             stream_attempt + 1, STREAM_START_RETRIES, retry, status_buffer[0]);

                    if (!(status_buffer[0] & VCMD_BUSY_STS_BIT) &&
                        !(status_buffer[0] & VCMD_ERR_STS_BIT)) {
                        dev_dbg(&client->dev, "Busy bit cleared, no error");
                        break;
                    }

                    // Reset bit failure is a hard error - don't retry
                    if (status_buffer[0] & VCMD_RST_STS_BIT) {
                        dev_err(&client->dev, "Camera reset failed (hard error)");
                        ret = -EIO;
                        goto error_unlock;
                    }

                    // Error bit during polling - skip sleep, go straight to retry
                    if (status_buffer[0] & VCMD_ERR_STS_BIT) {
                        dev_warn(&client->dev, "Camera error 0x%02x on attempt %d/%d",
                                 status_buffer[0], stream_attempt + 1, STREAM_START_RETRIES);
                        got_error = 1;
                        break;
                    }
                }

                msleep(100);
                retry++;
            }

            // Busy timeout is a hard error - don't retry
            if (retry >= max_retries && !got_error) {
                dev_err(&client->dev, "Camera remained busy after %dms (hard error)", timeout_ms);
                ret = -ETIMEDOUT;
                goto error_unlock;
            }

            // Error detected - skip stabilization sleep, retry immediately
            if (got_error) {
                if (stream_attempt < STREAM_START_RETRIES - 1) {
                    dev_warn(&client->dev, "Retrying stream start (attempt %d/%d)...",
                             stream_attempt + 2, STREAM_START_RETRIES);
                    if (write_regs(client, I2C_VD_BUFFER_RW, start_regs, sizeof(start_regs)) < 0) {
                        dev_err(&client->dev, "Failed to re-send start command");
                        ret = -EIO;
                        goto error_unlock;
                    }
                    continue;
                }

                // Final attempt failed - warn but don't close stream
                dev_err(&client->dev,
                        "Camera reported error after %d attempts (status: 0x%02x). "
                        "Stream may still be functional.",
                        STREAM_START_RETRIES, status_buffer[0]);
                break;
            }

            // Clean success - stabilization sleep only here
            msleep(stream_attempt < STREAM_START_RETRIES - 1 ? 2000 : 1000);
            dev_dbg(&client->dev, "Stream started successfully on attempt %d/%d",
                     stream_attempt + 1, STREAM_START_RETRIES);
            stream_success = 1;
            break;
        }

        // Set streaming flag regardless - camera may still be delivering frames
        // even if status register reported an error
        rs300->streaming = true;
        if (stream_success)
            dev_info(&client->dev, "Stream started successfully");
        else
            dev_warn(&client->dev, "Stream started with errors - check output");
    } else {
        dev_dbg(&client->dev, "Stopping stream");
        rs300_stop_streaming(rs300);
        rs300->streaming = false;
        dev_dbg(&client->dev, "Stream stopped, streaming flag cleared");
    }

    dev_dbg(&client->dev, "rs300_set_stream complete: streaming=%d, ret=%d",
            rs300->streaming, ret);
    mutex_unlock(&rs300->mutex);

    return ret;

error_unlock:
    dev_err(&client->dev, "=== STREAM ERROR EXIT: ret=%d ===", ret);
    /* Send STOP command to clean up camera state after failed START attempts */
    if (enable) {
        dev_info(&client->dev, "Sending STOP command to clean up after failed START");
        rs300_stop_streaming(rs300);
    }
    mutex_unlock(&rs300->mutex);
    return ret;
}

static const s64 link_freq_menu_items[] = {
	RS300_LINK_RATE,//80m
};
/* -----------------------------------------------------------------------------
 * V4L2 subdev internal operations
 */

/**
 * rs300_init_cfg - Initialize the pad format configuration
 * @sd: V4L2 subdevice
 * @state: V4L2 subdevice state
 *
 * Initialize the pad format with default values and the crop rectangle.
 * This function is called during driver initialization and when the device
 * is opened.
 */
static int rs300_init_cfg(struct v4l2_subdev *sd,
                           struct v4l2_subdev_state *state)
{
        struct i2c_client *client = v4l2_get_subdevdata(sd);
        struct v4l2_mbus_framefmt *format;
        int ret;

        dev_dbg(&client->dev, "rs300_init_cfg");

        /* Initialize the format for the image pad */
        {
                struct v4l2_subdev_format fmt_req = {
                        .which = V4L2_SUBDEV_FORMAT_ACTIVE,
                        .pad = IMAGE_PAD,
                };
                ret = v4l2_subdev_get_fmt(sd, state, &fmt_req);
                if (ret < 0)
                        return ret;
                format = &fmt_req.format;
        }
        format->code = supported_modes[mode].code;
        format->width = supported_modes[mode].width;
        format->height = supported_modes[mode].height;
        format->field = V4L2_FIELD_NONE;
        rs300_reset_colorspace(format);


	return 0;
}

static int rs300_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct rs300 *rs300 = to_rs300(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
    
    dev_dbg(&client->dev, "rs300_open");
	mutex_lock(&rs300->mutex);
	
	/* Initialize the format configuration */
	rs300_init_cfg(sd, fh->state);

	mutex_unlock(&rs300->mutex);
	return 0;
}


static int rs300_power_on(struct device *dev)
{
    struct v4l2_subdev *sd = dev_get_drvdata(dev);
    struct rs300 *rs300 = to_rs300(sd);
    int ret;

    dev_dbg(dev, "Powering on rs300");  
    
    ret = regulator_bulk_enable(rs300_NUM_SUPPLIES, rs300->supplies);
    if (ret) {
        dev_err(dev, "failed to enable regulators\n");
        return ret;
    }

    /* Reset sequence */
    /*gpiod_set_value_cansleep(rs300->reset_gpio, 1); // Assert reset
    msleep(100);  // Hold reset for 20ms
    gpiod_set_value_cansleep(rs300->reset_gpio, 0); // Release reset
    msleep(500);  // Wait 100ms for device to initialize after reset
*/
    dev_dbg(dev, "Power on complete");

    return 0;
}

static int rs300_power_off(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct rs300 *rs300 = to_rs300(sd);

	/* SECURITY FIX: Only access GPIO if initialized (prevents NULL deref on rmmod) */
	if (rs300->reset_gpio) {
		gpiod_set_value_cansleep(rs300->reset_gpio, 1); //logic high -> device tree defines reset: logic high = 0V (active low)
		dev_dbg(dev, "Resetting rs300");
	} else {
		dev_dbg(dev, "No reset GPIO configured, skipping reset");
	}

	regulator_bulk_disable(rs300_NUM_SUPPLIES, rs300->supplies);
	dev_dbg(dev, "Regulators disabled");

	return 0;
}

static int rs300_get_regulators(struct rs300 *rs300)
{
	struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
	unsigned int i;

	for (i = 0; i < rs300_NUM_SUPPLIES; i++)
		rs300->supplies[i].supply = rs300_supply_names[i];

	return devm_regulator_bulk_get(&client->dev, 
						rs300_NUM_SUPPLIES,
				       rs300->supplies);
}

static const struct v4l2_subdev_core_ops rs300_subdev_core_ops = {
	.log_status = v4l2_ctrl_subdev_log_status,
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
	.ioctl = rs300_ioctl, //NEEDED?
};

static const struct v4l2_subdev_video_ops rs300_subdev_video_ops = {
	.s_stream = rs300_set_stream,
};

static int rs300_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *sd_state,
			     struct v4l2_subdev_selection *sel)
{
	struct rs300 *rs300 = to_rs300(sd);
	const struct rs300_mode *mode;

	/* Validate pad */
	if (sel->pad >= NUM_PADS)
		return -EINVAL;

	if (sel->pad != IMAGE_PAD)
		return -EINVAL;

	mutex_lock(&rs300->mutex);
	mode = rs300->mode;

	/* Defensive check - mode should always be set during normal operation */
	if (!mode) {
		dev_err(sd->dev->parent, "get_selection: mode is NULL\n");
		mutex_unlock(&rs300->mutex);
		return -EINVAL;
	}

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP:
		/* All targets return active sensor dimensions */
		/* (Different physical sensors: 640×512, 384×288, 256×192) */
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = mode->width;
		sel->r.height = mode->height;
		mutex_unlock(&rs300->mutex);
		return 0;

	default:
		mutex_unlock(&rs300->mutex);
		return -EINVAL;
	}
}

static int rs300_set_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *sd_state,
			     struct v4l2_subdev_selection *sel)
{
	/* We don't support actual cropping, just return the full frame */
	return rs300_get_selection(sd, sd_state, sel);
}

static const struct v4l2_subdev_pad_ops rs300_subdev_pad_ops = {
	.enum_mbus_code = rs300_enum_mbus_code,
	.get_fmt = rs300_get_pad_fmt,
	.set_fmt = rs300_set_pad_fmt,
	.enum_frame_size = rs300_enum_frame_sizes,
	.get_selection = rs300_get_selection,
	.set_selection = rs300_set_selection,
};

static const struct v4l2_subdev_ops rs300_subdev_ops = {
	.core  = &rs300_subdev_core_ops,
	.video = &rs300_subdev_video_ops,
	.pad   = &rs300_subdev_pad_ops,
};

/*
static const struct v4l2_subdev_internal_ops rs300_subdev_internal_ops = {
	.open = rs300_open,
};
*/

static const struct v4l2_ctrl_config colormap_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 1,
    .name = "Colormap",
    .type = V4L2_CTRL_TYPE_MENU,
    .qmenu = colormap_menu,
    .min = 0,
    .max = 11,
    .def = 0,
};

static const struct v4l2_ctrl_config ffc_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 2,
    .name = "FFC Trigger",
    .type = V4L2_CTRL_TYPE_BUTTON,
    .min = 0,
    .max = 0,
    .step = 0,
    .def = 0,
};

static const struct v4l2_ctrl_config scene_mode_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 3,
    .name = "Scene Mode",
    .type = V4L2_CTRL_TYPE_MENU,
    .qmenu = scene_mode_menu,
    .min = 0,
    .max = 9,
    .def = 3,
};

static const struct v4l2_ctrl_config dde_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 4,
    .name = "Digital Detail Enhancement",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 0,
    .max = 100,
    .step = 1,
    .def = 50,
};

static const struct v4l2_ctrl_config spatial_nr_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 5,
    .name = "Spatial Noise Reduction",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 0,
    .max = 100,
    .step = 1,
    .def = 50,
};

static const struct v4l2_ctrl_config temporal_nr_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 6,
    .name = "Temporal Noise Reduction",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 0,
    .max = 100,
    .step = 1,
    .def = 50,
};

static const struct v4l2_ctrl_config autoshutter_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 8,
    .name = "Auto Shutter",
    .type = V4L2_CTRL_TYPE_BOOLEAN,
    .min = 0,
    .max = 1,
    .step = 1,  /* BOOLEAN needs step=1 (unlike BUTTON which uses step=0) */
    .def = 0,  /* Off by default */
};

static const struct v4l2_ctrl_config autoshutter_temp_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 9,
    .name = "Auto Shutter Temperature",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 0,    /* Start at 0 for V4L2 validation */
    .max = 100,  /* 3.12°C maximum */
    .step = 1,
    .def = 50,   /* 1.56°C default */
};

static const struct v4l2_ctrl_config autoshutter_min_interval_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 10,
    .name = "Auto Shutter Min Interval",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 0,    /* Start at 0 for V4L2 validation */
    .max = 300,
    .step = 1,
    .def = 1,    /* 1 second default */
};

static const struct v4l2_ctrl_config autoshutter_max_interval_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 11,
    .name = "Auto Shutter Max Interval",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 0,    /* Start at 0 for V4L2 validation */
    .max = 600,
    .step = 1,
    .def = 120,  /* 120 seconds default */
};

static const struct v4l2_ctrl_config camera_sleep_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 12,
    .name = "Camera Sleep",
    .type = V4L2_CTRL_TYPE_BOOLEAN,
    .min = 0,
    .max = 1,
    .step = 1,
    .def = 0,  /* Awake by default */
};

static const struct v4l2_ctrl_config antiburn_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 13,
    .name = "Anti-burn Protection",
    .type = V4L2_CTRL_TYPE_BOOLEAN,
    .min = 0,
    .max = 1,
    .step = 1,
    .def = 0,  /* Off by default */
};

static const char * const shutter_menu[] = {
    "Closed",
    "Open",
    NULL
};

static const struct v4l2_ctrl_config shutter_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 14,
    .name = "Shutter State",
    .type = V4L2_CTRL_TYPE_MENU,
    .qmenu = shutter_menu,
    .min = 0,
    .max = 1,
    .def = 1,  /* Open by default */
};

static const char * const hook_edge_menu[] = {
    "No Hook",
    "1st Gear",
    "2 Levels",
    NULL
};

static const struct v4l2_ctrl_config hook_edge_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 15,
    .name = "Hook Edge Position",
    .type = V4L2_CTRL_TYPE_MENU,
    .qmenu = hook_edge_menu,
    .min = 0,
    .max = 2,
    .def = 0,
};

static const char * const frame_rate_menu[] = {
    "25Hz",
    "30Hz",
    "50Hz",
    "60Hz",
    NULL
};

static const struct v4l2_ctrl_config frame_rate_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 16,
    .name = "Detector Frame Rate",
    .type = V4L2_CTRL_TYPE_MENU,
    .qmenu = frame_rate_menu,
    .min = 0,
    .max = 3,
    .def = 3,  /* 60Hz default */
};

static const struct v4l2_ctrl_config analog_output_fmt_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 17,
    .name = "Digital-Analog Output Format",
    .type = V4L2_CTRL_TYPE_BOOLEAN,
    .min = 0,
    .max = 1,
    .step = 1,
    .def = 0,
};

static const struct v4l2_ctrl_config output_mode_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 7,
    .name = "Output Mode",
    .type = V4L2_CTRL_TYPE_MENU,
    .qmenu = output_mode_menu,
    .min = 0,
    .max = 1,
    .def = 0,  /* Default to YUV (bypass). Set to 1 for Y16 (ISP). */
};

static int rs300_init_controls(struct rs300 *rs300)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    struct v4l2_ctrl_handler *ctrl_hdlr;
    // Define link frequency menu items - values must be in ascending order
    static const s64 link_freq_menu[] = {
        RS300_LINK_RATE  // Single link frequency for testing
    };
    int ret;
    u64 pixel_rate;

    dev_dbg(&client->dev, "Initializing controls");

    ctrl_hdlr = &rs300->ctrl_handler;
    ret = v4l2_ctrl_handler_init(ctrl_hdlr, 26);  /* 22 controls + HBLANK + VBLANK + EXPOSURE + ANALOGUE_GAIN */
    if (ret) {
        dev_err(&client->dev, "Failed to init ctrl handler: %d", ret);
        return ret;
    }

    /* Set the lock for the control handler */
    ctrl_hdlr->lock = &rs300->mutex;
    
    /* Add standard controls */
    rs300->link_frequency = v4l2_ctrl_new_int_menu(ctrl_hdlr, NULL,
        V4L2_CID_LINK_FREQ, 
        0, // Maximum index (not array size)
        0, // Default index
        link_freq_menu);
    
    if (rs300->link_frequency)
        rs300->link_frequency->flags |= V4L2_CTRL_FLAG_READ_ONLY;

    /* Initialize pixel rate based on default format (YUYV8_2X8 on Zero 2W) */
    pixel_rate = rs300_get_pixel_rate(MEDIA_BUS_FMT_YUYV8_2X8);
    rs300->pixel_rate = v4l2_ctrl_new_std(ctrl_hdlr, NULL,
                                      V4L2_CID_PIXEL_RATE,
                                      pixel_rate, pixel_rate, 1, 
                                      pixel_rate);
    
    if (rs300->pixel_rate)
        rs300->pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;

    /* Add mandatory V4L2 controls for libcamera ISP integration */
    /* HBLANK: Horizontal blanking (pixels beyond active area per line) */
    rs300->hblank = v4l2_ctrl_new_std(ctrl_hdlr, NULL,
                                      V4L2_CID_HBLANK,
                                      100, 100, 1,  /* Fixed at 100 pixels */
                                      100);

    if (rs300->hblank)
        rs300->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

    /* VBLANK: Vertical blanking (lines beyond active area per frame) */
    rs300->vblank = v4l2_ctrl_new_std(ctrl_hdlr, NULL,
                                      V4L2_CID_VBLANK,
                                      10, 10, 1,    /* Fixed at 10 lines */
                                      10);

    if (rs300->vblank)
        rs300->vblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

    /* EXPOSURE: Mandatory for libcamera integration */
    /* Range: 1 to sensor height (varies by module: 512, 384, or 288) */
    rs300->exposure = v4l2_ctrl_new_std(ctrl_hdlr, &rs300_ctrl_ops,
                                        V4L2_CID_EXPOSURE,
                                        1, rs300->mode->height, 1,
                                        rs300->mode->height);

    /* ANALOGUE_GAIN: Mandatory for libcamera integration */
    /* Fixed at 1 (thermal sensors don't have hardware gain) */
    rs300->analogue_gain = v4l2_ctrl_new_std(ctrl_hdlr, NULL,
                                             V4L2_CID_ANALOGUE_GAIN,
                                             1, 1, 1, 1);

    if (rs300->analogue_gain)
        rs300->analogue_gain->flags |= V4L2_CTRL_FLAG_READ_ONLY;

    rs300->brightness = v4l2_ctrl_new_std(ctrl_hdlr, &rs300_ctrl_ops,
                         V4L2_CID_BRIGHTNESS,
                         RS300_BRIGHTNESS_MIN, RS300_BRIGHTNESS_MAX,
                         RS300_BRIGHTNESS_STEP,
                         RS300_BRIGHTNESS_DEFAULT);

    /* Add standard contrast control */
    rs300->contrast = v4l2_ctrl_new_std(ctrl_hdlr, &rs300_ctrl_ops,
                         V4L2_CID_CONTRAST, 0, 100, 1, 50);

    /* Add custom controls with simpler configurations */
    rs300->colormap = v4l2_ctrl_new_custom(ctrl_hdlr, &colormap_ctrl, NULL);
    rs300->shutter_cal = v4l2_ctrl_new_custom(ctrl_hdlr, &ffc_ctrl, NULL);
    rs300->zoom = v4l2_ctrl_new_std(ctrl_hdlr, &rs300_ctrl_ops,
                    V4L2_CID_ZOOM_ABSOLUTE, 1, 8, 1, 1);
    rs300->scene_mode = v4l2_ctrl_new_custom(ctrl_hdlr, &scene_mode_ctrl, NULL);
    rs300->dde = v4l2_ctrl_new_custom(ctrl_hdlr, &dde_ctrl, NULL);
    rs300->spatial_nr = v4l2_ctrl_new_custom(ctrl_hdlr, &spatial_nr_ctrl, NULL);
    rs300->temporal_nr = v4l2_ctrl_new_custom(ctrl_hdlr, &temporal_nr_ctrl, NULL);
    rs300->output_mode = v4l2_ctrl_new_custom(ctrl_hdlr, &output_mode_ctrl, NULL);
    rs300->autoshutter = v4l2_ctrl_new_custom(ctrl_hdlr, &autoshutter_ctrl, NULL);
    rs300->autoshutter_temp = v4l2_ctrl_new_custom(ctrl_hdlr, &autoshutter_temp_ctrl, NULL);
    rs300->autoshutter_min_interval = v4l2_ctrl_new_custom(ctrl_hdlr, &autoshutter_min_interval_ctrl, NULL);
    rs300->autoshutter_max_interval = v4l2_ctrl_new_custom(ctrl_hdlr, &autoshutter_max_interval_ctrl, NULL);
    rs300->camera_sleep = v4l2_ctrl_new_custom(ctrl_hdlr, &camera_sleep_ctrl, NULL);
    rs300->antiburn = v4l2_ctrl_new_custom(ctrl_hdlr, &antiburn_ctrl, NULL);
    rs300->shutter = v4l2_ctrl_new_custom(ctrl_hdlr, &shutter_ctrl, NULL);
    rs300->hook_edge = v4l2_ctrl_new_custom(ctrl_hdlr, &hook_edge_ctrl, NULL);
    rs300->frame_rate = v4l2_ctrl_new_custom(ctrl_hdlr, &frame_rate_ctrl, NULL);
    rs300->analog_output_fmt = v4l2_ctrl_new_custom(ctrl_hdlr, &analog_output_fmt_ctrl, NULL);

    /* Check for errors */
    if (ctrl_hdlr->error) {
        ret = ctrl_hdlr->error;
        dev_err(&client->dev, "%s control init failed (%d)\n",
            __func__, ret);
        goto error;
    }
    
    /* Connect the control handler to the subdevice */
    rs300->sd.ctrl_handler = ctrl_hdlr;
    
    dev_dbg(&client->dev, "Control handler initialized successfully\n");

    return 0;

error:
    v4l2_ctrl_handler_free(ctrl_hdlr);
    mutex_destroy(&rs300->mutex);

    return ret;
}

static void rs300_free_controls(struct rs300 *rs300)
{
	v4l2_ctrl_handler_free(&rs300->ctrl_handler);
	mutex_destroy(&rs300->mutex);
}

static int rs300_get_device_name(struct rs300 *rs300)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    u8 result_buffer[40];  // Buffer to hold the device name response
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    
    dev_dbg(&client->dev, "Getting device name from camera");
    
    /* Add initial delay to ensure device is ready */
    msleep(50);
    
    /* Test I2C communication first */
    ret = read_regs(client, 0x0200, status_buffer, 1);
    if (ret) {
        dev_err(&client->dev, "Initial I2C communication test failed: %d", ret);
        return ret;
    }
    dev_dbg(&client->dev, "Initial I2C communication test passed");
    
    /* Construct the correct command buffer */
    cmd_buffer[0] = 0x01;  /* Start marker */
    cmd_buffer[1] = 0x01;  /* Command marker */
    cmd_buffer[2] = 0x81;  /* Command ID */
    cmd_buffer[3] = 0x00;  /* Reserved */
    cmd_buffer[4] = 0x01;  /* Parameter 1 */
    /* Fill remaining parameters */
    memset(&cmd_buffer[5], 0x00, 11);  /* Zero out bytes 5-15 */
    cmd_buffer[12] = 0x20; /* Set byte 12 to 0x20 */
    /* Calculate CRC/checksum for the command */
    unsigned short crc = do_crc(cmd_buffer, 16);
    cmd_buffer[16] = 0xFC;  /* CRC bytes from example */
    cmd_buffer[17] = 0x1E;
    
    dev_dbg(&client->dev, "Device name command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* Write command with retry */
    for (retry_count = 0; retry_count < max_retries; retry_count++) {
        ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
        if (ret == 0) {
            break;
        }
        dev_warn(&client->dev, "Write attempt %d failed: %d, retrying...", retry_count + 1, ret);
        msleep(50);  // Wait before retry
    }
    
    if (ret) {
        dev_err(&client->dev, "Failed to write device name command after %d retries: %d", max_retries, ret);
        return ret;
    }

    /* Read command status and wait for completion */
    while (retry_count < max_retries) {
        /* Wait for command processing */
        msleep(50);
        
        /* Read command status from status register (0x0200) */
        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read command status: %d", ret);
            return ret;
        }
        
        /* Check command status */
        dev_dbg(&client->dev, "Device name command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  /* Bits 2-7 */
        
        /* Check if command is still busy */
        if (is_busy) {
            dev_dbg(&client->dev, "Device name command is busy, retrying...");
            retry_count++;
            continue;
        }
        
        /* Check for errors */
        if (has_failed) {
            dev_err(&client->dev, "Device name command execution failed with error code: 0x%02X", error_code);
            return -EIO;
        }
        
        /* Command executed successfully, read the result */
        ret = read_regs(client, 0x1d00, result_buffer, sizeof(result_buffer));
        if (ret) {
            dev_err(&client->dev, "Failed to read device name result: %d", ret);
            return ret;
        }
        
        /* Extract and null-terminate the device name (expecting ASCII response) */
        char device_name[32] = {0};  // Larger buffer to be safe
        int name_length = 0;
        
        /* Look for ASCII text in the response */
        for (int i = 0; i < sizeof(result_buffer) && name_length < 31; i++) {
            if (result_buffer[i] >= ' ' && result_buffer[i] <= '~') {
                device_name[name_length++] = result_buffer[i];
            }
        }
        device_name[name_length] = '\0';  // Ensure null termination
        
        dev_dbg(&client->dev, "Camera device name: %s", device_name);
        dev_dbg(&client->dev, "Raw response: %*ph", (int)sizeof(result_buffer), result_buffer);
        
        return 0;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "Device name command timed out after %d retries", max_retries);
    return -ETIMEDOUT;
}

static int rs300_check_hwcfg(struct device *dev)
{
	struct fwnode_handle *endpoint;
	struct v4l2_fwnode_endpoint ep_cfg = {
		.bus_type = V4L2_MBUS_CSI2_DPHY
	};
	int ret = -EINVAL;

	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
	if (!endpoint) {
		dev_err(dev, "endpoint node not found\n");
		return -EINVAL;
	}

	if (v4l2_fwnode_endpoint_alloc_parse(endpoint, &ep_cfg)) {
		dev_err(dev, "could not parse endpoint\n");
		goto error_out;
	}
	
	if (ep_cfg.bus.mipi_csi2.num_data_lanes != 2) {
		dev_err(dev, "only 2 data lanes are currently supported\n");
		goto error_out;
	}
	
	if (ep_cfg.nr_of_link_frequencies != 1 ||
	    ep_cfg.link_frequencies[0] != RS300_LINK_RATE) {
		dev_err(dev, "Link frequency not supported: %lld\n",
			ep_cfg.link_frequencies[0]);
		goto error_out;
	}

	ret = 0;

error_out:
	v4l2_fwnode_endpoint_free(&ep_cfg);
	fwnode_handle_put(endpoint);

	return ret;
}

static int rs300_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct rs300 *rs300;
	int ret;

	dev_info(dev, "Starting rs300_probe");
	
	dev_dbg(dev, "driver version: %02x.%02x.%02x",
		DRIVER_VERSION >> 16,
		(DRIVER_VERSION & 0xff00) >> 8,
		DRIVER_VERSION & 0x00ff);

	dev_dbg(dev, "Allocating memory for rs300 structure");
	rs300 = devm_kzalloc(&client->dev, sizeof(*rs300), GFP_KERNEL);
	if (!rs300) {
		dev_err(dev, "Failed to allocate memory for rs300 structure");
		return -ENOMEM;
	}
	dev_dbg(dev, "Memory allocation successful");

	dev_dbg(dev, "Initializing V4L2 subdev");
	v4l2_i2c_subdev_init(&rs300->sd, client, &rs300_subdev_ops);
	dev_dbg(dev, "V4L2 subdev initialization complete");

	/* Check the hardware configuration in device tree */
	dev_dbg(dev, "Checking hardware configuration");
	if (rs300_check_hwcfg(dev)) {
		dev_err(dev, "Hardware configuration check failed");
		return -EINVAL;
	}
	dev_dbg(dev, "Hardware configuration check successful");

	dev_dbg(dev, "Getting regulators");
	ret = rs300_get_regulators(rs300);
	if (ret) {
		dev_err(dev, "Failed to get regulators: %d", ret);
		return ret;
	}
	dev_dbg(dev, "Regulators acquired successfully");

	/* Get reset GPIO 
	dev_dbg(dev, "Getting reset GPIO");
	rs300->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(rs300->reset_gpio)) {
		ret = PTR_ERR(rs300->reset_gpio);
		dev_err(dev, "Failed to get reset GPIO: %d", ret);
		return ret;
	}
	dev_dbg(dev, "Reset GPIO acquired successfully");
    */

	/* Power on the sensor */
	dev_dbg(dev, "Powering on the sensor");
	ret = rs300_power_on(dev);
	if (ret) {
		dev_err(dev, "Failed to power on rs300: %d", ret);
		goto error_power_off;
	}
	dev_dbg(dev, "Sensor powered on successfully");

	/* Get device name 
	ret = rs300_get_device_name(rs300);
	if (ret) {
		dev_warn(dev, "Failed to get device name: %d", ret);
		// Don't fail probe on this error, just warn
	}*/

	/* Set default mode to 0=640x512, 1=256x192, 2=384x288 */
	rs300->mode = &supported_modes[mode];

	/*
	 * Mode filtering: Only advertise the single resolution supported by this physical module.
	 * Each camera module (256, 384, or 640) has a fixed sensor resolution.
	 * Advertising multiple resolutions causes libcamera to attempt format changes,
	 * which the hardware cannot support and triggers driver bugs.
	 */
	rs300->available_modes = &supported_modes[mode];
	rs300->num_modes = 1;

	dev_info(dev, "Mode filtering: Hardware supports only %dx%d (mode=%d)",
		 rs300->available_modes->width, rs300->available_modes->height, mode);

	/* Initialize default format */
	rs300_set_default_format(rs300);

	/*
	 * YUV format configuration deferred to first stream start.
	 * The sensor is not ready for I2C commands during probe,
	 * causing -121 (EREMOTEIO) errors on register 0x1d00.
	 */
	rs300->yuv_format_configured = false;

	/* Initialize mutex */
	mutex_init(&rs300->mutex);
	
	/* Initialize controls BEFORE registering the subdevice */
	ret = rs300_init_controls(rs300);
	if (ret) {
		dev_err(dev, "failed to initialize controls\n");
		goto error_power_off;
	}
	
	/* Initialize subdev flags */
	rs300->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;
	rs300->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	/* Initialize pads */
	rs300->pad[IMAGE_PAD].flags = MEDIA_PAD_FL_SOURCE;

	/* Initialize media entity */
	ret = media_entity_pads_init(&rs300->sd.entity, NUM_PADS, rs300->pad);
	if (ret) {
		dev_err(dev, "failed to init entity pads: %d\n", ret);
		goto error_handler_free;
	}
	
	/* Register the subdevice */
	ret = v4l2_async_register_subdev_sensor(&rs300->sd);
	if (ret < 0) {
		dev_err(dev, "failed to register sensor sub-device: %d\n", ret);
		goto error_media_entity;
	}

	if (rs300->sd.ctrl_handler)
		dev_dbg(dev, "Subdevice control handler initialized\n");

	return 0;

error_media_entity:
	media_entity_cleanup(&rs300->sd.entity);

error_handler_free:
	v4l2_ctrl_handler_free(&rs300->ctrl_handler);		

error_power_off:
	rs300_power_off(dev);

	return ret;
}

static void rs300_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct rs300 *rs300 = to_rs300(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	rs300_free_controls(rs300);

	/*
	 * Disable regulators to ensure clean power state for module reload.
	 * Without this, regulators remain enabled after rmmod, causing issues
	 * when the module is reloaded without a reboot.
	 */
	regulator_bulk_disable(rs300_NUM_SUPPLIES, rs300->supplies);
	dev_info(&client->dev, "RS300 regulators disabled, driver removed\n");
}

static const struct i2c_device_id rs300_id[] = {
	{ "rs300", 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(i2c, rs300_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id rs300_of_match[] = {
	{ .compatible = "mini2,rs300"  },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, rs300_of_match);
#endif

static struct i2c_driver rs300_i2c_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.of_match_table = of_match_ptr(rs300_of_match),
	},
	.probe		= rs300_probe,
	.remove		= rs300_remove,
	.id_table	= rs300_id,
};

static int __init sensor_mod_init(void)
{
	return i2c_add_driver(&rs300_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&rs300_i2c_driver);
}

device_initcall_sync(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_AUTHOR("Kodrea");
MODULE_DESCRIPTION("Mini2/WN2 microbolometer thermal camera driver");
MODULE_LICENSE("GPL v2");
