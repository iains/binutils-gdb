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
#include "ppc64-darwin-tdep.h"

#include "solib.h"
#include "solib-darwin.h"
#include "dwarf2/frame.h"

#include "features/rs6000/powerpc-64-darwin.c"
#include "features/rs6000/powerpc-altivec64-darwin.c"

struct ppc_reg_offsets ppc64_darwin_reg_offsets;

struct regset ppc64_darwin_gregset =
{
  &ppc64_darwin_reg_offsets,
  ppc_supply_gregset,
  ppc_collect_gregset
};

struct regset ppc64_darwin_fpregset =
{
  &ppc64_darwin_reg_offsets,
  ppc_supply_fpregset,
  ppc_collect_fpregset
};

struct ppc64_darwin_vrreg_offset
{
  int vr0_offset;
  int vscr_offset;
  int vrsave_offset;
};

struct ppc64_darwin_vrreg_offset ppc64_darwin_vrreg_offsets;

struct regset ppc64_darwin_vrregset =
{
  &ppc64_darwin_vrreg_offsets,
  ppc_darwin_supply_vrregset,
  ppc_darwin_collect_vrregset
};
#if 0
static int
ppc64_vrreg_offset (struct gdbarch_tdep *tdep,
		  const struct ppc64_darwin_vrregset *offsets,
		  int regnum)
{
  if (regnum >= tdep->ppc_vr0_regnum
      && regnum < tdep->ppc_vr0_regnum + ppc_num_vrs)
    return offsets->vr0_offset + (regnum - tdep->ppc_vr0_regnum) * 16;

  if (regnum == PPC_VSCR_REGNUM)
    return offsets->vscr_offset;

  return -1;
}

/* Supply register REGNUM in the Altivec register set REGSET
   from the buffer specified by VRREGS and LEN to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.
   Nearly identical to the one in rs6000 - but we have VRSAVE with the
   GPRs.  */

void
ppc64_darwin_supply_vrregset (const struct regset *regset, struct regcache *regcache,
		     int regnum, const void *vrregs, size_t len)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep;
  const struct ppc_reg_offsets *offsets;
  size_t offset;
/*  unsigned long long t;*/
  gdb_byte *regs = (gdb_byte *)vrregs;

  if (!ppc_altivec_support_p (gdbarch))
    return;

  tdep = gdbarch_tdep (gdbarch);
  offsets = regset->descr;
  if (regnum == -1)
    {
      int i;

      for (i = tdep->ppc_vr0_regnum, offset = offsets->vr0_offset;
	   i < tdep->ppc_vr0_regnum + ppc_num_vrs;
	   i++, offset += 16)
        regcache_raw_supply (regcache, i, regs + offset);

      regcache_raw_supply (regcache, PPC_VSCR_REGNUM,
			   regs + offsets->vscr_offset);
      return;
    }

  offset = ppc_vrreg_offset (tdep, offsets, regnum);
/*  t = *(unsigned long long *)(regs + offset);
  printf_unfiltered ("Q reg= #%d vr0 = %d offset = %d, %16llx\n",
			   regnum, tdep->ppc_vr0_regnum, offset, t);*/

  if (offset >= 0)
    {
      if (regnum == PPC_VSCR_REGNUM)
	{
	  if (register_size (gdbarch, regnum) != 4)
	     printf_unfiltered ("unexpected size for VSCR #%d\n", register_size (gdbarch, regnum));
	}
      else if (register_size (gdbarch, regnum) != 16)
	printf_unfiltered ("unexpected size for VR%d #%d\n",
			   regnum-PPC_VR0_REGNUM, register_size (gdbarch, regnum));
      /* This might break... */
      regcache_raw_supply (regcache, regnum, regs + offset);
   }
}

/* Supply register REGNUM in the Altivec register set REGSET
   from the buffer specified by VRREGS and LEN to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.
   Nearly identical to the one in rs6000 - but we have VRSAVE with the
   GPRs.  */

void
ppc64_darwin_collect_vrregset (const struct regset *regset, struct regcache *regcache,
		     int regnum, const void *vrregs, size_t len)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep;
  const struct ppc_reg_offsets *offsets;
  size_t offset;
