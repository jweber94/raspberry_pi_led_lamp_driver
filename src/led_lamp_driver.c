#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>

#include<linux/fs.h> // needed to allocate a major device number
#include <linux/device.h> // needed for the automatic device file creation
#include <linux/kdev_t.h> // needed for the automatic device file creation

#include <linux/cdev.h> // inode abstraction

#include <linux/mutex.h>
#include <linux/delay.h> // for msleep()

#include <linux/atomic.h>

#define IRQ_PIN_INPUT_NO 26
#define IRQ_PIN_INPUT_NO_SHUTDOWN 13
#define IRQ_PIN_OUTPUT_NO 19

#define DEBUG // outcomment this if you want to generate a release version

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

/* inode abstraction */
static struct cdev lamp_cdev;

/* make the device file an exclusive ressource by using a mutex */
struct mutex dev_file_mutex;

/* buffer pointer for the user interaction */
static char user_interface_buffer [512]; // one char holds 8 bit (aka one byte) of memory - stack allocation is needed to avoid freeing heap memory while another user space application is accessing it or complicating the code because we do opener tracking
struct mutex user_buffer_mutex;
unsigned int access_counter = 0;
atomic_t printer_state = ATOMIC_INIT(-1);

/* Direct register access to enlight the LEDs */
#define BCM2837_GPIO_ADDRESS 0xFE200000
static unsigned int * gpio_registers_addr = NULL;
unsigned int lamp_pins [3] = {16, 20, 21};

unsigned int lightplay_time = 100;

/* Utility functions */
bool check_file_existance(void) {
    if (device_class  != NULL) {
        return true;
    }
    printk("ERROR: Device file does not exist!\n");
    return false;
}

unsigned long calc_last_irq_invocation(unsigned long * old_jiff_ptr) {
    unsigned long diff_down = jiffies - *old_jiff_ptr;
    *old_jiff_ptr = jiffies;
    return diff_down;
}

static void gpio_pin_off(unsigned int pin){
  unsigned int * gpio_off_register = (unsigned int*)((char *)gpio_registers_addr +0x28);
  *gpio_off_register = (1 << pin);
  return;
}

static void gpio_pin_on(unsigned int pin) {
  /* Set the gpio register gpset_n to on */
  unsigned int * gpset_register = (unsigned int *)((char *) gpio_registers_addr + 0x1c);
  *gpset_register = (1 << pin);
}


void lightplay_1(void) {
    #ifdef DEBUG
    printk("DEBUG: lightplay_1 invoked\n");
    #endif
    gpio_pin_on(16);
    msleep(lightplay_time);
    gpio_pin_off(16);
    gpio_pin_on(20);    
    msleep(lightplay_time);
    gpio_pin_off(20);
    gpio_pin_on(21);
    msleep(lightplay_time);
    gpio_pin_off(21);
}

void lightplay_2(void) {
    #ifdef DEBUG
    printk("DEBUG: lightplay_2 invoked\n");
    #endif
    gpio_pin_on(21);
    msleep(lightplay_time);
    gpio_pin_off(21);
    gpio_pin_on(20);
    msleep(lightplay_time);
    gpio_pin_off(20);
    gpio_pin_on(16);
    msleep(lightplay_time);
    gpio_pin_off(16);
}

void init_direct_register_leds(void) {
    /* Setting the registers to make the lamp pins GPIO output pins */
    int idx;
    for (idx = 0; idx < 3; idx++) {
        printk("INFO: Setting pin %d to its initial lamp state", lamp_pins[idx]);
        /* Getting the demanded bits of the GPIO registers - I found this code on the internet */
        unsigned int gpfsel_index_n = lamp_pins[idx]/10;
        unsigned int fsel_bit_pos = lamp_pins[idx]%10;
        unsigned int * gpfsel_n = gpio_registers_addr + gpfsel_index_n; // Get the correct GPFSEL register
    
        // setting all of the pin function selection bits to zeros
        *gpfsel_n &= ~(7 << (fsel_bit_pos*3)); // 7 is 111 in binary, 111 is left shifted by the bit position and then inverted, so 000; The &= is a bitwise comparison and therefore sets every bit to zero
        // setting the new value to the gpio (001 but with LSB)
        *gpfsel_n |= (1 << (fsel_bit_pos*3));

        // reset the gpioset and gpioclr registers
        unsigned int * gpio_off_register = (unsigned int*)((char *)gpio_registers_addr +0x28);
        *gpio_off_register |= (1 << lamp_pins[idx]);
    }
}

