# Driver development

## Basics
+ A linux driver is defined as a kernel module which can be loaded to the kernel at runtime or at boot time. There are three kinds of drivers:
    * char drivers: mostly used for device drivers
    * block drivers
    * network drivers
+ Kernel modules are stored as `.ko` files, which stands for _kernel object_ and that describe relocatable object code that the kernel can execute (and link against its symbols)
+ Kernel modules define callbacks to systemcalls.
    - Whenever a systemcall to a (e.g. char) driver file is made from user space, it will invoke the callback that is associated with the file by the kernel module
+ There are serval driver frameworks that can (and should) be used to have a common interface for interacting with different kinds of hardware (e.g. GPIO controllers, interrupt controllers, GPUs, PCIe interfaces, USB interface, input devices like mouses or keyboards and many more)
    - See [2] for the complete listing

### Drivers and Device Trees
+ Device Trees are used to inform the firmware, bootloader and the OS (especially the Linux driver) which hardware is located on which hardware adress on the running system.
    - They are needed, since not every hardware/periferal interface has a self-detection mechanism (like it is the case in USB or PCIe)
    - If we define a hardware component within the device tree, we need to define what driver should be used for that specific hardware. (we can define more then just one driver, but the OS will choose the first one that is parsed from left to right on the driver definition and is also found on the system)
        * This is the `compatible` attribute within the device tree
+ A basic device tree can be patched by a new definiton or can append configurations to some hardware definitions by including the base device tree and rewriting a part of it within the newly created file
+ Device trees are transpiled and can be split into multiple files. The more human readable for is the `.dtsi` (device tree include file which defines commonly used device tree settings) and the final `.dts` which defines a valid device tree that can be transpiled by the terminal program `dtc` (device tree compiler)
    - Patching the device tree is done by overlaying/overwriting the included tree
    - ***CAUTION***: `dtc` only checks the syntax of the `.dts` files. Transpilation will successfully terminate even if the defined device tree is not suitable for the hardware that it should describe!
    - See [3] at 49:00 for details
+ A good introduction to device trees can be found in [3] and a practical example can be found in [4]
+ ***Linking a driver to hardware via a device tree can only be done if the hardware is permanently connected (e.g. soldered to the circuit board of the device) to the system and can always be found at the same hardware adress by the processor!***
    - _Therefore, our LED lamp project is not suitable to be defined within the device tree of the targeted system, since the lamp must not be permanently connected to the raspberry pi!_
+ Device Trees are needed by the firmware (e.g. arm-trusted-firmware), by the bootloader (e.g. U-Boot) and the OS
    - They all expect the transpiled device tree located on a specific place on the hard disk file system!

### Drivers for not permanently connected devices
+ If we do not have an autodetectable device (like an USB or PCIe device) and it is not permanently connected to our system, we need to install and activate our driver manually!
    - Activation is done by `$ insmod /path/to/module.ko` or can be defined as _loading driver after boot_ by defining the module within `/etc/modules` or defining a file within `/etc/modprobe.d/` for the module
    - A kernel module needs to be installed/located within `/usr/lib/modules/$(uname -r)/kernel/drivers/<drivername>` in order to be found by `insmod` or `modprobe`
    - See [5] for details
+ Like described above, ***this is the way to load our kernel driver for the 3D printer lamp!***

## Driver Frameworks
+ In order to have a commonly used interface/api to interact with hardware, the linux kernel delivers a lot of kernel driver frameworks that can (***and should***) be used to abstract the hardware of the system away!
    - As an example (but on a different chip), we can see the pinctrl framework on the STM32 MCU in [9]:
        * The pinctrl-stm32 driver [10] implements the interaction with the specific chip on hardware adress level, uses the function signatures and adds this functionallity to the defined interface by the linux kernel data struct `gpio_chip` [11] and registers the gpio bank with the function `gpiochip_add_data` [12] to make the gpio pins available via the `pinctrl` hardware abstraction by the linux kernel!
        * The pinctrl interface can then be used by other _device specific_ kernel drivers on a higher abstraction layer or it can directly used within user space by the provided char device driver or by using a convenience library like gpiod [13]
        * ==> This is what the _2 System overview_ in [9] is describing 
    - Another example how we can implement device drivers based on kernel frameworks is [14] and its continuing blog posts with the _nintendo nunchunk_ controller. Here, the already existing i2c interface of the beaglebone black is used to implement another hardware driver that implements the Linux Input Subsystem [14] [15]

