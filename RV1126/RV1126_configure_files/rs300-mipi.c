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
#include <linux/rk-camera-module.h>
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

static int mode = 0; //0--1280;1--640
static int fps = 30;
static int width = 0;
static int height = 0;
static int type = 16;
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
static int read_regs(struct i2c_client *client,  u16 reg, u8 *val ,int len )
{
	struct i2c_msg msg[2];
	unsigned char data[4] = { 0, 0, 0, 0 };

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
	i2c_transfer(client->adapter, msg, 2);
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
static int write_regs(struct i2c_client *client,  u16 reg, u8 *val,int len)
{
	struct i2c_msg msg[1];
	unsigned char *outbuf = (unsigned char *)kmalloc(sizeof(unsigned char)*(len+2), GFP_KERNEL);
	int ret=0;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = len+2;
	msg->buf = outbuf;
	outbuf[0] = reg>>8;
    outbuf[1] = reg&0xff;
	memcpy(outbuf+2, val, len);
	ret = i2c_transfer(client->adapter, msg, 1);
	kfree(outbuf);
	return ret;
	// if (reg & I2C_VD_CHECK_ACCESS){
	// 	return ret;
	// }else
	// {
	// 	return check_access_done(client,2000);//命令超时控制，由于应用层已经控制这里不需要了
	// }
}



struct rs300_framesize {
	u16 width;
	u16 height;
	struct v4l2_fract max_fps;
	u32 code;
};

struct pll_ctrl_reg {
	unsigned int div;
	unsigned char reg;
};

static const char * const rs300_supply_names[] = {
	"dovdd",	/* Digital I/O power */
	"avdd",		/* Analog power */
	"dvdd",		/* Digital core power */
};

#define rs300_NUM_SUPPLIES ARRAY_SIZE(rs300_supply_names)

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

static  struct rs300_framesize rs300_framesizes[] = {
	{ /* 640*/
		.width		= 640,
		.height		= 900,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
	},
	{ /* 1280 */
		.width		= 1280,
		.height		= 1024,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
		//.code = MEDIA_BUS_FMT_SPD_2X8,
	},

	{ /* image+temp */
		.width		= 640,
		.height		= 512,
		.max_fps = {
			.numerator = 30,
			.denominator = 1,
		},
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
	}
};



static inline struct rs300 *to_rs300(struct v4l2_subdev *sd)
{
	return container_of(sd, struct rs300, sd);
}




static void rs300_get_default_format(struct v4l2_mbus_framefmt *format,int index)
{
	if(width>>(index*16))rs300_framesizes[index].width=(width>>(index*16))&0xffff;
	if(height>>(index*16))rs300_framesizes[index].height=(height>>(index*16))&0xffff;
	format->width = rs300_framesizes[index].width;
	format->height = rs300_framesizes[index].height;
	format->colorspace = V4L2_COLORSPACE_SRGB;
	format->code = rs300_framesizes[index].code;
	format->field = V4L2_FIELD_NONE;
}

/*
 * V4L2 subdev video and pad level operations
 */

static int rs300_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct rs300 *rs300 = to_rs300(sd);
	if (code->index >= 1)
		return -EINVAL;
	code->code = rs300_framesizes[rs300->module_index].code;

	return 0;
}

static int rs300_enum_frame_sizes(struct v4l2_subdev *sd,
				   struct v4l2_subdev_pad_config *cfg,
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
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct rs300 *rs300 = to_rs300(sd);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
		struct v4l2_mbus_framefmt *mf;

		mf = v4l2_subdev_get_try_format(sd, cfg, 0);
		mutex_lock(&rs300->lock);
		fmt->format = *mf;
		mutex_unlock(&rs300->lock);
		return 0;
#else
	return -ENOTTY;
#endif
	}

	mutex_lock(&rs300->lock);
	rs300_get_default_format(&fmt->format,rs300->module_index);
	mutex_unlock(&rs300->lock);
	return 0;
}

static int rs300_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct rs300 *rs300 = to_rs300(sd);
	rs300_framesizes[rs300->module_index].width = fmt->format.width;
	rs300_framesizes[rs300->module_index].height = fmt->format.height;
	rs300_get_default_format(&fmt->format,rs300->module_index);

	return 0;
}

