/*
 * VS1053B.h
 *
 *  Created on: Nov 15, 2018
 *      Author: John (Offline)
 */

#ifndef VS1053B_H_
#define VS1053B_H_

#include "spi_sem.h"
#include "vs10xx_uc.h"

class VS1053B {

public:
    void init_VS1053B();
    void WriteSci(uint8_t addr, uint16_t data);
    uint16_t ReadSci(uint8_t addr);
    int WriteSdi(const uint8_t *data, uint8_t bytes);
    virtual ~VS1053B();

private:
    VS1053B();
    GPIO DREQ;
    GPIO CS;
    GPIO DCS;
};

#endif /* VS1053B_H_ */
