#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "target.h"
#include "symfile.h"
#include "symtab.h"
#include "objfiles.h"
#include "regcache.h"
#include "regset.h"
#include "gdbarch.h"
#include "arch-utils.h"

#include "ppc-tdep.h"
#include "ppc-darwin-tdep.h"
#include "ppc-darwin-thread-state.h"

#include "darwin-nat.h"

#ifdef BFD64
#include "ppc64-darwin-tdep.h"
#endif

struct ppc_darwin_nat_target final : public darwin_nat_target
{
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  const struct target_desc *read_description ()  override;
};

const struct target_desc *
ppc_darwin_nat_target::read_description ()
{
#ifdef BFD64
#ifdef __LP64__
     return tdesc_powerpc_altivec64_darwin;
#endif
#endif
 return tdesc_powerpc_altivec32_darwin;
}


static struct ppc_darwin_nat_target darwin_target;

void
ppc_darwin_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  thread_t current_thread = regcache->ptid ().tid ();
  struct gdbarch *gdbarch = regcache->arch ();
  bool isppc64 = gdbarch_ptr_bit (gdbarch) == 64;

  if ((regno == -1) || ppc_darwin_reg_is_in_gp_set (gdbarch, regno))
    {
      ppc_darwin_thread_state_64_t gp_regs;
      unsigned int gp_count = PPC_DARWIN_THREAD_STATE_64_COUNT;
      kern_return_t ret = thread_get_state (current_thread,
					    PPC_DARWIN_THREAD_STATE_64,
					    (thread_state_t) & gp_regs,
					    &gp_count);
      if (ret != KERN_SUCCESS)
	{
	  printf_unfiltered (_("Error calling thread_get_state for "
				"GP registers for thread 0x%ulx"),
				 current_thread);
	  MACH_CHECK_ERROR (ret);
	}

#ifdef BFD64
      if (isppc64)
	ppc64_darwin_supply_gpr64_from_state64 (regcache, regno,
						&gp_regs, sizeof (gp_regs));
      else
#endif
	ppc_darwin_supply_gpr32_from_state64 (regcache, regno,
					    &gp_regs, sizeof (gp_regs));
   }

  if ((regno == -1) || ppc_darwin_reg_is_in_fp_set(gdbarch, regno))
    {
      ppc_darwin_thread_fpstate_t fp_regs;
      unsigned int fp_count = PPC_DARWIN_THREAD_FPSTATE_COUNT;
      kern_return_t ret = thread_get_state (current_thread,
					    PPC_DARWIN_THREAD_FPSTATE,
					    (thread_state_t) & fp_regs,
					    &fp_count);
      if (ret != KERN_SUCCESS)
	{
	  printf_unfiltered (_("Error calling thread_get_state for "
				"FP registers for thread 0x%ulx"),
				 current_thread);
	  MACH_CHECK_ERROR (ret);
	}

#ifdef BFD64
      if (isppc64)
	ppc_supply_fpregset (&ppc64_darwin_fpregset, regcache,
			       regno, &fp_regs, sizeof (fp_regs));
      else
#endif
	ppc_supply_fpregset (&ppc_darwin_fpregset, regcache,
			   regno, &fp_regs, sizeof (fp_regs));
    }

  if ((regno == -1) || ppc_darwin_reg_is_in_vec_set (gdbarch, regno))
    {
      ppc_darwin_thread_vpstate_t vp_regs;
      unsigned int vp_count = PPC_DARWIN_THREAD_VPSTATE_COUNT;
      kern_return_t ret = thread_get_state (current_thread,
					    PPC_DARWIN_THREAD_VPSTATE,
					    (thread_state_t) & vp_regs,
					    &vp_count);
      if (ret != KERN_SUCCESS)
	{
	  printf_unfiltered (_("Error calling thread_get_state for "
				"Altivec registers for thread 0x%ulx"),
				 current_thread);
	  MACH_CHECK_ERROR (ret);
	}

#ifdef BFD64
      if (isppc64)
	ppc_darwin_supply_vrregset (&ppc64_darwin_vrregset, regcache,
				  regno, &vp_regs, sizeof (vp_regs));
      else
#endif
	ppc_darwin_supply_vrregset (&ppc_darwin_vrregset, regcache,
				  regno, &vp_regs, sizeof (vp_regs));
    }
}

