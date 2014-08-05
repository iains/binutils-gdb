#ifndef __PPC64_DARWIN_TDEP_H__
#define __PPC64_DARWIN_TDEP_H__

#include "defs.h"
#include "frame.h"
#include "regcache.h"
#include "regset.h"
#include "target-descriptions.h"

extern const struct target_desc *tdesc_powerpc_64_darwin;
extern const struct target_desc *tdesc_powerpc_altivec64_darwin;

extern struct ppc_reg_offsets ppc64_darwin_reg_offsets;
extern struct regset ppc64_darwin_gregset;
extern struct regset ppc64_darwin_fpregset;
extern struct regset ppc64_darwin_vrregset;

extern void ppc64_darwin_supply_gpr64_from_state64 (struct regcache *, int,
						  const void *, size_t);

extern void ppc64_darwin_collect_gpr64_into_state64 (struct regcache *, int,
						   const void *, size_t);

#endif /* __PPC64_DARWIN_TDEP_H__ */
