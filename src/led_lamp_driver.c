#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/gpio.h>
#include <linux/interrupt.h>

#include <linux/jiffies.h>

#define IRQ_PIN_INPUT_NO 26
#define IRQ_PIN_OUTPUT_NO 19

// IRQ debouncing in software
static const unsigned long DEBOUNCE_JIFFIES = 20; // ms
static unsigned long old_jiffies = 0;
extern unsigned long volatile jiffies; // from the linux/jiffies.h - we need jiffies since the raspberry pi does not support hardware GPIO debouncing

// driver state
static bool lamp_activated = false;

/* IRQ number to which the IRQ_PIN_INPUT_NO pin is mapped to */
unsigned int irq_input_pin;


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
        printk("WARNING: Interrupt occured but the debounce time was not completed");
        return (irq_handler_t) IRQ_HANDLED; 
    }

    lamp_activated = true;
    printk("IRQ handler is executed!\n");
    old_jiffies = jiffies;
    return (irq_handler_t) IRQ_HANDLED;
}


/* Init and exit functions */
static int __init gpio_lamp_init(void) {
    printk("Initialize 3D printer lamp");

    /* setup the GPIO - using the linux provided GPIO abstraction (NOT direct register programming) */
    // input pin for the interrupt
    if (gpio_request(IRQ_PIN_INPUT_NO, "rpi-gpio-26")) { // GPIO 26 is the pin that should trigger the interrupt if it gets to the state HIGH
        printk("ERROR: Could not access GPIO 26\n");
        return -1;
    }
    if (gpio_direction_input(IRQ_PIN_INPUT_NO)) {
        printk("ERROR: Could not make GPIO 26 to an input pin\n");
        return -1;
    }
    // output pin for the interrupt
    if (gpio_request(IRQ_PIN_OUTPUT_NO, "rpi-gpio-19")) {
        printk("ERROR: Could not access GPIO 19\n");
        return -1;
    }
    if (gpio_direction_output(IRQ_PIN_OUTPUT_NO, 1)) { // make GPIO 19 to an output pin and configure its initial state to HIGH (,0) would result in a LOW level initial state)
        printk("ERROR: Could not make GPIO 19 to an output pin");
    }

    /* setup the interrupt */
    // get IRQ number that is associated by the GPIO pin 26 (IRQ_PIN_INPUT_NO) - this is defined within the device tree 
    irq_input_pin = gpio_to_irq(IRQ_PIN_INPUT_NO);
    // setup the IRQ service routine
    if (request_irq(irq_input_pin, (irq_handler_t) lamp_detection_irq_routine, IRQF_TRIGGER_RISING, "led_lamp_detection_irq", NULL) != 0) {
        printk("ERROR: Could not link the interrupt handler to the GPIO 26 interrupt pin");
        return -1;
    }

    printk("Initialize 3D printer lamp successful");
    return 0;
}

static void __exit gpio_lamp_exit(void) {
    printk("Uninitializing the 3D printer lamp");

    /* Free the GPIOs that are allocated by the linux GPIO interface abstraction */
    gpio_free(IRQ_PIN_INPUT_NO);
    gpio_free(IRQ_PIN_OUTPUT_NO);

    /* Free the IRQ of GPIO pin 26 IRQ_PIN_INPUT_NO */
    free_irq(irq_input_pin, NULL);

    printk("Uninitializing the 3D printer lamp successful");
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