void
ppc_darwin_nat_target::store_registers (struct regcache *regcache, int regno)
{
  thread_t current_thread = regcache->ptid ().tid ();
  struct gdbarch *gdbarch = regcache->arch ();
  bool isppc64 = gdbarch_ptr_bit (gdbarch) == 64;

  if ((regno == -1) || ppc_darwin_reg_is_in_gp_set (gdbarch, regno))
    {
      ppc_darwin_thread_state_64_t gp_regs;
      unsigned int gp_count = PPC_DARWIN_THREAD_STATE_64_COUNT;
      kern_return_t ret = thread_get_state (current_thread,
					    PPC_DARWIN_THREAD_STATE_64,
					    (thread_state_t) & gp_regs,
					    &gp_count);
      if (ret != KERN_SUCCESS)
	{
	  printf_unfiltered (_("Error calling thread_get_state for "
				"GP registers for thread 0x%ulx"),
				 current_thread);
	  MACH_CHECK_ERROR (ret);
	}

#ifdef BFD64
      if (isppc64)
        ppc64_darwin_collect_gpr64_into_state64 (regcache, regno,
					     &gp_regs, sizeof (gp_regs));
      else
#endif
        ppc_darwin_collect_gpr32_into_state64 (regcache, regno,
					     &gp_regs, sizeof (gp_regs));
      ret = thread_set_state (current_thread, PPC_DARWIN_THREAD_STATE_64,
                              (thread_state_t) & gp_regs,
                              PPC_DARWIN_THREAD_STATE_64_COUNT);
      MACH_CHECK_ERROR (ret);
    }

  if ((regno == -1) || ppc_darwin_reg_is_in_fp_set (gdbarch, regno))
    {
      ppc_darwin_thread_fpstate_t fp_regs;
      unsigned int fp_count = PPC_DARWIN_THREAD_FPSTATE_COUNT;
      kern_return_t ret = thread_get_state (current_thread,
					    PPC_DARWIN_THREAD_FPSTATE,
					    (thread_state_t) & fp_regs,
					    &fp_count);
      if (ret != KERN_SUCCESS)
	{
	  printf_unfiltered (_("Error calling thread_get_state for "
				"FP registers for thread 0x%ulx"),
				 current_thread);
	  MACH_CHECK_ERROR (ret);
	}

#ifdef BFD64
      if (isppc64)
	ppc_collect_fpregset (&ppc64_darwin_fpregset, regcache,
			      regno, &fp_regs, sizeof (fp_regs));
      else
#endif
	ppc_collect_fpregset (&ppc_darwin_fpregset, regcache,
			      regno, &fp_regs, sizeof (fp_regs));

      ret = thread_set_state (current_thread, PPC_DARWIN_THREAD_FPSTATE,
                              (thread_state_t) & fp_regs,
                              PPC_DARWIN_THREAD_FPSTATE_COUNT);
      MACH_CHECK_ERROR (ret);
    }

  if ((regno == -1) || ppc_darwin_reg_is_in_vec_set (gdbarch, regno))
    {
      ppc_darwin_thread_vpstate_t vp_regs;
      unsigned int vp_count = PPC_DARWIN_THREAD_VPSTATE_COUNT;
      kern_return_t ret = thread_get_state
        (current_thread, PPC_DARWIN_THREAD_VPSTATE, (thread_state_t) &vp_regs,
         &vp_count);
      if (ret != KERN_SUCCESS)
	{
	  printf_unfiltered (_("Error calling thread_get_state for "
				"Altivec registers for thread 0x%ulx"),
				 current_thread);
	  MACH_CHECK_ERROR (ret);
	}
#ifdef BFD64
      if (isppc64)
        ppc_darwin_collect_vrregset (&ppc64_darwin_vrregset, regcache,
				  regno, &vp_regs, sizeof (vp_regs));
      else
#endif
        ppc_darwin_collect_vrregset (&ppc_darwin_vrregset, regcache,
				  regno, &vp_regs, sizeof (vp_regs));
      /* Copy back the vrvalid state.  */
      ppc_darwin_supply_vrregset (&ppc_darwin_vrregset, regcache,
				  PPC_VRVALID_REGNUM, &vp_regs, sizeof (vp_regs));
      ret = thread_set_state (current_thread, PPC_DARWIN_THREAD_VPSTATE,
                              (thread_state_t) &vp_regs,
                              PPC_DARWIN_THREAD_VPSTATE_COUNT);

      MACH_CHECK_ERROR (ret);
    }
}

