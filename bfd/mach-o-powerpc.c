/* PowerPC Mach-O support for BFD.
   Copyright 2011-2023
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

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "libiberty.h"
#include "mach-o.h"
#include "mach-o/reloc.h"
#include "mach-o/powerpc.h"
#include "mach-o/external.h"

#define bfd_mach_o_object_p bfd_mach_o_powerpc_object_p
#define bfd_mach_o_core_p bfd_mach_o_powerpc_core_p
#define bfd_mach_o_mkobject bfd_mach_o_powerpc_mkobject

static bfd_cleanup
bfd_mach_o_powerpc_object_p (bfd *abfd)
{
  return bfd_mach_o_header_p (abfd, 0, 0, BFD_MACH_O_CPU_TYPE_POWERPC);
}

static bfd_cleanup
bfd_mach_o_powerpc_core_p (bfd *abfd)
{
  return bfd_mach_o_header_p (abfd, 0,
			      BFD_MACH_O_MH_CORE, BFD_MACH_O_CPU_TYPE_POWERPC);
}

static bool
bfd_mach_o_powerpc_mkobject (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata;

  if (!bfd_mach_o_mkobject_init (abfd))
    return false;

  mdata = bfd_mach_o_get_data (abfd);
  mdata->header.magic = BFD_MACH_O_MH_MAGIC;
  mdata->header.cputype = BFD_MACH_O_CPU_TYPE_POWERPC;
  mdata->header.cpusubtype = BFD_MACH_O_CPU_SUBTYPE_POWERPC_ALL;
  mdata->header.byteorder = BFD_ENDIAN_BIG;
  mdata->header.version = 1;

  return true;
}

static bfd_reloc_status_type
bfd_mach_o_powerpc_unhandled_reloc (bfd *abfd ATTRIBUTE_UNUSED,
				    arelent *reloc_entry,
				    asymbol *symbol ATTRIBUTE_UNUSED,
				    void *data ATTRIBUTE_UNUSED,
				    asection *input_section ATTRIBUTE_UNUSED,
				    bfd *output_bfd ATTRIBUTE_UNUSED,
				    char **error_message)
{
  if (error_message != NULL)
    {
      static char buf[60];
      sprintf (buf, _("MACH-O PPC BFD can't handle %s"),
	       reloc_entry->howto->name);
      *error_message = buf;
    }
  return bfd_reloc_dangerous;
}

/* Mach-O needs to use the addend and value fields in unusual ways that
   prevent us from using bfd_{install,apply}_reloc.  */

static bfd_reloc_status_type
bfd_mach_o_powerpc_check_reloc (bfd *abfd ATTRIBUTE_UNUSED,
				arelent *reloc_entry ATTRIBUTE_UNUSED,
				asymbol *symbol ATTRIBUTE_UNUSED,
				void *data ATTRIBUTE_UNUSED,
				asection *input_section ATTRIBUTE_UNUSED,
				bfd *output_bfd ATTRIBUTE_UNUSED,
				char **error_message ATTRIBUTE_UNUSED)
{
  /* TODO: any validation we can apply.	 */
  return bfd_reloc_ok;
}

/* Mach-O:

   1. uses REL relocations.
   2. supports relocations like "const + sym1 - sym2".
   3. supports 2 when sym1 and sym2 are in different sections.
      (but *not*, so far, if either sym1 or sym2 is undefined).
   4. supports mapping local symbols onto a section offset.

   The object format specifies a composite reloc structure that can contain
   the necessary information in one (no sym2) or two (with sym2) entries.

   The way in which these factors are mapped onto BFD relocs is thus:

   a.	For cases involving a subtraction, we use two relocs to represent it.

   b.	The offset contained in the section data contains:
	const + value(sym1) - value(sym2) where value() includes the section
	address for the symbol section as present in the object.

   c.	Where a relocation refers to a local symbol, as a section offset, we
	put that value into 'addend'.  The BFD symbol used is the section one
	for the relevant section.

   d.	When we have a 'scattered' relocation we set the bit 31 of the addend.

   ** PPC **

   In addition to these general points:

   Mach-O ppc relocations make a special pair entry for h?16/lo1? relocs.
   In these, the pair is used with the address field set to the 'other' half of
   the offset (lower or upper for h* or l* respectively).  This may occur in
   both 'vanilla' and 'scattered' cases. In the former the PAIR will have a
   'special' symbol index of 0xffffff.	In the latter, the value field will
   hold the section offset for the local symbol referenced.

   JBSR: TODO: fill me in....

   Implementation NOTE!!!

   In order to avoid proliferation of internal reloc tags (since SECT, LOC and
   PAIRS are all applicable to 8, 16, 32, {and 64} bit relocations) this table
   has been arranged so that contiguous blocks represent these cases - which
   can be referred to with the 'base' (8 bit) value + the size of the reloc as
   a power of two.

   If you alter that, you will need to adjust GAS as well.  */

