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

#include <stdio.h>          // printf()

#include "FreeRTOS.h"
#include "task.h"           // vRunTimeStatIsrEntry() and vRunTimeStatIsrExit()

#include "LPC17xx.h"        // IRQn_Type
#include "uart0_min.h"      // Uart0 initialization
#include "printf_lib.h"     // u0_dbg_printf()

#include "lpc_sys.h"        // sys_reboot()
#include "fault_registers.h"// FAULT registers to store upon crash



#if defined (__cplusplus)
extern "C"
{
// The entry point for the C++ library startup
extern void __libc_init_array(void);
#endif

/// CPU execution begins from this function
void isr_reset(void);

/// The common ISR handler for the chip level interrupts that forwards to the user interrupts
static void isr_forwarder_routine(void);

/// isr_forwarder_routine() will call this function unless user interrupt is registered
void isr_default_handler(void);

/// The hard fault handler
void isr_hard_fault_handler(unsigned long *hardfault_args);

/** @{ Weak ISR handlers; these are over-riden when user defines them elsewhere */
#define WEAK     __attribute__ ((weak))
#define ALIAS(f) __attribute__ ((weak, alias (#f)))

WEAK void isr_nmi(void);
WEAK void isr_hard_fault(void);
WEAK void isr_mem_fault(void);
WEAK void isr_bus_fault(void);
WEAK void isr_usage_fault(void);
WEAK void isr_debug_mon(void);
WEAK void isr_sys_tick(void);
/** @} */

/** @{ FreeRTOS Interrupt Handlers  */
extern void xPortSysTickHandler(void);  ///< OS timer or tick interrupt (for time slicing tasks)
extern void xPortPendSVHandler(void);   ///< Context switch is performed using this interrupt
extern void vPortSVCHandler(void);      ///< OS "supervisor" call to start first FreeRTOS task
/** @} */

/// Linker script (loader.ld) defines the initial stack pointer that we set at the interrupt vector
extern void _vStackTop(void);

#if defined (__cplusplus)
} // extern "C"
#endif

/** @{ External functions that we will call */
extern void low_level_init(void);
extern void high_level_init(void);
extern int main();
/** @} */

/**
 * CPU interrupt vector table that is loaded at the beginning of the CPU start
 * location by using the linker script that will place it at the isr_vector location.
 * CPU loads the stack pointer and begins execution from Reset vector.
 */
extern void (* const g_pfnVectors[])(void);
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) =
{
        // Core Level - CM3
        &_vStackTop,        // The initial stack pointer
        isr_reset,          // The reset handler
        isr_nmi,            // The NMI handler
        isr_hard_fault,     // The hard fault handler
        isr_mem_fault,      // The MPU fault handler
        isr_bus_fault,      // The bus fault handler
        isr_usage_fault,    // The usage fault handler
        0,                  // Reserved
        0,                  // Reserved
        0,                  // Reserved
        0,                  // Reserved
        vPortSVCHandler,    // FreeRTOS SVC-call handler (naked function so needs direct call - not a wrapper)
        isr_debug_mon,      // Debug monitor handler
        0,                  // Reserved
        xPortPendSVHandler, // FreeRTOS PendSV handler (naked function so needs direct call - not a wrapper)
        isr_sys_tick,       // FreeRTOS SysTick handler (we enclose inside a wrapper to track OS overhead)

        // Chip Level - LPC17xx - common ISR that will call the real ISR
        isr_forwarder_routine,      // 16, 0x40 - WDT
        isr_forwarder_routine,      // 17, 0x44 - TIMER0
        isr_forwarder_routine,      // 18, 0x48 - TIMER1
        isr_forwarder_routine,      // 19, 0x4c - TIMER2
        isr_forwarder_routine,      // 20, 0x50 - TIMER3
        isr_forwarder_routine,      // 21, 0x54 - UART0
        isr_forwarder_routine,      // 22, 0x58 - UART1
        isr_forwarder_routine,      // 23, 0x5c - UART2
        isr_forwarder_routine,      // 24, 0x60 - UART3
        isr_forwarder_routine,      // 25, 0x64 - PWM1
        isr_forwarder_routine,      // 26, 0x68 - I2C0
        isr_forwarder_routine,      // 27, 0x6c - I2C1
        isr_forwarder_routine,      // 28, 0x70 - I2C2
        isr_forwarder_routine,      // 29, 0x74 - SPI
        isr_forwarder_routine,      // 30, 0x78 - SSP0
        isr_forwarder_routine,      // 31, 0x7c - SSP1
        isr_forwarder_routine,      // 32, 0x80 - PLL0 (Main PLL)
        isr_forwarder_routine,      // 33, 0x84 - RTC
        isr_forwarder_routine,      // 34, 0x88 - EINT0
        isr_forwarder_routine,      // 35, 0x8c - EINT1
        isr_forwarder_routine,      // 36, 0x90 - EINT2
        isr_forwarder_routine,      // 37, 0x94 - EINT3
        isr_forwarder_routine,      // 38, 0x98 - ADC
        isr_forwarder_routine,      // 39, 0x9c - BOD
        isr_forwarder_routine,      // 40, 0xA0 - USB
        isr_forwarder_routine,      // 41, 0xa4 - CAN
        isr_forwarder_routine,      // 42, 0xa8 - GP DMA
        isr_forwarder_routine,      // 43, 0xac - I2S
        isr_forwarder_routine,      // 44, 0xb0 - Ethernet
        isr_forwarder_routine,      // 45, 0xb4 - RITINT
        isr_forwarder_routine,      // 46, 0xb8 - Motor Control PWM
        isr_forwarder_routine,      // 47, 0xbc - Quadrature Encoder
        isr_forwarder_routine,      // 48, 0xc0 - PLL1 (USB PLL)
        isr_forwarder_routine,      // 49, 0xc4 - USB Activity interrupt to wakeup
        isr_forwarder_routine,      // 50, 0xc8 - CAN Activity interrupt to wakeup
    };



