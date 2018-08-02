// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2013 The Chromium OS Authors.
 */

#include <common.h>
#include <initcall.h>
#include <efi.h>
#include <dm.h>

/* defined in internal */
struct uclass *uclass_find(enum uclass_id key);

DECLARE_GLOBAL_DATA_PTR;

void check_serial_addr(const char *prefix) {
  struct uclass *uc = uclass_find(UCLASS_SERIAL);
  struct udevice *dev = NULL;

  debug("\e[33m%s: uc@%p\e[0m\n", prefix, uc);
  if(!uc) return;
  uclass_foreach_dev(dev, uc) {
	void *p = (void *)devfdt_get_addr(dev);
	debug("\e[33m%s: addr@%p\e[0m\n", prefix, p);
  }
  debug("\e[33m%s: end\e[0m\n", prefix);
}

int initcall_run_list(const init_fnc_t init_sequence[])
{
	const init_fnc_t *init_fnc_ptr;

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		unsigned long reloc_ofs = 0;
		int ret;

		if (gd->flags & GD_FLG_RELOC)
			reloc_ofs = gd->reloc_off;
#ifdef CONFIG_EFI_APP
		reloc_ofs = (unsigned long)image_base;
#endif
		debug("initcall: %p", (char *)*init_fnc_ptr - reloc_ofs);
		if (gd->flags & GD_FLG_RELOC)
			debug(" (relocated to %p)\n", (char *)*init_fnc_ptr);
		else
			debug("\n");
		check_serial_addr("before");
		ret = (*init_fnc_ptr)();
		check_serial_addr("after");
		if (ret) {
			printf("initcall sequence %p failed at call %p (err=%d)\n",
			       init_sequence,
			       (char *)*init_fnc_ptr - reloc_ofs, ret);
			return -1;
		}
	}
	return 0;
}