static void rs300_get_module_inf(struct rs300 *rs300,
				  struct rkmodule_inf *inf)
{
	memset(inf, 0, sizeof(*inf));
	strlcpy(inf->base.sensor, DRIVER_NAME, sizeof(inf->base.sensor));
	strlcpy(inf->base.module, rs300->module_name,
		sizeof(inf->base.module));
	strlcpy(inf->base.lens, rs300->len_name, sizeof(inf->base.lens));
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
			v4l_info(client,"rs300 %d %d %d  \n",cmd, valp->wIndex,valp->wLength);
		}else{
			v4l_err(client, "rs300 args error \n");
			return -EFAULT;
		}
	}
	switch (cmd) {
	case RKMODULE_GET_MODULE_INFO:
		rs300_get_module_inf(rs300, (struct rkmodule_inf *)arg);
		break;

	case CMD_GET:
		data = kmalloc(valp->wLength, GFP_KERNEL);
		read_regs(client,valp->wIndex,data,valp->wLength);

		if (copy_to_user(valp->data, data, valp->wLength))
		{
			ret = -EFAULT;
			v4l_err(client, "error stop rs300\n");
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

static int rs300_s_stream(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct rs300 *rs300 = to_rs300(sd);
	unsigned short crcdata;
	int ret = 0;
	if (on) {
		if (rs300->streaming == 1)
			return 0;
		rs300->streaming = 1;
		start_regs[19]= type;
		start_regs[22]= rs300_framesizes[rs300->module_index].width&0xff;
		start_regs[23]= rs300_framesizes[rs300->module_index].width>>8;
		start_regs[24]= rs300_framesizes[rs300->module_index].height&0xff;
		start_regs[25]= rs300_framesizes[rs300->module_index].height>>8;
		//startpreview
		v4l_info(client,"tiny startpreview\n");
		//update crc
		crcdata= do_crc((uint8_t*)(start_regs+18),10);
		start_regs[14]=crcdata&0xff;
		start_regs[15]=crcdata>>8;
		
		crcdata= do_crc((uint8_t*)(start_regs),16);
		start_regs[16]=crcdata&0xff;
		start_regs[17]=crcdata>>8;
		
		if (write_regs(client, I2C_VD_BUFFER_RW,start_regs,sizeof(start_regs)) < 0) {
				v4l_err(client, "error start rs300\n");
				return -ENODEV;
		}
	}else {
		if (rs300->streaming == 0)
			return 0;
		rs300->streaming = 0;
		//stoppreview
		v4l_info(client,"tiny stoppreview\n");
		crcdata= do_crc((uint8_t*)(stop_regs+18),10);
		stop_regs[14]=crcdata&0xff;
		stop_regs[15]=crcdata>>8;
		
		crcdata= do_crc((uint8_t*)(stop_regs),16);
		stop_regs[16]=crcdata&0xff;
		stop_regs[17]=crcdata>>8;
		//停图再看时间太长就不关了
		/*
		if (write_regs(client, I2C_VD_BUFFER_RW,stop_regs,sizeof(stop_regs)) < 0) {
				v4l_err(client, "error stop rs300\n");
				return -ENODEV;
		}*/
	}
	return ret;
}

static int rs300_set_test_pattern(struct rs300 *rs300, int value)
{
	return 0;
}

static int rs300_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct rs300 *rs300 =
			container_of(ctrl->handler, struct rs300, ctrls);

	switch (ctrl->id) {
	case V4L2_CID_TEST_PATTERN:
		return rs300_set_test_pattern(rs300, ctrl->val);
	}

	return 0;
}

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
				v4l2_subdev_get_try_format(sd, fh->pad, 0);

	dev_dbg(&client->dev, "%s:\n", __func__);

	rs300_get_default_format(format,rs300->module_index);

	return 0;
}
#endif

static int rs300_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *config)
{
	config->type = V4L2_MBUS_CSI2;
	config->flags = 1 << (2 - 1) |
		      V4L2_MBUS_CSI2_CHANNEL_0 ;
		      //V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;

	return 0;
}