static reloc_howto_type powerpc_howto_table[]=
{
  /* 0 */
  HOWTO(BFD_RELOC_8, 0, 1, 8, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "8",
	false, 0xff, 0xff, false),
  HOWTO(BFD_RELOC_16, 0, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "16",
	false, 0xffff, 0xffff, false),
  HOWTO(BFD_RELOC_32, 0, 4, 32, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "32",
	false, 0xffffffff, 0xffffffff, false),
  HOWTO(BFD_RELOC_NONE, 0, 4, 32, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_unhandled_reloc, "ERROR-64B-RELOC",
	false, 0, 0, false),
  /* 4 */
  HOWTO(BFD_RELOC_8_PCREL, 0, 1, 8, true, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "DISP8",
	false, 0xff, 0xff, true),
  HOWTO(BFD_RELOC_16_PCREL, 0, 2, 16, true, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "DISP16",
	false, 0xffff, 0xffff, true),
  HOWTO(BFD_RELOC_32_PCREL, 0, 4, 32, true, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "DISP32",
	false, 0xffffffff, 0xffffffff, true),
  HOWTO(BFD_RELOC_NONE, 0, 4, 32, true, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_unhandled_reloc, "ERROR-64B-PC-RELOC",
	false, 0, 0, true),
  /* 8 */
  HOWTO(BFD_RELOC_PPC_B26, 0, 4, 26, true, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "BRANCH24",
	false, 0x03fffffc, 0x03fffffc, true),
  HOWTO(BFD_RELOC_PPC_B16, 0, 4, 16, true, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "BRANCH14",
	false, 0xfffc, 0xfffc, true),
  HOWTO(BFD_RELOC_NONE, 0, 4, 32, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_unhandled_reloc, "ERROR-NO-PB_LA_PTR",
	false, 0, 0, false),
  HOWTO(BFD_RELOC_NONE, 0, 4, 32, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_unhandled_reloc, "ERROR-NO-JBSR",
	false, 0, 0, false),
  /* 12 */
  HOWTO(BFD_RELOC_MACH_O_PAIR, 0, 1, 8, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "PAIR_8",
	false, 0xff, 0xff, false),
  HOWTO(BFD_RELOC_MACH_O_PAIR, 0, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "PAIR_16",
	false, 0xffff, 0xffff, false),
  HOWTO(BFD_RELOC_MACH_O_PAIR, 0, 4, 32, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "PAIR_32",
	false, 0xffffffff, 0xffffffff, false),
  HOWTO(BFD_RELOC_MACH_O_PAIR, 0, 4, 32, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_unhandled_reloc, "ERROR-64B-PAIR",
	false, 0xffffffff, 0xffffffff, false),
  /* 16 */
  HOWTO(BFD_RELOC_MACH_O_SECTDIFF, 0, 1, 8, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "SECTDIFF_8",
	false, 0xff, 0xff, false),
  HOWTO(BFD_RELOC_MACH_O_SECTDIFF, 0, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "SECTDIFF_16",
	false, 0xffff, 0xffff, false),
  HOWTO(BFD_RELOC_MACH_O_SECTDIFF, 0, 4, 32, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "SECTDIFF_32",
	false, 0xffffffff, 0xffffffff, false),
  HOWTO(BFD_RELOC_MACH_O_SECTDIFF, 0, 4, 32, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_unhandled_reloc, "SECTDIFF_ERROR-64",
	false, 0xffffffff, 0xffffffff, false),
  /* 20 */
  HOWTO(BFD_RELOC_MACH_O_LOCAL_SECTDIFF, 0, 1, 8, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "LOCDIFF_8",
	false, 0xff, 0xff, false),
  HOWTO(BFD_RELOC_MACH_O_LOCAL_SECTDIFF, 0, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "LOCDIFF_16",
	false, 0xffff, 0xffff, false),
  HOWTO(BFD_RELOC_MACH_O_LOCAL_SECTDIFF, 0, 4, 32, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "LOCDIFF_32",
	false, 0xffffffff, 0xffffffff, false),
  HOWTO(BFD_RELOC_MACH_O_LOCAL_SECTDIFF, 0, 4, 32, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_unhandled_reloc, "LOCDIFF_ERROR-64",
	false, 0xffffffff, 0xffffffff, false),
  /* 24 */
  HOWTO(BFD_RELOC_MACH_O_PPC_LO16, 0, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "LO16",
	false, 0xffff, 0xffff, false),
  HOWTO(BFD_RELOC_MACH_O_PPC_HI16, 16, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "HI16",
	false, 0xffff, 0xffff, false),
  HOWTO(BFD_RELOC_MACH_O_PPC_HA16, 16, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "HA16",
	false, 0xffff, 0xffff, false),
  HOWTO(BFD_RELOC_MACH_O_PPC_LO14, 0, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "LO14",
	false, 0xfffc, 0xfffc, false),
  /* 28 */
  HOWTO(BFD_RELOC_MACH_O_PPC_LO16_SECTDIFF, 0, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "LO16DIFF",
	false, 0xffff, 0xffff, false),
  HOWTO(BFD_RELOC_MACH_O_PPC_HI16_SECTDIFF, 16, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "HI16DIFF",
	false, 0xffff, 0xffff, false),
  HOWTO(BFD_RELOC_MACH_O_PPC_HA16_SECTDIFF, 16, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "HA16DIFF",
	false, 0xffff, 0xffff, false),
  HOWTO(BFD_RELOC_MACH_O_PPC_LO14_SECTDIFF, 0, 2, 16, false, 0,
	complain_overflow_bitfield,
	bfd_mach_o_powerpc_check_reloc, "LO14DIFF",
	false, 0xfffc, 0xfffc, false),
};
#if 0
/* TODO: unfinished. */
static bool
bfd_mach_o_powerpc_swap_reloc_in (arelent *res, bfd_mach_o_reloc_info *reloc)
{
  switch (reloc->r_type)
    {
      case BFD_MACH_O_PPC_RELOC_VANILLA:
	BFD_ASSERT (! reloc->r_scattered);
	BFD_ASSERT (reloc->r_length < 3);
	res->howto =
		&powerpc_howto_table[(reloc->r_pcrel << 2) | reloc->r_length];
	return true;
	break;

      case BFD_MACH_O_PPC_RELOC_PAIR:
	BFD_ASSERT (reloc->r_length < 3);
	res->howto = &powerpc_howto_table[12 + reloc->r_length];
/*	if (reloc->r_scattered)
	  res->address = res[-1].address;
	.. or leave it alone.  */
	return true;
	break;

      case BFD_MACH_O_PPC_RELOC_BR14:
	res->howto = &powerpc_howto_table[9];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_BR24:
	res->howto = &powerpc_howto_table[8];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_PB_LA_PTR:
	res->howto = &powerpc_howto_table[10];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_JBSR:
	res->howto = &powerpc_howto_table[11];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_SECTDIFF:
	BFD_ASSERT (reloc->r_scattered);
	BFD_ASSERT (reloc->r_length < 3);
	res->howto = &powerpc_howto_table[16 + reloc->r_length];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_LOCAL_SECTDIFF:
	BFD_ASSERT (reloc->r_scattered);
	BFD_ASSERT (reloc->r_length < 3);
	res->howto = &powerpc_howto_table[20 + reloc->r_length];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_LO16:
	res->howto = &powerpc_howto_table[24];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_HI16:
	res->howto = &powerpc_howto_table[25];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_HA16:
	res->howto = &powerpc_howto_table[26];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_LO14:
	res->howto = &powerpc_howto_table[27];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_LO16_SECTDIFF:
	res->howto = &powerpc_howto_table[28];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_HI16_SECTDIFF:
	res->howto = &powerpc_howto_table[29];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_HA16_SECTDIFF:
	res->howto = &powerpc_howto_table[30];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_LO14_SECTDIFF:
	res->howto = &powerpc_howto_table[31];
	return true;
	break;
      default:
fprintf (stderr, "requested : type %d pcrel %d len %d scatt %d extern %d\n",
	reloc->r_type, reloc->r_pcrel, reloc->r_length, reloc->r_scattered,
	reloc->r_extern);

	return false;
	break;
    }
}
#endif

