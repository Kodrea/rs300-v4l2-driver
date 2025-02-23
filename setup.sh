#!/usr/bin/bash

DRV_VERSION=0.0.1
DRV_NAME=rs300

echo "Uninstalling any previous ${DRV_NAME} module"
sudo dkms remove -m ${DRV_NAME} -v ${DRV_VERSION} --all

sudo mkdir -p /usr/src/${DRV_NAME}-${DRV_VERSION}

sudo cp -r $(pwd)/* /usr/src/${DRV_NAME}-${DRV_VERSION}

sudo dkms add -m ${DRV_NAME} -v ${DRV_VERSION}
sudo dkms build -m ${DRV_NAME} -v ${DRV_VERSION}
sudo dkms install -m ${DRV_NAME} -v ${DRV_VERSION}
