// SPDX-License-Identifier: GPL-2.0
/*
 * rs300 CMOS Image Sensor driver
 *
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd.
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
#include <linux/pm_runtime.h>


#define DRIVER_VERSION			KERNEL_VERSION(0, 0x01, 0x1)
#define DRIVER_NAME "rs300"
//80M (clk)* 2(double ) *2 (lan) /8
#define RS300_LINK_RATE (80 * 1000 * 1000)
#define RS300_PIXEL_RATE		(40 * 1000 * 1000)
#define RS300_BRIGHTNESS_MIN 0
#define RS300_BRIGHTNESS_MAX 100
#define RS300_BRIGHTNESS_STEP 10
#define RS300_BRIGHTNESS_DEFAULT 50

static int mode = 0; //0-640;1-256
static int fps = 30;
static int width = 0;
static int height = 0;
static int type = 16;
static int debug = 1;
module_param(mode, int, 0644);
module_param(fps, int, 0644);
module_param(width, int, 0644);
module_param(height, int, 0644);
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
		0x1e, //fps
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
		0x0e, //fps
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
	u32 width;
	u32 height;
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
	/* YUV 4:2:2 Formats*/
	MEDIA_BUS_FMT_YUYV8_1X16,
	MEDIA_BUS_FMT_UYVY8_1X16,
	MEDIA_BUS_FMT_YUYV8_2X8,
	MEDIA_BUS_FMT_UYVY8_2X8,
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

