/*
 * LapSpi.h
 *
 *  Created on: Sep 19, 2018
 *  Author: John Tumath
 *  Description: Header file for SPI Flash Memory Driver
 */

#ifndef LABSPI0_H_
#define LABSPI0_H_

#include "LPC17xx.h"
#include "sys_config.h"

class LabSpi0
{
 public:
    enum FrameModes
    {
        SPI = 0,
        TI = 1,
        Microwire = 2
    };

    /**
     * 1) Powers on SPPn peripheral
     * 2) Set peripheral clock
     * 3) Sets pins for specified peripheral to MOSI, MISO, and SCK
     *
     * @param data_size_select transfer size data width; To optimize the code, look for a pattern in the datasheet
     * @param format is the code format for which synchronous serial protocol you want to use.
     * @param divide is the how much to divide the clock for SSP; take care of error cases such as the value of 0, 1, and odd numbers
     *
     * @return true if initialization was successful
     */
    bool initialize(uint8_t data_size_select, FrameModes format, uint8_t divide);

    /**
     * Transfers a byte via SSP to an external device using the SSP data register.
     * This region must be protected by a mutex static to this class.
     *
     * @return received byte from external device via SSP data register.
     */
    uint8_t transfer(uint8_t send);

    void setspeed(float SPICLOCKMHZ);
    void setdivider(uint16_t setdiv);
    LabSpi0();
    ~LabSpi0();

 private:

};

#endif /* LABSPI0_H_ */
