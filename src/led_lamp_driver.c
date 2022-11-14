#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>

#include<linux/fs.h> // needed to allocate a major device number
#include <linux/device.h> // needed for the automatic device file creation
#include <linux/kdev_t.h> // needed for the automatic device file creation

#include <linux/mutex.h>

#define IRQ_PIN_INPUT_NO 26
#define IRQ_PIN_INPUT_NO_SHUTDOWN 13
#define IRQ_PIN_OUTPUT_NO 19

static const char * driver_name = "led_lamp_driver";

/* IRQ debouncing in software */
static const unsigned long DEBOUNCE_JIFFIES = 100; // ms
static unsigned long old_jiffies = 0;
static unsigned long old_jiffies_shutdown = 0;
extern unsigned long volatile jiffies; // from the linux/jiffies.h - we need jiffies since the raspberry pi does not support hardware GPIO debouncing

/* driver state */
static bool lamp_activated = false;

/* IRQ number to which the IRQ_PIN_INPUT_NO pin is mapped to */
unsigned int irq_input_pin;
unsigned int irq_input_pin_shutdown;

/* device file creation */
dev_t major_dev_no = 0; // dummy value until the major device number is dynamically allocated within the init function
static struct class * device_class;
static const char * dev_class_name = "lamp_dev_class";
static const char * dev_file_name = "printer_lamp";
bool failed_initialization = false;

/* Make the device file an exclusive ressource by using a mutex */
struct mutex dev_file_mutex;

/* Utility functions */
bool check_file_existance(void) {
    struct file *fp = (struct file *) NULL;

    fp = filp_open("/dev/printer_lamp", O_RDONLY, 0);
    if (IS_ERR(fp) || fp == NULL) {
        printk("ERROR: filp_open\n");
        return false;
    }
    return true;
}

unsigned long calc_last_irq_invocation(unsigned long * old_jiff_ptr) {
    unsigned long diff_down = jiffies - *old_jiff_ptr;
    *old_jiff_ptr = jiffies;
    return diff_down;
}

/* Interrupt service routine - this is executed when the interrupt is triggered */
// Turn off
static irq_handler_t lamp_shutdown_irq_handler(int irq, void *dev_id) {
    /* debounce */ 
    // check for timeout
    if (calc_last_irq_invocation(&old_jiffies_shutdown) < DEBOUNCE_JIFFIES) {
        printk("DEBUG: Interrupt occured but the debounce time was not completed\n");
        return (irq_handler_t) IRQ_HANDLED;
    }
    // check if shutdown could have happend
    if (!lamp_activated) {
        printk("WARNING: Shutting down even if the lamp was never turned on - skipping!");
        return (irq_handler_t) IRQ_HANDLED; 
    }

    printk("Lamp was shutting down - removing the device file\n");
    return (irq_handler_t) IRQ_WAKE_THREAD;
}

static irq_handler_t lamp_shutdown_irq_thrfn(int irq, void *dev_id) {
    mutex_lock(&dev_file_mutex);
    printk("INFO: lamp_shutdown_irq_thrfn was called!\n");
    
    // check if device file exists
    bool device_file_exists = check_file_existance();
    printk("DEBUG: device_file_exists = %d", device_file_exists);
    if (!device_file_exists) {
        printk("ERROR: The device file for the LED lamp driver does not exist - skipping!\n");
        mutex_unlock(&dev_file_mutex);
        return (irq_handler_t) IRQ_HANDLED;
    }

    // remove device file
    class_destroy(device_class);
    lamp_activated = false;

    printk("Removing the device file sucessful\n");
    mutex_unlock(&dev_file_mutex);
    return (irq_handler_t) IRQ_HANDLED;
}

