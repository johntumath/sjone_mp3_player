/*

* LabGPIOInterrupt.cpp
 *
 *  Created on: Sep 14, 2018
 *      Author: John Tumath
 */

#include <LabGPIOInterrupt.h>

LabGpioInterrupts::LabGpioInterrupts()
{

}

void LabGpioInterrupts::Initialize()
{
    NVIC_EnableIRQ(EINT3_IRQn);
}

bool LabGpioInterrupts::AttachInterruptHandler(uint8_t port, uint32_t pin, IsrPointer pin_isr, InterruptCondition condition)
{
    if (pin > 31 || (port != 0 && port != 2)) return false;
    switch (port)
    {
        case 0:
            LPC_GPIO0->FIODIR &= ~(1 << pin);
            if (condition == InterruptCondition::kFallingEdge || condition == InterruptCondition::kBothEdges)
            {
                LPC_GPIOINT->IO0IntEnF |= (1 << pin);
            }
            if (condition == InterruptCondition::kRisingEdge || condition == InterruptCondition::kBothEdges)
            {
                LPC_GPIOINT->IO0IntEnR |= (1 << pin);
            }
            break;
        case 2:
            LPC_GPIO2->FIODIR &= ~(1 << pin);
            if (condition == InterruptCondition::kFallingEdge || condition == InterruptCondition::kBothEdges)
            {
                LPC_GPIOINT->IO2IntEnF |= (1 << pin);
            }
            if (condition == InterruptCondition::kRisingEdge || condition == InterruptCondition::kBothEdges)
            {
                LPC_GPIOINT->IO2IntEnR |= (1 << pin);
            }
            break;
        default:
            return false;
            break;
    }

    port = (port) ? 1 : 0;
    if (condition == InterruptCondition::kRisingEdge || condition == InterruptCondition::kBothEdges)
    {
        pin_isr_map[port][pin][1] = pin_isr;
    }
    if (condition == InterruptCondition::kFallingEdge || condition == InterruptCondition::kBothEdges)
    {
        pin_isr_map[port][pin][0] = pin_isr;
    }
    return true;
}

void LabGpioInterrupts::HandleInterrupt()
{
    // Check interrupt pins
    for (int i = 0; i < 32; ++i)
    {
        if (LPC_GPIOINT->IO0IntStatF & (1 << i))
        {
            pin_isr_map[0][i][0]();
            LPC_GPIOINT->IO0IntClr |= (1 << i);
        }
        if (LPC_GPIOINT->IO0IntStatR & (1 << i))
        {
            pin_isr_map[0][i][1]();
            LPC_GPIOINT->IO0IntClr |= (1 << i);
        }
        if (LPC_GPIOINT->IO2IntStatF & (1 << i))
        {
            pin_isr_map[1][i][0]();
            LPC_GPIOINT->IO2IntClr |= (1 << i);
        }
        if (LPC_GPIOINT->IO2IntStatR & (1 << i))
        {
            pin_isr_map[1][i][1]();
            LPC_GPIOINT->IO2IntClr |= (1 << i);
        }
    }
}
