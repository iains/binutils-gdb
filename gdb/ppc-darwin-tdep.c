#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "target.h"
#include "symtab.h"
#include "regcache.h"
#include "objfiles.h"

#include "arch-utils.h"
#include "regset.h"

#include "ppc-tdep.h"

#include "osabi.h"

#include "ppc-darwin-thread-state.h"
#include "ppc-darwin-tdep.h"

#include "solib.h"
#include "solib-darwin.h"
#include "dwarf2/frame.h"

#include "user-regs.h"
#include "target-descriptions.h"

#include "features/rs6000/powerpc-32-darwin.c"
#include "features/rs6000/powerpc-altivec32-darwin.c"

struct ppc_reg_offsets ppc_darwin_reg_offsets;

struct regset ppc_darwin_gregset =
{
  &ppc_darwin_reg_offsets,
  ppc_supply_gregset,
  ppc_collect_gregset
};

struct regset ppc_darwin_fpregset =
{
  &ppc_darwin_reg_offsets,
  ppc_supply_fpregset,
  ppc_collect_fpregset
};

struct ppc_darwin_vrreg_offset
{
  int vr0_offset;
  int vscr_offset;
  int vrsave_offset;
  int vrvalid_offset;
};

struct ppc_darwin_vrreg_offset ppc_darwin_vrreg_offsets;

struct regset ppc_darwin_vrregset =
{
  &ppc_darwin_vrreg_offsets,
  ppc_darwin_supply_vrregset,
  ppc_darwin_collect_vrregset
};

/* We have slightly different reg. sets from the default.  */
int
ppc_darwin_reg_is_in_gp_set (struct gdbarch *gdbarch, int regno)
{
  if ((regno >= PPC_R0_REGNUM && regno < (PPC_R0_REGNUM + ppc_num_gprs))
      || ((regno >= PPC_PC_REGNUM) && (regno <= PPC_XER_REGNUM))
      || (regno == PPC_MQ_REGNUM)
      || (regno == PPC_VRSAVE_REGNUM))
    return 1;

  return 0;
}

int
ppc_darwin_reg_is_in_fp_set (struct gdbarch *gdbarch, int regno)
{
  if ((regno >= PPC_F0_REGNUM && regno < (PPC_F0_REGNUM + ppc_num_fprs))
      || (regno == PPC_FPSCR_REGNUM))
    return 1;

  return 0;
}

int
ppc_darwin_reg_is_in_vec_set (struct gdbarch *gdbarch, int regno)
{
  if ((regno >= PPC_VR0_REGNUM && regno < (PPC_VR0_REGNUM + ppc_num_vrs))
      || (regno == PPC_VSCR_REGNUM) || (regno == PPC_VRVALID_REGNUM))
    return 1;

  return 0;
}


static int
ppc_vrreg_offset (ppc_gdbarch_tdep *tdep,
		  const struct ppc_darwin_vrreg_offset *offsets,
		  int regnum)
{
  if (regnum >= PPC_VR0_REGNUM
      && regnum < PPC_VR0_REGNUM + ppc_num_vrs)
    return offsets->vr0_offset + (regnum - PPC_VR0_REGNUM) * 16;

  if (regnum == PPC_VSCR_REGNUM)
    return offsets->vscr_offset;

  if (regnum == PPC_VRVALID_REGNUM)
    return offsets->vrvalid_offset;

  /* PPC_VRSAVE_REGNUM is done with the GPR set.  */
  return -1;
}

/* Supply register REGNUM in the Altivec register set REGSET
   from the buffer specified by VRREGS and LEN to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.
   Nearly identical to the one in rs6000 - but we have VRSAVE with the
   GPRs.  */

