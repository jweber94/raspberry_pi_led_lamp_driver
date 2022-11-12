#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>


/* Init and exit functions */
static int __init gpio_lamp_init(void) {
    printk("Initialize 3D printer lamp");

    printk("Initialize 3D printer lamp successful");
    return 0;
}

static void __exit gpio_lamp_exit(void) {
    printk("Uninitializing the 3D printer lamp");

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
