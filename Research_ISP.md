# Pi 5 ISP Research for RS300 Thermal Camera

## Research Plan: ISP Denoising for Thermal Imaging

### Background
The RS300 thermal camera outputs YUV422 thermal images (not temperature data). We want to explore using the Pi 5's hardware ISP (PISP) for denoising and image enhancement while maintaining thermal image quality.

### Research Questions
1. **Denoising Types Available:**
   - What specific denoising algorithms does Pi 5 ISP support?
   - Spatial denoising vs temporal denoising capabilities
   - Configuration options and parameters

2. **Format Compatibility:**
   - Does Pi 5 ISP natively support YUV422 input from thermal cameras?
   - Any format conversion requirements?
   - Pipeline configuration needed for thermal image processing

3. **Setup Requirements:**
   - Media controller link configuration changes needed
   - Device tree or kernel module parameters
   - ISP calibration or tuning files required

4. **Performance vs Quality:**
   - Latency impact of ISP processing
   - Quality improvements for thermal imaging
   - Comparison with direct CSI2 capture

### Current Status
- **Working Setup:** RS300 → CSI2 → rp1-cfe-csi2_ch0 → /dev/video0 (bypasses ISP)
- **Target Setup:** RS300 → CSI2 → pisp-fe → rp1-cfe-fe_image0 → /dev/video4 (uses ISP)
- **Format:** YUV422 (YUYV8_1X16) at 640x512@60fps

### Initial Findings from Web Research

#### Pi 5 ISP Capabilities
- **Hardware-based ISP:** Built into BCM2712/RP1, developed by Raspberry Pi
- **Spatial Denoising:** Pixel neighborhood analysis for noise reduction
- **Temporal Denoising:** Frame-to-frame averaging for noise reduction
- **Memory-to-memory processing:** Can process pre-captured frames

#### Denoising Types
1. **Spatial Denoising:**
   - Analyzes pixel neighborhoods
   - Removes shot noise and thermal noise
   - Preserves edges while smoothing noise
   - Wider filter support than previous generations

2. **Temporal Denoising:**
   - Compares current frame with previous frames
   - Averages similar regions to reduce noise
   - Hardware-accelerated implementation
   - Particularly effective for static scenes

#### Configuration Modes
- **Auto mode:** Standard spatial denoise with adaptive settings
- **Video mode:** Extra-fast color denoise optimized for real-time
- **Image mode:** High-quality color denoise for still captures

### Research Tasks

#### 1. Check Existing Codebase
- [ ] Search for ISP configuration files in the project
- [ ] Look for device tree ISP settings
- [ ] Check for existing ISP test scripts or examples
- [ ] Review kernel module parameters related to ISP

#### 2. ISP Pipeline Configuration
- [ ] Document current media controller topology
- [ ] Create ISP routing configuration commands
- [ ] Test basic ISP pipeline setup
- [ ] Verify format compatibility through ISP

#### 3. Denoising Testing
- [ ] Capture comparison samples (direct vs ISP)
- [ ] Test different denoising modes
- [ ] Measure latency impact
- [ ] Evaluate thermal image quality

#### 4. Performance Analysis
- [ ] Benchmark frame rates with/without ISP
- [ ] Memory usage comparison
- [ ] CPU utilization impact
- [ ] Power consumption differences

### Expected Outcomes
- **Immediate:** ISP should work with YUV422 without additional setup
- **Quality:** Spatial denoising should improve thermal image clarity
- **Performance:** Some latency increase, but manageable for 60fps
- **Compatibility:** Standard V4L2 controls should be available

### Implementation Plan
1. **Phase 1:** Basic ISP pipeline setup and format verification
2. **Phase 2:** Denoising configuration and testing
3. **Phase 3:** Performance optimization and comparison
4. **Phase 4:** Integration into configure_media.sh script

### Questions for Further Research
- Are there thermal-specific ISP tuning parameters?
- Can temporal denoising be tuned for thermal imaging characteristics?
- What V4L2 controls are available for real-time denoising adjustment?
- How does ISP processing affect thermal image histogram distribution?

### Next Steps
1. Check Pi 5 ISP documentation for YUV422 support
2. Test basic ISP pipeline configuration
3. Capture sample images for before/after comparison
4. Document optimal ISP settings for thermal imaging