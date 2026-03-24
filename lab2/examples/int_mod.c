#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/module.h>
#include <linux/interrupt.h>
#include <asm/arch/pxa-regs.h>
#include <asm-arm/arch/hardware.h>

#define MYGPIO 113

irqreturn_t gpio_irq(int irq, void *dev_id, struct pt_regs *regs)
{
    // no debounce, active for both edges
    printk("Button IRQ\n");
    return IRQ_HANDLED;
}

static int my_init_module(void)
{
    printk("Hello world!\n");

    pxa_gpio_mode(MYGPIO | GPIO_IN);
    int irq = IRQ_GPIO(MYGPIO);
    if (request_irq(irq, &gpio_irq, SA_INTERRUPT | SA_TRIGGER_RISING,
                    "mygpio", NULL) != 0)
    {
        printk("irq not acquired \n");
        return -1;
    }
    else
    {
        printk("irq %d acquired successfully \n", irq);
    }

    return 0;
}

static void my_cleanup_module(void)
{
    free_irq(IRQ_GPIO(MYGPIO), NULL);

    printk("Bye world!\n");
}

module_init(my_init_module);
module_exit(my_cleanup_module);