// SPDX-License-Identifier: GPL-2.0
/*
 * rs300 CMOS Image Sensor driver
 *
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd.
 */

// TODO: Remove unused headers
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
#define DRIVER_NAME "rs300"
//80M (clk)* 2(double ) *2 (lan) /8

#define RS300_LINK_RATE (80 * 1000 * 1000)       /* 80MHz link rate matching device tree */
#define RS300_PIXEL_RATE	(200 * 1000 * 1000)  /* TEST: Conservative 30MHz pixel rate for 8-bit */
#define RS300_PIXEL_RATE_16BIT	(400 * 1000 * 1000)  /* TEST: Conservative 60MHz pixel rate for 16-bit */
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

#define NUM_COLORMAP_ITEMS (ARRAY_SIZE(colormap_menu) - 1) // Account for NULL terminator

// Mode must be set before running setup.sh
// TODO: Make mode adjustable during runtime
static int mode = 0; //0-640; 1-256; 2-384
static int fps = 30;
static int pWidth = 0;
static int pHeight = 0;
static int type = 16;
static int debug = 1;
module_param(mode, int, 0644);
module_param(fps, int, 0644);
module_param(pWidth, int, 0644);
module_param(pHeight, int, 0644);
module_param(type, int, 0644);
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Debug level (0-1)");

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

static  u8 start_regs[] = {
		0x01, 0x30, 0xc1, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x0a, 0x00,
		0x00, 0x00, //crc [16]
		0x2F, 0x0D, //crc [18]
		0x00, //path
		0x16, //src
		0x03, //dst
		0x3c, // 60 fps
		0x80, 0x02, //width&0xff, width>>8
		0x00, 0x02, //height&0xff, height>>8
		0x00, 0x00
};

static  u8 stop_regs[]={
		0x01, 0x30, 0xc2, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x0a, 0x00,
		0x00, 0x00, //crc [16]
		0x2F, 0x0D, //crc [18]
		0x01, //path
		0x16, //src
		0x00, //dst
		0x3c, //fps
		0x80, 0x02, //width&0xff, width>>8
		0x00, 0x02, //height&0xff, height>>8
		0x00, 0x00
    
};

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

enum pad_types {
	IMAGE_PAD,
	METADATA_PAD,
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
	/* YUV 4:2:2 Formats - Prioritize 16-bit packed for Pi 5 RP1-CFE compatibility */
	MEDIA_BUS_FMT_YUYV8_1X16,  /* 16-bit packed - preferred for RP1-CFE */
	MEDIA_BUS_FMT_UYVY8_1X16,  /* 16-bit packed - alternative */
	MEDIA_BUS_FMT_YUYV8_2X8,   /* 8-bit dual lane - legacy */
	MEDIA_BUS_FMT_UYVY8_2X8,   /* 8-bit dual lane - legacy */
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
	struct v4l2_ctrl *brightness;
	struct v4l2_ctrl *shutter_cal;  /* Shutter calibration button */
	struct v4l2_ctrl *colormap;  /* Colormap selection control */
	struct v4l2_ctrl *zoom;  // Custom zoom control
	struct v4l2_ctrl *scene_mode;  /* Scene mode selection control */
	struct v4l2_ctrl *dde;
	struct v4l2_ctrl *contrast;
	struct v4l2_ctrl *spatial_nr;
	struct v4l2_ctrl *temporal_nr;

	/* Current mode */
	const struct rs300_mode *mode;

	/*
	 * Mutex for serialized access:
	 * Protect sensor module set pad format and start/stop streaming safely.
	 */
	struct mutex mutex;

	/* Streaming on/off */
	bool streaming;
};

static struct rs300_mode supported_modes[] = {
    { /* 640 - Primary mode for Pi 5 */
        .width      = 640,
        .height     = 512,
        .max_fps = {
            .numerator = 60,
            .denominator = 1,
        },
        .code = MEDIA_BUS_FMT_YUYV8_1X16,  /* 16-bit packed for RP1-CFE compatibility */
    },
    { /* 256 - MIPI video not currently working, but I2C commands are working */
        .width      = 256,
        .height     = 192,  
        .max_fps = {
            .numerator = 25,
            .denominator = 1,
        },
        .code = MEDIA_BUS_FMT_YUYV8_1X16,  /* 16-bit packed for RP1-CFE compatibility */
    },
        { /* 384*/
        .width      = 384,
        .height     = 288,  
        .max_fps = {
            .numerator = 30,
            .denominator = 1,
        },
        .code = MEDIA_BUS_FMT_YUYV8_1X16,  /* 16-bit packed for RP1-CFE compatibility */
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

	dev_info(&client->dev, "rs300_get_format_code: input code=0x%x", code);

	for (i = 0; i < ARRAY_SIZE(codes); i++) {
		dev_info(&client->dev, "  Checking supported code[%d]=0x%x", i, codes[i]);
		if (codes[i] == code)
			break;
	}

	if (i >= ARRAY_SIZE(codes)) {
		dev_warn(&client->dev, "Format code 0x%x not found, defaulting to 0x%x", code, codes[0]);
		i = 0; /* Default to first supported code (YUYV8_1X16) */
	}

	dev_info(&client->dev, "rs300_get_format_code: returning code=0x%x", codes[i]);
	return codes[i];
}

static long rs300_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	long ret = 0;
	unsigned char *data;
	struct ioctl_data * valp;

