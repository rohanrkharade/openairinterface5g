/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

// CORESET#0 of 38.213 Table 13-0 (Rel-18): SSB on the 3 MHz raster (index 0 to 9) or at n100 GSCN 41638 (10 and 11)

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "openair2/LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "executables/softmodem-common.h"

softmodem_params_t *get_softmodem_params(void)
{
  return NULL;
}

static int failures = 0;

#define CHECK_EQ(a, b)                                                                     \
  do {                                                                                     \
    if ((a) != (b)) {                                                                      \
      printf("%s:%d: %s = %d, expected %d\n", __FILE__, __LINE__, #a, (int)(a), (int)(b)); \
      failures++;                                                                          \
    }                                                                                      \
  } while (0)

static NR_Type0_PDCCH_CSS_config_t get_type0(int coreset_zero, nr_ssb_raster_t raster)
{
  NR_MIB_t mib = {.subCarrierSpacingCommon = NR_MIB__subCarrierSpacingCommon_scs15or60,
                  .pdcch_ConfigSIB1 = {.controlResourceSetZero = coreset_zero, .searchSpaceZero = 2}};
  NR_Type0_PDCCH_CSS_config_t cfg = {0};
  // SSB index 0 in symbol 2, SSB period 20 ms, CORESET#0 at the SSB (offset 0 rows) or 2 RBs below it
  get_type0_PDCCH_CSS_config_parameters(&cfg, 0, &mib, 10, 0, 2, NR_SubcarrierSpacing_kHz15, FR1, 100, 52, 0, 2, 2, raster);
  return cfg;
}

static void check_row(int index, nr_ssb_raster_t raster, int rbs, int size, int symbols, int offset, bool non_int)
{
  NR_Type0_PDCCH_CSS_config_t cfg = get_type0(index, raster);
  CHECK_EQ(cfg.num_rbs, rbs);
  CHECK_EQ(cfg.coreset0_size, size);
  CHECK_EQ(cfg.num_symbols, symbols);
  CHECK_EQ(cfg.rb_offset, offset);
  CHECK_EQ(cfg.non_interleaved, non_int);
  CHECK_EQ(cfg.cset_start_rb, 2 - offset);
}

int main(void)
{
  logInit();
  // 3 MHz raster: 12 RBs, or 24 RBs punctured to 15
  check_row(0, NR_SSB_RASTER_3MHZ, 12, 12, 2, 0, false);
  check_row(1, NR_SSB_RASTER_3MHZ, 12, 12, 3, 0, false);
  check_row(2, NR_SSB_RASTER_3MHZ, 24, 15, 2, 0, false);
  check_row(5, NR_SSB_RASTER_3MHZ, 24, 15, 3, 2, false);
  check_row(6, NR_SSB_RASTER_3MHZ, 24, 15, 2, 0, true);
  check_row(9, NR_SSB_RASTER_3MHZ, 24, 15, 3, 2, true);
  // n100 GSCN 41638: 24 RBs punctured to 20 in a 5 MHz channel, interleaved
  check_row(10, NR_SSB_RASTER_GSCN_41638, 24, 20, 2, 0, false);
  check_row(11, NR_SSB_RASTER_GSCN_41638, 24, 20, 3, 0, false);
  // other SSBs: 38.213 Table 13-1, index 0 is 24 RBs, 2 symbols, offset 0, no puncturing
  check_row(0, NR_SSB_RASTER_DEFAULT, 24, 24, 2, 0, false);
  if (failures) {
    printf("%d failures\n", failures);
    return 1;
  }
  printf("All Type0-PDCCH CORESET tests passed.\n");
  return 0;
}