/* Interrupt service routine - this is executed when the interrupt is triggered */
// Turn off
static irq_handler_t lamp_shutdown_irq_handler(int irq, void *dev_id) {   
    // debounce 
    if (calc_last_irq_invocation(&old_jiffies_shutdown) < DEBOUNCE_JIFFIES) {
        #ifdef DEBUG
            printk("DEBUG: Debounce lamp deactivation\n");
        #endif
        return (irq_handler_t) IRQ_HANDLED;
    }
    // check if shutdown could have happend
    mutex_lock(&dev_file_mutex);
    if (!lamp_activated) {
        printk("WARNING: Shutting down request even if the lamp was never turned on - skipping!\n");
        mutex_unlock(&dev_file_mutex);
        return (irq_handler_t) IRQ_HANDLED; 
    }
    mutex_unlock(&dev_file_mutex);

    printk("INFO: Lamp was shutting down - removing the device file\n");
    return (irq_handler_t) IRQ_WAKE_THREAD;
}

static irq_handler_t lamp_shutdown_irq_thrfn(int irq, void *dev_id) {
    mutex_lock(&dev_file_mutex);
    if (lamp_activated) {
        lightplay_2();
    }
    // check if device file exists
    if (!check_file_existance()) {
        printk("ERROR: The device file for the LED lamp driver does not exist - skipping!\n");
        mutex_unlock(&dev_file_mutex);
        return (irq_handler_t) IRQ_HANDLED;
    }

    // remove device file
    device_destroy(device_class, major_dev_no);
    lamp_activated = false;
    
    printk("INFO: Removing the device file sucessful\n");
    mutex_unlock(&dev_file_mutex);
    return (irq_handler_t) IRQ_HANDLED;
}

// Turn on
static irq_handler_t lamp_detection_irq_handler(int irq, void *dev_id) {    
    // debounce
    if (calc_last_irq_invocation(&old_jiffies) < DEBOUNCE_JIFFIES) {
        #ifdef DEBUG
            printk("DEBUG: Debounce lamp activation\n");
        #endif
        return (irq_handler_t) IRQ_HANDLED;
    }

    if (mutex_is_locked(&dev_file_mutex) == 1) {
        #ifdef DEBUG
            printk("WARNING: Mutex is currently locked - skipping!\n");
        #endif
        return (irq_handler_t) IRQ_HANDLED;
    }

    mutex_lock(&dev_file_mutex);
    if (lamp_activated) {
        #ifdef DEBUG
            printk("WARNING: Lamp is already activated\n");
        #endif
        mutex_unlock(&dev_file_mutex);
        return (irq_handler_t) IRQ_HANDLED;
    }
    mutex_unlock(&dev_file_mutex);
    
    if (failed_initialization) {
        printk("ERROR: Initialization of the device file failed the last time. Please check your system and reload this kernel module.\n");
        return (irq_handler_t) IRQ_HANDLED;
    }
    return (irq_handler_t) IRQ_WAKE_THREAD;
}

