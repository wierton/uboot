// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, <wd@denx.de>
 */

#include <common.h>
#include <asm/cacheops.h>
#ifdef CONFIG_MIPS_L2_CACHE
#include <asm/cm.h>
#endif
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/system.h>

DECLARE_GLOBAL_DATA_PTR;

static void probe_l2(void)
{
#ifdef CONFIG_MIPS_L2_CACHE
	unsigned long conf2, sl;
	bool l2c = false;

	if (!(read_c0_config1() & MIPS_CONF_M))
		return;

	conf2 = read_c0_config2();

	if (__mips_isa_rev >= 6) {
		l2c = conf2 & MIPS_CONF_M;
		if (l2c)
			l2c = read_c0_config3() & MIPS_CONF_M;
		if (l2c)
			l2c = read_c0_config4() & MIPS_CONF_M;
		if (l2c)
			l2c = read_c0_config5() & MIPS_CONF5_L2C;
	}

	if (l2c && config_enabled(CONFIG_MIPS_CM)) {
		gd->arch.l2_line_size = mips_cm_l2_line_size();
	} else if (l2c) {
		/* We don't know how to retrieve L2 config on this system */
		BUG();
	} else {
		sl = (conf2 & MIPS_CONF2_SL) >> MIPS_CONF2_SL_SHF;
		gd->arch.l2_line_size = sl ? (2 << sl) : 0;
	}
#endif
}

void mips_cache_probe(void)
{
#ifdef CONFIG_SYS_CACHE_SIZE_AUTO
	unsigned long conf1, il, dl;

	conf1 = read_c0_config1();

	il = (conf1 & MIPS_CONF1_IL) >> MIPS_CONF1_IL_SHF;
	dl = (conf1 & MIPS_CONF1_DL) >> MIPS_CONF1_DL_SHF;

	gd->arch.l1i_line_size = il ? (2 << il) : 0;
	gd->arch.l1d_line_size = dl ? (2 << dl) : 0;
#endif
	probe_l2();
}

static inline unsigned long icache_line_size(void)
{
#ifdef CONFIG_SYS_CACHE_SIZE_AUTO
	return gd->arch.l1i_line_size;
#else
	return CONFIG_SYS_ICACHE_LINE_SIZE;
#endif
}

static inline unsigned long dcache_line_size(void)
{
#ifdef CONFIG_SYS_CACHE_SIZE_AUTO
	return gd->arch.l1d_line_size;
#else
	return CONFIG_SYS_DCACHE_LINE_SIZE;
#endif
}

static inline unsigned long scache_line_size(void)
{
#ifdef CONFIG_MIPS_L2_CACHE
	return gd->arch.l2_line_size;
#else
	return 0;
#endif
}

#define cache_loop(start, end, lsize, ops...) do {			\
	const void *addr = (const void *)(start & ~(lsize - 1));	\
	const void *aend = (const void *)((end - 1) & ~(lsize - 1));	\
	const unsigned int cache_ops[] = { ops };			\
	unsigned int i;							\
									\
	if (!lsize)							\
		break;							\
									\
	for (; addr <= aend; addr += lsize) {				\
		for (i = 0; i < ARRAY_SIZE(cache_ops); i++)		\
			mips_cache(cache_ops[i], addr);			\
	}								\
} while (0)

//                               set  way  linesize dup
volatile uint8_t cache_flush_array[256 * 4 * 8 * 64 + 4];

void flush_cache(ulong start_addr, ulong size)
{
  debug("mips flush cache start...\n");

	unsigned long ilsize = icache_line_size();
	unsigned long dlsize = dcache_line_size();
	unsigned long slsize = scache_line_size();

	/* aend will be miscalculated when size is zero, so we return here */
	if (size == 0)
		return;

	if ((ilsize == dlsize) && !slsize) {
		/* flush I-cache & D-cache simultaneously */
		cache_loop(start_addr, start_addr + size, ilsize,
			   HIT_WRITEBACK_INV_D, HIT_INVALIDATE_I);
		goto ops_done;
	}

	/* flush D-cache */
	cache_loop(start_addr, start_addr + size, dlsize, HIT_WRITEBACK_INV_D);

	/* flush L2 cache */
	cache_loop(start_addr, start_addr + size, slsize, HIT_WRITEBACK_INV_SD);

	/* flush I-cache */
	cache_loop(start_addr, start_addr + size, ilsize, HIT_INVALIDATE_I);

ops_done:
	/* ensure cache ops complete before any further memory accesses */
	sync();

	/* ensure the pipeline doesn't contain now-invalid instructions */
	instruction_hazard_barrier();

#ifdef _CONFIG_MACH_NJUOOP

#warning "should not use this flush cache function"

  const size_t cache_size = ARRAY_SIZE(cache_flush_array);
  uint32_t *p = (void *)&cache_flush_array[cache_size - 4];
  *p = 0x03e00008;

  // clear dcache
  for(int i = 0; i < cache_size - 4; i++)
	cache_flush_array[i] = 0;

  // clear icache
  ((void (*)(void))(void *)&cache_flush_array)();

#endif

  debug("mips flush cache end...\n");
}

void flush_dcache_range(ulong start_addr, ulong stop)
{
	unsigned long lsize = dcache_line_size();
	unsigned long slsize = scache_line_size();

	/* aend will be miscalculated when size is zero, so we return here */
	if (start_addr == stop)
		return;

	cache_loop(start_addr, stop, lsize, HIT_WRITEBACK_INV_D);

	/* flush L2 cache */
	cache_loop(start_addr, stop, slsize, HIT_WRITEBACK_INV_SD);

	/* ensure cache ops complete before any further memory accesses */
	sync();
}

void invalidate_dcache_range(ulong start_addr, ulong stop)
{
	unsigned long lsize = dcache_line_size();
	unsigned long slsize = scache_line_size();

	/* aend will be miscalculated when size is zero, so we return here */
	if (start_addr == stop)
		return;

	/* invalidate L2 cache */
	cache_loop(start_addr, stop, slsize, HIT_INVALIDATE_SD);

	cache_loop(start_addr, stop, lsize, HIT_INVALIDATE_D);

	/* ensure cache ops complete before any further memory accesses */
	sync();
}