static  struct rs300_mode supported_modes[] = {
	{ /* 640*/
		.width		= 640,
		.height		= 512,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_YUYV8_1X16,
	},
	{ /* 256*/
		.width		= 256,
		.height		= 256,
		.max_fps = {
			.numerator = 25,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_YUYV8_1X16,
	}
};

static inline struct rs300 *to_rs300(struct v4l2_subdev *sd)
{
	return container_of(sd, struct rs300, sd);
}

static u32 rs300_get_format_code(struct rs300 *rs300, u32 code)
{
	unsigned int i;

	lockdep_assert_held(&rs300->mutex);	

	for (i = 0; i < ARRAY_SIZE(codes); i++)
		if (codes[i] == code)
			break;

	if (i >= ARRAY_SIZE(codes))
		i = 0;	

	return codes[i];
}

static long rs300_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct rs300 *rs300 = to_rs300(sd);
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

static void rs300_set_default_format(struct rs300 *rs300)
{
	struct v4l2_mbus_framefmt *fmt;

	fmt = &rs300->fmt;
	fmt->code = MEDIA_BUS_FMT_YUYV8_1X16;
	fmt->colorspace = V4L2_COLORSPACE_SRGB;
	fmt->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(fmt->colorspace);
	fmt->quantization = V4L2_MAP_QUANTIZATION_DEFAULT(false,
							  fmt->colorspace,
							  fmt->ycbcr_enc);
	fmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(fmt->colorspace);
	fmt->width = supported_modes[0].width;
	fmt->height = supported_modes[0].height;
	fmt->field = V4L2_FIELD_NONE;
}	

/*
 * V4L2 subdev video and pad level operations
 */
static int rs300_set_test_pattern(struct rs300 *rs300, int value)
{
	return 0;
}

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
        msleep(50);
        
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

static int rs300_shutter_calibration(struct rs300 *rs300)
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
        msleep(100);  /* Longer wait time for FFC */
        
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

static int rs300_set_ctrl(struct v4l2_ctrl *ctrl)
{
    struct rs300 *rs300 =
        container_of(ctrl->handler, struct rs300, ctrl_handler);
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    int ret = 0;

    switch (ctrl->id) {
    case V4L2_CID_TEST_PATTERN:
        ret = rs300_set_test_pattern(rs300, ctrl->val);
        break;
    case V4L2_CID_BRIGHTNESS:
        ret = rs300_brightness_correct(rs300, ctrl->val);
        break;
    case V4L2_CID_DO_WHITE_BALANCE:
        /* This is our shutter calibration button */
        if (ctrl->val == 1) {
            ret = rs300_shutter_calibration(rs300);
        }
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
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	
	if (code->pad >= NUM_PADS)
		return -EINVAL;

	if (code->pad == IMAGE_PAD) {
		if (code->index >= ARRAY_SIZE(supported_modes))
			return -EINVAL;

		mutex_lock(&rs300->mutex);
		code->code = supported_modes[code->index].code;
		mutex_unlock(&rs300->mutex);
	} else {
		if (code->index > 0)
			return -EINVAL;

		code->code = MEDIA_BUS_FMT_SENSOR_DATA;
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

		fse->min_width = 640;
		fse->max_width = fse->min_width;
		fse->min_height = 512;
		fse->max_height = fse->min_height;
	}

	return 0;
}

static void rs300_reset_colorspace(struct v4l2_mbus_framefmt *fmt)
{
	fmt->colorspace = V4L2_COLORSPACE_SRGB;
	fmt->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(fmt->colorspace);
	fmt->quantization = V4L2_MAP_QUANTIZATION_DEFAULT(false,
							  fmt->colorspace,
							  fmt->ycbcr_enc);
	fmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(fmt->colorspace);
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

static int __rs300_get_pad_fmt(struct rs300 *rs300,
				struct v4l2_subdev_state *sd_state,
				struct v4l2_subdev_format *fmt)
{
	if (fmt->pad >= NUM_PADS)
		return -EINVAL;	

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		struct v4l2_mbus_framefmt *try_fmt =
			v4l2_subdev_get_try_format(&rs300->sd, sd_state, 
							fmt->pad);
		try_fmt->code = fmt->pad == IMAGE_PAD ?
				rs300_get_format_code(rs300, try_fmt->code) :
				MEDIA_BUS_FMT_SENSOR_DATA;
		fmt->format = *try_fmt;
	} else {
		if (fmt->pad == IMAGE_PAD) {
			rs300_update_image_pad_format(rs300, rs300->mode, 
								fmt);
			fmt->format.code = rs300_get_format_code(rs300, 
								rs300->fmt.code); //is this correct?
		}
	}
	return 0;
}

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

static int rs300_set_pad_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *sd_state,
			  struct v4l2_subdev_format *fmt)
{
	struct rs300 *rs300 = to_rs300(sd);
	const struct rs300_mode *mode;
	struct v4l2_mbus_framefmt *framefmt;
	int brightness_max, brightness_def, pixel_rate;
	unsigned int i;

	if (fmt->pad >= NUM_PADS)
		return -EINVAL;

	mutex_lock(&rs300->mutex);

	if (fmt->pad == IMAGE_PAD) {
		for (i = 0; i < ARRAY_SIZE(codes); i++)
			if (codes[i] == fmt->format.code)
				break;
		if (i >= ARRAY_SIZE(codes))
			i = 0;

		fmt->format.code = rs300_get_format_code(rs300, codes[i]);

		mode = v4l2_find_nearest_size(supported_modes,
					      ARRAY_SIZE(supported_modes),
					      width, height,
					      fmt->format.width,
					      fmt->format.height);
		rs300_update_image_pad_format(rs300, mode, fmt);
		if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
			framefmt = v4l2_subdev_get_try_format(sd, sd_state,
								fmt->pad);
			*framefmt = fmt->format;
		} else if (rs300->mode != mode ||
			   rs300->fmt.code != fmt->format.code) {

			rs300->fmt = fmt->format;
			rs300->mode = mode;

		} else {
			if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {	
				framefmt = v4l2_subdev_get_try_format(sd, sd_state,
								fmt->pad);
				*framefmt = fmt->format;
			}
		}
	}

	mutex_unlock(&rs300->mutex);

	return 0;
}

