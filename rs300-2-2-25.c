// SPDX-License-Identifier: GPL-2.0
/*
 * rs300 - Infisense RS300 Thermal Sensor driver
 *
 * Copyright (C) 2024 Your Organization
 * Ported from RV1126 to Raspberry Pi 4B by Your Name
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mediabus.h>

#define DRIVER_NAME "rs300"
#define RS300_PIXEL_RATE       40000000
#define RS300_XVCLK_FREQ       24000000

static const char * const rs300_supply_names[] = {
    "VANA",    // Digital I/O
    "VDIG",     // Analog
    "VDDL",     // Digital Core
};
#define RS300_NUM_SUPPLIES ARRAY_SIZE(rs300_supply_names)

struct rs300_mode {
    u32 width;
    u32 height;
    struct v4l2_fract frame_interval;
    u32 code;
};

struct rs300 {
    struct v4l2_subdev sd;
    struct media_pad pad;
    struct v4l2_ctrl_handler ctrls;
    struct regulator_bulk_data supplies[RS300_NUM_SUPPLIES];
    struct clk *xvclk;
    struct gpio_desc *pwdn_gpio;
    struct mutex lock;
    struct i2c_client *client;
    const struct rs300_mode *mode;
    bool streaming;
};

static const struct rs300_mode supported_modes[] = {
    {
        .width = 640,
        .height = 512,
        .frame_interval = { 1, 30 },
        .code = MEDIA_BUS_FMT_YUYV8_2X8,
    },
    {
        .width = 1280,
        .height = 1024,
        .frame_interval = { 1, 30 },
        .code = MEDIA_BUS_FMT_YUYV8_2X8,
    },
};

static inline struct rs300 *to_rs300(struct v4l2_subdev *sd)
{
    return container_of(sd, struct rs300, sd);
}

/* CRC calculation from original driver */
static unsigned short rs300_crc(unsigned char *ptr, int len)
{
    unsigned int i;
    unsigned short crc = 0x0000;
    
    while(len--) {
        crc ^= (unsigned short)(*ptr++) << 8;
        for (i = 0; i < 8; ++i) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

static int rs300_write_regs(struct i2c_client *client, u16 reg, 
                           const u8 *buf, u16 len)
{
    struct i2c_msg msg;
    u8 *data;
    int ret;
    
    data = kmalloc(len + 2, GFP_KERNEL);
    if (!data)
        return -ENOMEM;
    
    data[0] = reg >> 8;
    data[1] = reg & 0xff;
    memcpy(data + 2, buf, len);
    
    msg.addr = client->addr;
    msg.flags = 0;
    msg.buf = data;
    msg.len = len + 2;
    
    ret = i2c_transfer(client->adapter, &msg, 1);
    kfree(data);
    
    return ret == 1 ? 0 : -EIO;
}

static int rs300_send_start(struct i2c_client *client)
{
    u8 start_regs[] = { /* Original start sequence */ };
    unsigned short crc;
    
    /* Update parameters */
    start_regs[22] = supported_modes[0].width & 0xff;
    start_regs[23] = supported_modes[0].width >> 8;
    start_regs[24] = supported_modes[0].height & 0xff;
    start_regs[25] = supported_modes[0].height >> 8;

    /* Calculate CRCs */
    crc = rs300_crc(&start_regs[18], 10);
    start_regs[14] = crc & 0xff;
    start_regs[15] = crc >> 8;
    
    crc = rs300_crc(start_regs, 16);
    start_regs[16] = crc & 0xff;
    start_regs[17] = crc >> 8;

    return rs300_write_regs(client, 0x1d00, start_regs, sizeof(start_regs));
}

static int rs300_send_stop(struct i2c_client *client)
{
    u8 stop_regs[] = { /* Original stop sequence */ };
    /* Similar CRC calculation as start */
    return rs300_write_regs(client, 0x1d00, stop_regs, sizeof(stop_regs));
}

static int rs300_power_on(struct device *dev)
{
    struct rs300 *rs300 = dev_get_drvdata(dev);
    int ret;
    
    ret = regulator_bulk_enable(RS300_NUM_SUPPLIES, rs300->supplies);
    if (ret) {
        dev_err(dev, "Failed to enable regulators\n");
        return ret;
    }
    
    if (rs300->pwdn_gpio) {
        gpiod_set_value_cansleep(rs300->pwdn_gpio, 0);
        usleep_range(2000, 5000);
    }
    
    ret = clk_prepare_enable(rs300->xvclk);
    if (ret) {
        dev_err(dev, "Failed to enable xvclk\n");
        regulator_bulk_disable(RS300_NUM_SUPPLIES, rs300->supplies);
        return ret;
    }
    
    usleep_range(5000, 10000);
    return 0;
}

static int rs300_power_off(struct device *dev)
{
    struct rs300 *rs300 = dev_get_drvdata(dev);
    
    clk_disable_unprepare(rs300->xvclk);
    if (rs300->pwdn_gpio)
        gpiod_set_value_cansleep(rs300->pwdn_gpio, 1);
    regulator_bulk_disable(RS300_NUM_SUPPLIES, rs300->supplies);
    return 0;
}

/* Add this after rs300_power_off function */
static const struct dev_pm_ops rs300_pm_ops = {
    SET_RUNTIME_PM_OPS(rs300_power_off, rs300_power_on, NULL)
    SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
                          pm_runtime_force_resume)
};

static int rs300_s_stream(struct v4l2_subdev *sd, int enable)
{
    struct rs300 *rs300 = to_rs300(sd);
    struct i2c_client *client = rs300->client;
    int ret = 0;
    
    mutex_lock(&rs300->lock);
    if (rs300->streaming == enable)
        goto unlock;
    
    if (enable) {
        ret = pm_runtime_get_sync(&client->dev);
        if (ret < 0)
            goto unlock;
        
        ret = rs300_send_start(client);
        if (ret)
            dev_err(&client->dev, "Start streaming failed\n");
    } else {
        ret = rs300_send_stop(client);
        if (ret)
            dev_err(&client->dev, "Stop streaming failed\n");
        pm_runtime_put(&client->dev);
    }
    
    rs300->streaming = enable;
    
unlock:
    mutex_unlock(&rs300->lock);
    return ret;
}

/* V4L2 Subdev Operations */
static const struct v4l2_subdev_video_ops rs300_video_ops = {
    .s_stream = rs300_s_stream,
};

static int rs300_enum_mbus_code(struct v4l2_subdev *sd,
                               struct v4l2_subdev_state *cfg,
                               struct v4l2_subdev_mbus_code_enum *code)
{
    if (code->index >= ARRAY_SIZE(supported_modes))
        return -EINVAL;
    
    code->code = supported_modes[code->index].code;
    return 0;
}

static int rs300_get_fmt(struct v4l2_subdev *sd,
                        struct v4l2_subdev_state *cfg,
                        struct v4l2_subdev_format *fmt)
{
    struct rs300 *rs300 = to_rs300(sd);
    
    fmt->format.width = rs300->mode->width;
    fmt->format.height = rs300->mode->height;
    fmt->format.code = rs300->mode->code;
    fmt->format.field = V4L2_FIELD_NONE;
    fmt->format.colorspace = V4L2_COLORSPACE_RAW;
    
    return 0;
}

/* Update pad operations structure */
static const struct v4l2_subdev_pad_ops rs300_pad_ops = {
    .enum_mbus_code = rs300_enum_mbus_code,
    .get_fmt = rs300_get_fmt,
};

static const struct v4l2_subdev_ops rs300_subdev_ops = {
    .video = &rs300_video_ops,
    .pad = &rs300_pad_ops,
};

static int rs300_probe(struct i2c_client *client)
{
    dev_info(&client->dev, "RS300 probe started\n");
    struct device *dev = &client->dev;
    struct rs300 *rs300;
    int ret;
    
    rs300 = devm_kzalloc(dev, sizeof(*rs300), GFP_KERNEL);
    if (!rs300)
        return -ENOMEM;
    
    rs300->client = client;
    mutex_init(&rs300->lock);
    
    /* Hardware resources */
    rs300->xvclk = devm_clk_get(dev, "xvclk");
    if (IS_ERR(rs300->xvclk))
        return dev_err_probe(dev, PTR_ERR(rs300->xvclk), "Failed to get xvclk\n");
    
    rs300->pwdn_gpio = devm_gpiod_get_optional(dev, "pwdn", GPIOD_OUT_HIGH);
    if (IS_ERR(rs300->pwdn_gpio))
        return PTR_ERR(rs300->pwdn_gpio);
    
    ret = devm_regulator_bulk_get(dev, RS300_NUM_SUPPLIES, rs300->supplies);
    if (ret) {
        for (int i = 0; i < RS300_NUM_SUPPLIES; i++) {
            dev_err(dev, "Supply %d: %s\n", i, 
                   rs300->supplies[i].supply ?: "NULL");
        }
        return ret;
    }
    
    /* Initialize V4L2 subdev */
    v4l2_i2c_subdev_init(&rs300->sd, client, &rs300_subdev_ops);
    rs300->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
    rs300->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
    
    /* Media pad */
    rs300->pad.flags = MEDIA_PAD_FL_SOURCE;
    ret = media_entity_pads_init(&rs300->sd.entity, 1, &rs300->pad);
    if (ret)
        return ret;
    
    /* Runtime PM */
    pm_runtime_set_active(dev);
    pm_runtime_enable(dev);
    pm_runtime_idle(dev);
    
    ret = v4l2_async_register_subdev_sensor(&rs300->sd);
    if (ret) {
        dev_err(dev, "Failed to register subdev\n");
        goto err_pm;
    }
    
    dev_info(dev, "RS300 initialized successfully\n");
    return 0;
    
err_pm:
    pm_runtime_disable(dev);
    media_entity_cleanup(&rs300->sd.entity);
    return ret;
}

static void rs300_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    struct rs300 *rs300 = to_rs300(sd);
    
    v4l2_async_unregister_subdev(sd);
    media_entity_cleanup(&rs300->sd.entity);
    pm_runtime_disable(&client->dev);
}

static const struct of_device_id rs300_of_match[] = {
    { .compatible = "infisense,rs300" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, rs300_of_match);

static struct i2c_driver rs300_i2c_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = rs300_of_match,
        .pm = pm_sleep_ptr(&rs300_pm_ops),
    },
    .probe = rs300_probe,
    .remove = rs300_remove,
};

module_i2c_driver(rs300_i2c_driver);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Infisense RS300 Thermal Sensor Driver");
MODULE_LICENSE("GPL");