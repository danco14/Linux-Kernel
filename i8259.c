/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Mask for interrupts 0 - 7 */
static uint8_t mask[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/*
 * i8259_init
 *    DESCRIPTION: Initializes the PIC
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Initializes the PIC to cascade mode and masks all interrupts
 */
void i8259_init(void) {
    /* Set the current masks for slave and master (all masked) */
    master_mask = 0xFF;
    slave_mask = 0xFF;

    /* Mask all PIC interrupts */
    outb(master_mask, MASTER_8259_PORT+1);
    outb(slave_mask, SLAVE_8259_PORT+1);

    /* Master ICWs */
    outb(ICW1, MASTER_8259_PORT);          // Master ICW1
    outb(ICW2_MASTER, MASTER_8259_PORT+1); // Master ICW2
    outb(ICW3_MASTER, MASTER_8259_PORT+1); // Master ICW3
    outb(ICW4, MASTER_8259_PORT+1);        // Master ICW4

    /* Slave ICWs */
    outb(ICW1, SLAVE_8259_PORT);           // Slave ICW1
    outb(ICW2_SLAVE, SLAVE_8259_PORT+1);   // Slave ICW2
    outb(ICW3_SLAVE, SLAVE_8259_PORT+1);   // Slave ICW3
    outb(ICW4, SLAVE_8259_PORT+1);         // Slave ICW4

    /* Turn off all PIC interrupts */
    outb(master_mask, MASTER_8259_PORT+1);
    outb(slave_mask, SLAVE_8259_PORT+1);

}

/*
 * enable_irq
 *    DESCRIPTION: This function enables a secific IR pin on the PIC
 *    INPUTS: uint32_t irq_num - the IR pin to unmask
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Unmasks an IR pin and changes either slave or master mask
 */
void enable_irq(uint32_t irq_num) {
    if(irq_num < 8){
      master_mask &=  ~mask[irq_num];
      outb(master_mask, MASTER_8259_PORT+1);
    } else {
      slave_mask &= ~mask[irq_num-8];
      outb(slave_mask, SLAVE_8259_PORT+1);
    }
}

/*
 * disable_irq
 *    DESCRIPTION: Disables an IR pin on the PIC
 *    INPUTS: uint32_t irq_num - The IR pin to mask
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Masks either the master or slave IR pin, and sets the master or slave mask
 */
 void disable_irq(uint32_t irq_num) {
    if(irq_num < 8){
      master_mask |= mask[irq_num];
      outb(master_mask, MASTER_8259_PORT+1);
    } else {
      slave_mask |= mask[irq_num-8];
      outb(slave_mask, SLAVE_8259_PORT+1);
    }
}

/*
 * send_eoi
 *    DESCRIPTION: Sends EOI for the specified IR pin
 *    INPUTS: uint32_t irq_num - The IR pin to send the EOI to
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Sends either one or two EOIs, indicating the interrupt is done
 */
void send_eoi(uint32_t irq_num) {
    if(irq_num >= 8){
      outb(EOI|(irq_num-8), SLAVE_8259_PORT);
      outb(EOI|0x2, MASTER_8259_PORT);
    } else {
      outb(EOI|irq_num, MASTER_8259_PORT);
    }
}
