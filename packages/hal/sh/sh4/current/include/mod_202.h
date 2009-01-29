#ifndef CYGONCE_HAL_MOD_77xx_H
#define CYGONCE_HAL_MOD_77xx_H

//=============================================================================
//
//      mod_202.h
//
//      List modules available on CPU
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   nickg
// Date:        2003-08-20
// Purpose:     Define modules (and versions) available on this CPU.
// Usage:       Included from <cyg/hal/sh_regs.h>
//
//              
//####DESCRIPTIONEND####
//
//=============================================================================

//-----------------------------------------------------------------------------
// Modules provided by the CPU

#define CYGARC_SH_MOD_CPG  1
#define CYGARC_SH_MOD_SCIF 1
#define CYGARC_SH_MOD_UBC  1
#define CYGARC_SH_MOD_INTC 2

//-----------------------------------------------------------------------------
// Extra details for Cache Module (CAC)

//=============================================================================
// Icache
// Cache dimenions
#define CYGARC_SH_MOD_CAC_I_SIZE        16384 // Size of cache in bytes
#define CYGARC_SH_MOD_CAC_I_LINE_SIZE   32    // Size of a cache line
#define CYGARC_SH_MOD_CAC_I_WAYS        2     // Associativity of the cache

// Cache addressing information
// entry: bits 12 -  5
#define CYGARC_SH_MOD_ICAC_ADDRESS_BASE   0xf0000000
#define CYGARC_SH_MOD_ICAC_ADDRESS_TOP    0xf0004000
#define CYGARC_SH_MOD_ICAC_ADDRESS_STEP   0x00000020
// V : bit 0
// Writing zero to V forces an invalidate of the line
#define CYGARC_SH_MOD_ICAC_ADDRESS_FLUSH  0x00000000


//=============================================================================
// Dcache
// Cache dimenions
#define CYGARC_SH_MOD_CAC_D_SIZE        32768 // Size of cache in bytes
#define CYGARC_SH_MOD_CAC_D_LINE_SIZE   32    // Size of a cache line
#define CYGARC_SH_MOD_CAC_D_WAYS        2     // Associativity of the cache

// Cache addressing information
// entry: bits 13 -  5
#define CYGARC_SH_MOD_DCAC_ADDRESS_BASE   0xf4000000
#define CYGARC_SH_MOD_DCAC_ADDRESS_TOP    0xf4008000
#define CYGARC_SH_MOD_DCAC_ADDRESS_STEP   0x00000020
// U : bit 1
// V : bit 0
// Writing zero to both forces a flush of the line if it is dirty.
#define CYGARC_SH_MOD_DCAC_ADDRESS_FLUSH  0x00000000

//-----------------------------------------------------------------------------
// Extra details for interrupt handling
#define CYGARC_SH_SOFTWARE_IP_UPDATE

#endif // CYGONCE_HAL_MOD_202_H