	valp=(struct ioctl_data *)arg;
	if((cmd==CMD_GET)||(cmd==CMD_SET)){
		if((valp!=NULL) &&(valp->data!=NULL) ){
			dev_info(&client->dev,"rs300 %d %d %d  \n",cmd, valp->wIndex,valp->wLength);
		}else{
			dev_err(&client->dev, "rs300 args error \n");
			return -EFAULT;
		}
	}
	switch (cmd) {

	case CMD_GET:
		data = kmalloc(valp->wLength, GFP_KERNEL);
		read_regs(client,valp->wIndex,data,valp->wLength);

		if (copy_to_user(valp->data, data, valp->wLength))
		{
			ret = -EFAULT;
			dev_err(&client->dev, "error stop rs300\n");
		}
		kfree(data);                                                                                                                                               
		break;
	case CMD_SET:
		write_regs(client,valp->wIndex,valp->data,valp->wLength);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
static void rs300_reset_colorspace(struct v4l2_mbus_framefmt *fmt)
{
	/* Use video-compatible colorspace for RP1-CFE ISP pipeline compatibility */
	/* Thermal data disguised as standard video YUV for ISP processing */
	fmt->colorspace = V4L2_COLORSPACE_SMPTE170M;     /* Standard SDTV YUV colorspace */
	fmt->ycbcr_enc = V4L2_YCBCR_ENC_601;            /* ITU-R BT.601 YCbCr encoding */
	fmt->quantization = V4L2_QUANTIZATION_LIM_RANGE; /* Broadcast-legal limited range */
	fmt->xfer_func = V4L2_XFER_FUNC_709;            /* Standard video transfer function */
}

/* Calculate pixel rate based on format */
static u64 rs300_get_pixel_rate(u32 format_code)
{
	switch (format_code) {
	case MEDIA_BUS_FMT_YUYV8_1X16:
	case MEDIA_BUS_FMT_UYVY8_1X16:
		/* 16-bit packed formats require higher pixel rate */
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
    
    dev_info(&client->dev, "rs300_set_default_format");
    
    /* Initialize the default format */
    fmt = &rs300->fmt;
    fmt->code = supported_modes[mode].code;
    fmt->width = supported_modes[mode].width;
    fmt->height = supported_modes[mode].height;
    fmt->field = V4L2_FIELD_NONE;
    rs300_reset_colorspace(fmt);
    
    /* Set the default mode */
    rs300->mode = &supported_modes[mode];
    
    dev_info(&client->dev, "Default format set: code=0x%x, %dx%d",
        fmt->code, fmt->width, fmt->height);
}	

/*
 * V4L2 subdev video and pad level operations
 */
static int rs300_set_test_pattern(struct rs300 *rs300, int value)
{
	return 0;
}

///////////////////// TODO: Reduce repeditive code for functions that send commands to the camera ///////////////////////

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
    
    dev_info(&client->dev, "Getting current brightness value from camera");
    
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
    
    dev_info(&client->dev, "Get brightness command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
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
        dev_info(&client->dev, "Get brightness command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  /* Bits 2-7 */
        
        /* Check if command is still busy */
        if (is_busy) {
            dev_info(&client->dev, "Get brightness command is busy, retrying...");
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
                dev_info(&client->dev, "Error: Correct");
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
        
        dev_info(&client->dev, "Brightness result buffer: %*ph", (int)sizeof(result_buffer), result_buffer);
        
        /* Based on the command structure, the brightness value should be in byte 4 */
        *brightness_value = result_buffer[4];
        
        dev_info(&client->dev, "Current brightness value: %d (0x%02X)", *brightness_value, *brightness_value);
        return 0;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "Get brightness command timed out after %d retries", max_retries);
    return -ETIMEDOUT;
}

static int rs300_set_dde(struct rs300 *rs300, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    
    dev_info(&client->dev, "Setting DDE to %d", value);
    
    /* Validate value range */
    if (value < 0 || value > 100) {
        dev_err(&client->dev, "Invalid DDE value: %d (valid range: 0-100)", value);
        return -EINVAL;
    }
    
    /* Construct the command buffer */
    cmd_buffer[0] = 0x10;  /* Command Class */
    cmd_buffer[1] = 0x04;  /* Module Command Index */
    cmd_buffer[2] = 0x45;  /* SubCmd */
    cmd_buffer[3] = 0x00;  /* Reserved */
    cmd_buffer[4] = value; /* Parameter value */
    
    /* Fill remaining parameters with zeros */
    memset(&cmd_buffer[5], 0, 11);
    
    /* Calculate CRC */
    unsigned short crc = do_crc(cmd_buffer, 16);
    cmd_buffer[16] = crc & 0xFF;
    cmd_buffer[17] = (crc >> 8) & 0xFF;
    
    dev_info(&client->dev, "DDE command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* Write command */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write DDE command: %d", ret);
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
                dev_err(&client->dev, "DDE command failed with error code: 0x%02X", error_code);
                return -EIO;
            }
            dev_info(&client->dev, "DDE set successfully");
            return 0;
        }
        
        retry_count++;
    }
    
    dev_err(&client->dev, "DDE command timed out");
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
    
    dev_info(&client->dev, "Setting YUV format to %d (0=UYVY, 1=VYUY, 2=YUYV, 3=YVYU)", format);
    
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
    
    dev_info(&client->dev, "YUV format command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
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
            dev_info(&client->dev, "YUV format set to %d successfully", format);
            return 0;
        }
        
        retry_count++;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "YUV format command timed out after %d retries", max_retries);
    return -ETIMEDOUT;
}

static int rs300_set_contrast(struct rs300 *rs300, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    
    dev_info(&client->dev, "Setting contrast to %d", value);
    
    /* Validate value range */
    if (value < 0 || value > 100) {
        dev_err(&client->dev, "Invalid contrast value: %d (valid range: 0-100)", value);
        return -EINVAL;
    }
    
    /* Construct the command buffer */
    cmd_buffer[0] = 0x10;  /* Command Class */
    cmd_buffer[1] = 0x04;  /* Module Command Index */
    cmd_buffer[2] = 0x4A;  /* SubCmd */
    cmd_buffer[3] = 0x00;  /* Reserved */
    cmd_buffer[4] = value; /* Parameter value */
    
    /* Fill remaining parameters with zeros */
    memset(&cmd_buffer[5], 0, 11);
    
    /* Calculate CRC */
    unsigned short crc = do_crc(cmd_buffer, 16);
    cmd_buffer[16] = crc & 0xFF;
    cmd_buffer[17] = (crc >> 8) & 0xFF;
    
    dev_info(&client->dev, "Contrast command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* Write command */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write contrast command: %d", ret);
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
                dev_err(&client->dev, "Contrast command failed with error code: 0x%02X", error_code);
                return -EIO;
            }
            dev_info(&client->dev, "Contrast set successfully");
            return 0;
        }
        
        retry_count++;
    }
    
    dev_err(&client->dev, "Contrast command timed out");
    return -ETIMEDOUT;
}

