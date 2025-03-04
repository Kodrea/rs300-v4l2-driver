#!/usr/bin/bash

DRV_VERSION=0.0.1
DRV_IMX=rs300

echo "Uninstalling any previous ${DRV_IMX} module"
sudo dkms remove -m ${DRV_IMX} -v ${DRV_VERSION} --all

sudo mkdir -p /usr/src/${DRV_IMX}-${DRV_VERSION}

sudo cp -r $(pwd)/* /usr/src/${DRV_IMX}-${DRV_VERSION}

sudo dkms add -m ${DRV_IMX} -v ${DRV_VERSION}
sudo dkms build -m ${DRV_IMX} -v ${DRV_VERSION}
sudo dkms install -m ${DRV_IMX} -v ${DRV_VERSION}