static bool
bfd_mach_o_powerpc_canonicalize_one_reloc (bfd *       abfd,
					   struct mach_o_reloc_info_external * raw,
					   arelent *   res,
					   asymbol **  syms,
					   arelent *   res_base)
{
  bfd_mach_o_reloc_info reloc;

  if (!bfd_mach_o_pre_canonicalize_one_reloc (abfd, raw, &reloc, res, syms))
    return false;

  if (reloc.r_scattered)
    {
      switch (reloc.r_type)
	{
	case BFD_MACH_O_PPC_RELOC_PAIR:
//	case BFD_MACH_O_GENERIC_RELOC_PAIR:
	  /* PR 21813: Check for a corrupt PAIR reloc at the start.  */
	  if (res == res_base)
	    return false;
	  BFD_ASSERT (reloc.r_length < 3);
	  res->howto = &powerpc_howto_table[12 + reloc.r_length];
	  res->address = res[-1].address;
	  return true;

	case BFD_MACH_O_PPC_RELOC_SECTDIFF:
//	case BFD_MACH_O_GENERIC_RELOC_SECTDIFF:
	  BFD_ASSERT (reloc.r_scattered);
	  BFD_ASSERT (reloc.r_length < 3);
	  res->howto = &powerpc_howto_table[16 + reloc.r_length];
	  return true;

	case BFD_MACH_O_PPC_RELOC_LOCAL_SECTDIFF:
//	case BFD_MACH_O_GENERIC_RELOC_LOCAL_SECTDIFF:
	  BFD_ASSERT (reloc.r_scattered);
	  BFD_ASSERT (reloc.r_length < 3);
	  res->howto = &powerpc_howto_table[20 + reloc.r_length];
	  return true;
	  break;
	default:
	  break;
	}
    }
  else
    {
      switch (reloc.r_type)
	{
	case BFD_MACH_O_PPC_RELOC_VANILLA:
//	case BFD_MACH_O_GENERIC_RELOC_VANILLA:
	  BFD_ASSERT (reloc.r_length < 3);
	  res->howto =
		&powerpc_howto_table[(reloc.r_pcrel << 2) | reloc.r_length];
	  return true;
	  break;
	default:
	  break;
	}
    }

  switch (reloc.r_type)
    {
      case BFD_MACH_O_PPC_RELOC_BR14:
	res->howto = &powerpc_howto_table[9];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_BR24:
	res->howto = &powerpc_howto_table[8];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_PB_LA_PTR:
	res->howto = &powerpc_howto_table[10];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_JBSR:
	res->howto = &powerpc_howto_table[11];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_LO16:
	res->howto = &powerpc_howto_table[24];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_HI16:
	res->howto = &powerpc_howto_table[25];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_HA16:
	res->howto = &powerpc_howto_table[26];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_LO14:
	res->howto = &powerpc_howto_table[27];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_LO16_SECTDIFF:
	res->howto = &powerpc_howto_table[28];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_HI16_SECTDIFF:
	res->howto = &powerpc_howto_table[29];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_HA16_SECTDIFF:
	res->howto = &powerpc_howto_table[30];
	return true;
	break;
      case BFD_MACH_O_PPC_RELOC_LO14_SECTDIFF:
	res->howto = &powerpc_howto_table[31];
	return true;
	break;
      default:
fprintf (stderr, "requested : type %d pcrel %d len %d scatt %d extern %d\n",
	reloc.r_type, reloc.r_pcrel, reloc.r_length, reloc.r_scattered,
	reloc.r_extern);

	return false;
	break;
    }
  return false;
}