static int rs300_enum_frame_interval(struct v4l2_subdev *sd,
				       struct v4l2_subdev_pad_config *cfg,
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

static const struct v4l2_subdev_core_ops rs300_subdev_core_ops = {
	.log_status = v4l2_ctrl_subdev_log_status,
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
	.ioctl = rs300_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = rs300_compat_ioctl32,
#endif
};

static int rs300_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{

	fi->interval = rs300_framesizes[0].max_fps;

	return 0;
}

static const struct v4l2_subdev_video_ops rs300_subdev_video_ops = {
	.s_stream = rs300_s_stream,
	.g_mbus_config = rs300_g_mbus_config,
	.g_frame_interval = rs300_g_frame_interval,
};

static const struct v4l2_subdev_pad_ops rs300_subdev_pad_ops = {
	.enum_mbus_code = rs300_enum_mbus_code,
	.enum_frame_size = rs300_enum_frame_sizes,
	.enum_frame_interval = rs300_enum_frame_interval,
	.get_fmt = rs300_get_fmt,
	.set_fmt = rs300_set_fmt,
};

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static const struct v4l2_subdev_ops rs300_subdev_ops = {
	.core  = &rs300_subdev_core_ops,
	.video = &rs300_subdev_video_ops,
	.pad   = &rs300_subdev_pad_ops,
};

static const struct v4l2_subdev_internal_ops rs300_subdev_internal_ops = {
	.open = rs300_open,
};
#endif




static int rs300_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct device_node *node = dev->of_node;
	struct v4l2_subdev *sd;
	struct rs300 *rs300;
	char facing[2];
	int ret;

	dev_info(dev, "driver version: %02x.%02x.%02x",
		DRIVER_VERSION >> 16,
		(DRIVER_VERSION & 0xff00) >> 8,
		DRIVER_VERSION & 0x00ff);

	rs300 = devm_kzalloc(&client->dev, sizeof(*rs300), GFP_KERNEL);
	if (!rs300)
		return -ENOMEM;

	ret = of_property_read_u32(node, RKMODULE_CAMERA_MODULE_INDEX,
				   &rs300->module_index);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_MODULE_FACING,
				       &rs300->module_facing);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_MODULE_NAME,
				       &rs300->module_name);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_LENS_NAME,
				       &rs300->len_name);


	rs300->client = client;
 
 
	//获取设备树配置引脚，这里非常关键，RKISP1的驱动不会自动配置pinctrl切换引脚复用到dvp模式。
	//cif驱动包含引脚状态切换代码，无需再控制,注意引脚与网口复用
	/*
 	struct pinctrl *pinctrl=devm_pinctrl_get(&client->dev);
	struct pinctrl_state * pins_default = pinctrl_lookup_state(
					pinctrl,
					"rockchip,camera_default");
	if(IS_ERR(pins_default))
		dev_err(&client->dev, "%s: pinctrl_lookup_state error\n",
			__func__);				
	 ret = pinctrl_select_state(pinctrl, pins_default);
	if (ret < 0){
		dev_err(&client->dev, "%s: pinctrl_select_state error %d\n",
			__func__, ret);
		goto error;
	}*/
	v4l2_ctrl_handler_init(&rs300->ctrls, 2);
	rs300->link_frequency =	v4l2_ctrl_new_int_menu(&rs300->ctrls, NULL,
		V4L2_CID_LINK_FREQ, 0, 0, link_freq_menu_items);
			
	rs300->pixel_rate = v4l2_ctrl_new_std(&rs300->ctrls, &rs300_ctrl_ops,
					  V4L2_CID_PIXEL_RATE, 0,
					  rs300_PIXEL_RATE, 1,
					  rs300_PIXEL_RATE);

	rs300->sd.ctrl_handler = &rs300->ctrls;

	if (rs300->ctrls.error) {
		dev_err(&client->dev, "%s: control initialization error %d\n",
			__func__, rs300->ctrls.error);
		return  rs300->ctrls.error;
	}

	sd = &rs300->sd;
	v4l2_i2c_subdev_init(sd, client, &rs300_subdev_ops);


#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	sd->internal_ops = &rs300_subdev_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE ;
			 
#endif

#if defined(CONFIG_MEDIA_CONTROLLER)
	rs300->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&sd->entity, 1, &rs300->pad);
	if (ret < 0) {
		v4l2_ctrl_handler_free(&rs300->ctrls);
		return ret;
	}
#endif

	mutex_init(&rs300->lock);


	memset(facing, 0, sizeof(facing));
	if (strcmp(rs300->module_facing, "back") == 0)
		facing[0] = 'b';
	else
		facing[0] = 'f';

	snprintf(sd->name, sizeof(sd->name), "m%02d_%s_%s %s",
		 rs300->module_index, facing,
		 DRIVER_NAME, dev_name(sd->dev));
		 printk("rs300 %s\n",sd->name);
	ret = v4l2_async_register_subdev_sensor_common(sd);
	if (ret)
		goto error;

	dev_info(&client->dev, "%s sensor driver registered !!\n", sd->name);

	return 0;

error:
	v4l2_ctrl_handler_free(&rs300->ctrls);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif
	mutex_destroy(&rs300->lock);
	return ret;
}

static int rs300_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct rs300 *rs300 = to_rs300(sd);

	v4l2_ctrl_handler_free(&rs300->ctrls);
	v4l2_async_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif
	mutex_destroy(&rs300->lock);

	return 0;
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