void
ppc_darwin_supply_vrregset (const struct regset *regset, struct regcache *regcache,
		     int regnum, const void *vrregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
//  printf_unfiltered ("VR regcache %p for reg %d(%s)\n", (void *)regcache, regnum, user_reg_map_regnum_to_name (gdbarch, regnum));
  const struct ppc_darwin_vrreg_offset *offsets;
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (!ppc_altivec_support_p (gdbarch))
    return;

  const gdb_byte *regs = (gdb_byte *)vrregs;
  offsets = (const ppc_darwin_vrreg_offset*)regset->regmap;
  if (regnum == -1)
    {
      for (int i = tdep->ppc_vr0_regnum, offset = offsets->vr0_offset;
	   i < tdep->ppc_vr0_regnum + ppc_num_vrs;
	   i++, offset += 16)
	ppc_supply_reg (regcache, i, regs, offset, 16);

      ppc_supply_reg (regcache, PPC_VSCR_REGNUM, regs, offsets->vscr_offset, 4);
      /* PPC_VRSAVE_REGNUM is done with the GPR set.  */
      if (offsets->vrvalid_offset != 0)
	ppc_supply_reg (regcache, PPC_VRVALID_REGNUM, regs, offsets->vrvalid_offset, 4);
      return;
    }

  int offset = ppc_vrreg_offset (tdep, offsets, regnum);
  if (offset >= 0)
    {
      if (regnum == PPC_VSCR_REGNUM || regnum == PPC_VRVALID_REGNUM)
	{
	  if (register_size (gdbarch, regnum) != 4)
	    printf_unfiltered ("unexpected size for VSCR/VRSAVE/VRVALID #%d\n",
				register_size (gdbarch, regnum));
	}
      else if (register_size (gdbarch, regnum) != 16)
	{
	  printf_unfiltered ("unexpected size for VR%d #%d\n",
			     regnum-PPC_VR0_REGNUM, register_size (gdbarch, regnum));
	  if ((unsigned)(regs + offset) & 0x0f)
	    printf_unfiltered ("unexpected alignment for VR%d #%u\n",
				regnum-PPC_VR0_REGNUM, (unsigned)(regs + offset) & 0x0f);
	}
      /* This might break... */
      ppc_supply_reg (regcache, regnum, regs, offset, register_size (gdbarch, regnum));
      if (register_size (gdbarch, regnum) == 16 && offsets->vrvalid_offset != 0)
	ppc_supply_reg (regcache, PPC_VRVALID_REGNUM, regs, offsets->vrvalid_offset, 4);
   }
  else
    printf_unfiltered ("no offset for reg #%d (%s)\n", regnum,
			user_reg_map_regnum_to_name (gdbarch, regnum));

}

/* Supply register REGNUM in the Altivec register set REGSET
   from the buffer specified by VRREGS and LEN to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.
   Nearly identical to the one in rs6000 - but we have VRSAVE with the
   GPRs.  */

void
ppc_darwin_collect_vrregset (const struct regset *regset,
	                     const struct regcache *regcache,
	                     int regnum, void *vrregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const struct ppc_darwin_vrreg_offset *offsets;
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (!ppc_altivec_support_p (gdbarch))
    return;

  offsets = (const ppc_darwin_vrreg_offset*)regset->regmap;
  gdb_byte *regs = (gdb_byte *)vrregs;
  if (regnum == -1)
    {
      for (int i = tdep->ppc_vr0_regnum, offset = offsets->vr0_offset;
	   i < tdep->ppc_vr0_regnum + ppc_num_vrs;
	   i++, offset += 16)
	ppc_collect_reg (regcache, i, regs, offset, 16);

      ppc_collect_reg (regcache, PPC_VSCR_REGNUM, regs,
		       offsets->vscr_offset, 4);
      /* PPC_VRSAVE_REGNUM is done with the GPR set.  */

      if (offsets->vrvalid_offset != 0)
	{
	  ppc_darwin_thread_vpstate *vr = (ppc_darwin_thread_vpstate*)vrregs;
	  /* We wrote all the vr regs, claim that they are valid.  */
	  vr->save_vrvalid = 0xffffffff;
//	  /* Update our copy of this to reflect the change.  */
//	  ppc_supply_reg (regcache, PPC_VRVALID_REGNUM, regs,
//			  offsets->vrvalid_offset, 4);
	}
      return;
    }

  int offset = ppc_vrreg_offset (tdep, offsets, regnum);

  if (offset >= 0) 
    {
      if (regnum == PPC_VSCR_REGNUM || regnum == PPC_VRVALID_REGNUM)
	{
	  if (register_size (gdbarch, regnum) != 4)
	    printf_unfiltered ("unexpected size for VSCR/VRVALID #%d\n",
				register_size (gdbarch, regnum));
	}
      else if (register_size (gdbarch, regnum) != 16)
	printf_unfiltered ("unexpected size for VR%d #%d\n",
			   regnum-PPC_VR0_REGNUM,
			   register_size (gdbarch, regnum));
      /* This might break... */
      ppc_collect_reg (regcache, regnum, regs, offset, register_size (gdbarch, regnum));

     /* We will claim that all regs are valid, to follow the behaviour of older
        Darwin GDB implementations.  */
      if (offsets->vrvalid_offset != 0
	  && register_size (gdbarch, regnum) == 16
	  && (regnum >= PPC_VR0_REGNUM && regnum < PPC_VR0_REGNUM + ppc_num_vrs))
	{
	  ppc_darwin_thread_vpstate *vr = (ppc_darwin_thread_vpstate*)vrregs;
	  unsigned int bit_num = regnum - PPC_VR0_REGNUM;
	  unsigned int mask = 1 << (ppc_num_vrs - 1 - bit_num);
	  /* We force-write this to say the written reg is valid.  */
	  vr->save_vrvalid |= mask;
//	  /* .. and copy that back.  */
//	  ppc_supply_reg (regcache, PPC_VRVALID_REGNUM, regs,
//			  offsets->vrvalid_offset, 4);
	}
   }
}

