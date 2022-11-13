#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>

#include<linux/fs.h> // needed to allocate a major device number
#include <linux/device.h> // needed for the automatic device file creation
#include <linux/kdev_t.h> // needed for the automatic device file creation

#define IRQ_PIN_INPUT_NO 26
#define IRQ_PIN_OUTPUT_NO 19

static const char * driver_name = "led_lamp_driver";

/* IRQ debouncing in software */
static const unsigned long DEBOUNCE_JIFFIES = 20; // ms
static unsigned long old_jiffies = 0;
extern unsigned long volatile jiffies; // from the linux/jiffies.h - we need jiffies since the raspberry pi does not support hardware GPIO debouncing

/* driver state */
static bool lamp_activated = false;

/* IRQ number to which the IRQ_PIN_INPUT_NO pin is mapped to */
unsigned int irq_input_pin;

/* device file creation */
dev_t major_dev_no = 0; // dummy value until the major device number is dynamically allocated within the init function
static struct class * device_class;
static const char * dev_class_name = "lamp_dev_class";
static const char * dev_file_name = "printer_lamp";
bool failed_initialization = false;

/* Interrupt service routine - this is executed when the interrupt is triggered */
static irq_handler_t lamp_detection_irq_routine(unsigned int irq_pin, void * dev_id, struct pt_regs * regs) {
    /*
    * irq_pin: An interrupt handler could be invoked by multiple interrupt pins
    * dev_id: Device ID that caused the interrupt
    * pt_regs: Shows hardware specific registers - commonly used for debugging purposes
    */

    // debounce 
    unsigned long diff = jiffies - old_jiffies;
    if (diff < DEBOUNCE_JIFFIES) {
        printk("WARNING: Interrupt occured but the debounce time was not completed\n");
        return (irq_handler_t) IRQ_NONE;
    }

    if (failed_initialization) {
        printk("Initialization of the device file failed the last time. Please check your system and reload this kernel module.\n");
        return (irq_handler_t) IRQ_HANDLED;
    }
    
    if (!lamp_activated) {
        // create the device file
        if (IS_ERR(device_create(device_class, NULL, major_dev_no, NULL, dev_file_name))) {
            printk("ERROR: Can not create the device file\n");
            failed_initialization = true;
            goto r_device;
        }
    }

    lamp_activated = true;
    printk("IRQ handler is executed!\n");
    old_jiffies = jiffies;
    return (irq_handler_t) IRQ_HANDLED;

r_device:
    class_destroy(device_class);
    return (irq_handler_t) IRQ_HANDLED;
}


/* Init and exit functions */
static int __init gpio_lamp_init(void) {
    printk("Initialize 3D printer lamp");

    /* setup the GPIO - using the linux provided GPIO abstraction (NOT direct register programming) */
    // input pin for the interrupt
    if (gpio_request(IRQ_PIN_INPUT_NO, "rpi-gpio-26")) { // GPIO 26 is the pin that should trigger the interrupt if it gets to the state HIGH
        printk("ERROR: Can not access GPIO 26\n");
        return -1;
    }
    if (gpio_direction_input(IRQ_PIN_INPUT_NO)) {
        printk("ERROR: Can not make GPIO 26 to an input pin\n");
        return -1;
    }
    // output pin for the interrupt
    if (gpio_request(IRQ_PIN_OUTPUT_NO, "rpi-gpio-19")) {
        printk("ERROR: Can not access GPIO 19\n");
        return -1;
    }
    if (gpio_direction_output(IRQ_PIN_OUTPUT_NO, 1)) { // make GPIO 19 to an output pin and configure its initial state to HIGH (,0) would result in a LOW level initial state)
        printk("ERROR: Can not make GPIO 19 to an output pin\n");
    }

    /* setup the interrupt */
    // get IRQ number that is associated by the GPIO pin 26 (IRQ_PIN_INPUT_NO) - this is defined within the device tree 
    irq_input_pin = gpio_to_irq(IRQ_PIN_INPUT_NO);
    // setup the IRQ service routine
    if (request_irq(irq_input_pin, (irq_handler_t) lamp_detection_irq_routine, IRQF_TRIGGER_RISING, "led_lamp_detection_irq", NULL) != 0) {
        printk("ERROR: Can not link the interrupt handler to the GPIO 26 interrupt pin\n");
        return -1;
    }

    /* Allocate a major device number - the concrete device file will be created in the interrupt handler */
    if (alloc_chrdev_region(&major_dev_no, 0, 1, driver_name) < 0) { // returns 0 on success and < 0 if it fails; the first argument is the pointer where the output should be written to
        printk("ERROR: Can not allocate a major device number!\n");
        return -1;
    } else {
        printk("Major device number = %d; Minor device number = %d\n", MAJOR(major_dev_no), MINOR(major_dev_no));
    }
    device_class = class_create(THIS_MODULE, dev_class_name);
    if (IS_ERR(device_class)) {
        printk("ERROR: Can not create the struct class for the device\n");
        goto r_class;
    }

    printk("Initialize 3D printer lamp successful\n");
    return 0;

r_class:
    unregister_chrdev_region(major_dev_no,1);
    return -1;
}

static void __exit gpio_lamp_exit(void) {
    printk("Uninitializing the 3D printer lamp\n");

    /* Free the GPIOs that are allocated by the linux GPIO interface abstraction */
    gpio_free(IRQ_PIN_INPUT_NO);
    gpio_free(IRQ_PIN_OUTPUT_NO);

    /* Free the IRQ of GPIO pin 26 IRQ_PIN_INPUT_NO */
    free_irq(irq_input_pin, NULL);

    /* Unregister the major device file number */
    device_destroy(device_class, major_dev_no);
    class_destroy(device_class);
    unregister_chrdev_region(major_dev_no, 1);
    

    printk("Uninitializing the 3D printer lamp successful\n");
    return;
}


/* Registering the kernel functions */
module_init(gpio_lamp_init);
module_exit(gpio_lamp_exit);

/* Meta information about the kernel module */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jens Weber");
MODULE_DESCRIPTION("Kernel driver for the 3D printer lamp.");
MODULE_VERSION("0.0.0");