### Kernel Build System and Macros
+ The kernel build system that is used to build the kernel itself and that is needed to build the self developed and/or out-of-tree kernel modules is called ***KBuild*** (more information, see section _The KBuild build system_)
+ A valid char driver implements at least the module initialization and the module exit/deletion callback. These callbacks need to be passed to the `module_init(fcn_ptr)` and `module_exit(fcn_ptr)` functions of the kernel that are defined within `#include <linux/module.h>` [16]
    - The functions needs to add a `__init` macro from [17], which describes their function signature [18]
        * This macro makes the kernel use less memory since it knows that these functions just one time executed (when the module is initialized and is exited). Therefore it can free their memory after their invokation!
+ The kernel build system is invoked by `make -C $(KDIR)`
    - Within the Makefile (e.g. in ../src/Makefile), another build system based on make is invoked, which is in this case the kernel build system. This is done by the `-C` command that changes the folder before the `make` executable runs its normal job [23] 
    - With the `M=$(shell pwd)` we set an environment variable for the invoked kernel build system that is called `M` that points to the current directoy where our kernel module source code is located [22]
    - The last `modules` command tells the kernel build system that we want to build its target `modules`
        * See the implementation in `/lib/modules/$(uname -r)/build/Makefile`
+ Our newly created object file needs to be added to the kernel build system. Its name *must* correspond to the `.c` file that should be build as the kernel module!

#### The KBuild build system
+ The KBuild [27] build system is the build system that the kernel is using to compile itself and that is needed to compile kernel modules. It is invoked by the `make` commands that are described above!
    - It is also possible to develop kernel modules within more then just one file.
    - Also, the KBuild system delivers a mechanism to install the compiled kernel modules on the system.
    - See [24] for a very good explaination by kernel.org how to use it to compile kernel modules!
    - ***If you want to develop kernel modules and install them, make sure to refer to the KBuild system properly and use it as you should use it! This will help to provide a clean kernel module project!***

#### In-Tree vs Out-of-Tree Kernel modules
+ Kernel modules that are permanently used on the targeted system need to be installed on it. You can find the installed modules on your linux system within `/usr/lib/modules/$(uname -r)`
    - Self developed kernel modules will be placed under `/usr/lib/modules/$(uname -r)/extra` on default!
        * You can specify a custom folder name or a custom location within KBuild. See [24] for details on this.
+ Kernel modules can be installed _out-of-tree_ or _in-tree_:
    - Out-of-Tree:
        * The *source code* of the kernel module is placed anywhere on the system
        * In order to build the kernel module and/or install it on the system, you need the kernel headers and the KBuild system (which comes with the kernel headers) installed on the system to compile it and make it loadable by the running kernel
        * This is the most common way for device drivers that needed for hardware that is not commonly used.
            * To bring a kernel driver In-Tree to a kernel version it is a lot of work and since there are many kernel version, it will be even more work to do! Therefore you need to have a good reason to go through that process to get accepted by the kernel maintainers!
    - In-Tree:
        * The *source-code* is available during compilation of the kernel and is delivered with the kernel package. Therefore, there is no need to compile it on the target system!
        * If you build your custom kernel by compiling it yourself, the source code needs to be placed in a certain directory and some configuration files need to be adjusted. After that, you can choose the kernel module via the `menuconfig` terminal interface in order to build it together with the kernel!
        * You can compile your kernel module in tree if you are in charge of the kernel/linux distribution itself, e.g. if you are using yocto for it!
            - See [26] how to insert an in-tree kernel module in yocto  
+ ***Since the 3D printer lamp is just used on an octopi or a few devices, I choose to install the kernel module for it out-of-tree. Maybe I will develop a _3DprinterOS_ in the future with yocto, targeted on a stm32 machine or I will ask the octoprints kernel maintainers to include my module to the kernel.***