static irq_handler_t lamp_detection_irq_thrfn(int irq, void *dev_id) {
    mutex_lock(&dev_file_mutex);
    #ifdef DEBUG
        printk("INFO: lamp_detection_irq_thrfn was called!\n");
    #endif

    // check actual input state after deboune time of the input GPIO
    msleep(100); // sleep 200 msecs to wait until bouncing has finished
    if (gpio_get_value(IRQ_PIN_INPUT_NO) == 0) {
        #ifdef DEBUG
            printk("WARNING: Turn on interrupt was called even if the input state of the detection GPIO is LOW - skipping!\n");
        #endif
        mutex_unlock(&dev_file_mutex);
        return (irq_handler_t) IRQ_HANDLED;
    }

    // create the device file
    if (IS_ERR(device_create(device_class, NULL, major_dev_no, NULL, dev_file_name))) {
        printk("ERROR: Can not create the device file - a major error occured!\n");
        failed_initialization = true;
        goto r_device;
    }
    lamp_activated = true;
    
    lightplay_1(); // light greetings after turning on the lamp
    
    printk("INFO: Creating the /dev/printer_lamp device file sucessful\n");
    mutex_unlock(&dev_file_mutex);
    return (irq_handler_t) IRQ_HANDLED;

r_device:
    class_destroy(device_class);
    mutex_unlock(&dev_file_mutex);
    return (irq_handler_t) IRQ_HANDLED;
}

/* File operation functions */

static int syscall_open(struct inode *inode, struct file *file)
{
        // allocate memory on the heap to do the user interaction
        access_counter += 1;
        #ifdef DEBUG
            printk("INFO: Printer lamp driver opened. Currently there are %d user space applications that opened the device.\n", access_counter);
        #endif
        return 0;
}

static int syscall_close(struct inode *inode, struct file *file)
{
        access_counter -= 1;
        #ifdef DEBUG
            printk("INFO: Printer lamp driver closed. Currently there are %d user space applications that opened the device.\n", access_counter);
        #endif
        return 0;
}

static ssize_t syscall_read(struct file *filp, char __user *userp, size_t size, loff_t *off)
{       
    #ifdef DEBUG
        printk("DEBUG: printer_lamp read was called!\n");
    #endif
    uint32_t current_state;
    size_t not_copied = 0;
    current_state = atomic_read(&printer_state);
    printk("DEBUG: Returning current state %ld", current_state);
    if (current_state > 7 || current_state < 0) { 
        return simple_read_from_buffer(userp, size, off, "Invalid state\n", 14);
    } else {
        return simple_read_from_buffer(userp, size, off, &current_state, sizeof(current_state)); // 4 bit because https://raspberry-projects.com/pi/programming-in-c/memory/variables
    }
}

//void transform_state(unsigned int aimed_state) {
    
//}

static ssize_t syscall_write(struct file *filp, const char __user *userp, size_t size, loff_t *off)
{       
        #ifdef DEBUG
            printk("DEBUG: printer_lamp write was called\n");
        #endif
        unsigned int requested_lamp_state = UINT_MAX;
        uint32_t current_state;
        // reset the user interaction memory
        mutex_lock(&user_buffer_mutex);
        /* CAUTION: In order to avoid an overwriting by another user space application, we need to parse out the relevant information while the lock is still active - the processing can then be done while the lock is released in order to not lock for an unnecessary long time */
        memset(user_interface_buffer, 0x0, sizeof(user_interface_buffer));
        if (copy_from_user(user_interface_buffer, userp, size)) {
            printk("ERROR: Could not read user space data");
            return 0;
        }
        #ifdef DEBUG
            printk("DEBUG: Data from user space is: %s", user_interface_buffer);
        #endif
        if (sscanf(user_interface_buffer, "%d", &requested_lamp_state) != 1) { // https://cplusplus.com/reference/cstdio/sscanf/
            printk("ERROR: Inproper data format submitted by the user space application!\n");
            mutex_unlock(&user_buffer_mutex);
            return size;
        }
        mutex_unlock(&user_buffer_mutex);

        if (requested_lamp_state < 0 || requested_lamp_state > 7) {
            printk("WARNING: The requested lamp state was %d, you can only choose between state 0 and 7!\n", requested_lamp_state);
            return size;
        }

        if (requested_lamp_state == 6 || requested_lamp_state == 7) {
            current_state = atomic_read(&printer_state);
        }

        printk("DEBUG: Current state is %d", current_state);

        atomic_set(&printer_state, (int) requested_lamp_state);
        switch (requested_lamp_state) {
            case 0:
                printk("INFO: Lamp is going to state 0\n");
                gpio_pin_on(16);
                break;
            case 1:
                printk("INFO: Lamp is going to state 1\n");
                gpio_pin_on(20);
                break;
            case 2:
                printk("INFO: Lamp is going to state 2\n");
                gpio_pin_on(21);
                break;
            case 3:
                printk("INFO: Lamp is going to state 3\n");
                gpio_pin_off(16);
                break;
            case 4:
                printk("INFO: Lamp is going to state 4\n");
                gpio_pin_off(20);
                break;
            case 5:
                printk("INFO: Lamp is going to state 5\n");
                gpio_pin_off(21);
                break;
            case 6:
                printk("INFO: Lamp is going to state 6\n");
                lightplay_1();
                break;
            case 7:
                printk("INFO: Lamp is going to state 7\n");
                lightplay_2();
                if (current_state != UINT_MAX) {
                    gpio_pin_on(current_state);
                }
                break;
            default:
                printk("ERROR: Received invalid state!\n");
                if (current_state != UINT_MAX) {
                    gpio_pin_on(current_state);
                }
                break;
        }
        #ifdef DEBUG
        printk("DEBUG: Finished LED setting\n");
        #endif
        return size;
}