static int rs300_set_spatial_nr(struct rs300 *rs300, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    
    dev_info(&client->dev, "Setting spatial noise reduction to %d", value);
    
    /* Validate value range */
    if (value < 0 || value > 100) {
        dev_err(&client->dev, "Invalid spatial NR value: %d (valid range: 0-100)", value);
        return -EINVAL;
    }
    
    /* Construct the command buffer */
    cmd_buffer[0] = 0x10;  /* Command Class */
    cmd_buffer[1] = 0x04;  /* Module Command Index */
    cmd_buffer[2] = 0x4B;  /* SubCmd */
    cmd_buffer[3] = 0x00;  /* Reserved */
    cmd_buffer[4] = value; /* Parameter value */
    
    /* Fill remaining parameters with zeros */
    memset(&cmd_buffer[5], 0, 11);
    
    /* Calculate CRC */
    unsigned short crc = do_crc(cmd_buffer, 16);
    cmd_buffer[16] = crc & 0xFF;
    cmd_buffer[17] = (crc >> 8) & 0xFF;
    
    dev_info(&client->dev, "Spatial NR command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* Write command */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write spatial NR command: %d", ret);
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
                dev_err(&client->dev, "Spatial NR command failed with error code: 0x%02X", error_code);
                return -EIO;
            }
            dev_info(&client->dev, "Spatial NR set successfully");
            return 0;
        }
        
        retry_count++;
    }
    
    dev_err(&client->dev, "Spatial NR command timed out");
    return -ETIMEDOUT;
}

