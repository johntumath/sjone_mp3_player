/*
 * VS1053B.cpp
 *
 *  Created on: Nov 15, 2018
 *      Author: John (Offline)
 */

#include <VS1053B.h>

VS1053B* VS1053B::instance = 0;

void VS1053B::init_VS1053B()
{
    //Init SPI0
    ssp0_init(12);
    ssp0_set_max_clock(12);
    //Init DREQ GPIO (P0.01)
    //DREQ = GPIO(P0_1);
    DREQ.setAsInput();
    //Init CS GPIO (P0.29)
    //CS = GPIO(P0_29);
    CS.setAsOutput();
    //Init DCS GPIO (P0.00)
    //DCS = GPIO(P0_0);
    DCS.setAsOutput();
}

void VS1053B::WriteSci(uint8_t addr, uint16_t data)
{
    spi0_lock();
    while(DREQ.read()==0);
    CS.setLow();
    ssp0_exchange_byte(2);
    ssp0_exchange_byte(addr);
    ssp0_exchange_byte((uint8_t)(data >> 8));
    ssp0_exchange_byte((uint8_t)(data & 0xFF));
    CS.setHigh();
    spi0_unlock();
}

uint16_t VS1053B::ReadSci(uint8_t addr)
{
    spi0_lock();
    uint16_t read;
    while(DREQ.read()==0);
    CS.setLow();
    ssp0_exchange_byte(3);
    ssp0_exchange_byte(addr);
    read = (uint16_t)ssp0_exchange_byte(0);
    read |= ssp0_exchange_byte(0);
    CS.setHigh();
    spi0_unlock();
    return read;
}

int VS1053B::WriteSdi(const uint8_t* data, uint8_t bytes)
{
    if (bytes > 32) return -1;
    spi0_lock();
    while(DREQ.read()==0);
    DCS.setLow();
    for (uint8_t i=0; i < bytes; ++i)
    {
        ssp0_exchange_byte(*data++);
    }
    DCS.setHigh();
    spi0_unlock();
    return 0;
}
VS1053B* VS1053B::getInstance()
{
    if (instance == 0)
    {
        instance = new VS1053B();
    }

    return instance;
}

VS1053B::VS1053B(){}

VS1053B::~VS1053B()
{
}