### char Drivers and their locations
+ There are multiple locations where you can and should store your device driver files. Where you store your device file depends on what it should do.
    - `/proc/<devicename>`: Here are device file for the interaction between the kernel and the user space stored to access system configurations or get information about the system (aka the chip and its soldered components)
    - `/dev/<devicename>`: Here are device files for the interaction with external hardware stored (like i2c devices that are *not* part of the system itself)
+ Device numbers:
    - If we create a file within the `/dev` filesystem, we need to give the created device file a major and a minor device file number.
        * Major: Describes the driver that should handle the device
        * Minor: Describes the concrete hardware instance that should be adressed by the driver
        * See [19] for details and [20] for a tutorial. We can create device files by the kernel module itself and do not rely on the `$ mknode` command, see [21]

### Accessing registers from C code
+ You can assign an adress to an `unsigned int*` datatype, e.g. `unsigned int * reg = 0x3F200000` and if you want to write to that adress, you can just dereference that pointer (`*reg = 0x00000000` to set the value at the address `0x3F200000` to the value `0` in case that we have a 32 bit system)


## References
+ [1] https://www.youtube.com/watch?v=juGNPLdjLH4&t=305s (common introduction)
+ [2] https://www.kernel.org/doc/html/v5.4/driver-api/index.html
+ [3] https://www.youtube.com/watch?v=a9CZ1Uk3OYQ 
+ [4] https://krinkinmu.github.io/2020/07/12/linux-kernel-modules.html
+ [5] https://www.cyberciti.biz/faq/linux-how-to-load-a-kernel-module-automatically-at-boot-time/
+ [6] https://github.com/ARM-software/arm-trusted-firmware
+ [7] https://de.wikipedia.org/wiki/Das_U-Boot
+ [8] https://www.kernel.org/doc/html/latest/driver-api/index.html
+ [9] https://wiki.st.com/stm32mpu/wiki/Pinctrl_overview
+ [10] https://github.com/STMicroelectronics/linux/blob/v5.15-stm32mp/drivers/pinctrl/stm32/pinctrl-stm32.c
+ [11] https://www.kernel.org/doc/html/latest/driver-api/gpio/driver.html#controller-drivers-gpio-chip
+ [12] https://www.kernel.org/doc/html/latest/driver-api/gpio/index.html?highlight=gpiochip_add_data#c.gpiochip_add_data
+ [13] https://lloydrochester.com/post/hardware/libgpiod-intro-rpi/
+ [14] https://www.kernel.org/doc/html/latest/input/input-programming.html
+ [15] https://krinkinmu.github.io/2020/07/25/input-device.html
+ [16] https://github.com/torvalds/linux/blob/master/include/linux/module.h
+ [17] https://github.com/torvalds/linux/blob/master/include/linux/module.h
+ [18] https://stackoverflow.com/questions/8832114/what-does-init-mean-in-the-linux-kernel-code
+ [19] https://stackoverflow.com/questions/12564824/use-of-major-and-minor-device-numbers#:~:text=The%20major%20number%20and%20minor,using%20the%20same%20device%20driver.
+ [20] https://www.youtube.com/watch?v=h7ybJMYyqDQ&list=PLCGpd0Do5-I3b5TtyqeF1UdyD4C-S-dMa&index=3
+ [21] https://stackoverflow.com/questions/5970595/how-to-create-a-device-node-from-the-init-module-code-of-a-linux-kernel-module
+ [22] https://stackoverflow.com/questions/20301591/m-option-in-make-command-makefile
+ [23] https://linux.die.net/man/1/make
+ [24] https://www.kernel.org/doc/Documentation/kbuild/modules.txt
+ [25] https://wiki.postmarketos.org/wiki/Out-of-tree_kernel_modules#:~:text=The%20modules%20that%20can%20be,source%20code%20is%20located%20elsewhere.
+ [26] https://stackoverflow.com/questions/72974408/how-to-build-in-tree-linux-kernel-module-in-yocto
+ [27] https://www.kernel.org/doc/html/latest/kbuild/kbuild.html