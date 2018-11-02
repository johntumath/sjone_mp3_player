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

#include "uart0.hpp"
#include "LPC17xx.h"     // LPC_UART0_BASE
#include "lpc_isr.h"
#include "sys_config.h"  // sys_get_cpu_clock()



static void uart0_isr(void)
{
    Uart0::getInstance().handleInterrupt();
}

bool Uart0::init(unsigned int baudRate, int rxQSize, int txQSize)
{
    // Configure PINSEL for UART0
    LPC_PINCON->PINSEL0 &= ~(0xF << 4); // Clear values
    LPC_PINCON->PINSEL0 |=  (0x5 << 4); // Set values for UART0 Rx/Tx

    // Set UART0 Peripheral Clock divider to 1
    lpc_pclk(pclk_uart0, clkdiv_1);
    const unsigned int pclk = sys_get_cpu_clock();

    isr_register(UART0_IRQn, uart0_isr);
    return UartDev::init(pclk, baudRate, rxQSize, txQSize);
}

Uart0::Uart0() : UartDev((unsigned int*)LPC_UART0_BASE)
{
    // Nothing to do here other than handing off LPC_UART0_Base address to UART_Base
}