static int rs300_set_temporal_nr(struct rs300 *rs300, int value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    
    dev_info(&client->dev, "Setting temporal noise reduction to %d", value);
    
    /* Validate value range */
    if (value < 0 || value > 100) {
        dev_err(&client->dev, "Invalid temporal NR value: %d (valid range: 0-100)", value);
        return -EINVAL;
    }
    
    /* Construct the command buffer */
    cmd_buffer[0] = 0x10;  /* Command Class */
    cmd_buffer[1] = 0x04;  /* Module Command Index */
    cmd_buffer[2] = 0x4C;  /* SubCmd */
    cmd_buffer[3] = 0x00;  /* Reserved */
    cmd_buffer[4] = value; /* Parameter value */
    
    /* Fill remaining parameters with zeros */
    memset(&cmd_buffer[5], 0, 11);
    
    /* Calculate CRC */
    unsigned short crc = do_crc(cmd_buffer, 16);
    cmd_buffer[16] = crc & 0xFF;
    cmd_buffer[17] = (crc >> 8) & 0xFF;
    
    dev_info(&client->dev, "Temporal NR command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* Write command */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write temporal NR command: %d", ret);
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
                dev_err(&client->dev, "Temporal NR command failed with error code: 0x%02X", error_code);
                return -EIO;
            }
            dev_info(&client->dev, "Temporal NR set successfully");
            return 0;
        }
        
        retry_count++;
    }
    
    dev_err(&client->dev, "Temporal NR command timed out");
    return -ETIMEDOUT;
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
    
    dev_info(&client->dev, "Command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
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
        dev_info(&client->dev, "Command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  // Bits 2-7

        /* Check if command is still busy */
        if (is_busy) {
            dev_info(&client->dev, "Command is busy, retrying...");
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
        
        dev_info(&client->dev, "Current colormap value: %d (0x%02X)", *colormap_value, *colormap_value);
        return 0;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "Get colormap command timed out after %d retries", max_retries);
    return -ETIMEDOUT;

}

static int rs300_set_colormap(struct rs300 *rs300, int colormap_value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    int current_colormap;
    
    dev_info(&client->dev, "Setting colormap to %d", colormap_value);
    
    /* Validate colormap value range */
    if (colormap_value < 0 || colormap_value > 11) {
        dev_err(&client->dev, "Invalid colormap value: %d (valid range: 0-11)",
                colormap_value);
        return -EINVAL;
    }
    
    /* Construct the command buffer */
    cmd_buffer[0] = 0x10;             /* Command Class */
    cmd_buffer[1] = 0x03;             /* Module Command Index */
    cmd_buffer[2] = 0x45;             /* SubCmd */
    cmd_buffer[3] = 0x00;             /* Reserved */
    cmd_buffer[4] = 0x00;             /* Parameter 1 (0x00) */
    cmd_buffer[5] = colormap_value;   /* Parameter 2 (0-11) */
    
    /* Fill remaining parameters with zeros */
    memset(&cmd_buffer[6], 0, 10);
    
    /* Calculate CRC/checksum for the command */
    unsigned short crc = do_crc(cmd_buffer, 16);
    /* Swap byte order to match the expected format */
    cmd_buffer[16] = crc & 0xFF;         /* Low byte of CRC first */
    cmd_buffer[17] = (crc >> 8) & 0xFF;  /* High byte of CRC second */
    
    dev_info(&client->dev, "Command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
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
        dev_info(&client->dev, "Command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  // Bits 2-7
        
        /* Check if command is still busy */
        if (is_busy) {
            dev_info(&client->dev, "Command is busy, retrying...");
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
                dev_info(&client->dev, "Error: Correct");
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
            /* Wait a moment before getting the colormap */
            msleep(100);
            
            /* Get the current colormap to verify the change */
            ret = rs300_get_colormap(rs300, &current_colormap);
            if (ret) {
                dev_warn(&client->dev, "Failed to get current colormap: %d", ret);
            } else {
                if (current_colormap == colormap_value) {
                    dev_info(&client->dev, "Colormap successfully set and verified: %d", current_colormap);
                } else {
                    dev_warn(&client->dev, "Colormap mismatch! Set: %d, Got: %d", 
                             colormap_value, current_colormap);
                }
            }
        }
        
        return 0;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "Command timed out after %d retries", max_retries);
    return -ETIMEDOUT;
}

static int rs300_shutter_cal(struct rs300 *rs300)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
	int ret;
    int retry_count = 0;
    const int max_retries = 5;
    
    dev_info(&client->dev, "Triggering shutter calibration (FFC)");
    
    /* Construct the command buffer based on the example (shutter command) */
    cmd_buffer[0] = 0x10;  /* Command Class */
    cmd_buffer[1] = 0x02;  /* Module Command Index */
    cmd_buffer[2] = 0x43;  /* SubCmd - 0x43 for shutter/FFC */
    cmd_buffer[3] = 0x00;  /* Reserved */
    
    /* Fill remaining parameters with zeros */
    memset(&cmd_buffer[4], 0, 12);

    /* Calculate CRC/checksum for the command */
    unsigned short crc = do_crc(cmd_buffer, 16);
    /* Swap byte order to match the expected format */
    cmd_buffer[16] = crc & 0xFF;         /* Low byte of CRC first */
    cmd_buffer[17] = (crc >> 8) & 0xFF;  /* High byte of CRC second */
    
    dev_info(&client->dev, "Shutter command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* STEP 1: Write command to command buffer register (0x1d00) */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write shutter command: %d", ret);
        return ret;
    }
    
    /* STEP 2: Read command status and wait for completion */
    while (retry_count < max_retries) {
        /* Wait for command processing - FFC might take longer */
        msleep(1000);  /* Longer wait time for FFC */
        
        /* Read command status from status register (0x0200) */
        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_err(&client->dev, "Failed to read command status: %d", ret);
            return ret;
        }
        
        /* Check command status */
        dev_info(&client->dev, "Shutter command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  // Bits 2-7
        
        /* Check if command is still busy */
        if (is_busy) {
            dev_info(&client->dev, "Shutter command is busy, retrying...");
            retry_count++;
            continue;
        }
        
        /* Check for errors */
        if (has_failed) {
            /* Bit 1 set: Command execution failed */
            dev_err(&client->dev, "Shutter command execution failed with error code: 0x%02X", error_code);
            
            /* Interpret error code */
            switch (error_code) {
            case 0x00:
                dev_info(&client->dev, "Error: Correct");
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
            dev_warn(&client->dev, "Shutter command completed but with error code: 0x%02X", error_code);
        }
        
        /* Command executed successfully */
        dev_info(&client->dev, "Shutter calibration command executed successfully");
        return 0;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "Shutter command timed out after %d retries", max_retries);
    return -ETIMEDOUT;
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
    
    /* Map 0-100 brightness to appropriate parameter values */
    if (brightness_value == 0)
        brightness_param = 0x00;
    else if (brightness_value <= 10)
        brightness_param = 0x0A;
    else if (brightness_value <= 20)
        brightness_param = 0x14;
    else if (brightness_value <= 30)
        brightness_param = 0x1E;
    else if (brightness_value <= 40)
        brightness_param = 0x28;
    else if (brightness_value <= 50)
        brightness_param = 0x32;
    else if (brightness_value <= 60)
        brightness_param = 0x3C;
    else if (brightness_value <= 70)
        brightness_param = 0x46;
    else if (brightness_value <= 80)
        brightness_param = 0x50;
    else if (brightness_value <= 90)
        brightness_param = 0x5A;
    else
        brightness_param = 0x64;
    
    dev_info(&client->dev, "Setting brightness correctly to %d (param: 0x%02X)", 
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
    
    dev_info(&client->dev, "Command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
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
        dev_info(&client->dev, "Command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  // Bits 2-7
        
        /* Check if command is still busy */
        if (is_busy) {
            dev_info(&client->dev, "Command is busy, retrying...");
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
                dev_info(&client->dev, "Error: Correct");
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
                    dev_info(&client->dev, "Brightness successfully set and verified: %d", current_brightness);
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
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    
    dev_info(&client->dev, "Setting zoom to %dx", zoom_level);
    
    /* Validate zoom level */
    if (zoom_level < 1 || zoom_level > 8) {
        dev_err(&client->dev, "Invalid zoom level: %d (valid range: 1-8)", zoom_level);
        return -EINVAL;
    }
    
    /* Construct the command buffer */
    cmd_buffer[0] = 0x01;  /* Fixed value */
    cmd_buffer[1] = 0x31;  /* Fixed value */
    cmd_buffer[2] = 0x42;  /* Fixed value */
    cmd_buffer[3] = 0x00;  /* Fixed value */
    cmd_buffer[4] = 0x00;  /* Fixed value */
    cmd_buffer[5] = zoom_level * 10;  /* Convert zoom level to command value */
    /* Fill remaining values */
    memset(&cmd_buffer[6], 0x00, 10);  /* Bytes 6-15 are 0x00 */
    cmd_buffer[16] = 0x06;  /* Fixed value */
    cmd_buffer[17] = 0x0A;  /* Fixed value */
    
    dev_info(&client->dev, "Zoom command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* Write command to command buffer register (0x1d00) */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write zoom command: %d", ret);
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
        dev_info(&client->dev, "Zoom command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  /* Bits 2-7 */
        
        /* Check if command is still busy */
        if (is_busy) {
            dev_info(&client->dev, "Zoom command is busy, retrying...");
            retry_count++;
            continue;
        }
        
        /* Check for errors */
        if (has_failed) {
            dev_err(&client->dev, "Zoom command execution failed with error code: 0x%02X", error_code);
            return -EIO;
        }
        
        /* Command executed successfully */
        dev_info(&client->dev, "Zoom set to %dx successfully", zoom_level);
        return 0;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "Zoom command timed out after %d retries", max_retries);
    return -ETIMEDOUT;
}

static int rs300_set_scene_mode(struct rs300 *rs300, int scene_mode_value)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 5;
    
    dev_info(&client->dev, "Setting scene mode to %d", scene_mode_value);
    
    /* Validate scene mode value range */
    if (scene_mode_value < 0 || scene_mode_value > 9) {
        dev_err(&client->dev, "Invalid scene mode value: %d (valid range: 0-9)",
                scene_mode_value);
        return -EINVAL;
    }
    
    /* Construct the command buffer */
    cmd_buffer[0] = 0x10;             /* Command Class */
    cmd_buffer[1] = 0x04;             /* Module Command Index */
    cmd_buffer[2] = 0x42;             /* SubCmd */
    cmd_buffer[3] = 0x00;             /* Reserved */
    cmd_buffer[4] = scene_mode_value;  /* Parameter 1 (scene mode value) */
    
    /* Fill remaining parameters with zeros */
    memset(&cmd_buffer[5], 0, 11);
    
    /* Calculate CRC/checksum for the command */
    unsigned short crc = do_crc(cmd_buffer, 16);
    cmd_buffer[16] = crc & 0xFF;         /* Low byte of CRC first */
    cmd_buffer[17] = (crc >> 8) & 0xFF;  /* High byte of CRC second */
    
    dev_info(&client->dev, "Scene mode command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* Write command to command buffer register (0x1d00) */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_err(&client->dev, "Failed to write scene mode command: %d", ret);
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
        dev_info(&client->dev, "Scene mode command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  /* Bits 2-7 */
        
        /* Check if command is still busy */
        if (is_busy) {
            dev_info(&client->dev, "Scene mode command is busy, retrying...");
            retry_count++;
            continue;
        }
        
        /* Check for errors */
        if (has_failed) {
            dev_err(&client->dev, "Scene mode command execution failed with error code: 0x%02X", error_code);
            return -EIO;
        }
        
        /* Command executed successfully */
        dev_info(&client->dev, "Scene mode set successfully");
        return 0;
    }
    
    /* If we get here, we've exceeded max retries */
    dev_err(&client->dev, "Scene mode command timed out after %d retries", max_retries);
    return -ETIMEDOUT;
}

static int rs300_set_ctrl(struct v4l2_ctrl *ctrl)
{
    struct rs300 *rs300 =
        container_of(ctrl->handler, struct rs300, ctrl_handler);
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    int ret = 0;

    /* Add debug info */
    dev_info(&client->dev, "Setting control ID 0x%x to value %d\n", 
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
        dev_info(&client->dev, "FFC trigger received\n");
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
	
	dev_info(&client->dev, "rs300_enum_mbus_code: pad=%d, index=%d", code->pad, code->index);
	
	if (code->pad >= NUM_PADS) {
		dev_err(&client->dev, "Invalid pad %d (max %d)", code->pad, NUM_PADS-1);
		return -EINVAL;
	}

	if (code->pad == IMAGE_PAD) {
		if (code->index >= ARRAY_SIZE(supported_modes)) {
			dev_err(&client->dev, "Invalid index %d for IMAGE_PAD (max %lu)", 
				code->index, ARRAY_SIZE(supported_modes)-1);
			return -EINVAL;
		}

		mutex_lock(&rs300->mutex);
		code->code = supported_modes[code->index].code;
		dev_info(&client->dev, "IMAGE_PAD[%d]: returning format code 0x%x (%s)",
			 code->index, code->code, 
			 code->code == MEDIA_BUS_FMT_YUYV8_1X16 ? "YUYV8_1X16" :
			 code->code == MEDIA_BUS_FMT_YUYV8_2X8 ? "YUYV8_2X8" : "OTHER");
		mutex_unlock(&rs300->mutex);
	} else {
		if (code->index > 0) {
			dev_err(&client->dev, "Invalid index %d for METADATA_PAD (only 0 supported)", code->index);
			return -EINVAL;
		}

		code->code = MEDIA_BUS_FMT_SENSOR_DATA;
		dev_info(&client->dev, "METADATA_PAD: returning SENSOR_DATA format (0x%x)", code->code);
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
		if (fse->index >= ARRAY_SIZE(supported_modes))
			return -EINVAL;

		mutex_lock(&rs300->mutex);
		code = rs300_get_format_code(rs300, fse->code);
		mutex_unlock(&rs300->mutex);
	
	fse->min_width  = supported_modes[fse->index].width;
	fse->max_width  = fse->min_width;
	fse->min_height = supported_modes[fse->index].height;
	fse->max_height = fse->min_height;
	} else {
		if (fse->code != MEDIA_BUS_FMT_SENSOR_DATA || fse->index > 0)
			return -EINVAL;

		fse->min_width = supported_modes[fse->index].width;
		fse->max_width = fse->min_width;
		fse->min_height = supported_modes[fse->index].height;
		fse->max_height = fse->min_height;
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

	dev_info(&client->dev, "rs300_get_pad_fmt: pad=%d, which=%d", 
		fmt->pad, fmt->which);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
        // Use v4l2_subdev_get_fmt instead of v4l2_subdev_get_try_format
        try_fmt = v4l2_subdev_get_fmt(&rs300->sd, sd_state, fmt->pad);

        // Copy the format from userspace (fmt->format) to that location (*try_fmt)
        *try_fmt = fmt->format;

		dev_info(&client->dev, "Get TRY format: code=0x%x, %dx%d",
			fmt->format.code, fmt->format.width, fmt->format.height);
	} else {
		/* Return the active format */
		if (fmt->pad == IMAGE_PAD) {
			fmt->format = rs300->fmt;
			dev_info(&client->dev, "Get ACTIVE format: code=0x%x, %dx%d",
				fmt->format.code, fmt->format.width, fmt->format.height);
                
            // Debug current active mode
            if (rs300->mode) {
                dev_info(&client->dev, "Current active mode: %dx%d @ %d/%d fps",
                    rs300->mode->width, rs300->mode->height,
                    rs300->mode->max_fps.denominator, rs300->mode->max_fps.numerator);
            } else {
                dev_info(&client->dev, "No active mode set yet");
            }
		} else if (fmt->pad == METADATA_PAD && NUM_PADS > 1) {
			/* Set metadata format to match image format for RP1-CFE compatibility */
			fmt->format.code = rs300->fmt.code;  /* Use same format as image pad */
			fmt->format.width = 16384;  /* Standard metadata width for CSI-2 */
			fmt->format.height = 1;     /* Single line metadata */
			fmt->format.field = V4L2_FIELD_NONE;
			/* Copy colorspace parameters from image format */
			fmt->format.colorspace = rs300->fmt.colorspace;
			fmt->format.ycbcr_enc = rs300->fmt.ycbcr_enc;
			fmt->format.quantization = rs300->fmt.quantization;
			fmt->format.xfer_func = rs300->fmt.xfer_func;
			dev_info(&client->dev, "Get METADATA format: code=0x%x, %dx%d",
				fmt->format.code, fmt->format.width, fmt->format.height);
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

	dev_info(&client->dev, "rs300_set_pad_fmt input: pad=%d, which=%d, code=0x%x, width=%d, height=%d",
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
		dev_info(&client->dev, "rs300_set_pad_fmt searching for nearest mode to %dx%d", 
			fmt->format.width, fmt->format.height);

		/* Print all supported modes for debugging */
		for (i = 0; i < ARRAY_SIZE(supported_modes); i++) {
			dev_info(&client->dev, "Supported mode[%d]: %dx%d", 
				i, supported_modes[i].width, supported_modes[i].height);
		}

		/* Use v4l2_find_nearest_size correctly */
		mode = v4l2_find_nearest_size(supported_modes,
					      ARRAY_SIZE(supported_modes),
					      width, height,
					      fmt->format.width, fmt->format.height);

		/* Update the format with the selected mode */
		dev_info(&client->dev, "rs300_set_pad_fmt selected mode: width=%d, height=%d", 
			mode->width, mode->height);

		rs300_update_image_pad_format(rs300, mode, fmt);

		if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
			/* Just update the try format */
			framefmt = v4l2_subdev_get_fmt(sd, sd_state, fmt->pad);
			*framefmt = fmt->format;
			dev_info(&client->dev, "Set TRY format: code=0x%x, %dx%d",
				framefmt->code, framefmt->width, framefmt->height);
		} else {
			/* Update the active format and mode */
			rs300->fmt = fmt->format;
			rs300->mode = mode;
			
			/* Update pixel rate control based on new format */
			if (rs300->pixel_rate) {
				u64 new_pixel_rate = rs300_get_pixel_rate(rs300->fmt.code);
				v4l2_ctrl_s_ctrl_int64(rs300->pixel_rate, new_pixel_rate);
				dev_info(&client->dev, "Updated pixel rate to %llu for format 0x%x",
					 new_pixel_rate, rs300->fmt.code);
			}
			
			dev_info(&client->dev, "Set ACTIVE format: code=0x%x, %dx%d",
				rs300->fmt.code, rs300->fmt.width, rs300->fmt.height);
		}
	} else if (fmt->pad == METADATA_PAD && NUM_PADS > 1) {
		/* Handle metadata pad format if needed */
		if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
			framefmt = v4l2_subdev_get_fmt(sd, sd_state, fmt->pad);
			*framefmt = fmt->format;
		}
		/* For active format, we don't change anything as metadata format is fixed */
	}

	mutex_unlock(&rs300->mutex);
	return 0;
}

static int rs300_set_framefmt(struct rs300 *rs300)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    
    dev_info(&client->dev, "Setting frame format: code=0x%x, %dx%d", 
             rs300->fmt.code, rs300->fmt.width, rs300->fmt.height);
    
    switch (rs300->fmt.code) {
    case MEDIA_BUS_FMT_YUYV8_1X16:
        dev_info(&client->dev, "Using YUYV8_1X16 format (16-bit packed, preferred for RP1-CFE)");
        /* 16-bit packed format - should be compatible with Pi 5 RP1-CFE */
        return 0;
    case MEDIA_BUS_FMT_UYVY8_1X16:
        dev_info(&client->dev, "Using UYVY8_1X16 format (16-bit packed alternative)");
        return 0;
    case MEDIA_BUS_FMT_YUYV8_2X8:
        dev_info(&client->dev, "Using YUYV8_2X8 format (8-bit dual lane, legacy)");
        return 0;
    case MEDIA_BUS_FMT_UYVY8_2X8:
        dev_info(&client->dev, "Using UYVY8_2X8 format (8-bit dual lane, legacy)");
        return 0;
    default:
        dev_err(&client->dev, "Unsupported format code: 0x%x", rs300->fmt.code);
        dev_err(&client->dev, "Supported formats: YUYV8_1X16(0x%x), UYVY8_1X16(0x%x), YUYV8_2X8(0x%x), UYVY8_2X8(0x%x)",
                MEDIA_BUS_FMT_YUYV8_1X16, MEDIA_BUS_FMT_UYVY8_1X16, 
                MEDIA_BUS_FMT_YUYV8_2X8, MEDIA_BUS_FMT_UYVY8_2X8);
        return -EINVAL;
    }        
}

static void rs300_stop_streaming(struct rs300 *rs300)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);

    dev_info(&client->dev, "Stopping streaming");

    /* Write stop registers */
    if (write_regs(client, I2C_VD_BUFFER_RW, stop_regs, sizeof(stop_regs)) < 0) {
        dev_err(&client->dev, "Error writing stop registers");
    }

    dev_info(&client->dev, "Streaming stopped");
}

static int rs300_set_fps(struct rs300 *rs300, int fps)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret;
    int retry_count = 0;
    const int max_retries = 15;
    unsigned short crc;
    
    //check if fps is 25, 30, 50, or 60
    //if not exit function but don't end program
    if (fps != 25 && fps != 30 && fps != 50 && fps != 60) {
        dev_warn(&client->dev, "Invalid FPS value: %d", fps);
        return 0;
    }
    
    dev_info(&client->dev, "Setting camera to %d fps", fps);
    
    /* Construct the command buffer for setting FPS */
    cmd_buffer[0] = 0x10;  /* Command Class */
    cmd_buffer[1] = 0x10;  /* Module Command Index */
    cmd_buffer[2] = 0x46;  /* SubCmd -  MIPI */
    cmd_buffer[3] = 0x00;  /* Reserved */
    cmd_buffer[4] = 0x01;  /* Parameter 1 - Enable*/
    cmd_buffer[5] = 0x03;  /* Parameter 2 - MIPI Progressive*/
    cmd_buffer[6] = fps;   /* Parameter 3 - FPS */
    cmd_buffer[9] = 0x00; 
    cmd_buffer[7] = 0x00; 
    cmd_buffer[8] = 0x00; 
    cmd_buffer[10] = 0x00;
    cmd_buffer[11] = 0x00;
    cmd_buffer[12] = 0x00;  
    cmd_buffer[13] = 0x00;
    cmd_buffer[14] = 0x00;
    cmd_buffer[15] = 0x00;
    
    /* Calculate CRC/checksum for the command */
    crc = do_crc(cmd_buffer, 16);
    /* Swap byte order to match the expected format */
    cmd_buffer[16] = crc & 0xFF;         /* Low byte of CRC first */
    cmd_buffer[17] = (crc >> 8) & 0xFF;  /* High byte of CRC second */
    
    dev_info(&client->dev, "FPS command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
    /* Write command to command buffer register (0x1d00) */
    ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
    if (ret) {
        dev_warn(&client->dev, "Failed to write FPS command: %d", ret);
        return 0;  // Changed from return ret
    }
    
    /* Read command status and wait for completion */
    while (retry_count < max_retries) {
        msleep(300);
        
        ret = read_regs(client, 0x0200, status_buffer, 1);
        if (ret) {
            dev_warn(&client->dev, "Failed to read command status: %d", ret);
            return 0;  // Changed from return ret
        }
        
        dev_info(&client->dev, "FPS command status: 0x%02X", status_buffer[0]);
        
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  /* Bits 2-7 */
        
        if (!is_busy) {
            if (has_failed) {
                dev_warn(&client->dev, "FPS command execution failed with error code: 0x%02X", error_code);
                return 0;  // Changed from return -EIO
            }
            dev_info(&client->dev, "FPS set to %d successfully", fps);
            return 0;
        }
        
        dev_info(&client->dev, "FPS command is busy, retrying...");
        retry_count++;
    }
    
    dev_warn(&client->dev, "FPS command timed out after %d retries", max_retries);
    return 0;  // Changed from return -ETIMEDOUT
}

static void rs300_debug_pipeline_state(struct rs300 *rs300, const char *context)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    
    dev_err(&client->dev, "=== PIPELINE STATE [%s] ===", context);
    dev_err(&client->dev, "Streaming: %d", rs300->streaming);
    dev_err(&client->dev, "Active format: 0x%x (%dx%d)", 
            rs300->fmt.code, rs300->fmt.width, rs300->fmt.height);
    dev_err(&client->dev, "Current mode: %dx%d @ %d/%d fps", 
            rs300->mode->width, rs300->mode->height,
            rs300->mode->max_fps.denominator, rs300->mode->max_fps.numerator);
    dev_err(&client->dev, "Pixel rate for format: %llu", rs300_get_pixel_rate(rs300->fmt.code));
    dev_err(&client->dev, "=== END PIPELINE STATE ===");
}

static int rs300_set_stream(struct v4l2_subdev *sd, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct rs300 *rs300 = to_rs300(sd);
    unsigned short crcdata;
    u8 status_buffer[1];
    int ret = 0;

    dev_err(&client->dev, "=== RS300_SET_STREAM CALLED: enable=%d ===", enable);
    dev_err(&client->dev, "STREAM DEBUG: Current streaming state=%d", rs300->streaming);
    
    rs300_debug_pipeline_state(rs300, "STREAM_ENTRY");
    
    // Add detailed format info when streaming starts
    if (enable) {
        dev_err(&client->dev, "=== STREAM START DEBUG ===");
        dev_err(&client->dev, "Format: 0x%x (%s), Resolution: %dx%d", 
            rs300->fmt.code,
            rs300->fmt.code == MEDIA_BUS_FMT_YUYV8_1X16 ? "YUYV8_1X16" :
            rs300->fmt.code == MEDIA_BUS_FMT_YUYV8_2X8 ? "YUYV8_2X8" : "OTHER",
            rs300->fmt.width, rs300->fmt.height);
        dev_err(&client->dev, "Mode: %dx%d @ %d/%d fps", 
            rs300->mode->width, rs300->mode->height,
            rs300->mode->max_fps.denominator, rs300->mode->max_fps.numerator);
        dev_err(&client->dev, "Pixel rate: %llu Hz, Link rate: %d Hz", 
            rs300_get_pixel_rate(rs300->fmt.code), RS300_LINK_RATE);
        dev_err(&client->dev, "Colorspace: %d, Quantization: %d, Transfer: %d",
            rs300->fmt.colorspace, rs300->fmt.quantization, rs300->fmt.xfer_func);
    } else {
        dev_err(&client->dev, "=== STREAM STOP DEBUG ===");
    }

    mutex_lock(&rs300->mutex);
    if (rs300->streaming == enable) {
        dev_info(&client->dev, "Stream already in desired state");
        mutex_unlock(&rs300->mutex);
        return 0;
    }

    if (enable) {

        // Set FPS first
        ret = rs300_set_fps(rs300, fps);
        if (ret) {
            dev_err(&client->dev, "Failed to set camera to %d fps: %d", fps, ret);
            goto error_unlock;
        }
        dev_info(&client->dev, "FPS is set to %d", fps);

        rs300->streaming = enable;
        start_regs[19] = type;
        start_regs[21] = fps;  // Add this line to set the FPS from the module parameter
        start_regs[22] = rs300->mode->width & 0xff;
        start_regs[23] = rs300->mode->width >> 8;
        start_regs[24] = rs300->mode->height & 0xff;
        start_regs[25] = rs300->mode->height >> 8;

        dev_info(&client->dev, "Start registers before CRC: %*ph", (int)sizeof(start_regs), start_regs);

        //update crc
        crcdata = do_crc((uint8_t*)(start_regs+18), 10);
        start_regs[14] = crcdata & 0xff;
        start_regs[15] = crcdata >> 8;
        
        crcdata = do_crc((uint8_t*)(start_regs), 16);
        start_regs[16] = crcdata & 0xff;
        start_regs[17] = crcdata >> 8;
        
        dev_info(&client->dev, "Start registers after CRC: %*ph", (int)sizeof(start_regs), start_regs);
        dev_info(&client->dev, "Writing start registers to device");
        
        if (write_regs(client, I2C_VD_BUFFER_RW, start_regs, sizeof(start_regs)) < 0) {
            dev_err(&client->dev, "error start rs300\n");
            goto error_unlock;
        }

        // Read back the registers to verify they were written correctly
        u8 verify_regs[sizeof(start_regs)];
        if (read_regs(client, I2C_VD_BUFFER_RW, verify_regs, sizeof(verify_regs)) == 0) {
            dev_info(&client->dev, "Read back registers: %*ph", (int)sizeof(verify_regs), verify_regs);
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
        
        dev_info(&client->dev, "Stream started successfully");

        // Add retry loop for busy status
        int retry = 0;
        const int max_retries = 10;  // Adjust as needed
        while (retry < max_retries) {
            ret = read_regs(client, I2C_VD_BUFFER_STATUS, status_buffer, 1);
            if (ret == 0) {
                dev_info(&client->dev, "Stream status check %d: 0x%02x", retry, status_buffer[0]);
                
                if (!(status_buffer[0] & VCMD_BUSY_STS_BIT)) {
                    dev_info(&client->dev, "Camera is ready");
                    break;
                }
                
                if (status_buffer[0] & VCMD_RST_STS_BIT) {
                    dev_err(&client->dev, "Camera reset failed");
                    ret = -EIO;
                    goto error_unlock;
                }
                
                if (status_buffer[0] & VCMD_ERR_STS_BIT) {
                    dev_err(&client->dev, "Camera error: 0x%02x", status_buffer[0] & VCMD_ERR_STS_BIT);
                    ret = -EIO;
                    goto error_unlock;
                }
            }
            
            msleep(100);  // Wait 100ms between checks
            retry++;
        }

        if (retry >= max_retries) {
            dev_err(&client->dev, "Camera remained busy after %d retries", max_retries);
            ret = -ETIMEDOUT;
            goto error_unlock;
        }

        // Verify streaming status
        msleep(2000);  // Wait a bit after busy clear
        ret = read_regs(client, I2C_VD_BUFFER_STATUS, status_buffer, 1);
        if (ret == 0) {
            dev_info(&client->dev, "Final stream status: 0x%02x", status_buffer[0]);
            if (status_buffer[0] & VCMD_ERR_STS_BIT) {
                dev_err(&client->dev, "Camera reported error after stream start");
                ret = -EIO;
                goto error_unlock;
            }
        }
    } else {
        dev_err(&client->dev, "Stopping stream");
        rs300_stop_streaming(rs300);
        dev_err(&client->dev, "Stream stopped");
    }

    rs300->streaming = enable;
    dev_err(&client->dev, "=== STREAM FUNCTION COMPLETE: enable=%d, ret=%d ===", enable, ret);
    mutex_unlock(&rs300->mutex);

    return ret;

error_unlock:
    dev_err(&client->dev, "=== STREAM ERROR EXIT: ret=%d ===", ret);
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

	dev_info(&client->dev, "rs300_init_cfg");

	/* Initialize the format for the image pad */
	format = v4l2_subdev_get_fmt(sd, state, IMAGE_PAD);
	format->code = supported_modes[mode].code;
	format->width = supported_modes[mode].width;
	format->height = supported_modes[mode].height;
	format->field = V4L2_FIELD_NONE;
	rs300_reset_colorspace(format);

	/* Initialize the format for the metadata pad if needed */
	if (NUM_PADS > 1) {
		format = v4l2_subdev_get_fmt(sd, state, METADATA_PAD);
		format->code = MEDIA_BUS_FMT_SENSOR_DATA;
		format->width = 0;  /* Set appropriate width for metadata */
		
		format->height = 0; /* Set appropriate height for metadata */
		format->field = V4L2_FIELD_NONE;
	}

	return 0;
}

static int rs300_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct rs300 *rs300 = to_rs300(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
    
    dev_info(&client->dev, "rs300_open");
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

    dev_info(dev, "Powering on rs300");  
    
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
    dev_info(dev, "Power on complete");

    return 0;
}

static int rs300_power_off(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct rs300 *rs300 = to_rs300(sd);

	gpiod_set_value_cansleep(rs300->reset_gpio, 1); //logic high -> device tree defines reset: logic high = 0V (active low)
    dev_info(dev, "Resetting rs300");
	regulator_bulk_disable(rs300_NUM_SUPPLIES, rs300->supplies);
    dev_info(dev, "Regulators disabled");

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
	const struct rs300_mode *mode = rs300->mode;

	if (sel->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;

	sel->r.left = 0;
	sel->r.top = 0;
	sel->r.width = mode->width;
	sel->r.height = mode->height;
	return 0;
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
    .step = 1,
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
    .step = 1,
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

static int rs300_init_controls(struct rs300 *rs300)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    struct v4l2_ctrl_handler *ctrl_hdlr;
    // Define link frequency menu items - values must be in ascending order
    static const s64 link_freq_menu[] = {
        RS300_LINK_RATE  // Single link frequency for testing
    };
    int ret;

    dev_info(&client->dev, "Initializing controls");
    
    ctrl_hdlr = &rs300->ctrl_handler;
    ret = v4l2_ctrl_handler_init(ctrl_hdlr, 11);
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

    /* Initialize pixel rate based on default format (YUYV8_1X16) */
    u64 pixel_rate = rs300_get_pixel_rate(MEDIA_BUS_FMT_YUYV8_1X16);
    rs300->pixel_rate = v4l2_ctrl_new_std(ctrl_hdlr, NULL,
                                      V4L2_CID_PIXEL_RATE,
                                      pixel_rate, pixel_rate, 1, 
                                      pixel_rate);
    
    if (rs300->pixel_rate)
        rs300->pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;
    
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

    /* Check for errors */
    if (ctrl_hdlr->error) {
        ret = ctrl_hdlr->error;
        dev_err(&client->dev, "%s control init failed (%d)\n",
            __func__, ret);
        goto error;
    }
    
    /* Connect the control handler to the subdevice */
    rs300->sd.ctrl_handler = ctrl_hdlr;
    
    dev_info(&client->dev, "Control handler initialized successfully\n");

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
    
    dev_info(&client->dev, "Getting device name from camera");
    
    /* Add initial delay to ensure device is ready */
    msleep(50);
    
    /* Test I2C communication first */
    ret = read_regs(client, 0x0200, status_buffer, 1);
    if (ret) {
        dev_err(&client->dev, "Initial I2C communication test failed: %d", ret);
        return ret;
    }
    dev_info(&client->dev, "Initial I2C communication test passed");
    
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
    
    dev_info(&client->dev, "Device name command buffer: %*ph", (int)sizeof(cmd_buffer), cmd_buffer);
    
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
        dev_info(&client->dev, "Device name command status: 0x%02X", status_buffer[0]);
        
        /* Extract status components */
        bool is_busy = (status_buffer[0] & 0x01) != 0;
        bool has_failed = (status_buffer[0] & 0x02) != 0;
        u8 error_code = (status_buffer[0] >> 2) & 0x3F;  /* Bits 2-7 */
        
        /* Check if command is still busy */
        if (is_busy) {
            dev_info(&client->dev, "Device name command is busy, retrying...");
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
        
        dev_info(&client->dev, "Camera device name: %s", device_name);
        dev_info(&client->dev, "Raw response: %*ph", (int)sizeof(result_buffer), result_buffer);
        
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
	
	dev_info(dev, "driver version: %02x.%02x.%02x",
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

	/* Set default mode to 0=640x512, 1=256x192 */
	rs300->mode = &supported_modes[mode];

	/* Initialize default format */
	rs300_set_default_format(rs300);

	/* Configure hardware YUV format to match driver expectation */
	ret = rs300_set_yuv_format(rs300, 2); /* 0 = UYVY format, 1=VYUY format, 2 = YUYV format, 3=YVYU format */
	if (ret) {
		dev_warn(dev, "Failed to set YUV format to UYVY: %d (continuing anyway)", ret);
		/* Don't fail probe on this error, as it's not critical for basic operation */
	}

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
	if (NUM_PADS > 1)
		rs300->pad[METADATA_PAD].flags = MEDIA_PAD_FL_SOURCE;

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

	/* Add debug message to verify control handler is still set */
	if (rs300->sd.ctrl_handler) {
		dev_info(dev, "Subdevice has control handler initialized successfully\n");
	} else {
		dev_warn(dev, "Subdevice control handler is NULL!\n");
	}

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

}

static const struct i2c_device_id rs300_id[] = {
	{ "rs300", 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(i2c, rs300_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id rs300_of_match[] = {
	{ .compatible = "infisense,rs300"  },
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

MODULE_AUTHOR("infisense");
MODULE_DESCRIPTION("rs300 ir camera driver");
MODULE_LICENSE("GPL v2");