/*  unsigned long long t;*/
  gdb_byte *regs = (gdb_byte *)vrregs;

  if (!ppc_altivec_support_p (gdbarch))
    return;

  tdep = gdbarch_tdep (gdbarch);
  offsets = regset->descr;
  if (regnum == -1)
    {
      int i;

      for (i = tdep->ppc_vr0_regnum, offset = offsets->vr0_offset;
	   i < tdep->ppc_vr0_regnum + ppc_num_vrs;
	   i++, offset += 16)
        regcache_raw_collect (regcache, i, regs + offset);

      regcache_raw_collect (regcache, PPC_VSCR_REGNUM,
			   regs + offsets->vscr_offset);
      return;
    }

  offset = ppc_vrreg_offset (tdep, offsets, regnum);

  if (offset >= 0)
    {
      if (regnum == PPC_VSCR_REGNUM)
	{
	  if (register_size (gdbarch, regnum) != 4)
	     printf_unfiltered ("unexpected size for VSCR #%d\n", register_size (gdbarch, regnum));
	}
      else if (register_size (gdbarch, regnum) != 16)
	printf_unfiltered ("unexpected size for VR%d #%d\n",
			   regnum-PPC_VR0_REGNUM, register_size (gdbarch, regnum));
      /* This might break... */
      regcache_raw_collect (regcache, regnum, regs + offset);

/*  t = *(unsigned long long *)(regs + offset);
  printf_unfiltered ("P reg= #%d vr0 = %d offset = %d, %16llx\n",
			   regnum, tdep->ppc_vr0_regnum, offset, t);*/
   }
}
#endif

void
ppc64_darwin_supply_gpr64_from_state64 (struct regcache *regcache, int regno,
				      const void *gregs, size_t len)
{
  warning ("called ppc64 for reg %d\n", regno);
#if 0
  int size, i;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  gdb_byte *regs = (gdb_byte *) gregs;

  size = register_size (gdbarch, PPC_R0_REGNUM); /* size for r0-31, pc, ps, lr, ctr.  */
  if (size != 8)
    printf_unfiltered ("unexpected size for GPR #%d - ignored\n", size);

  if (regno == -1)
    {
      size_t off = ppc_darwin_reg_offsets.r0_offset;
      for (i = 0; i < ppc_num_gprs; ++i, off += 8)
	  regcache_raw_supply (regcache, PPC_R0_REGNUM+i, regs + off);

      regcache_raw_supply (regcache, PPC_PC_REGNUM,
				regs + ppc_darwin_reg_offsets.pc_offset);
      regcache_raw_supply (regcache, PPC_MSR_REGNUM,
				regs + ppc_darwin_reg_offsets.ps_offset);
      regcache_raw_supply (regcache, PPC_LR_REGNUM,
				regs + ppc_darwin_reg_offsets.lr_offset);
      regcache_raw_supply (regcache, PPC_CTR_REGNUM,
				regs + ppc_darwin_reg_offsets.ctr_offset);
      /* VSAVE is done here.  */
      regcache_raw_supply (regcache, PPC_VRSAVE_REGNUM,
				regs + ppc_darwin_reg_offsets.vrsave_offset);

      size = register_size (gdbarch, PPC_CR_REGNUM);
      if (size != 8)
	printf_unfiltered ("unexpected size for CR/XER/MQ #%d - ignored\n", size);
      regcache_raw_supply (regcache, PPC_CR_REGNUM,
				regs + ppc_darwin_reg_offsets.cr_offset);
      regcache_raw_supply (regcache, PPC_XER_REGNUM,
				regs + ppc_darwin_reg_offsets.xer_offset);
      /* No MQ.  */
      return;
    }

  size = register_size (gdbarch, regno);
  if (size <= 0)
    {
      printf_unfiltered ("no size for register #%d - not set\n", regno);
      return;
    }

  /* This code makes the assumption that were storing an m64 machine into
     an m64 state.  */
  if (size != 8)
   printf_unfiltered ("unexpected size for REG #%d - ignored\n", size);

  /* Guess, we might set the PC most often.  */
  if (regno == PPC_PC_REGNUM )
    regcache_raw_supply (regcache, PPC_PC_REGNUM,
				regs + ppc_darwin_reg_offsets.pc_offset);
  else if (regno >= PPC_R0_REGNUM && regno <= (PPC_R0_REGNUM + ppc_num_gprs))
    {
      size_t off = ppc_darwin_reg_offsets.r0_offset + (regno-PPC_R0_REGNUM)*8;
      regcache_raw_supply (regcache, regno, regs + off);
    }
  else if (regno == PPC_MSR_REGNUM )
    regcache_raw_supply (regcache, PPC_MSR_REGNUM,
				regs + ppc_darwin_reg_offsets.ps_offset);
  else if (regno == PPC_LR_REGNUM )
    regcache_raw_supply (regcache, PPC_LR_REGNUM,
				regs + ppc_darwin_reg_offsets.lr_offset);
  else if (regno == PPC_CTR_REGNUM )
    regcache_raw_supply (regcache, PPC_CTR_REGNUM,
				regs + ppc_darwin_reg_offsets.ctr_offset);
  else if (regno == PPC_CR_REGNUM )
    regcache_raw_supply (regcache, PPC_CR_REGNUM,
			  regs + ppc_darwin_reg_offsets.cr_offset);
  else if (regno == PPC_XER_REGNUM )
    regcache_raw_supply (regcache, PPC_XER_REGNUM,
			  regs + ppc_darwin_reg_offsets.xer_offset);
  else if (regno == PPC_VRSAVE_REGNUM )
    regcache_raw_supply (regcache, PPC_VRSAVE_REGNUM,
			  regs + ppc_darwin_reg_offsets.vrsave_offset);
  else
    printf_unfiltered ("Unhandled register #%d - not supplied\n", regno);
  /* No MQ.  */
#endif
  return;
}

