/*
 * VS1053B.h
 *
 *  Created on: Nov 15, 2018
 *      Author: John (Offline)
 */

#ifndef VS1053B_H_
#define VS1053B_H_

#include "spi_sem.h"
#include "ssp0.h"
#include "gpio.hpp"
#include "vs10xx_uc.h"

class VS1053B {

public:
    static VS1053B* getInstance();
    void init_VS1053B();
    void WriteSci(uint8_t addr, uint16_t data);
    uint16_t ReadSci(uint8_t addr);
    int WriteSdi(const uint8_t *data, uint8_t bytes);
    virtual ~VS1053B();

private:
    static VS1053B* instance;
    VS1053B();
    GPIO DREQ = GPIO(P0_1);
    GPIO CS = GPIO(P0_29);
    GPIO DCS = GPIO(P0_0);
};

typedef union
{
    uint16_t status;
    struct
    {
        uint8_t SSREFERENCESEL: 1;
        uint8_t SSADCLOCK: 1;
        uint8_t SSAPDOWN1: 1;
        uint8_t SSAPDOWN2: 1;
        uint8_t SSVER: 4;
        uint8_t :2;
        uint8_t SSVCMDISABLE: 1;
        uint8_t SSVCMOVERLOAD: 1;
        uint8_t SSSWING: 3;
        uint8_t SSDONOTJUMP: 1;
    } __attribute__((packed));
} VS1053b_status_t;

#endif /* VS1053B_H_ */
