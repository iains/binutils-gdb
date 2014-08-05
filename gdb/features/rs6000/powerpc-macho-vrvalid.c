/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: powerpc-macho-vrvalid.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_powerpc_powerpc_macho_vrvalid (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.powerpc.darwin");
  tdesc_create_reg (feature, "vrvalid", regnum++, 1, "vector", 32, "int");
  return regnum;
}
