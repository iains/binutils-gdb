/* Mach-O support for PowerPC in BFD.
   Copyright 2012
   Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef _MACH_O_POWERPC_H
#define _MACH_O_POWERPC_H

/* PowerPC relocations (Mach-O PowerPC64 is the same).  */

/* As per BFD_MACH_O_GENERIC_RELOC_VANILLA.  */
#define BFD_MACH_O_PPC_RELOC_VANILLA	    0

/* For PPC, the pair appears in both scattered and non-scattered relocs. */
#define BFD_MACH_O_PPC_RELOC_PAIR	    1

 /* 14 and 24 bit word address branch displacements, so these are 16 and
    26 bit address displacements, respectively.  */
#define BFD_MACH_O_PPC_RELOC_BR14	    2
#define BFD_MACH_O_PPC_RELOC_BR24	    3

/* These supply the upper or lower parts of an offset which is loaded
   in two instructions on PPC.  In each case a (non-scattered) PAIR 
   follows with the value encoding the 'other half' component.  */
#define BFD_MACH_O_PPC_RELOC_HI16	    4
#define BFD_MACH_O_PPC_RELOC_LO16	    5

/* In this case, the low 16bits are sign-extended before adding to the
   high.  */
#define BFD_MACH_O_PPC_RELOC_HA16	    6

/* As per LO16, except that the two bottom bits are assumed 0 and not
   stored in the instruction.  Used for 64 bit ld/st.  */
#define BFD_MACH_O_PPC_RELOC_LO14	    7

/* The next two are as per BFD_MACH_O_GENERIC_RELOC_SECTDIFF and
   BFD_MACH_O_GENERIC_RELOC_LOCAL_SECTDIFF respectively.  */
#define BFD_MACH_O_PPC_RELOC_SECTDIFF	    8
#define BFD_MACH_O_PPC_RELOC_LOCAL_SECTDIFF 15

/* As per RELOC_HI/LO/HA16/14 above, but between sections.  */
#define BFD_MACH_O_PPC_RELOC_HI16_SECTDIFF  10
#define BFD_MACH_O_PPC_RELOC_LO16_SECTDIFF  11
#define BFD_MACH_O_PPC_RELOC_HA16_SECTDIFF  12
#define BFD_MACH_O_PPC_RELOC_LO14_SECTDIFF  14

/* PPC variant for prebound lazy pointer.  */
#define BFD_MACH_O_PPC_RELOC_PB_LA_PTR	    9

/* ??? This is used by the linker only (?) to represent a branch that can
   be directed to an island, if necessary, or directly to the target if
   it's reachable. */
#define BFD_MACH_O_PPC_RELOC_JBSR	    13

#endif /* _MACH_O_POWERPC_H */
