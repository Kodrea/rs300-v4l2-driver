#!/usr/bin/bash

DRV_VERSION=0.0.1
DRV_IMX=rs300

# Color codes for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}=== RS300 Driver Setup ===${NC}"
echo ""

echo "Uninstalling any previous ${DRV_IMX} module"
sudo dkms remove -m ${DRV_IMX} -v ${DRV_VERSION} --all

sudo mkdir -p /usr/src/${DRV_IMX}-${DRV_VERSION}

sudo cp -r $(pwd)/* /usr/src/${DRV_IMX}-${DRV_VERSION}

sudo dkms add -m ${DRV_IMX} -v ${DRV_VERSION}
sudo dkms build -m ${DRV_IMX} -v ${DRV_VERSION}
sudo dkms install -m ${DRV_IMX} -v ${DRV_VERSION}

echo ""
echo -e "${GREEN}✓ Driver installation complete${NC}"
echo ""

# Offer to install auto-configuration at boot
echo -e "${BLUE}=== Boot-Time Auto-Configuration Setup ===${NC}"
echo ""
echo "The RS300 camera requires media controller configuration after each boot."
echo "You can either:"
echo "  1. Run ./configure_media.sh manually after each reboot (default)"
echo "  2. Install automatic configuration via systemd service"
echo "  3. Install automatic configuration via udev rule (event-driven)"
echo "  4. Install both systemd + udev (most robust)"
echo ""
echo "For more details, see: BOOT_CONFIGURATION.md"
echo ""

read -p "Would you like to install automatic boot configuration? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "Select auto-configuration method:"
    echo "  1) Systemd service (runs at boot)"
    echo "  2) Udev rule (event-driven, when hardware detected)"
    echo "  3) Both systemd + udev (recommended for reliability)"
    echo "  4) Skip auto-configuration"
    echo ""
    read -p "Select option (1-4): " -n 1 -r
    echo

    INSTALL_SYSTEMD=false
    INSTALL_UDEV=false

    case $REPLY in
        1)
            INSTALL_SYSTEMD=true
            ;;
        2)
            INSTALL_UDEV=true
            ;;
        3)
            INSTALL_SYSTEMD=true
            INSTALL_UDEV=true
            ;;
        4)
            echo "Skipping auto-configuration installation"
            ;;
        *)
            echo "Invalid selection, skipping auto-configuration"
            ;;
    esac

    # Install systemd service
    if [ "$INSTALL_SYSTEMD" = true ]; then
        echo ""
        echo -e "${BLUE}Installing systemd service...${NC}"

        # Update service file with current directory path
        CURRENT_DIR=$(pwd)
        sudo cp rs300-media-config.service /etc/systemd/system/
        sudo sed -i "s|/home/cody/rs300-v4l2-driver|${CURRENT_DIR}|g" /etc/systemd/system/rs300-media-config.service

        # Reload systemd and enable service
        sudo systemctl daemon-reload
        sudo systemctl enable rs300-media-config.service

        echo -e "${GREEN}✓ Systemd service installed and enabled${NC}"
        echo "  Service will run automatically at boot"
        echo "  Check status: sudo systemctl status rs300-media-config.service"
        echo "  View logs: sudo journalctl -u rs300-media-config.service"
    fi

    # Install udev rule
    if [ "$INSTALL_UDEV" = true ]; then
        echo ""
        echo -e "${BLUE}Installing udev rule...${NC}"

        # Update udev rule with current directory path
        CURRENT_DIR=$(pwd)
        sudo cp 99-rs300.rules /etc/udev/rules.d/
        sudo sed -i "s|/home/cody/rs300-v4l2-driver|${CURRENT_DIR}|g" /etc/udev/rules.d/99-rs300.rules

        # Reload udev rules
        sudo udevadm control --reload-rules
        sudo udevadm trigger

        echo -e "${GREEN}✓ Udev rule installed${NC}"
        echo "  Rule will trigger when RS300 hardware is detected"
        echo "  Logs: /var/log/rs300-udev-config.log"
    fi

    if [ "$INSTALL_SYSTEMD" = true ] || [ "$INSTALL_UDEV" = true ]; then
        echo ""
        echo -e "${GREEN}Auto-configuration installed successfully!${NC}"
        echo "After reboot, camera should be ready automatically."
        echo ""
    fi
else
    echo ""
    echo "Skipping auto-configuration installation."
    echo "You will need to run ./configure_media.sh after each reboot."
    echo ""
fi

echo -e "${BLUE}=== Setup Complete ===${NC}"
echo ""
echo "Next steps:"
echo "  1. Reboot your Raspberry Pi: sudo reboot"
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "  2. After reboot, run: ./configure_media.sh"
    echo "  3. Test camera: ./test_camera.sh"
else
    echo "  2. Camera should be ready after reboot (auto-configured)"
    echo "  3. Test camera: ./test_camera.sh"
fi
echo ""
echo "For troubleshooting, see BOOT_CONFIGURATION.md"
echo ""
