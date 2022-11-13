# Raspberry Pi GPIO LED-lamp driver
This is the code and theory behind my 3D printer lamp. The aim of this project is at the one hand to develop a cool lamp to enlight my 3D printer and on the other hand to learn about linux driver development.
The main focus of this project is to learn something. I am totally aware that there are more convenient ways to achieve the same result within a linux driver.

## Requirements
+ To compile it on the target system: `$ sudo apt install raspberrypi-kernel-headers` needs to be installed.
    - Tested with kernel release `5.15.56-v7l+` (See with `$ uname -r`. The kernel headers as well as the build system for kernel modules can be found within `/lib/modules/$(uname -r)`)

## Approach
We want to develop a device driver for an ARM platform (aka the Raspberry Pi 4 and/or Raspberry Pi 2B). The aim is to control 3 LEDs with 3 GPIO pins on the raspberry pi and use two more pins to implement a device recognition via an interrupt.

The driver should be able to activate or deactivate all three LEDs individually and to invoke a light play. (That should happen when the lamp device is connected to the raspberry pi and could invoked from the user space as well.)

More over, I want to develop a library that abstracts the read- and write-systemcalls from and to the char device driver away in order to make a convenient use of the LED-lamp.

After everything is implemented properly, I want to develop a small application that can communicate with [octoprint](https://octoprint.org/) via its [API](https://docs.octoprint.org/en/master/api/printer.html#retrieve-the-current-printer-state) via REST to activate some defined LED-light color on the lamp, depending on the current [state](https://docs.octoprint.org/en/master/api/datamodel.html#sec-api-datamodel-printer-state) of the 3D printer. 

Maybe I will enhance the lighting by sending telegram messages to my phone to get notifyed if the state of my printer has changed (e.g. after printing is finished).

### Electrical circuit
+ TBD
+ Important notes:
    - All ground pins [1] on the raspberry pi are hardwired to ground
    - There is a difference between the GPIO name (which corresponds to the BCM GPIO interface from the BCM2835 datasheet [2]) and the GPIO number on the board. If we want to do direct register programming or use the linux GPIO interface, we need to refer to the GPIO names as pins and *not* to the enumeration from the top left to the bottom right!
+ We use the six pins at the bottom of the GPIO interface (board pin 35, 36, 37, 38, 39 and 40; GPIO 19, GPIO 16, GPIO26, GPIO 20, Ground at pin 39, GPIO 21)

### Mechanical Design
+ TBD and after that it can be found within the folder `./3D_model`
+ The lamp housing is printed on my creality Ender 3 S1 with PTEG filament. See [my cura profile](https://github.com/jweber94/3d_printing/blob/master/cura_profiles/221108_ender3s1_pteg_profile.curaprofile) for the printer settings. I printed with [this](https://www.amazon.de/GEEETECH-Filament-Spool-3D-Drucker-Schwarz/dp/B08BZPZRFK) filament and it worked quiet well.

### Logical design
+ System call definition:
    - To light up/down a LED, we have 6 states that are enumerated and a 7th state that invokes the light play
    - To invoke such a LED state, you need to write the following, single digit number to `/dev/my_lamp`:
        * `0`: white LED on
        * `1`: green LED on
        * `2`: blue LED on
        * `3`: while LED off
        * `4`: green LED off
        * `5`: blue LED off
        * `6`: invoke light play
    - After the light play, all LEDs are turned off by default 
+ The creation of the char device driver file `/dev/my_lamp` is done within the interrupt handler function of the kernel module, depending on the level of the _device recognition pin_ of the GPIO interface (GPIO 26)!

### Installing the driver
+ The driver will be installed via a debian package, see [13] for details how that works

## Learning purpose
+ In order to learn about interacting directly with hardware based on hardware adresses, I want to program the GPIO control directly by interacting with the GPIO registers of the BCM2835!
    - The documentation on how to interact with the GPIO controller of the raspberry pi's processor (the BCM2538) can be found in [3] and is explained more clearly within [4] and [5]
    - I am totally aware that there are much more convenient frameworks to interact with GPIOs under linux then direct register programming (e.g. via the pinctrl driver framework that is delivered by the linux kernel, see [6] and [7] (other chip but the logic behind it stays the same!) for details) with tooling like gpiod [8] and its application like it is described within [9]
    - _I've choosen the GPIO register interface since it is not that hard to understand and it is well documented with a lot of other ressources that I do not mention here on the internet`_
+ To not make the project unnecessarily complicated, I will use the Linux generic IRQ framework to work with hardware interrupts and avoid programming the interrupt controller on register level
    - See [10] for the useage documentation
    - And see [11] and [12] for whats behind the implementation on the hardware level
+ ***What I did not do until now:*** Implementing a hardware driver that is registered by a kernel driver framework (like `pinctrl`) 

## Theory Linux driver programming
+ See `./theory_device_drivers` for details and the basis for design decisions

## References
+ [1] https://de.pinout.xyz/
+ [2] https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
+ [3] https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
+ [4] https://www.youtube.com/watch?v=jN7Fm_4ovio&t=20s
+ [5] https://www.youtube.com/watch?v=mshVdGlGwBs
+ [6] https://www.kernel.org/doc/html/v5.4/driver-api/pinctl.html
+ [7] https://wiki.st.com/stm32mpu/wiki/Pinctrl_overview
+ [8] https://lloydrochester.com/post/hardware/libgpiod-intro-rpi/
+ [9] https://wiki.st.com/stm32mpu/wiki/How_to_control_a_GPIO_in_userspace
+ [10] https://www.kernel.org/doc/html/v4.12/core-api/genericirq.html
+ [11] https://s-matyukevich.github.io/raspberry-pi-os/docs/lesson03/linux/interrupt_controllers.html
+ [12] https://wiki.st.com/stm32mpu/wiki/Interrupt_overview
+ [13] TODO
+ https://docs.octoprint.org/en/master/api/printer.html#retrieve-the-current-printer-state (Octoprint API)
+ https://docs.octoprint.org/en/master/api/datamodel.html#sec-api-datamodel-printer-state (Octoprint API - printer state description)

# Next ToDos
+ Implement basic driver with a char file in `/dev` that is auto created
+ Implement Interrupt with GPIO (https://www.youtube.com/watch?v=oCTNuwO9_FA&list=PLCGpd0Do5-I3b5TtyqeF1UdyD4C-S-dMa&index=12) but without blocking the read and write
+ Implement Interrupt with GPIO direct registers
+ Implement the LED enlighting with direct registers but without the coupleing to the interrupt
+ Make the coupleing to the interrupt (creating the `/dev` file within the interrupt)
+ Implement the reset to wait for the interrupt after the lamp is disconnected
+ Create debian package for installing the driver