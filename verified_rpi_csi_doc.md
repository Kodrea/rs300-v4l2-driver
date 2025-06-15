# Verified Raspberry Pi 5 CSI-2 Format Documentation

This document contains verified information about Raspberry Pi 5 RP1-CFE (Camera Front End) CSI-2 format support, extracted from official Raspberry Pi Linux kernel driver source code.

## Summary

The Raspberry Pi 5 RP1-CFE driver **only supports 16-bit packed YUV formats** via CSI-2, not the 8-bit dual lane formats¹. This is a critical requirement for camera drivers interfacing with Pi 5.

## Supported YUYV/UYVY Formats

### UYVY Format Support¹
- **V4L2 Pixel Format**: `V4L2_PIX_FMT_UYVY`
- **Media Bus Format**: `MEDIA_BUS_FMT_UYVY8_1X16` ✅ (16-bit packed)
- **Depth**: 16 bits
- **CSI Data Type**: `MIPI_CSI2_DT_YUV422_8B`

### YUYV Format Support¹
- **V4L2 Pixel Format**: `V4L2_PIX_FMT_YUYV`  
- **Media Bus Format**: `MEDIA_BUS_FMT_YUYV8_1X16` ✅ (16-bit packed)
- **Depth**: 16 bits
- **CSI Data Type**: `MIPI_CSI2_DT_YUV422_8B`

### Additional Supported YUV Formats¹
- **YVYU**: `V4L2_PIX_FMT_YVYU` with `MEDIA_BUS_FMT_YVYU8_1X16`
- **VYUY**: `V4L2_PIX_FMT_VYUY` with `MEDIA_BUS_FMT_VYUY8_1X16`

## Unsupported Formats

The RP1-CFE driver **does NOT support** 8-bit dual lane formats¹:
- ❌ `MEDIA_BUS_FMT_UYVY8_2X8` 
- ❌ `MEDIA_BUS_FMT_YUYV8_2X8`
- ❌ `MEDIA_BUS_FMT_YVYU8_2X8`
- ❌ `MEDIA_BUS_FMT_VYUY8_2X8`

## Format Validation Logic

The RP1-CFE driver performs strict format matching in the `cfe_video_link_validate()` function²:

```c
for (i = 0; i < ARRAY_SIZE(formats); i++) {
    if (formats[i].code == remote_fmt->code &&
        formats[i].fourcc == pix_fmt->pixelformat) {
        fmt = &formats[i];
        break;
    }
}
if (!fmt) {
    cfe_err("Format mismatch!");  // Error message seen in dmesg
    return -EINVAL;
}
```

This validation ensures that²:
1. The media bus format code matches exactly
2. The V4L2 pixel format matches exactly
3. Both must be present in the supported formats array

## Media Pipeline Configuration

For proper pipeline configuration with UYVY format³:

```bash
# Set consistent 16-bit packed format throughout pipeline
media-ctl -V "'sensor':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
v4l2-ctl -d /dev/video0 --set-fmt-video=pixelformat=UYVY,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range
```

## Error Messages

When incompatible formats are used, the following error appears in kernel logs²:
```
rp1-cfe 1f00110000.csi: Format mismatch!
rp1-cfe 1f00110000.csi: Failed to start media pipeline: -22
```

## Implementation Requirements

For camera drivers targeting Raspberry Pi 5¹:

1. **Use 16-bit packed formats**: `MEDIA_BUS_FMT_*8_1X16` variants only
2. **Avoid 8-bit dual lane formats**: `MEDIA_BUS_FMT_*8_2X8` variants will be rejected
3. **Set complete colorspace parameters**: Include field, colorspace, xfer, ycbcr, and quantization³
4. **Configure all pipeline components**: Sensor → CSI-2 → Video device must all use consistent formats³

## References and Sources

### Official Raspberry Pi Linux Kernel Sources
- **RP1-CFE Format Definitions**: [cfe_fmts.h](https://github.com/raspberrypi/linux/blob/rpi-6.12.y/drivers/media/platform/raspberrypi/rp1_cfe/cfe_fmts.h)
- **RP1-CFE Driver Source**: [cfe.c](https://github.com/raspberrypi/linux/blob/rpi-6.12.y/drivers/media/platform/raspberrypi/rp1_cfe/cfe.c)
- **Format Validation Function**: `cfe_video_link_validate()` in cfe.c

### Official Linux Kernel Documentation
- **RP1-CFE Driver Documentation**: [Raspberry Pi PiSP Camera Front End (rp1-cfe)](https://docs.kernel.org/admin-guide/media/raspberrypi-rp1-cfe.html)

### Community Resources
- **Raspberry Pi Forums**: [Video capture in YUYV format](https://forums.raspberrypi.com/viewtopic.php?t=319361)
- **Raspberry Pi Forums**: [ADV7282-M on rpi5](https://forums.raspberrypi.com/viewtopic.php?p=2320275)
- **Raspberry Pi Forums**: [Failed to start media pipeline discussions](https://forums.raspberrypi.com/viewtopic.php?t=364984)

### Related Patch Series
- **Linux Kernel Mailing List**: [PATCH v6 4/4] media: admin-guide: Document the Raspberry Pi CFE (rp1-cfe) - [Tomi Valkeinen](https://lore.kernel.org/lkml/20241003-rp1-cfe-v6-4-d6762edd98a8@ideasonboard.com/)

## Verification Date
This documentation was verified on June 15, 2025, using the official Raspberry Pi Linux kernel sources from the rpi-6.12.y branch.

## Related Issues
- GitHub: [Failed to start media pipeline: -32 on pi5 · Issue #194 · raspberrypi/libcamera](https://github.com/raspberrypi/libcamera/issues/194)

---

## Footnotes

**¹** Source: [cfe_fmts.h](https://github.com/raspberrypi/linux/blob/rpi-6.12.y/drivers/media/platform/raspberrypi/rp1_cfe/cfe_fmts.h) - Official Raspberry Pi Linux kernel driver format definitions. This file defines the exact supported format combinations for the RP1-CFE driver, including the specific media bus format codes and V4L2 pixel formats.

**²** Source: [cfe.c](https://github.com/raspberrypi/linux/blob/rpi-6.12.y/drivers/media/platform/raspberrypi/rp1_cfe/cfe.c) - Official Raspberry Pi Linux kernel driver source code. The `cfe_video_link_validate()` function contains the format validation logic and error message generation.

**³** Source: [Raspberry Pi Forums - ADV7282-M on rpi5](https://forums.raspberrypi.com/viewtopic.php?p=2320275) and [Video capture in YUYV format](https://forums.raspberrypi.com/viewtopic.php?t=319361) - Community documentation showing working media pipeline configuration examples for Pi 5.