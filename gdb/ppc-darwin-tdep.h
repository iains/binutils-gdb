#ifndef __PPC_DARWIN_TDEP_H__
#define __PPC_DARWIN_TDEP_H__

#include "defs.h"
#include "frame.h"
#include "regcache.h"
#include "regset.h"
#include "target-descriptions.h"

extern const struct target_desc *tdesc_powerpc_32_darwin;
extern const struct target_desc *tdesc_powerpc_altivec32_darwin;

int darwin_dwarf_signal_frame_p (struct gdbarch *, struct frame_info *);

extern struct ppc_reg_offsets ppc_darwin_reg_offsets;
extern struct regset ppc_darwin_gregset;
extern struct regset ppc_darwin_fpregset;
extern struct regset ppc_darwin_vrregset;

extern int ppc_darwin_reg_is_in_gp_set (struct gdbarch *, int);
extern int ppc_darwin_reg_is_in_fp_set (struct gdbarch *, int);
extern int ppc_darwin_reg_is_in_vec_set (struct gdbarch *, int);

extern void ppc_darwin_supply_vrregset (const struct regset *, struct regcache *,
					int , const void *, size_t );

extern void ppc_darwin_collect_vrregset (const struct regset *, const struct regcache *,
					 int, void *, size_t);

extern void ppc_darwin_supply_gpr32_from_state64 (struct regcache *, int,
						  const void *, size_t);

extern void ppc_darwin_collect_gpr32_into_state64 (struct regcache *, int,
						   const void *, size_t);

#endif /* __PPC_DARWIN_TDEP_H__ */
