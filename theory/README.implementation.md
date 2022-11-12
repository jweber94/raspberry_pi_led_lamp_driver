# Implementation details

## Development Setup
+ In order to have a convenient development experience, I choose VSCode with the "Visual Studio Code Remote - SSH" extension [1].
    - In order to use it, we need to place a RSA public key from our development machine onto the remote raspberry pi machine and the pi needs to have an ssh server running on it. See [2] for how this is done.
    If you are not on a machine that is running a supported OS, you can enable the VSCode remote extension by installing the demanded packages. A list what a remote machine needs to have can be found under [3]

## GPIO interfacing
+ The kernel provides a GPIO abstraction that we use to program our interrupt and to not overcomplicate the code
    - See [4] for details
+ To make a GPIO to an output pin with an initial state, we use `gpio_set_output(GPIO_num, initial_state)`, where `initial_state = 0` to set the default state to LOW and `initial_state = 1` to set the default state to HIGH. [4]
    - If you want to update the value of an output pin afterwards, use `gpio_set_value(GPIO_num, gpio_state)`, see [6] for more details

## Programming the interrupt
+ As a reference for programming the interrupt, I learned a lot from the video in [5]
    - The documentation of the Linux Kernel Interrupt API can be found here: [7]
+ If we program an interrupt based on a GPIO input, we will probably run into an bouncing problem: The interrupt will be triggered mutliple times because at the time when the interrupt is triggered, the input voltage will oscillate a few times before it is stable.
    - This can be avoided at the software level by capturing interrupts from the same device that are invoked immediatly after the first interrupt from the device occured! (See the implementation in `../src/led_lamp_driver.c` at the IRQ handler `lamp_detection_irq_routine`). You can find a reference at [8]
    - Another possability is to debounce with a capacitor in parallel to the switch. See https://circuitdigest.com/electronic-circuits/what-is-switch-bouncing-and-how-to-prevent-it-using-debounce-circuit for details
        * This was not done in my example since I want to learn more about linux kernel development and not about electric circuit design


## References
+ [1] https://code.visualstudio.com/docs/remote/ssh
+ [2] https://serverfault.com/questions/2429/how-do-you-setup-ssh-to-authenticate-using-keys-instead-of-a-username-password
+ [3] https://code.visualstudio.com/docs/remote/linux
+ [4] https://www.kernel.org/doc/Documentation/gpio/gpio-legacy.txt
+ [5] https://www.youtube.com/watch?v=oCTNuwO9_FA&list=PLCGpd0Do5-I3b5TtyqeF1UdyD4C-S-dMa&index=14
+ [6] https://www.linux-magazin.de/ausgaben/2013/10/kern-technik/2/
+ [7] https://docs.kernel.org/core-api/genericirq.html
+ [8] https://embetronicx.com/tutorials/linux/device-drivers/gpio-linux-device-driver-using-raspberry-pi/
+ [9] https://circuitdigest.com/electronic-circuits/what-is-switch-bouncing-and-how-to-prevent-it-using-debounce-circuit