/* TODO: Implement this.  */
static bool
bfd_mach_o_powerpc_swap_reloc_out (arelent *rel, bfd_mach_o_reloc_info *rinfo)
{
  rinfo->r_address = rel->address;
  rinfo->r_pcrel = 0;
  rinfo->r_extern = 0;

  /* We pass the scattered location flag through the addend MSB.  */
  rinfo->r_scattered = (rel->addend & BFD_MACH_O_SR_SCATTERED)? 1 : 0 ;
  rel->addend &= ~BFD_MACH_O_SR_SCATTERED;

  switch (rel->howto->type)
    {
    case BFD_RELOC_32_PCREL:
    case BFD_RELOC_16_PCREL:
    case BFD_RELOC_8_PCREL:
    case BFD_RELOC_32:
    case BFD_RELOC_16:
    case BFD_RELOC_8:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_VANILLA;
      break;

    case BFD_RELOC_PPC_B26:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_BR24;
      break;

    case BFD_RELOC_PPC_B16:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_BR14;
      break;

    /* Used for inter-sect relocs in mdynamic-no-pic.  */
    case BFD_RELOC_MACH_O_PPC_LO16:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_LO16;
      break;

    case BFD_RELOC_MACH_O_PPC_HI16:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_HI16;
      break;

    case BFD_RELOC_MACH_O_PPC_HA16:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_HA16;
      break;

    case BFD_RELOC_MACH_O_PPC_LO14:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_LO14;
      break;

    /* Used for inter-sect relocs in PIC.  */
    case BFD_RELOC_MACH_O_PPC_LO16_SECTDIFF:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_LO16_SECTDIFF;
      rinfo->r_scattered = 1;
      break;

    case BFD_RELOC_MACH_O_PPC_HI16_SECTDIFF:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_HI16_SECTDIFF;
      rinfo->r_scattered = 1;
      break;

    case BFD_RELOC_MACH_O_PPC_HA16_SECTDIFF:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_HA16_SECTDIFF;
      rinfo->r_scattered = 1;
      break;

    case BFD_RELOC_MACH_O_PPC_LO14_SECTDIFF:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_LO14_SECTDIFF;
      rinfo->r_scattered = 1;
      break;

    /* inter-section relocs for non-instruction cases.	*/
    case BFD_RELOC_MACH_O_SECTDIFF:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_SECTDIFF;
      rinfo->r_scattered = 1;
      break;

    case BFD_RELOC_MACH_O_LOCAL_SECTDIFF:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_LOCAL_SECTDIFF;
      rinfo->r_scattered = 1;
      break;

    /* PAIR applicable to both scattered and non-scattered cases.  */
    case BFD_RELOC_MACH_O_PAIR:
      rinfo->r_type = BFD_MACH_O_PPC_RELOC_PAIR;
      break;

    default:
      return false;
    }

  rinfo->r_pcrel = rel->howto->pc_relative;
  rinfo->r_length = rel->howto->size; /* OK for m32.  */
  if ((*rel->sym_ptr_ptr)->flags & BSF_SECTION_SYM)
    {
      /* mdynamic-no-pic sym+const.  */
      if (rinfo->r_scattered)
	rinfo->r_value = rel->addend;
      else
	rinfo->r_value = (*rel->sym_ptr_ptr)->section->target_index;
    }
  else
    {
      rinfo->r_extern = 1;
      rinfo->r_value = (*rel->sym_ptr_ptr)->udata.i;
    }
  return true;
}