//*****************************************************************************
// Functions to carry out the initialization of RW and BSS data sections. These
// are written as separate functions rather than being inlined within the
// ResetISR() function in order to cope with MCUs with multiple banks of
// memory.
//*****************************************************************************
__attribute__ ((section(".after_vectors")))
void data_init(unsigned int romstart, unsigned int start, unsigned int len)
{
    unsigned int *pulDest = (unsigned int*) start;
    unsigned int *pulSrc = (unsigned int*) romstart;
    unsigned int loop;
    for (loop = 0; loop < len; loop = loop + 4)
        *pulDest++ = *pulSrc++;
}

__attribute__ ((section(".after_vectors")))
void bss_init(unsigned int start, unsigned int len)
{
    unsigned int *pulDest = (unsigned int*) start;
    unsigned int loop;
    for (loop = 0; loop < len; loop = loop + 4)
        *pulDest++ = 0;
}


//*****************************************************************************
extern unsigned int __data_section_table;
extern unsigned int __data_section_table_end;
//extern unsigned int __bss_section_table;
extern unsigned int __bss_section_table_end;

//*****************************************************************************
// Code Entry Point : The CPU RESET Handler
// This sets up the system and copies global memory contensts from FLASH to RAM
// and initializes C/C++ environment
//*****************************************************************************
extern "C" {
__attribute__ ((section(".after_vectors"), naked))
void isr_reset(void)
{
    // remove compiler warning
    (void)g_pfnVectors;

    /**
     * The hyperload bootloader sets the MSP/PSP upon a true reset, which is when the
     * LPC17xx (Cortex-M3) sets the values of the stack pointer.  But since we are
     * booting after a bootloader, we have to manually setup the stack pointers ourselves.
     */
    do {
        const uint32_t topOfStack = (uint32_t) &_vStackTop;
        __set_PSP(topOfStack);
        __set_MSP(topOfStack);
    } while(0);

    do {
        // Copy data from FLASH to RAM
        unsigned int LoadAddr, ExeAddr, SectionLen;
        unsigned int *SectionTableAddr;

        // Load base address of Global Section Table
        SectionTableAddr = &__data_section_table;

        // Copy the data sections from flash to SRAM.
        while (SectionTableAddr < &__data_section_table_end)
        {
            LoadAddr = *SectionTableAddr++;
            ExeAddr = *SectionTableAddr++;
            SectionLen = *SectionTableAddr++;
            data_init(LoadAddr, ExeAddr, SectionLen);
        }

        // At this point, SectionTableAddr = &__bss_section_table;
        // Zero fill the bss segment
        while (SectionTableAddr < &__bss_section_table_end)
        {
            ExeAddr = *SectionTableAddr++;
            SectionLen = *SectionTableAddr++;
            bss_init(ExeAddr, SectionLen);
        }
    } while (0) ;

    #if defined (__cplusplus)
        __libc_init_array();    // Call C++ library initialization
    #endif

    do {
        low_level_init();   // Initialize minimal system, such as Clock & UART
        high_level_init();  // Initialize high level board specific features
        main();             // Finally call main()
    } while(0);

    // In case main() exits:
    uart0_init(SYS_CFG_UART0_BPS);
    u0_dbg_put("main() should never exit on this system\n");
    while (1) {
        ;
    }
}
}