void
ppc_darwin_supply_gpr32_from_state64 (struct regcache *regcache, int regno,
				      const void *gregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
//  printf_unfiltered ("regcache %p for reg %d(%s)\n", (void *) regcache, regno, user_reg_map_regnum_to_name (gdbarch, regno));

  /* size for r0-31, pc, ps, lr, ctr.  */
  int size = register_size (gdbarch, PPC_R0_REGNUM);
  if (size != 4)
    printf_unfiltered ("unexpected size for GPR #%d - ignored\n", size);
  if (ppc_darwin_reg_offsets.r0_offset == 0)
    printf_unfiltered ("ppc_darwin_reg_offsets r0_offset unset?\n");

  const gdb_byte *regs = (gdb_byte *) gregs;
  if (regno == -1)
    {
      size_t off = ppc_darwin_reg_offsets.r0_offset;
      for (int i = 0; i < ppc_num_gprs; ++i, off += 8)
	  ppc_supply_reg (regcache, PPC_R0_REGNUM+i, regs, off + 4, 4);

      ppc_supply_reg (regcache, PPC_PC_REGNUM, regs,
		      ppc_darwin_reg_offsets.pc_offset + 4, 4);
      ppc_supply_reg (regcache, PPC_MSR_REGNUM, regs,
		      ppc_darwin_reg_offsets.ps_offset + 4, 4);
      ppc_supply_reg (regcache, PPC_LR_REGNUM, regs,
		      ppc_darwin_reg_offsets.lr_offset + 4, 4);
      ppc_supply_reg (regcache, PPC_CTR_REGNUM, regs,
		      ppc_darwin_reg_offsets.ctr_offset + 4, 4);
      /* VSAVE is done here.  */
      ppc_supply_reg (regcache, PPC_VRSAVE_REGNUM, regs,
		      ppc_darwin_vrreg_offsets.vrsave_offset, 4);

      size = register_size (gdbarch, PPC_CR_REGNUM);
      if (size != 4)
	printf_unfiltered ("unexpected size for CR/XER/MQ #%d - ignored\n", size);
      ppc_supply_reg (regcache, PPC_CR_REGNUM, regs,
		      ppc_darwin_reg_offsets.cr_offset, 4);
      ppc_supply_reg (regcache, PPC_XER_REGNUM, regs,
		      ppc_darwin_reg_offsets.xer_offset + 4, 4);
      /* No MQ.  */
      return;
    }

  size = register_size (gdbarch, regno);
  if (size <= 0)
    {
      printf_unfiltered ("no size for register #%d - not set\n", regno);
      return;
    }

  /* This code makes the assumption that were storing an m32 machine into
     an m64 state.  */
  if (size != 4)
   printf_unfiltered ("unexpected size for REG #%d - ignored\n", size);

  /* Guess, we might set the PC most often.  */
  if (regno == PPC_PC_REGNUM)
    ppc_supply_reg (regcache, PPC_PC_REGNUM, regs,
		    ppc_darwin_reg_offsets.pc_offset + 4, 4);
  else if (regno >= PPC_R0_REGNUM && regno <= (PPC_R0_REGNUM + ppc_num_gprs))
    {
      size_t off = ppc_darwin_reg_offsets.r0_offset
		   + (regno-PPC_R0_REGNUM)*8 + 4;
      ppc_supply_reg (regcache, regno, regs, off, 4);
    }
  else if (regno == PPC_MSR_REGNUM)
    ppc_supply_reg (regcache, PPC_MSR_REGNUM, regs,
		    ppc_darwin_reg_offsets.ps_offset + 4, 4);
  else if (regno == PPC_LR_REGNUM)
    ppc_supply_reg (regcache, PPC_LR_REGNUM, regs,
		    ppc_darwin_reg_offsets.lr_offset + 4, 4);
  else if (regno == PPC_CTR_REGNUM)
    ppc_supply_reg (regcache, PPC_CTR_REGNUM, regs,
		    ppc_darwin_reg_offsets.ctr_offset + 4, 4);
  else if (regno == PPC_CR_REGNUM)
    ppc_supply_reg (regcache, PPC_CR_REGNUM, regs,
		    ppc_darwin_reg_offsets.cr_offset, 4);
  else if (regno == PPC_XER_REGNUM)
    ppc_supply_reg (regcache, PPC_XER_REGNUM, regs,
		    ppc_darwin_reg_offsets.xer_offset + 4, 4);
  else if (regno == PPC_VRSAVE_REGNUM )
    ppc_supply_reg (regcache, PPC_VRSAVE_REGNUM, regs,
		    ppc_darwin_vrreg_offsets.vrsave_offset, 4);
  else
    printf_unfiltered ("Unhandled register #%d - not supplied\n", regno);
  /* No MQ.  */
  return;
}

