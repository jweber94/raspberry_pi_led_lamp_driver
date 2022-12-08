#!/usr/bin/bash

echo "Start creating debian package folder structure"

mkdir -p $(pwd)/printer_lamp/DEBIAN $(pwd)/printer_lamp/etc/octolamp $(pwd)/printer_lamp/usr/bin/octolamp $(pwd)/printer_lamp/etc/dbus-1/system.d $(pwd)/printer_lamp/usr/lib/systemd/system

# copy and create config files
cp $(pwd)/../userspace_program/driver_communication_service/config/driver_service.ini $(pwd)/printer_lamp/etc/octolamp
cp $(pwd)/../userspace_program/printer_communication_service/config_example/lamp_config.ini $(pwd)/printer_lamp/etc/octolamp
cp $(pwd)/../userspace_program/driver_communication_service/config/jens.printerlamp.driver_interaction.conf $(pwd)/printer_lamp/etc/dbus-1/system.d

cp $(pwd)/../userspace_program/driver_communication_service/systemd/printer_lamp_driver_service.service $(pwd)/printer_lamp/usr/lib/systemd/system
cp $(pwd)/../userspace_program/printer_communication_service/systemd/printer_lamp_octoprint_service.service $(pwd)/printer_lamp/usr/lib/systemd/system

touch $(pwd)/printer_lamp/DEBIAN/postinst
touch $(pwd)/printer_lamp/DEBIAN/control

chmod 775 $(pwd)/printer_lamp/DEBIAN/postinst
chmod 775 $(pwd)/printer_lamp/DEBIAN/control

echo "Finish creating folder structure"

echo "
systemctl daemon-reload
systemctl enable printer_lamp_driver_service.service
systemctl enable printer_lamp_octoprint_service.service
systemctl start printer_lamp_driver_service.service
systemctl start printer_lamp_octoprint_service.service
echo "Foo=Bar" >> /etc/udev/rules.d/99-com.rules" >> $(pwd)/printer_lamp/DEBIAN/postinst

echo "
Package: myPackage
Version: 1.0.0-1
Architecture: amd64
Depends: 
Maintainer: Jens Weber <jweber94@gmail.com>
Description: Printer lamp debian package." >> $(pwd)/printer_lamp/DEBIAN/control

chmod 775 $(pwd)/printer_lamp/DEBIAN/postinst

# create and copy binarys
pushd $(pwd)/../userspace_program/driver_communication_service
pwd
make clean
make build
popd 
pwd


echo "Copying files"
cp $(pwd)/../userspace_program/driver_communication_service/build/bin/driver_interaction $(pwd)/printer_lamp/usr/bin/octolamp
mkdir -p $(pwd)/printer_lamp/usr/bin/octolamp/octoprint_interaction
cp $(pwd)/../userspace_program/printer_communication_service/app.py $(pwd)/printer_lamp/usr/bin/octolamp/octoprint_interaction
cp -r $(pwd)/../userspace_program/printer_communication_service/src $(pwd)/printer_lamp/usr/bin/octolamp/octoprint_interaction
echo "Finish copying files"