// fops object to link against the inode file that descrives the char device file
static struct file_operations fops =
{
    .owner      = THIS_MODULE,
    .read       = syscall_read,
    .write      = syscall_write,
    .open       = syscall_open,
    .release    = syscall_close
};

/* Init and exit functions */
static int __init gpio_lamp_init(void) {
    printk("INFO: Initialize 3D printer lamp\n");

    /* For the direct register control */
    // Assign the physical adress through the MMU (Memory Management Unit) to the variable to access it in the write functions
    gpio_registers_addr = (int*)ioremap(BCM2837_GPIO_ADDRESS, PAGE_SIZE); // https://os.inf.tu-dresden.de/l4env/doc/html/dde_linux/group__mod__res.html#gd8fce5b58ae2fa9f4158fe408610ccc5
    if (gpio_registers_addr == NULL) {
      printk("Failed to map GPIO memory to driver");
      return -1;
    }

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

    /* Allocate a major device number - the concrete device file will be created in the interrupt handler */
    if (alloc_chrdev_region(&major_dev_no, 0, 1, driver_name) < 0) { // returns 0 on success and < 0 if it fails; the first argument is the pointer where the output should be written to
        printk("ERROR: Can not allocate a major device number!\n");
        return -1;
    } else {
        printk("INFO: Major device number = %d; Minor device number = %d\n", MAJOR(major_dev_no), MINOR(major_dev_no));
    }
    // Creating cdev structure
    cdev_init(&lamp_cdev, &fops);
    // Adding character device to the system
    if((cdev_add(&lamp_cdev, major_dev_no, 1)) < 0){
        pr_err("Cannot add the device to the system\n");
        goto r_class;
    }

    device_class = class_create(THIS_MODULE, dev_class_name);
    if (IS_ERR(device_class)) {
        printk("ERROR: Can not create the struct class for the device\n");
        goto r_class;
    }

    /* setup the interrupt */
    mutex_init(&dev_file_mutex);
    // avoid bouncing on startup since we have set pin 19 to the default value to HIGH
    old_jiffies = jiffies;
    old_jiffies_shutdown = jiffies;
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

    // secure user buffer if more then one user space application is accessing it at the same time
    mutex_init(&user_buffer_mutex);
    atomic_set(&printer_state, UINT_MAX);

    // create direct register start state
    init_direct_register_leds();
    
    printk("INFO: Initialize 3D printer lamp successful\n");
    return 0;

r_class:
    unregister_chrdev_region(major_dev_no,1);
    return -1;
}

static void __exit gpio_lamp_exit(void) {
    printk("INFO: Uninitializing the 3D printer lamp\n");

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
    cdev_del(&lamp_cdev);
    unregister_chrdev_region(major_dev_no, 1);
    
    lamp_activated = false;
    printk("INFO: Uninitializing the 3D printer lamp successful\n");
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