/* We specifically don't touch the upper parts of the 64bit regs when
   our execution mode is m32.  */

void
ppc_darwin_collect_gpr32_into_state64 (struct regcache *regcache, int regno,
				     const void *gregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
//  printf_unfiltered ("called ppc32 for reg %d(%s)\n", regno, user_reg_map_regnum_to_name (gdbarch, regno));

  /* size for r0-31, pc, ps, lr, ctr.  */
  int size = register_size (gdbarch, PPC_R0_REGNUM);
  if (size != 4)
    printf_unfiltered ("unexpected size for GPR #%d - ignored\n", size);

  if (ppc_darwin_reg_offsets.r0_offset == 0)
    printf_unfiltered ("ppc_darwin_reg_offsets r0_offset unset?\n");

  gdb_byte *regs = (gdb_byte *) gregs;
  if (regno == -1)
    {
      size_t off = ppc_darwin_reg_offsets.r0_offset;
      for (int i = 0; i < ppc_num_gprs; ++i, off += 8)
	  ppc_collect_reg (regcache, PPC_R0_REGNUM+i, regs, off + 4, 4);

      ppc_collect_reg (regcache, PPC_PC_REGNUM, regs,
		       ppc_darwin_reg_offsets.pc_offset + 4, 4);
      ppc_collect_reg (regcache, PPC_MSR_REGNUM, regs,
		       ppc_darwin_reg_offsets.ps_offset + 4, 4);
      ppc_collect_reg (regcache, PPC_LR_REGNUM, regs,
		       ppc_darwin_reg_offsets.lr_offset + 4, 4);
      ppc_collect_reg (regcache, PPC_CTR_REGNUM, regs,
		       ppc_darwin_reg_offsets.ctr_offset + 4, 4);
      /* VSAVE is done here.  */
      ppc_collect_reg (regcache, PPC_VRSAVE_REGNUM, regs,
		       ppc_darwin_vrreg_offsets.vrsave_offset, 4);

      size = register_size (gdbarch, PPC_CR_REGNUM);
      if (size != 4)
	printf_unfiltered ("unexpected size for CR/XER/MQ #%d - ignored\n", size);
      ppc_collect_reg (regcache, PPC_CR_REGNUM, regs,
			ppc_darwin_reg_offsets.cr_offset, 4);
      ppc_collect_reg (regcache, PPC_XER_REGNUM, regs,
			ppc_darwin_reg_offsets.xer_offset + 4, 4);
      /* No MQ.  */
      return;
    }

  size = register_size (gdbarch, regno);
  if (size <= 0)
    {
      printf_unfiltered ("no size for register #%d - not set\n", regno);
      return;
    }

  /* This code makes the assumption that were storing an m32 machine into
     an m64 state.  */
  if (size != 4)
   printf_unfiltered ("unexpected size for REG #%d - ignored\n", size);

  /* Guess, we might set the PC most often.  */
  if (regno == PPC_PC_REGNUM)
    ppc_collect_reg (regcache, PPC_PC_REGNUM, regs,
		     ppc_darwin_reg_offsets.pc_offset + 4, 4);
  else if (regno >= PPC_R0_REGNUM && regno <= (PPC_R0_REGNUM + ppc_num_gprs))
    {
      size_t off = ppc_darwin_reg_offsets.r0_offset
		   + (regno-PPC_R0_REGNUM)*8 + 4;
      ppc_collect_reg (regcache, regno, regs, off, 4);
    }
  else if (regno == PPC_MSR_REGNUM)
    ppc_collect_reg (regcache, PPC_MSR_REGNUM, regs,
		     ppc_darwin_reg_offsets.ps_offset + 4, 4);
  else if (regno == PPC_LR_REGNUM)
    ppc_collect_reg (regcache, PPC_LR_REGNUM, regs,
		     ppc_darwin_reg_offsets.lr_offset + 4, 4);
  else if (regno == PPC_CTR_REGNUM)
    ppc_collect_reg (regcache, PPC_CTR_REGNUM, regs,
		     ppc_darwin_reg_offsets.ctr_offset + 4, 4);
  else if (regno == PPC_CR_REGNUM)
    ppc_collect_reg (regcache, PPC_CR_REGNUM, regs,
		     ppc_darwin_reg_offsets.cr_offset, 4);
  else if (regno == PPC_XER_REGNUM)
    ppc_collect_reg (regcache, PPC_XER_REGNUM, regs,
		     ppc_darwin_reg_offsets.xer_offset + 4, 4);
  else if (regno == PPC_VRSAVE_REGNUM)
    ppc_collect_reg (regcache, PPC_VRSAVE_REGNUM, regs,
		     ppc_darwin_vrreg_offsets.vrsave_offset, 4);
  else
    printf_unfiltered ("Unhandled register #%d - not set\n", regno);
  /* No MQ.  */
  return;
}

