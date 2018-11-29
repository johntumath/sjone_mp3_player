/*
 * LapSpi.cpp
 *
 *  Created on: Sep 19, 2018
 *  Author: John Tumath
 *	Description: Implementation file for SPI Flash Memory Driver
 */

#include <LabSPI0.h>

bool LabSpi0::initialize(uint8_t data_size_select, FrameModes format, uint8_t divide)
{

    // THIS IS FOR SPI0
    // Power on the SPPn peripheral

    LPC_SC->PCONP |= (1 << 21); //SSP0 Power Enable

    // Set the peripheral clock
    LPC_SC->PCLKSEL1 &= ~(3 << 10); //Should set 11:10 to zero for CLK/4

    // Set pins for for specified peripheral
    //FOR SPI0 MISO (P0.17), MOSI (P0.18), SCLK (P0.15)
    LPC_PINCON->PINSEL0 &= ~(3 << 30);
    LPC_PINCON->PINSEL0 |= (2 << 30);
    LPC_PINCON->PINSEL1 &= ~(3 << 2);
    LPC_PINCON->PINSEL1 |= (2 << 4);
    LPC_PINCON->PINSEL1 &= ~(3 << 4);
    LPC_PINCON->PINSEL1 |= (2 << 4);

    // Set Data Size Select
    if (data_size_select < 4 || data_size_select > 16) return false;
    LPC_SSP0->CR0 = data_size_select - 1;

    // Set Frame Format
    LPC_SSP0->CR0 &= ~(3 << 4);
    LPC_SSP0->CR0 |= (format << 4);

    // Enable SSP1 as Master

    // Setup SSP1 Clock Prescale Register
    if (divide < 2 || divide > 254 || divide % 2) return false;
    LPC_SSP0->CPSR = divide;
    return true;
}

uint8_t LabSpi0::transfer(uint8_t send)
{
    LPC_SSP0->DR = send;
    while (LPC_SSP0->SR & (1 << 4)); //Wait until SSP0 is busy
    return LPC_SSP0->DR;
}

void LabSpi0::setspeed(float SPICLOCKMHZ)
{
    float divider = (sys_get_cpu_clock()/4)/(SPICLOCKMHZ*1000000);
    if(divider < 2) divider = 2;
    if(divider > 254) divider = 254;
    LPC_SSP0->CPSR = (uint16_t)divider;
}

void LabSpi0::setdivider(uint16_t setdiv)
{
    if (setdiv < 2 || setdiv > 254 || setdiv % 2) LPC_SSP0->CPSR = 2;
    else LPC_SSP0->CPSR = setdiv;
}

LabSpi0::LabSpi0()
{
}

LabSpi0::~LabSpi0()
{
}