void
ppc64_darwin_collect_gpr64_into_state64 (struct regcache *regcache, int regno,
				     const void *gregs, size_t len)
{
  warning ("oops called ppc64 for reg %d\n", regno);
#if 0
  int size, i;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  gdb_byte *regs = (gdb_byte *) gregs;

  size = register_size (gdbarch, PPC_R0_REGNUM); /* size for r0-31, pc, ps, lr, ctr.  */
  if (size != 8)
    printf_unfiltered ("unexpected size for GPR #%d - ignored\n", size);

  if (regno == -1)
    {
      size_t off = ppc_darwin_reg_offsets.r0_offset;
      for (i = 0; i < ppc_num_gprs; ++i, off += 8)
	  regcache_raw_collect (regcache, PPC_R0_REGNUM+i, regs + off);

      regcache_raw_collect (regcache, PPC_PC_REGNUM,
				regs + ppc_darwin_reg_offsets.pc_offset);
      regcache_raw_collect (regcache, PPC_MSR_REGNUM,
				regs + ppc_darwin_reg_offsets.ps_offset);
      regcache_raw_collect (regcache, PPC_LR_REGNUM,
				regs + ppc_darwin_reg_offsets.lr_offset);
      regcache_raw_collect (regcache, PPC_CTR_REGNUM,
				regs + ppc_darwin_reg_offsets.ctr_offset);
      /* VSAVE is done here.  */
      regcache_raw_collect (regcache, PPC_VRSAVE_REGNUM,
				regs + ppc_darwin_reg_offsets.vrsave_offset);

      size = register_size (gdbarch, PPC_CR_REGNUM);
      if (size != 8)
	printf_unfiltered ("unexpected size for CR/XER/MQ #%d - ignored\n", size);
      regcache_raw_collect (regcache, PPC_CR_REGNUM,
				regs + ppc_darwin_reg_offsets.cr_offset);
      regcache_raw_collect (regcache, PPC_XER_REGNUM,
				regs + ppc_darwin_reg_offsets.xer_offset);
      /* No MQ.  */
      return;
    }

  size = register_size (gdbarch, regno);
  if (size <= 0)
    {
      printf_unfiltered ("no size for register #%d - not set\n", regno);
      return;
    }

  /* This code makes the assumption that were storing an m64 machine into
     an m64 state.  */
  if (size != 8)
   printf_unfiltered ("unexpected size for REG #%d - ignored\n", size);

  /* Guess, we might set the PC most often.  */
  if (regno == PPC_PC_REGNUM )
    regcache_raw_collect (regcache, PPC_PC_REGNUM,
				regs + ppc_darwin_reg_offsets.pc_offset);
  else if (regno >= PPC_R0_REGNUM && regno <= (PPC_R0_REGNUM + ppc_num_gprs))
    {
      size_t off = ppc_darwin_reg_offsets.r0_offset + (regno-PPC_R0_REGNUM)*8;
      regcache_raw_collect (regcache, regno, regs + off);
    }
  else if (regno == PPC_MSR_REGNUM )
    regcache_raw_collect (regcache, PPC_MSR_REGNUM,
				regs + ppc_darwin_reg_offsets.ps_offset);
  else if (regno == PPC_LR_REGNUM )
    regcache_raw_collect (regcache, PPC_LR_REGNUM,
				regs + ppc_darwin_reg_offsets.lr_offset);
  else if (regno == PPC_CTR_REGNUM )
    regcache_raw_collect (regcache, PPC_CTR_REGNUM,
				regs + ppc_darwin_reg_offsets.ctr_offset);
  else if (regno == PPC_CR_REGNUM )
    regcache_raw_collect (regcache, PPC_CR_REGNUM,
			  regs + ppc_darwin_reg_offsets.cr_offset);
  else if (regno == PPC_XER_REGNUM )
    regcache_raw_collect (regcache, PPC_XER_REGNUM,
			  regs + ppc_darwin_reg_offsets.xer_offset);
  else if (regno == PPC_VRSAVE_REGNUM )
    regcache_raw_collect (regcache, PPC_VRSAVE_REGNUM,
			  regs + ppc_darwin_reg_offsets.vrsave_offset);
  else
    printf_unfiltered ("Unhandled register #%d - not set\n", regno);
  /* No MQ.  */
#endif
  return;
}