/**
 * Array of IRQs that the user can register, which we default to the weak ISR handler.
 * The user can either define the real one to override the weak handler, or the user
 * can call the isr_register() API to change the function pointer at this array.
 */
typedef void (*isr_func_t) (void);

/// Typedef of ISR function and its name
typedef struct {
    isr_func_t func;
    const char * name;
} isr_func_name_t;

static isr_func_name_t g_isr_array[] = {
    {isr_default_handler, "WDT   " }, // 16, 0x40 - WDT
    {isr_default_handler, "TIMER0" }, // 17, 0x44 - TIMER0
    {isr_default_handler, "TIMER1" }, // 18, 0x48 - TIMER1
    {isr_default_handler, "TIMER2" }, // 19, 0x4c - TIMER2
    {isr_default_handler, "TIMER3" }, // 20, 0x50 - TIMER3
    {isr_default_handler, "UART0 " }, // 21, 0x54 - UART0
    {isr_default_handler, "UART1 " }, // 22, 0x58 - UART1
    {isr_default_handler, "UART2 " }, // 23, 0x5c - UART2
    {isr_default_handler, "UART3 " }, // 24, 0x60 - UART3
    {isr_default_handler, "PWM1  " }, // 25, 0x64 - PWM1
    {isr_default_handler, "I2C0  " }, // 26, 0x68 - I2C0
    {isr_default_handler, "I2C1  " }, // 27, 0x6c - I2C1
    {isr_default_handler, "I2C2  " }, // 28, 0x70 - I2C2
    {isr_default_handler, "SPI   " }, // 29, 0x74 - SPI
    {isr_default_handler, "SSP0  " }, // 30, 0x78 - SSP0
    {isr_default_handler, "SSP1  " }, // 31, 0x7c - SSP1
    {isr_default_handler, "PLL0  " }, // 32, 0x80 - PLL0 (Main PLL)
    {isr_default_handler, "RTC   " }, // 33, 0x84 - RTC
    {isr_default_handler, "EINT0 " }, // 34, 0x88 - EINT0
    {isr_default_handler, "EINT1 " }, // 35, 0x8c - EINT1
    {isr_default_handler, "EINT2 " }, // 36, 0x90 - EINT2
    {isr_default_handler, "EINT3 " }, // 37, 0x94 - EINT3
    {isr_default_handler, "ADC   " }, // 38, 0x98 - ADC
    {isr_default_handler, "BOD   " }, // 39, 0x9c - BOD
    {isr_default_handler, "USB   " }, // 40, 0xA0 - USB
    {isr_default_handler, "CAN   " }, // 41, 0xa4 - CAN
    {isr_default_handler, "GP DMA" }, // 42, 0xa8 - GP DMA
    {isr_default_handler, "I2S   " }, // 43, 0xac - I2S
    {isr_default_handler, "Ethern" }, // 44, 0xb0 - Ethernet
    {isr_default_handler, "RITINT" }, // 45, 0xb4 - RITINT
    {isr_default_handler, "Motor " }, // 46, 0xb8 - Motor Control PWM
    {isr_default_handler, "Quadra" }, // 47, 0xbc - Quadrature Encoder
    {isr_default_handler, "PLL1  " }, // 48, 0xc0 - PLL1 (USB PLL)
    {isr_default_handler, "USBAct" }, // 49, 0xc4 - USB Activity interrupt to wakeup
    {isr_default_handler, "CAN   " }, // 50, 0xc8 - CAN Activity interrupt to wakeup
};

/**A
 * This function allows the user to register a function for the interrupt service routine.
 * Registration of an IRQ is not necessary if the weak ISR has been over-riden.
 */
extern "C" void isr_register(IRQn_Type num, void (*isr_func_ptr) (void))
{
    if ((int)num >= 0)
    {
        g_isr_array[num].func = isr_func_ptr;

        // Register the ISR name for FreeRTOS trace.  This returns pointer to the name itself, so we don't need returned data
        xTraceSetISRProperties(g_isr_array[num].name, NVIC_GetPriority(num));
    }
}

/**
 * This is the common IRQ handler for the chip (or peripheral) interrupts.  We have this
 * common IRQ here to allow more flexibility for the user to register their interrupt.
 * User can either override the aliased IRQ handler, or use the isr_register() API to
 * register it any of their own functions during runtime.
 */
