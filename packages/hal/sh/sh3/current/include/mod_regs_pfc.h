//=============================================================================
//
//      mod_regs_pfc.h
//
//      PFC (pin function controller) Module register definitions
//
//=============================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// The contents of this file are subject to the Red Hat eCos Public License 
// Version 1.1 (the "License"); you may not use this file except in         
// compliance with the License.  You may obtain a copy of the License at    
// http://www.redhat.com/                                                   
//                                                                          
// Software distributed under the License is distributed on an "AS IS"      
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the 
// License for the specific language governing rights and limitations under 
// the License.                                                             
//                                                                          
// The Original Code is eCos - Embedded Configurable Operating System,      
// released September 30, 1998.                                             
//                                                                          
// The Initial Developer of the Original Code is Red Hat.                   
// Portions created by Red Hat are                                          
// Copyright (C) 1998, 1999, 2000 Red Hat, Inc.                             
// All Rights Reserved.                                                     
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   jskov
// Contributors:jskov
// Date:        2000-10-30
//              
//####DESCRIPTIONEND####
//
//=============================================================================

#define CYGARC_REG_PACR                 0xa4000100 // port A control
#define CYGARC_REG_PBCR                 0xa4000102 // port B control
#define CYGARC_REG_PCCR                 0xa4000104 // port C control
#define CYGARC_REG_PDCR                 0xa4000106 // port D control
#define CYGARC_REG_PECR                 0xa4000108 // port E control
#define CYGARC_REG_PFCR                 0xa400010a // port F control
#define CYGARC_REG_PGCR                 0xa400010c // port G control
#define CYGARC_REG_PHCR                 0xa400010e // port H control
#define CYGARC_REG_PJCR                 0xa4000110 // port J control
#define CYGARC_REG_PKCR                 0xa4000112 // port K control
#define CYGARC_REG_PLCR                 0xa4000114 // port L control
#define CYGARC_REG_SCPCR                0xa4000116 // SC port control

#define CYGARC_REG_PADR                 0xa4000120 // port A data
#define CYGARC_REG_PBDR                 0xa4000122 // port B data
#define CYGARC_REG_PCDR                 0xa4000124 // port C data
#define CYGARC_REG_PDDR                 0xa4000126 // port D data
#define CYGARC_REG_PEDR                 0xa4000128 // port E data
#define CYGARC_REG_PFDR                 0xa400012a // port F data
#define CYGARC_REG_PGDR                 0xa400012c // port G data
#define CYGARC_REG_PHDR                 0xa400012e // port H data
#define CYGARC_REG_PJDR                 0xa4000130 // port J data
#define CYGARC_REG_PKDR                 0xa4000132 // port K data
#define CYGARC_REG_PLDR                 0xa4000134 // port L data
#define CYGARC_REG_SCPDR                0xa4000136 // SC port data


// Port C
#define CYGARC_REG_PCCR_PC7_MASK        0xc000
#define CYGARC_REG_PCCR_PC7_shift       14
#define CYGARC_REG_PCCR_PC6_MASK        0x3000
#define CYGARC_REG_PCCR_PC6_shift       12
#define CYGARC_REG_PCCR_PC5_MASK        0x0c00
#define CYGARC_REG_PCCR_PC5_shift       10
#define CYGARC_REG_PCCR_PC4_MASK        0x0300
#define CYGARC_REG_PCCR_PC4_shift       8
#define CYGARC_REG_PCCR_PC3_MASK        0x00c0
#define CYGARC_REG_PCCR_PC3_shift       6
#define CYGARC_REG_PCCR_PC2_MASK        0x0030
#define CYGARC_REG_PCCR_PC2_shift       4
#define CYGARC_REG_PCCR_PC1_MASK        0x000c
#define CYGARC_REG_PCCR_PC1_shift       2
#define CYGARC_REG_PCCR_PC0_MASK        0x0003
#define CYGARC_REG_PCCR_PC0_shift       0

#define CYGARC_REG_PCCR_MD_OTHER        0x0
#define CYGARC_REG_PCCR_MD_OUT          0x1
#define CYGARC_REG_PCCR_MD_IN_ON        0x2
#define CYGARC_REG_PCCR_MD_IN_OFF       0x3

#define CYGARC_REG_PCDR_PC7DT           0x80
#define CYGARC_REG_PCDR_PC6DT           0x40
#define CYGARC_REG_PCDR_PC5DT           0x20
#define CYGARC_REG_PCDR_PC4DT           0x10
#define CYGARC_REG_PCDR_PC3DT           0x08
#define CYGARC_REG_PCDR_PC2DT           0x04
#define CYGARC_REG_PCDR_PC1DT           0x02
#define CYGARC_REG_PCDR_PC0DT           0x01