static int rs300_set_framefmt(struct rs300 *rs300)
{
	switch (rs300->fmt.code) {
	case MEDIA_BUS_FMT_YUYV8_1X16:
		return 0;
	case MEDIA_BUS_FMT_YUYV8_2X8:
		return 0;
	case MEDIA_BUS_FMT_UYVY8_2X8:
		return 0;
	}		

	return -EINVAL;
}

static void rs300_stop_streaming(struct rs300 *rs300)
{
	struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
	int ret;

	/* add device shutdown regs*/
	/**/

	pm_runtime_put(&client->dev);
}

static int rs300_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct rs300 *rs300 = to_rs300(sd);
	unsigned short crcdata;
	int ret = 0;

	mutex_lock(&rs300->mutex);
	if (rs300->streaming == enable) {
		mutex_unlock(&rs300->mutex);
		return 0;
	}

	if (enable) {
	
		ret = pm_runtime_resume_and_get(&client->dev);
		if (ret < 0)
			return ret;

		rs300->streaming = enable;
		start_regs[19]= type;
		start_regs[22]= rs300->mode->width&0xff;
		start_regs[23]= rs300->mode->width>>8;
		start_regs[24]= rs300->mode->height&0xff;
		start_regs[25]= rs300->mode->height>>8;

		//update crc
		crcdata= do_crc((uint8_t*)(start_regs+18),10);
		start_regs[14]=crcdata&0xff;
		start_regs[15]=crcdata>>8;
		
		crcdata= do_crc((uint8_t*)(start_regs),16);
		start_regs[16]=crcdata&0xff;
		start_regs[17]=crcdata>>8;
		
		if (write_regs(client, I2C_VD_BUFFER_RW,start_regs,sizeof(start_regs)) < 0) {
				dev_err(&client->dev, "error start rs300\n");
				goto error_unlock;
		}

		ret = rs300_set_framefmt(rs300);
		if (ret) {
			dev_err(&client->dev, "error set framefmt\n");
			goto error_unlock;
		}
		
	} else {
		rs300_stop_streaming(rs300);
	}

	rs300->streaming = enable;
	mutex_unlock(&rs300->mutex);

	return ret;

error_unlock:
	mutex_unlock(&rs300->mutex);

	return ret;
}

static const s64 link_freq_menu_items[] = {
	RS300_LINK_RATE,//80m
};
/* -----------------------------------------------------------------------------
 * V4L2 subdev internal operations
 */


static int rs300_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct rs300 *rs300 = to_rs300(sd);
	struct v4l2_mbus_framefmt *try_img_fmt =
		v4l2_subdev_get_try_format(sd, fh->state, IMAGE_PAD);

	mutex_lock(&rs300->mutex);
	
	/* Initialize try_fmt */
	try_img_fmt->width = supported_modes[0].width;
	try_img_fmt->height = supported_modes[0].height;
	try_img_fmt->code = supported_modes[0].code;
	try_img_fmt->field = V4L2_FIELD_NONE;

	mutex_unlock(&rs300->mutex);
	return 0;
}


static int rs300_power_on(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct rs300 *rs300 = to_rs300(sd);
	int ret;
	ret = regulator_bulk_enable(rs300_NUM_SUPPLIES, rs300->supplies);
	if (ret) {
		dev_err(dev, "failed to enable regulators\n");
		return ret;
	}

	gpiod_set_value_cansleep(rs300->reset_gpio, 1);

	return 0;
}

