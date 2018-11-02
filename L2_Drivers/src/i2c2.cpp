/*
 *     SocialLedge.com - Copyright (C) 2013
 *
 *     This file is part of free software framework for embedded processors.
 *     You can use it and/or distribute it as long as this copyright header
 *     remains unmodified.  The code is free for personal use and requires
 *     permission to use in a commercial product.
 *
 *      THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 *      OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 *      I SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 *      CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *     You can reach the author of this software at :
 *          p r e e t . w i k i @ g m a i l . c o m
 */

#include "I2C2.hpp"
#include "LPC17xx.h"
#include "lpc_isr.h"



static void i2c2_isr()
{
    I2C2::getInstance().handleInterrupt();
}

bool I2C2::init(unsigned int speedInKhz)
{
    /**
     * Before I2C is initialized, check to be sure that the I2C wires are logic "1"
     * means that they are pulled high, otherwise there may be a short circuit.
     *
     * I2C2 is on P0.10, and P0.11
     */
    const uint32_t i2c_pin_mask = ( (1<<10) | (1<<11) );
    const bool i2c_wires_are_pulled_high = (i2c_pin_mask == (LPC_GPIO0->FIOPIN & i2c_pin_mask) );

    LPC_PINCON->PINMODE0 &= ~(0xF << 20); // Both pins with Pull-Up Enabled
    LPC_PINCON->PINMODE0 |=  (0xA << 20); // Disable both pull-up and pull-down

    // Enable Open-drain for I2C2 on pins P0.10 and P0.11
    LPC_PINCON->PINMODE_OD0 |= i2c_pin_mask;

    LPC_PINCON->PINSEL0 &= ~(0xF << 20);  // Clear
    LPC_PINCON->PINSEL0 |=  (0xA << 20);  // Enable I2C Pins: SDA, SCL

    lpc_pclk(pclk_i2c2, clkdiv_8);
    const uint32_t pclk = sys_get_cpu_clock() / 8;

    /**
     * I2C wires should be pulled high for normal operation, so if they are, initialize I2C
     * otherwise disable operations on I2C since I2C has a likely hardware BUS fault such as:
     *  - I2C SDA/SCL with no pull-up
     *  - I2C SDA/SCL shorted to ground
     */
    bool status = false;
    if (i2c_wires_are_pulled_high) {
        status = I2C_Base::init(pclk, speedInKhz);
        isr_register(I2C2_IRQn, i2c2_isr);
    }
    else {
        disableOperation();
    }

    return status;
}

I2C2::I2C2() : I2C_Base((LPC_I2C_TypeDef*) LPC_I2C2_BASE)
{

}
