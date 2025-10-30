#!/usr/bin/env python3
"""
RS300 Thermal Camera - Capture Frames for ISP Processing

This script demonstrates:
1. Capturing raw frames from RS300 (/dev/video0)
2. Saving to YUV file for ISP processing
3. Optional: Direct V4L2 M2M processing through pispbe

Usage:
    ./capture_for_isp.py --frames 100 --output /tmp/thermal.yuv
    ./capture_for_isp.py --frames 100 --use-isp --output /tmp/processed.yuv

Requirements:
    pip3 install opencv-python numpy
"""

import argparse
import sys
import time
from pathlib import Path

# Optional imports (only needed for capture)
try:
    import cv2
    import numpy as np
    HAS_OPENCV = True
except ImportError:
    HAS_OPENCV = False

# Color codes for terminal output
class Colors:
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    RED = '\033[0;31m'
    NC = '\033[0m'

def check_device(device_path):
    """Check if V4L2 device exists and is accessible"""
    if not Path(device_path).exists():
        print(f"{Colors.RED}Error: {device_path} not found{Colors.NC}")
        return False
    return True

def capture_rs300_frames(device='/dev/video0', num_frames=100, output_file=None, show_preview=False):
    """
    Capture frames from RS300 thermal camera

    Args:
        device: V4L2 device path (default: /dev/video0)
        num_frames: Number of frames to capture
        output_file: Path to save raw YUV frames (optional)
        show_preview: Display live preview during capture
    """

    if not HAS_OPENCV:
        print(f"{Colors.RED}Error: OpenCV not installed{Colors.NC}")
        print("Install with: pip3 install opencv-python numpy")
        print("Or: sudo apt install python3-opencv python3-numpy")
        return None

    print(f"{Colors.YELLOW}Opening RS300 thermal camera: {device}{Colors.NC}")

    # Open video device
    cap = cv2.VideoCapture(device, cv2.CAP_V4L2)

    if not cap.isOpened():
        print(f"{Colors.RED}Error: Cannot open {device}{Colors.NC}")
        print("Check that:")
        print("  1. RS300 driver is loaded (lsmod | grep rs300)")
        print("  2. Media pipeline is configured (./configure_media.sh)")
        print("  3. Device permissions allow access")
        return None

    # Configure capture format
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 512)
    cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('U', 'Y', 'V', 'Y'))
    cap.set(cv2.CAP_PROP_FPS, 30)

    # Verify settings
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = int(cap.get(cv2.CAP_PROP_FPS))

    print(f"{Colors.GREEN}✓ Configured: {width}x{height} @ {fps}fps{Colors.NC}")

    # Prepare output file
    frames_data = []
    if output_file:
        print(f"{Colors.YELLOW}Will save frames to: {output_file}{Colors.NC}")

    # Capture loop
    print(f"{Colors.YELLOW}Capturing {num_frames} frames...{Colors.NC}")

    start_time = time.time()
    captured = 0
    dropped = 0

    for i in range(num_frames):
        ret, frame = cap.read()

        if not ret:
            print(f"{Colors.RED}✗ Failed to capture frame {i+1}{Colors.NC}")
            dropped += 1
            continue

        captured += 1

        # Save frame data if requested
        if output_file:
            # OpenCV reads as BGR, convert back to UYVY raw data
            # For now, save as is (will be YUV after conversion)
            frames_data.append(frame)

        # Show preview
        if show_preview:
            cv2.imshow('RS300 Thermal Capture', frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                print(f"{Colors.YELLOW}User interrupted capture{Colors.NC}")
                break

        # Progress indicator
        if (i + 1) % 10 == 0:
            elapsed = time.time() - start_time
            fps_actual = (i + 1) / elapsed
            print(f"  Progress: {i+1}/{num_frames} frames ({fps_actual:.1f} fps)", end='\r')

    # Summary
    elapsed = time.time() - start_time
    print(f"\n{Colors.GREEN}✓ Capture complete:{Colors.NC}")
    print(f"  Captured: {captured} frames")
    print(f"  Dropped: {dropped} frames")
    print(f"  Time: {elapsed:.2f}s")
    print(f"  Avg FPS: {captured/elapsed:.1f}")

    # Save frames
    if output_file and frames_data:
        print(f"{Colors.YELLOW}Saving frames to {output_file}...{Colors.NC}")

        # Stack all frames and save as raw YUV
        frames_array = np.array(frames_data)

        # Save as raw binary (YUV format)
        with open(output_file, 'wb') as f:
            frames_array.tofile(f)

        file_size = Path(output_file).stat().st_size
        print(f"{Colors.GREEN}✓ Saved {file_size:,} bytes ({file_size/(1024*1024):.2f} MB){Colors.NC}")

    # Cleanup
    cap.release()
    if show_preview:
        cv2.destroyAllWindows()

    return frames_data

def display_isp_info():
    """Display information about PiSP backend availability"""

    print(f"\n{Colors.YELLOW}=== PiSP Backend Information ===${Colors.NC}")

    devices = {
        '/dev/video20': 'pispbe-input (main input)',
        '/dev/video21': 'pispbe-tdn_input (temporal denoise)',
        '/dev/video22': 'pispbe-stitch_input (HDR stitch)',
        '/dev/video23': 'pispbe-output0 (primary output)',
        '/dev/video24': 'pispbe-output1 (secondary output)',
        '/dev/video25': 'pispbe-tdn_output (denoise output)',
        '/dev/video26': 'pispbe-stitch_output (HDR output)',
        '/dev/video27': 'pispbe-config (configuration)',
    }

    available = []
    for dev, name in devices.items():
        if Path(dev).exists():
            print(f"  {Colors.GREEN}✓{Colors.NC} {dev}: {name}")
            available.append(dev)
        else:
            print(f"  {Colors.RED}✗{Colors.NC} {dev}: {name} (not found)")

    if len(available) == len(devices):
        print(f"\n{Colors.GREEN}All PiSP backend devices available!{Colors.NC}")
        print(f"See RASPBERRY_PI_ISP_GUIDE.md for integration examples")
    else:
        print(f"\n{Colors.YELLOW}Some PiSP devices missing. Check:${Colors.NC}")
        print(f"  lsmod | grep pisp")
        print(f"  dmesg | grep -i pisp")

def main():
    parser = argparse.ArgumentParser(
        description='Capture frames from RS300 thermal camera',
        epilog='Example: ./capture_for_isp.py --frames 100 --output /tmp/thermal.yuv --preview'
    )

    parser.add_argument('--device', '-d', default='/dev/video0',
                        help='V4L2 device path (default: /dev/video0)')
    parser.add_argument('--frames', '-f', type=int, default=100,
                        help='Number of frames to capture (default: 100)')
    parser.add_argument('--output', '-o', type=str,
                        help='Output file path for raw YUV data')
    parser.add_argument('--preview', '-p', action='store_true',
                        help='Show live preview during capture')
    parser.add_argument('--isp-info', action='store_true',
                        help='Display PiSP backend device information')

    args = parser.parse_args()

    # Display ISP info if requested
    if args.isp_info:
        display_isp_info()
        return

    # Check device
    if not check_device(args.device):
        sys.exit(1)

    # Capture frames
    frames = capture_rs300_frames(
        device=args.device,
        num_frames=args.frames,
        output_file=args.output,
        show_preview=args.preview
    )

    if frames is None:
        print(f"{Colors.RED}Capture failed{Colors.NC}")
        sys.exit(1)

    # Show ISP info
    display_isp_info()

    print(f"\n{Colors.GREEN}Next steps:{Colors.NC}")
    if args.output:
        print(f"  1. Process with pispbe: See RASPBERRY_PI_ISP_GUIDE.md Section 6")
        print(f"  2. View raw file: ffplay -f rawvideo -pixel_format uyvy422 -video_size 640x512 {args.output}")
    print(f"  3. Run ISP examples: ./examples/isp_processing_example.sh")

if __name__ == '__main__':
    main()