// Turn on
static irq_handler_t lamp_detection_irq_handler(int irq, void *dev_id) {
    // debounce
    if (calc_last_irq_invocation(&old_jiffies) < DEBOUNCE_JIFFIES) {
        printk("DEBUG: Interrupt occured but the debounce time was not completed\n");
        return (irq_handler_t) IRQ_HANDLED;
    }
    if (lamp_activated) {
        printk("WARNING: Lamp is already activated\n");
        return (irq_handler_t) IRQ_HANDLED;
    }

    printk("Lamp detected - creating the device file\n");
    return (irq_handler_t) IRQ_WAKE_THREAD;
}

static irq_handler_t lamp_detection_irq_thrfn(int irq, void *dev_id) {
    mutex_lock(&dev_file_mutex);
    printk("INFO: lamp_detection_irq_thrfn was called!\n");
    if (failed_initialization) {
        printk("Initialization of the device file failed the last time. Please check your system and reload this kernel module.\n");
        return (irq_handler_t) IRQ_HANDLED;
    }
    
    // create the device file
    if (IS_ERR(device_create(device_class, NULL, major_dev_no, NULL, dev_file_name))) {
        printk("ERROR: Can not create the device file\n");
        failed_initialization = true;
        goto r_device;
    }
    lamp_activated = true;
    
    printk("Creating the device file sucessful\n");
    mutex_unlock(&dev_file_mutex);
    return (irq_handler_t) IRQ_HANDLED;

r_device:
    class_destroy(device_class);
    mutex_unlock(&dev_file_mutex);
    return (irq_handler_t) IRQ_HANDLED;
}

/* Init and exit functions */
static int __init gpio_lamp_init(void) {
    printk("Initialize 3D printer lamp");

    /* setup the GPIO - using the linux provided GPIO abstraction (NOT direct register programming) */
    // input pin for the interrupt
    if (gpio_request(IRQ_PIN_INPUT_NO, "rpi-gpio-26")) { // GPIO 26 is the pin that should trigger the interrupt if it gets to the state from LOW to HIGH
        printk("ERROR: Can not access GPIO 26\n");
        return -1;
    }
    if (gpio_request(IRQ_PIN_INPUT_NO_SHUTDOWN, "rpi-gpio-13")) { // GPIO 13 is the pin that should trigger the interrupt if it gets to the state from HIGH to LOW
        printk("ERROR: Can not access GPIO 13\n");
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
    mutex_init(&dev_file_mutex);
    // get IRQ number that is associated by the GPIO pin 26 (IRQ_PIN_INPUT_NO) - this is defined within the device tree 
    irq_input_pin = gpio_to_irq(IRQ_PIN_INPUT_NO);
    // setup the IRQ service routine for the turn on
    if (request_threaded_irq(irq_input_pin, (void *) lamp_detection_irq_handler, (void *) lamp_detection_irq_thrfn, IRQF_TRIGGER_RISING, "led_lamp_detection_irq", NULL) != 0) {
        printk("ERROR: Can not link the interrupt handler to the GPIO 26 interrupt pin\n");
        return -1;
    }
    // setup the IRQ service routine for the turn off - on GPIO 13
    irq_input_pin_shutdown = gpio_to_irq(IRQ_PIN_INPUT_NO_SHUTDOWN);
    if (request_threaded_irq(irq_input_pin_shutdown, (void *) lamp_shutdown_irq_handler, (void *) lamp_shutdown_irq_thrfn, IRQF_TRIGGER_FALLING, "led_lamp_shutdown_irq", NULL) != 0) {
        printk("ERROR: Can not link the shutdown interrupt handler to the GPIO 26 interrupt pin\n");
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
    gpio_free(IRQ_PIN_INPUT_NO_SHUTDOWN);
    gpio_free(IRQ_PIN_OUTPUT_NO);

    /* Free the IRQ of GPIO pin 26 IRQ_PIN_INPUT_NO */
    free_irq(irq_input_pin, NULL);
    free_irq(irq_input_pin_shutdown, NULL);

    /* Unregister the major device file number */
    device_destroy(device_class, major_dev_no);
    class_destroy(device_class);
    unregister_chrdev_region(major_dev_no, 1);
    
    lamp_activated = false;
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