/* Return true if the PC of THIS_FRAME is in a signal trampoline which
   may have DWARF-2 CFI.

   On Darwin, signal trampolines have DWARF-2 CFI but it has only one FDE
   that covers only the indirect call to the user handler.
   Without this function, the frame is recognized as a normal frame which is
   not expected.  */

static int
darwin_dwarf_signal_frame_p (struct gdbarch *gdbarch,
			     frame_info_ptr this_frame)
{
/* TODO.  */
  return 0;
}

/* Force down to the next lower 16 byte boundary.  */

static CORE_ADDR
ppc_darwin_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
   return (addr & -16);
}

/* Sequence of bytes for breakpoint instruction.  */

static const unsigned char *
ppc_darwin_breakpoint_from_pc (struct gdbarch *gdbarch,
			       CORE_ADDR * addr, int *size)
{
  static unsigned char breakpoint[] = { 0x7f, 0xe0, 0x00, 0x08 };
  *size = 4;
  return breakpoint;
}

/* ====================== */

static void
ppc_darwin_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
//  const struct target_desc *tdesc = info.target_desc;
  struct tdesc_arch_data *tdesc_data = info.tdesc_data;

  tdep->wordsize = 4;

  tdep->ppc_toc_regnum = -1;
  if (tdesc_data)
    {
      const struct tdesc_feature *feature;
//      fprintf (stderr, "number of regs : %d\n", gdbarch_num_regs (gdbarch));
      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.power.darwin");
      if (feature != NULL)
	{
	  tdesc_numbered_register (feature, tdesc_data,
				   PPC_VRVALID_REGNUM, "vrvalid");
          ppc_darwin_vrreg_offsets.vrvalid_offset = offsetof (ppc_darwin_thread_vpstate_t, save_vrvalid);
//	  fprintf (stderr, "setting vrvalid etc\n");
	}
      else
	{
//	  fprintf (stderr, "did not find org.gnu.gdb.power.darwin\n");
          ppc_darwin_vrreg_offsets.vrvalid_offset = 0;

	}
    }
  else
    fprintf (stderr, "no tdesc info\n");

  /* Avoid initializing the register offsets more than once.  */
  if (ppc_darwin_reg_offsets.r0_offset == 0)
    {
      /* GP register set.  */
      ppc_darwin_reg_offsets.gpr_size = 4;
      ppc_darwin_reg_offsets.xr_size = 4;
      ppc_darwin_reg_offsets.r0_offset = offsetof (ppc_darwin_thread_state_64_t, gpregs);
      ppc_darwin_reg_offsets.lr_offset = offsetof (ppc_darwin_thread_state_64_t, lr);
      ppc_darwin_reg_offsets.cr_offset = offsetof (ppc_darwin_thread_state_64_t, cr);
      ppc_darwin_reg_offsets.xer_offset = offsetof (ppc_darwin_thread_state_64_t, xer);
      ppc_darwin_reg_offsets.ctr_offset = offsetof (ppc_darwin_thread_state_64_t, ctr);
      ppc_darwin_reg_offsets.pc_offset = offsetof (ppc_darwin_thread_state_64_t, srr0);
      ppc_darwin_reg_offsets.ps_offset = offsetof (ppc_darwin_thread_state_64_t, srr1);
      ppc_darwin_vrreg_offsets.vrsave_offset = offsetof (ppc_darwin_thread_state_64_t, vrsave);
      ppc_darwin_reg_offsets.mq_offset = -1 ;

      /* FP register set.  */
      ppc_darwin_reg_offsets.f0_offset = offsetof (ppc_darwin_thread_fpstate_t, fpregs);
      ppc_darwin_reg_offsets.fpscr_offset = offsetof (ppc_darwin_thread_fpstate_t, fpscr);
      ppc_darwin_reg_offsets.fpscr_size = 4;

      /* AltiVec register set.  */
      ppc_darwin_vrreg_offsets.vr0_offset = offsetof (ppc_darwin_thread_vpstate_t, save_vr);
      ppc_darwin_vrreg_offsets.vscr_offset = offsetof (ppc_darwin_thread_vpstate_t, save_vscr);
      /* vrsave done above in the GPRs.  */
    }

  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);
  set_gdbarch_long_double_format (gdbarch, floatformats_ibm_long_double);

