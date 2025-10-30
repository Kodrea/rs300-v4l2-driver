# RS300 Hardware Purchasing Guide

Complete guide to purchasing the RS300 thermal camera module and required accessories for Raspberry Pi integration.

## Current Availability

### Official Store (Recommended)

**Purple River Technology - thermal-image.com**

- **Direct Link**: [RS300 Mini2 640×512 9mm Module](https://www.thermal-image.com/product/mini2-640x512-9mm-thermal-imaging-camera-module-for-drones/)
- **Product**: Mini2 640×512 with 9mm lens
- **Includes**: Thermal camera module + custom Raspberry Pi adapter board
- **Discount Code**: `HXZUK8WG`
  - ⚠️ **IMPORTANT**: Use this code to ensure your order includes the custom RPi adapter board
  - **Savings**: $30 USD off
  - **Required**: Without this code, you may receive their standard board instead

**Available Resolutions**:
- ✅ **640×512** - Currently listed with 9mm lens
- 📧 **384×288** - Contact customer support to order
- 📧 **256×192** - Contact customer support to order

**Available Lens Options**:
- ✅ **9mm** - Currently listed (standard)
- 📧 **15mm** - Contact customer support
- 📧 **25mm** - Contact customer support
- 📧 **Custom** - Contact customer support for other options

**Customer Support**:
- Purple River Technology provides excellent customer support
- Contact them directly for:
  - Different resolutions (384×288, 256×192)
  - Alternative lens options
  - Custom PCB modifications
  - Technical assistance

### Update History

**May 2, 2025**:
- Boards now available for purchase with module
- Currently only 640×512 with 9mm lens listed
- Custom RPi adapter board included with discount code
- Other configurations available via customer support

**March 27, 2025**:
- Announcement: MIPI CSI-2 boards for Raspberry Pi approximately two weeks out

## Alternative Sources

### Alibaba Stores

**Note**: These sources were used during development. Official store (thermal-image.com) is now the recommended purchasing method.

#### 1. Purple River Technology (Alibaba)
- **Store**: [Purple River Alibaba Store](https://purpleriver.en.alibaba.com/index.html?spm=a2700.details.0.0.1a245460PSIafr&from=detail&productId=1601081970203)
- **Experience**: Excellent customer support
- **Custom PCB**: Purple River developed the custom Raspberry Pi adapter board
- **Previous Purchase**: 256×192 module with 15mm lens
- **Recommendation**: ⭐⭐⭐⭐⭐

#### 2. Shenzhen Chengen Thermovisiontechnology (Alibaba)
- **Store**: [Chengen Alibaba Store](https://cersnv.en.alibaba.com/index.html?spm=a2700.details.0.0.7d0f2dfbCpGJwE&from=detail&productId=1601308525861)
- **Previous Purchase**: 640×512 module with 25mm lens
- **Note**: Used with Purple River PCB adapter board
- **Same Module**: Both sellers provide the same Mini2 module

## What's Included

### When Ordering with Discount Code

✅ **RS300 Mini2 Thermal Module**
- Thermal imaging camera
- Chosen resolution (640×512, 384×288, or 256×192)
- Chosen lens (9mm, 15mm, 25mm, etc.)

✅ **Custom Raspberry Pi Adapter Board**
- 15-pin FPC connector (compatible with Raspberry Pi cameras)
- USB 2.0 interface
- CVBS analog output
- I2C communication interface
- Proper pinout for Raspberry Pi integration

### Board Design History

**Original Design**:
- Required same-sided FPC ribbon cable
- Not compatible with standard Raspberry Pi camera cables

**Updated Design** (Current):
- Compatible with reverse head cable (standard for Raspberry Pi cameras)
- **Critical for**: Using 15-pin to 22-pin adapter cable for Pi 5, Zero, and Compute Modules
- Standard 15-pin FPC connection

## Required Accessories

### For Raspberry Pi 5

**22-pin to 15-pin FPC Adapter Cable** (Required)
- Converts Pi 5's 22-pin FPC connector to standard 15-pin
- Available from Raspberry Pi retailers
- Example: [Adafruit #5832](https://www.adafruit.com/product/5832)
- Cost: ~$6-10 USD

**15-pin FPC Ribbon Cable** (Usually included with module)
- Same-sided (reverse head) cable
- Standard Raspberry Pi camera cable type
- Length: 15-30cm typical

### For Raspberry Pi 4B

**15-pin FPC Ribbon Cable** (Usually included with module)
- Same-sided (reverse head) cable
- Standard Raspberry Pi camera cable
- Connects directly to Pi 4 CSI port

### Optional Accessories

**USB 2.0 Cable** (for USB mode testing)
- Micro USB or USB-C depending on board revision
- Usually included with module

**CVBS Cable** (for analog output)
- Composite video cable
- Optional, for analog video applications

## Module Selection Guide

### Resolution Selection

**Choose 640×512 if**:
- ✅ You need maximum detail and resolution
- ✅ You're using Raspberry Pi 5 or 4B
- ✅ You want 60fps via MIPI (not available via USB)
- ✅ You have adequate cooling and power supply

**Choose 384×288 if**:
- ✅ You need good detail with lower bandwidth
- ✅ 60fps via USB is acceptable
- ✅ You want balance of performance and cost

**Choose 256×192 if**:
- ✅ Lower resolution is sufficient
- ✅ You prefer 50Hz PAL standard frame rates
- ✅ USB mode is acceptable (MIPI troubleshooting in progress)
- ⚠️ Note: MIPI support on Pi 4 is being troubleshooted

### Lens Selection

| Lens | FOV | Best For |
|------|-----|----------|
| **9mm** | Wide | General purpose, close-range thermal imaging |
| **15mm** | Medium | Balanced FOV, moderate distances |
| **25mm** | Narrow | Long-range detection, detailed far objects |

**Recommendation**: Start with 9mm (default) for general purpose use

## Platform Compatibility

Before purchasing, verify your Raspberry Pi is compatible:

| Platform | Compatibility | Notes |
|----------|---------------|-------|
| **Raspberry Pi 5** | ✅ Recommended | Full support, 60fps, all features |
| **Raspberry Pi 4B** | ✅ Compatible | 60fps, requires manual config |
| **Pi Zero 2W** | ❌ Not Compatible | Power limitations cause brownouts |
| **Older Pi Models** | ❌ Not Tested | Not recommended |

**See**: [Hardware Compatibility Matrix](compatibility.md) for detailed testing results

## Total Cost Estimate

**Complete Setup for Raspberry Pi 5**:
- RS300 Module (640×512, 9mm): ~$XXX USD (check thermal-image.com)
- Discount (HXZUK8WG): -$30 USD
- Pi 5 Adapter Cable (22-pin to 15-pin): ~$8 USD
- **Total**: ~$XXX USD (varies by module config)

**Complete Setup for Raspberry Pi 4B**:
- RS300 Module (640×512, 9mm): ~$XXX USD (check thermal-image.com)
- Discount (HXZUK8WG): -$30 USD
- FPC Cable (usually included): $0
- **Total**: ~$XXX USD (varies by module config)

**Platform Cost** (if you don't have one):
- Raspberry Pi 5 (8GB): ~$80 USD
- Raspberry Pi 4B (8GB): ~$75 USD
- Power supply: ~$8-12 USD
- microSD card (32GB+): ~$10 USD

## Ordering Process

### Step-by-Step

1. **Visit Store**: Go to [thermal-image.com](https://www.thermal-image.com/product/mini2-640x512-9mm-thermal-imaging-camera-module-for-drones/)

2. **Select Configuration**:
   - For listed products: Add to cart directly
   - For other resolutions/lenses: Contact customer support first

3. **Apply Discount Code**: Enter `HXZUK8WG` at checkout
   - ⚠️ **Verify**: Confirm custom RPi board is included in order
   - ⚠️ **Contact Support**: If discount doesn't apply or board not listed

4. **Verify Order Includes**:
   - ✅ RS300 Mini2 thermal module (correct resolution)
   - ✅ Custom Raspberry Pi adapter board
   - ✅ FPC ribbon cable
   - ✅ USB cable (usually)

5. **Complete Purchase**: Follow standard checkout process

6. **Installation**: Once received, follow [Installation Guide](../getting-started/)

## Support & Resources

### Pre-Purchase Questions

**Contact Purple River Technology**:
- Website: [thermal-image.com](https://www.thermal-image.com/)
- Ask about:
  - Different resolutions
  - Lens options
  - Custom configurations
  - Bulk orders
  - Technical specifications

### Post-Purchase Support

**Driver & Software Support**:
- 📖 [Installation Guide](../getting-started/)
- 🐛 [GitHub Issues](https://github.com/Kodrea/rs300-v4l2-driver/issues)
- 📺 [Video Tutorials](https://linktr.ee/kodrea)
- 📖 [Full Documentation](../../)

**Hardware Support**:
- Contact Purple River Technology for hardware issues
- Check [Troubleshooting Guide](../../TROUBLESHOOTING.md) for common issues

## Video Tutorials

**Installation & Setup Guides**:
- 📺 [Link Tree](https://linktr.ee/kodrea) - Collection of tutorial videos
- Topics covered:
  - Unboxing and hardware inspection
  - Driver installation
  - First thermal capture
  - Camera controls and settings

## Warranty & Returns

**Important**: Check with Purple River Technology for:
- Warranty period and coverage
- Return policy
- RMA process
- Support contact information

## Developer/Bulk Orders

For developers, researchers, or bulk orders:
- Contact Purple River Technology directly
- Potential for:
  - Volume discounts
  - Custom PCB modifications
  - Technical collaboration
  - Priority support

---

**Ready to Install?**
- [Installation Guide (Pi 5) →](../getting-started/installation-pi5.md)
- [Installation Guide (Pi 4) →](../getting-started/installation-pi4.md)
- [Hardware Compatibility →](compatibility.md)