/* TODO: Implement this.  */
static reloc_howto_type *
bfd_mach_o_powerpc_bfd_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				       bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = 0; i < sizeof (powerpc_howto_table) / sizeof (*powerpc_howto_table); i++)
    if (code == powerpc_howto_table[i].type)
      return &powerpc_howto_table[i];
  return NULL;
}

/* TODO: Implement this.  */
static reloc_howto_type *
bfd_mach_o_powerpc_bfd_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
					  const char *name ATTRIBUTE_UNUSED)
{
  return NULL;
}

/* TODO: Implement this.  */
static bool
bfd_mach_o_powerpc_print_thread (bfd *abfd ATTRIBUTE_UNUSED,
				 bfd_mach_o_thread_flavour *thread,
				 void *vfile,
				 char *buf ATTRIBUTE_UNUSED)
{
  FILE *file = (FILE *) vfile;

  switch (thread->flavour)
    {
      case BFD_MACH_O_PPC_THREAD_STATE:
      case BFD_MACH_O_PPC_FLOAT_STATE:
      case BFD_MACH_O_PPC_EXCEPTION_STATE:
      case BFD_MACH_O_PPC_VECTOR_STATE:
	fprintf (file, " TODO --- print the PPC thread state for %lu\n", thread->flavour);
	return true;
	break;
      case BFD_MACH_O_PPC_THREAD_STATE64:
      case BFD_MACH_O_PPC_EXCEPTION_STATE64:
      case BFD_MACH_O_PPC_THREAD_STATE_NONE:
    default:
	fprintf (file, "UNEXPECTED PPC thread flavour %lu\n", thread->flavour);
      break;
    }
  return false;
}