static void isr_forwarder_routine(void)
{
    /* Inform FreeRTOS run-time counter API that we are inside an ISR such that it
     * won't think that the task is using the CPU.
     */
    vRunTimeStatIsrEntry();

    /* Get the IRQ number we are in.  Note that ICSR's real ISR bits are offset by 16.
     * We can read ICSR register too, but let's just read 8-bits directly.
     */
    const unsigned char isr_num = (*((unsigned char*) 0xE000ED04)) - 16; // (SCB->ICSR & 0xFF) - 16;

    // Trace the entry of this ISR
    vTraceStoreISRBegin((traceHandle) g_isr_array[isr_num].name);

    /* Lookup the function pointer we want to call and make the call */
    isr_func_t isr_to_service = g_isr_array[isr_num].func;

    /* If the user has not over-riden the "weak" isr name, or not registerd the new one using
     * isr_register(), then it will point to the isr_default_handler.
     */
    if (isr_default_handler == isr_to_service)
    {
        u0_dbg_printf("%u IRQ was triggered, but no IRQ service was defined!\n", isr_num);
        while(1);
    }
    else
    {
        isr_to_service();
    }
    vTraceStoreISREnd(0);

    /* Inform FreeRTOS that we have exited the ISR */
    vRunTimeStatIsrExit();
}


__attribute__ ((section(".after_vectors")))
void isr_hard_fault(void)
{
    __asm("MOVS   R0, #4  \n"
            "MOV    R1, LR  \n"
            "TST    R0, R1  \n"
            "BEQ    _MSP    \n"
            "MRS    R0, PSP \n"
            "B      isr_hard_fault_handler  \n"
            "_MSP:  \n"
            "MRS    R0, MSP \n"
            "B      isr_hard_fault_handler  \n"
    ) ;
}

__attribute__ ((section(".after_vectors"))) void isr_nmi(void)        { u0_dbg_put("NMI Fault\n"); while(1); }
__attribute__ ((section(".after_vectors"))) void isr_mem_fault(void)  { u0_dbg_put("Mem Fault\n"); while(1); }
__attribute__ ((section(".after_vectors"))) void isr_bus_fault(void)  { u0_dbg_put("BUS Fault\n"); while(1); }
__attribute__ ((section(".after_vectors"))) void isr_usage_fault(void){ u0_dbg_put("Usage Fault\n"); while(1); }
__attribute__ ((section(".after_vectors"))) void isr_debug_mon(void)  { u0_dbg_put("DBGMON Fault\n"); while(1); }

/// If an IRQ is not registered, we end up at this stub function
__attribute__ ((section(".after_vectors"))) void isr_default_handler(void) { u0_dbg_put("IRQ not registered!"); while(1); }

/// Wrap the FreeRTOS tick function such that we get a true measure of how much CPU tasks are using
__attribute__ ((section(".after_vectors"))) void isr_sys_tick(void)
{
    vRunTimeStatIsrEntry();
    xPortSysTickHandler();
    vRunTimeStatIsrExit();
}

/**
 * This is called from the HardFault_HandlerAsm with a pointer the Fault stack as the parameter.
 * We can then read the values from the stack and place them into local variables for ease of reading.
 * We then read the various Fault Status and Address Registers to help decode cause of the fault.
 * The function ends with a BKPT instruction to force control back into the debugger
 */
void isr_hard_fault_handler(unsigned long *hardfault_args)
{
    volatile unsigned int stacked_r0 ;
    volatile unsigned int stacked_r1 ;
    volatile unsigned int stacked_r2 ;
    volatile unsigned int stacked_r3 ;
    volatile unsigned int stacked_r12 ;
    volatile unsigned int stacked_lr ;
    volatile unsigned int stacked_pc ;
    volatile unsigned int stacked_psr ;

    stacked_r0 = ((unsigned long)hardfault_args[0]) ;
    stacked_r1 = ((unsigned long)hardfault_args[1]) ;
    stacked_r2 = ((unsigned long)hardfault_args[2]) ;
    stacked_r3 = ((unsigned long)hardfault_args[3]) ;
    stacked_r12 = ((unsigned long)hardfault_args[4]) ;
    stacked_lr = ((unsigned long)hardfault_args[5]) ;
    stacked_pc = ((unsigned long)hardfault_args[6]) ;
    stacked_psr = ((unsigned long)hardfault_args[7]) ;

    FAULT_EXISTS = FAULT_PRESENT_VAL;
    FAULT_PC = stacked_pc;
    FAULT_LR = stacked_lr - 1;
    FAULT_PSR = stacked_psr;

    sys_reboot();

    /* Prevent compiler warnings */
    (void) stacked_r0 ;
    (void) stacked_r1 ;
    (void) stacked_r2 ;
    (void) stacked_r3 ;
    (void) stacked_r12 ;
}
