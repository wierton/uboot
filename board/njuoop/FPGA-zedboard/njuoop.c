#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

/* initialize the DDR Controller and PHY */
int dram_init(void)
{
    /* MIG IP block is smart and doesn't need SW
     * to do any init */
    gd->ram_size = CONFIG_SYS_SDRAM_SIZE;   /* in bytes */

    return 0;
}

#ifdef CONFIG_RESET_PHY_R
void reset_phy(void)
{

}
#endif