static const mach_o_section_name_xlat text_section_names_xlat[] =
  {
    /* Bumped alignment on PPC.	 */
    {	".cstring",				"__cstring",
	SEC_READONLY | SEC_DATA | SEC_LOAD | SEC_MERGE | SEC_STRINGS,
						BFD_MACH_O_S_CSTRING_LITERALS,
	BFD_MACH_O_S_ATTR_NONE,			2},
    {	".symbol_stub",				"__symbol_stub",
	SEC_LOAD,				BFD_MACH_O_S_SYMBOL_STUBS,
	BFD_MACH_O_S_ATTR_PURE_INSTRUCTIONS,	2},
    {	".symbol_stub1",			"__symbol_stub1",
	SEC_LOAD,				BFD_MACH_O_S_SYMBOL_STUBS,
	BFD_MACH_O_S_ATTR_PURE_INSTRUCTIONS,	2},
    {	".picsymbol_stub",			"__picsymbol_stub",
	SEC_LOAD,				BFD_MACH_O_S_SYMBOL_STUBS,
	BFD_MACH_O_S_ATTR_PURE_INSTRUCTIONS,	2},
    {	".picsymbol_stub1",			"__picsymbolstub1",
	SEC_LOAD,				BFD_MACH_O_S_SYMBOL_STUBS,
	BFD_MACH_O_S_ATTR_PURE_INSTRUCTIONS,	2},
    { NULL, NULL, 0, 0, 0, 0}
  };

static const mach_o_section_name_xlat data_section_names_xlat[] =
  {
    {	".non_lazy_symbol_pointer",	"__nl_symbol_ptr",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS,
	BFD_MACH_O_S_ATTR_NONE,		2},
    {	".lazy_symbol_pointer",		"__la_symbol_ptr",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_LAZY_SYMBOL_POINTERS,
	BFD_MACH_O_S_ATTR_NONE,		2},
    { NULL, NULL, 0, 0, 0, 0}
  };

const mach_o_segment_name_xlat mach_o_ppc_segsec_names_xlat[] =
  {
    { "__TEXT", text_section_names_xlat },
    { "__DATA", data_section_names_xlat },
    { NULL, NULL }
  };

#define bfd_mach_o_canonicalize_one_reloc  bfd_mach_o_powerpc_canonicalize_one_reloc
#define bfd_mach_o_swap_reloc_out bfd_mach_o_powerpc_swap_reloc_out
//#define bfd_mach_o_swap_reloc_in bfd_mach_o_powerpc_swap_reloc_in
/* TODO: Implement this.  */
#define bfd_mach_o_print_thread bfd_mach_o_powerpc_print_thread

#define bfd_mach_o_tgt_seg_table mach_o_ppc_segsec_names_xlat
#define bfd_mach_o_section_type_valid_for_tgt NULL

#define bfd_mach_o_bfd_reloc_type_lookup bfd_mach_o_powerpc_bfd_reloc_type_lookup
#define bfd_mach_o_bfd_reloc_name_lookup bfd_mach_o_powerpc_bfd_reloc_name_lookup

#define TARGET_NAME		powerpc_mach_o_vec
#define TARGET_STRING		"mach-o-powerpc"
#define TARGET_ARCHITECTURE	bfd_arch_powerpc
#define TARGET_PAGESIZE		4096
#define TARGET_BIG_ENDIAN	1
#define TARGET_ARCHIVE		0
#define TARGET_PRIORITY		0
#include "mach-o-target.c"

#undef TARGET_NAME
#undef TARGET_STRING
#undef TARGET_ARCHITECTURE
#undef TARGET_PAGESIZE
#undef TARGET_BIG_ENDIAN
#undef TARGET_ARCHIVE
#undef TARGET_PRIORITY