void
darwin_check_osabi (darwin_inferior *inf, thread_t thread)
{
  if (gdbarch_osabi (target_gdbarch ()) == GDB_OSABI_UNKNOWN)
    {
      /* Attaching to a process.  Let's figure out what kind it is.  */
      ppc_thread_state_t gp_regs;
      unsigned int gp_count = PPC_THREAD_STATE_COUNT;
      kern_return_t ret;

//      printf_unfiltered (_("checking OSabi = count in = %d\n"), gp_count);
      ret = thread_get_state (thread, PPC_THREAD_STATE,
			      (thread_state_t) &gp_regs, &gp_count);
      if (ret != KERN_SUCCESS)
	{
	  MACH_CHECK_ERROR (ret);
	  return;
	}

//      printf_unfiltered (_("checking OSabi = count out = %d\n"), gp_count);

      gdbarch_info info;
      gdbarch_info_fill (&info);
      info.byte_order = gdbarch_byte_order (target_gdbarch ());
      info.osabi = GDB_OSABI_DARWIN;
      if (gp_count == PPC_DARWIN_THREAD_STATE_64_COUNT)
	info.bfd_arch_info = bfd_lookup_arch (bfd_arch_powerpc, bfd_mach_ppc);
      else
	{
	  info.bfd_arch_info = bfd_lookup_arch (bfd_arch_powerpc, bfd_mach_ppc);
	  printf_unfiltered ("we do not implement 32b thread state yet\n");
	}

      /*   TODO ppc64 ... */

      gdbarch_update_p (info);
    }
  else
    printf_unfiltered ("Darwin ABI already known?\n");
}

/* We need to:
   (a) determine if we are trying to single step the return from
       a signal handler.
   (b) if we are, find the context that will be restored when we exit
       the handler and set the single-step bit in the MSR of that
       context ...
   */
static int
ppc_darwin_sstep_at_sigreturn (ppc_darwin_thread_state_64_t *gp_regs)
{
 /* TODO */
  return 0;
}

#define PPC_SINGLE_STEP_ENABLE (1 << 10)
#define PPC_SINGLE_STEP_BRANCH (1 <<  9)

void
darwin_set_sstep (thread_t thread, int enable)
{
  unsigned long long bit;
  ppc_darwin_thread_state_64_t gp_regs;
  unsigned int gp_count = PPC_DARWIN_THREAD_STATE_64_COUNT;
  kern_return_t ret;

  if (enable && ppc_darwin_sstep_at_sigreturn (&gp_regs))
    return;

  ret = thread_get_state (thread, PPC_DARWIN_THREAD_STATE_64,
			  (thread_state_t) &gp_regs, &gp_count);
  if (ret != KERN_SUCCESS)
    {
      printf_unfiltered (_("Error calling thread_get_state for "
			   " (sstep) thread 0x%ulx"), thread);
      MACH_CHECK_ERROR (ret);
    }

  bit = (enable ? PPC_SINGLE_STEP_ENABLE : 0);
  if ((gp_regs.srr1 & PPC_SINGLE_STEP_ENABLE) == bit)
    return ;

  gp_regs.srr1 &= ~PPC_SINGLE_STEP_ENABLE;
  gp_regs.srr1 |= bit;

  ret = thread_set_state (thread, PPC_DARWIN_THREAD_STATE_64,
			  (thread_state_t) &gp_regs,
			  PPC_DARWIN_THREAD_STATE_64_COUNT);
  if (ret != KERN_SUCCESS)
    {
      printf_unfiltered (_("Error calling thread_set_state for "
			   " (sstep) thread 0x%ulx"), thread);
      MACH_CHECK_ERROR (ret);
    }
}

void _initialize_ppc_darwin_nat ();
void
_initialize_ppc_darwin_nat ()
{
  add_inf_child_target (&darwin_target);
}
