# Kernel driver
+ This is the kernel driver that implements the direct register interaction to turn on and shut down the leds of the printer lamp.
+ You need to have the kernel headers installed to compile it
	- In order to program a kernel driver you need to use the kernel driver build system

## Making the `/dev/printer_lamp` file accessable from user space
+ To make the device file accessable without root privileges, we need to place a rule for the path and file at the udev configuration. In order to do this, we need the following:
	- Add 
	```
	# my custom rules
	KERNEL=="printer_lamp", GROUP="printer_lamp", MODE="0777"
	```
	  to `/etc/udev/rules.d/99-com.rules` on the targeted raspberry pi system

### What is udev?
+ `udev` stands for _userspace /dev_ and is a system program of the linux system that enables to trigger events when an external device is hotplugged to our system.
	- _Basically exactly what we want for our printer lamp driver_
+ `udev` monitors hotplug events on the kernel and uses information from sysfs to trigger scripts that are linked to the device hotplugging within `/etc/udev/rules.d/99-com.rules`
	- Especially the (device-) file access rights are freely configurable with udev! (which is more or less the main use case)
+ If you want to add more of your own udev rules, consider creating a separat `xx-com.rules` file under `/etc/udev/rules.d/`, where `xx` is a number that is smaller then 99, since udev loads (and overwrites) rules from files with a higher number to a lower number.