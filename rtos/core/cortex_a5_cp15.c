
//-----------------------------------------------------------------------------
// Reg Reads                    Writes
//----------------------------------------------------------------------------
// 0   ID code                  Unpredictable
// 0   cache type               Unpredictable
// 0   TCM status               Unpredictable
// 1   Control                  Control
// 2   Translation table base   Translation table base
// 3   Domain access control    Domain access control
// 4                                                       (Reserved)
// 5   Data fault status        Data fault status
// 5   Instruction fault status Instruction fault status
// 6   Fault address            Fault address
// 7   cache operations         cache operations
// 8   Unpredictable            TLB operations
// 9   cache lockdown           cache lockdown
// 9   TCM region               TCM region
// 10  TLB lockdown             TLB lockdown
// 11                                                      (Reserved) 
// 12                                                      (Reserved) 
// 13  FCSE PID                 FCSE PID
// 13  Context ID               Context ID
// 14                                                      (Reserved)
// 15  Test configuration       Test configuration
//-----------------------------------------------------------------------------


/** \page cp15_f CP15 Functions.
 *
 * \section CP15 function Usage
 *
 * Methods to manage the Coprocessor 15. Coprocessor 15, or System Control 
 * Coprocessor CP15, is used to configure and control all the items in the 
 * list below:
 * <ul>
 * <li> ARM core
 * <li> caches (Icache, Dcache and write buffer)
 * <li> TCM
 * <li> MMU
 * <li> Other system options
 * </ul>
 * \section Usage
 *
 * -# Enable or disable D cache with Enable_D_cache and Disable_D_cache
 * -# Enable or disable I cache with Enable_I_cache and Disable_I_cache
 *
 * Related files:\n
 * \ref cp15.h\n
 * \ref cp15.c.\n
 */

/** \file */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "cortex_a5_cp15.h"
#include "uart.h"
#if defined(__ICCARM__)
  #include <intrinsics.h>
#endif

#undef SendUart0String
#define SendUart0String(...)

/*----------------------------------------------------------------------------
 *        Global functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Check Instruction cache
 * \return 0 if I_cache disable, 1 if I_cache enable
 */
unsigned int CP15_IsIcacheEnabled(void)
{
    unsigned int control;

    control = CP15_ReadControl();
    return ((control & (1 << CP15_I_BIT)) != 0);
} 


/**
 * \brief  Enable Instruction cache
 */
void CP15_EnableIcache(void)
{
    unsigned int control;

    control = CP15_ReadControl();

    // Check if cache is disabled
    if ((control & (1 << CP15_I_BIT)) == 0) 
	 {
        control |= (1 << CP15_I_BIT);
        CP15_WriteControl(control);
    }
}


/**
 * \brief  Disable Instruction cache
 */
void CP15_DisableIcache(void)
{
    unsigned int control;

    control = CP15_ReadControl();

    // Check if cache is enabled
    if ((control & (1 << CP15_I_BIT)) != 0) 
	 {
        control &= ~(1ul << CP15_I_BIT);
        CP15_WriteControl(control);        
    }
} 

/**
 * \brief  Check MMU
 * \return  0 if MMU disable, 1 if MMU enable
 */
unsigned int CP15_IsMMUEnabled(void)
{
    unsigned int control;

    control = CP15_ReadControl();
    return ((control & (1 << CP15_M_BIT)) != 0);
} 


/**
 * \brief  Enable MMU
 */
void CP15_EnableMMU(void)
{
    unsigned int control;

    control = CP15_ReadControl();

    // Check if MMU is disabled
    if ((control & (1 << CP15_M_BIT)) == 0) 
	 {
        control |= (1 << CP15_M_BIT);
        CP15_WriteControl(control);        
        SendUart0String("MMU enabled.\n\r");
    }
    else 
	 {
        SendUart0String("MMU is already enabled.\n\r");
    }
}


/**
 * \brief  Disable MMU
 */
void CP15_DisableMMU(void)
{
    unsigned int control;

    control = CP15_ReadControl();

    // Check if MMU is enabled
    if ((control & (1 << CP15_M_BIT)) != 0) 
	 {
        control &= ~(1ul << CP15_M_BIT);
        control &= ~(1ul << CP15_C_BIT);
        CP15_WriteControl(control);        
        SendUart0String("MMU disabled.\n\r");
    }
    else 
	 {
        SendUart0String("MMU is already disabled.\n\r");
    }
}


/**
 * \brief  Check D_cache
 * \return  0 if D_cache disable, 1 if D_cache enable (with MMU of course)
 */
unsigned int CP15_IsDcacheEnabled(void)
{
    unsigned int control;

    control = CP15_ReadControl();
    return ((control & ((1 << CP15_C_BIT)||(1 << CP15_M_BIT))) != 0);
} 

/**
 * \brief  Enable Data cache
 */
void CP15_EnableDcache(void)
{
    unsigned int control;

    control = CP15_ReadControl();

    if( !CP15_IsMMUEnabled() ) 
	 {
        SendUart0String("Do nothing: MMU not enabled\n\r");
    }
    else 
	 {
        // Check if cache is disabled
        if ((control & (1 << CP15_C_BIT)) == 0) 
		  {
            control |= (1 << CP15_C_BIT);
            CP15_WriteControl(control);        
            SendUart0String("D cache enabled.\n\r");
        }
        else 
		  {
            SendUart0String("D cache is already enabled.\n\r");
        }
    }
}

/**
 * \brief  Disable Data cache
 */
void CP15_DisableDcache(void)
{
    unsigned int control;

    control = CP15_ReadControl();

    // Check if cache is enabled
    if ((control & (1 << CP15_C_BIT)) != 0) 
	 {
        control &= ~(1ul << CP15_C_BIT);
        CP15_WriteControl(control);
        SendUart0String("D cache disabled.\n\r");
    }
    else 
	 {
        SendUart0String("D cache is already disabled.\n\r");
    }
}