/*  set_gdbarch_push_dummy_call (gdbarch, ppc_darwin_abi_push_dummy_call);
  set_gdbarch_return_value (gdbarch, ppc_darwin_abi_return_value);TBD*/

  set_gdbarch_frame_align (gdbarch, ppc_darwin_frame_align);
  set_gdbarch_frame_red_zone_size (gdbarch, 224);

/*  set_gdbarch_skip_prologue (gdbarch, ppc_skip_prologue);*/
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_decr_pc_after_break (gdbarch, 0);

  set_gdbarch_breakpoint_from_pc (gdbarch, ppc_darwin_breakpoint_from_pc);

  dwarf2_frame_set_signal_frame_p (gdbarch, darwin_dwarf_signal_frame_p);

  set_gdbarch_so_ops (gdbarch, &darwin_so_ops);
}


static enum gdb_osabi
ppc_mach_o_osabi_sniffer (bfd *abfd)
{
  if (!bfd_check_format (abfd, bfd_object))
    return GDB_OSABI_UNKNOWN;

  if (bfd_get_arch (abfd) == bfd_arch_powerpc
      && bfd_get_mach (abfd) == bfd_mach_ppc)
    return GDB_OSABI_DARWIN;

  return GDB_OSABI_UNKNOWN;
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_ppc_darwin_tdep;

void
_initialize_ppc_darwin_tdep ()
{
  gdbarch_register_osabi_sniffer (bfd_arch_unknown, bfd_target_mach_o_flavour,
                                  ppc_mach_o_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_powerpc, bfd_mach_ppc,
			  GDB_OSABI_DARWIN, ppc_darwin_init_abi);

  initialize_tdesc_powerpc_32_darwin ();
  initialize_tdesc_powerpc_altivec32_darwin ();
}