static int rs300_power_off(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct rs300 *rs300 = to_rs300(sd);

	gpiod_set_value_cansleep(rs300->reset_gpio, 0);
	regulator_bulk_disable(rs300_NUM_SUPPLIES, rs300->supplies);

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

static const struct v4l2_subdev_pad_ops rs300_subdev_pad_ops = {
	.enum_mbus_code = rs300_enum_mbus_code,
	.get_fmt = rs300_get_pad_fmt,
	.set_fmt = rs300_set_pad_fmt,
	.enum_frame_size = rs300_enum_frame_sizes,
};

static const struct v4l2_subdev_ops rs300_subdev_ops = {
	.core  = &rs300_subdev_core_ops,
	.video = &rs300_subdev_video_ops,
	.pad   = &rs300_subdev_pad_ops,
};

static const struct v4l2_subdev_internal_ops rs300_subdev_internal_ops = {
	.open = rs300_open,
};

static int rs300_init_controls(struct rs300 *rs300)
{
	struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
	struct v4l2_ctrl_handler *ctrl_hdlr;
	int ret;

	ctrl_hdlr = &rs300->ctrl_handler;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 4); /* Increased for shutter cal */
	if (ret)
		return ret;

	ctrl_hdlr->lock = &rs300->mutex;
	
	rs300->link_frequency =	v4l2_ctrl_new_int_menu(ctrl_hdlr, NULL,
		V4L2_CID_LINK_FREQ, 0, 0, link_freq_menu_items);
			
	rs300->pixel_rate = v4l2_ctrl_new_std(&rs300->ctrl_handler, &rs300_ctrl_ops,
					  V4L2_CID_PIXEL_RATE, 0,
					  RS300_PIXEL_RATE, 1,
					  RS300_PIXEL_RATE);

	rs300->brightness = v4l2_ctrl_new_std(ctrl_hdlr, &rs300_ctrl_ops,
					     V4L2_CID_BRIGHTNESS,
					     RS300_BRIGHTNESS_MIN, RS300_BRIGHTNESS_MAX,
					     RS300_BRIGHTNESS_STEP,
					     RS300_BRIGHTNESS_DEFAULT);

	/* Add shutter calibration button */
	rs300->shutter_cal = v4l2_ctrl_new_std(ctrl_hdlr, &rs300_ctrl_ops,
										  V4L2_CID_DO_WHITE_BALANCE, 0, 1, 1, 0);

	if (ctrl_hdlr->error) {
		ret = ctrl_hdlr->error;
		dev_err(&client->dev, "%s control init failed (%d)\n",
			__func__, ret);
		goto error;
	}
	
	rs300->sd.ctrl_handler = ctrl_hdlr;

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

	/* Get reset GPIO */
	dev_dbg(dev, "Getting reset GPIO");
	rs300->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(rs300->reset_gpio)) {
		ret = PTR_ERR(rs300->reset_gpio);
		dev_err(dev, "Failed to get reset GPIO: %d", ret);
		return ret;
	}
	dev_dbg(dev, "Reset GPIO acquired successfully");

	/* Power on the sensor */
	dev_dbg(dev, "Powering on the sensor");
	ret = rs300_power_on(dev);
	if (ret) {
		dev_err(dev, "Failed to power on rs300: %d", ret);
		goto error_power_off;
	}
	dev_dbg(dev, "Sensor powered on successfully");

	/* Set default mode to 640x512 */
	rs300->mode = &supported_modes[0];

	/* Initialize default format */
	rs300_set_default_format(rs300);
	
	ret = rs300_init_controls(rs300);
	if (ret) {
		dev_err(dev, "failed to initialize controls\n");
		goto error_power_off;
	}
	
	/* Initialize subdev */
	rs300->sd.internal_ops = &rs300_subdev_internal_ops;
	rs300->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;
	rs300->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	/* Initialize source pad */
	rs300->pad[0].flags = MEDIA_PAD_FL_SOURCE;

	ret = media_entity_pads_init(&rs300->sd.entity, NUM_PADS, rs300->pad);
	if (ret) {
		dev_err(dev, "failed to init entity pads: %d\n", ret);
		goto error_handler_free;
	}
	
	ret = v4l2_async_register_subdev_sensor(&rs300->sd);
	if (ret < 0) {
		dev_err(dev, "failed to register sensor sub-device: %d\n", ret);
		goto error_media_entity;
	}

	/* Enable runtime PM and turn off the device */
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_idle(dev);

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

	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		pm_runtime_suspend(&client->dev);	
	pm_runtime_set_suspended(&client->dev);
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