static int
ppc64_darwin_dwarf_signal_frame_p (struct gdbarch *gdbarch,
			     frame_info_ptr this_frame)
{
/* TODO.  */
  return 0;
}

/* Sequence of bytes for breakpoint instruction.  */

static const unsigned char *
ppc64_darwin_breakpoint_from_pc (struct gdbarch *gdbarch,
			       CORE_ADDR * addr, int *size)
{
  static unsigned char breakpoint[] = { 0x7f, 0xe0, 0x00, 0x08 };
  *size = 4;
  return breakpoint;
}

/* Force down to the next lower 16 byte boundary.  */

static CORE_ADDR
ppc64_darwin_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
   return (addr & -16);
}

/* ====================== */

static void
ppc_darwin_init_abi_64 (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  tdep->wordsize = 8;

  tdep->ppc_toc_regnum = -1;

  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);
  set_gdbarch_long_double_format (gdbarch, floatformats_ibm_long_double);

  /* Avoid initializing the register offsets more than once.  */
  if (ppc64_darwin_reg_offsets.r0_offset == 0)
    {
      /* GP register set.  */
      ppc64_darwin_reg_offsets.gpr_size = 8;
      ppc64_darwin_reg_offsets.xr_size = 8;
      ppc64_darwin_reg_offsets.r0_offset = offsetof (ppc_darwin_thread_state_64_t, gpregs);
      ppc64_darwin_reg_offsets.lr_offset = offsetof (ppc_darwin_thread_state_64_t, lr);
      ppc64_darwin_reg_offsets.cr_offset = offsetof (ppc_darwin_thread_state_64_t, cr);
      ppc64_darwin_reg_offsets.xer_offset = offsetof (ppc_darwin_thread_state_64_t, xer);
      ppc64_darwin_reg_offsets.ctr_offset = offsetof (ppc_darwin_thread_state_64_t, ctr);
      ppc64_darwin_reg_offsets.pc_offset = offsetof (ppc_darwin_thread_state_64_t, srr0);
      ppc64_darwin_reg_offsets.ps_offset = offsetof (ppc_darwin_thread_state_64_t, srr1);
      ppc64_darwin_vrreg_offsets.vrsave_offset = offsetof (ppc_darwin_thread_state_64_t, vrsave);
      ppc64_darwin_reg_offsets.mq_offset = -1 ;

      /* FP register set.  */
      ppc64_darwin_reg_offsets.f0_offset = offsetof (ppc_darwin_thread_fpstate_t, fpregs);
      ppc64_darwin_reg_offsets.fpscr_offset = offsetof (ppc_darwin_thread_fpstate_t, fpscr);
      ppc64_darwin_reg_offsets.fpscr_size = 4;

      /* AltiVec register set.  */
      ppc64_darwin_vrreg_offsets.vr0_offset = offsetof (ppc_darwin_thread_vpstate_t, save_vr);
      ppc64_darwin_vrreg_offsets.vscr_offset = offsetof (ppc_darwin_thread_vpstate_t, save_vscr);
      /* VR Valid not done... vrsave in the GPRs.  */
    }

/*  set_gdbarch_push_dummy_call (gdbarch, ppc_darwin_abi_push_dummy_call);
  set_gdbarch_return_value (gdbarch, ppc_darwin_abi_return_value);TBD*/

  set_gdbarch_frame_align (gdbarch, ppc64_darwin_frame_align);
  set_gdbarch_frame_red_zone_size (gdbarch, 288);

/*  set_gdbarch_skip_prologue (gdbarch, ppc_skip_prologue);*/
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_decr_pc_after_break (gdbarch, 0);

  set_gdbarch_breakpoint_from_pc (gdbarch, ppc64_darwin_breakpoint_from_pc);

  dwarf2_frame_set_signal_frame_p (gdbarch, ppc64_darwin_dwarf_signal_frame_p);

  set_gdbarch_so_ops (gdbarch, &darwin_so_ops);
}


static enum gdb_osabi
ppc64_mach_o_osabi_sniffer (bfd *abfd)
{
  if (!bfd_check_format (abfd, bfd_object))
    return GDB_OSABI_UNKNOWN;

  if (bfd_get_arch (abfd) == bfd_arch_powerpc
      && bfd_get_mach (abfd) == bfd_mach_ppc64)
    return GDB_OSABI_DARWIN;

  return GDB_OSABI_UNKNOWN;
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_ppc64_darwin_tdep;

void
_initialize_ppc64_darwin_tdep ()
{
  gdbarch_register_osabi_sniffer (bfd_arch_unknown, bfd_target_mach_o_flavour,
                                  ppc64_mach_o_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_powerpc, bfd_mach_ppc64,
			  GDB_OSABI_DARWIN, ppc_darwin_init_abi_64);

  initialize_tdesc_powerpc_64_darwin ();
  initialize_tdesc_powerpc_altivec64_darwin ();
}
