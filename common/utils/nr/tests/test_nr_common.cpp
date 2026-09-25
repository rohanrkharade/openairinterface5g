/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <gtest/gtest.h>
extern "C" {
#include "nr_common.h"
#include "common/utils/LOG/log.h"
extern const nr_bandentry_t nr_bandtable[];
}

TEST(nr_common, nr_timer) {
  NR_timer_t timer;
  nr_timer_setup(&timer, 10, 1);
  nr_timer_start(&timer);
  EXPECT_TRUE(nr_timer_is_active(&timer));
  EXPECT_FALSE(nr_timer_expired(&timer));
  for (auto i = 0; i < 10; i++) {
    nr_timer_tick(&timer);
  }
  EXPECT_FALSE(nr_timer_is_active(&timer));
  EXPECT_TRUE(nr_timer_expired(&timer));
}

// SSB reference frequency (Hz) of a GSCN below 3 GHz, 38.101-1 Table 5.4.3.1-1
static uint64_t ssref_from_gscn(int gscn)
{
  const int M = 3 + 2 * (((gscn % 3) + 1) % 3 - 1); // GSCN = 3N + (M - 3) / 2, M in {1, 3, 5}
  const int N = (gscn - (M - 3) / 2) / 3;
  return (uint64_t)N * 1200000 + M * 50000;
}

TEST(nr_band_n100, band_table)
{
  const nr_bandentry_t *b = &nr_bandtable[get_nr_table_idx(100, 0)];
  EXPECT_EQ(b->band, 100);
  EXPECT_EQ(b->ul_min, 874400);
  EXPECT_EQ(b->ul_max, 880000);
  EXPECT_EQ(b->dl_min, 919400);
  EXPECT_EQ(b->dl_max, 925000);
  EXPECT_EQ(b->N_OFFs_UL, 174880);
  EXPECT_EQ(b->N_OFFs_DL, 183880);
  EXPECT_EQ(get_frame_type(100, 0), FDD);
  EXPECT_EQ(get_delta_duplex(100, 0), -45000);
  EXPECT_EQ(get_freq_range_from_band(100), FR1);
}

TEST(nr_band_n101, band_table)
{
  for (int scs = 0; scs < 2; scs++) {
    const nr_bandentry_t *b = &nr_bandtable[get_nr_table_idx(101, scs)];
    EXPECT_EQ(b->band, 101);
    EXPECT_EQ(b->dl_min, 1900000);
    EXPECT_EQ(b->dl_max, 1910000);
    EXPECT_EQ(b->N_OFFs_DL, 380000);
    EXPECT_EQ(get_frame_type(101, scs), TDD);
  }
  EXPECT_EQ(get_freq_range_from_band(101), FR1);
}

TEST(nr_band_n100, arfcn_conversion)
{
  // band edges, 38.101-1 Table 5.4.2.3-1
  EXPECT_EQ(from_nrarfcn(100, 0, 183880), 919400000);
  EXPECT_EQ(from_nrarfcn(100, 0, 185000), 925000000);
  EXPECT_EQ(from_nrarfcn(100, 0, 174880), 874400000);
  EXPECT_EQ(from_nrarfcn(100, 0, 176000), 880000000);
  for (uint32_t arfcn : {174880u, 175500u, 176000u, 183880u, 184370u, 185000u})
    EXPECT_EQ(to_nrarfcn(from_nrarfcn(100, 0, arfcn)), arfcn);
}

TEST(nr_band_n101, arfcn_conversion)
{
  EXPECT_EQ(from_nrarfcn(101, 0, 380000), 1900000000);
  EXPECT_EQ(from_nrarfcn(101, 1, 382000), 1910000000);
  for (uint32_t arfcn : {380000u, 380450u, 380930u, 382000u})
    EXPECT_EQ(to_nrarfcn(from_nrarfcn(101, 0, arfcn)), arfcn);
}

TEST(nr_band_n100, ssb_raster)
{
  // 38.101-1 Table 5.4.3.3-1: n100, 15 kHz, GSCN 2303 - <1> - 2307
  for (int gscn = 2303; gscn <= 2307; gscn++)
    check_ssb_raster(ssref_from_gscn(gscn), 100, 0);
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(2302), 100, 0), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(2308), 100, 0), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(2305), 100, 1), "Couldn't find band");
  // absoluteFrequencySSB of gnb.sa.band100.25prb.usrpb205mini.conf
  check_ssb_raster(from_nrarfcn(100, 0, 184370), 100, 0);
}

TEST(nr_band_n101, ssb_raster)
{
  // 38.101-1 Table 5.4.3.3-1: n101, 15 kHz 4754 - <1> - 4768, 30 kHz 4760 - <1> - 4764
  for (int gscn = 4754; gscn <= 4768; gscn++)
    check_ssb_raster(ssref_from_gscn(gscn), 101, 0);
  for (int gscn = 4760; gscn <= 4764; gscn++)
    check_ssb_raster(ssref_from_gscn(gscn), 101, 1);
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(4753), 101, 0), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(4769), 101, 0), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(4759), 101, 1), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(4765), 101, 1), "does not belong to GSCN range");
  // absoluteFrequencySSB of the band101 usrpb205mini configs
  check_ssb_raster(from_nrarfcn(101, 0, 380450), 101, 0);
  check_ssb_raster(from_nrarfcn(101, 1, 380910), 101, 1);
}

TEST(nr_band_n100, ssb_case)
{
  EXPECT_EQ(set_ssb_case(0, 100), 0); // case A
  EXPECT_EQ(set_ssb_case(0, 101), 0); // case A
  EXPECT_EQ(set_ssb_case(1, 101), 2); // case C
}

TEST(nr_band_n100, carrier_within_band)
{
  // gnb.sa.band100.25prb.usrpb205mini.conf: 5 MHz, 15 kHz, DL pointA 183950, UL pointA 174950
  EXPECT_TRUE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 183950), 0, 25, false, true));
  EXPECT_TRUE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 174950), 0, 25, true, true));
  // 10 MHz does not fit into the 5.6 MHz of n100
  EXPECT_FALSE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 183880), 0, 52, false, false));
  // RBs start at the lower band edge: RBs fit, guard band does not
  EXPECT_TRUE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 183880), 0, 25, false, false));
  EXPECT_FALSE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 183880), 0, 25, false, true));
  // RBs above the upper band edge
  EXPECT_FALSE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 184200), 0, 25, false, false));
  // offsetToCarrier moves the carrier up
  EXPECT_FALSE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 183950), 5, 25, false, false));
  // DL frequencies are not inside the UL band
  EXPECT_FALSE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 183950), 0, 25, true, false));
}

TEST(nr_band_n101, carrier_within_band)
{
  // band101 usrpb205mini configs: 5 MHz, 15 kHz and 10 MHz, 30 kHz inside the band including guard bands
  EXPECT_TRUE(nr_carrier_within_band(101, 0, from_nrarfcn(101, 0, 380090), 0, 25, false, true));
  EXPECT_TRUE(nr_carrier_within_band(101, 1, from_nrarfcn(101, 1, 380136), 0, 24, false, true));
  EXPECT_TRUE(nr_carrier_within_band(101, 1, from_nrarfcn(101, 1, 380136), 0, 24, true, true));
  // pointA at the band edge: RBs fit, guard bands do not
  EXPECT_TRUE(nr_carrier_within_band(101, 0, from_nrarfcn(101, 0, 380000), 0, 25, false, false));
  EXPECT_FALSE(nr_carrier_within_band(101, 0, from_nrarfcn(101, 0, 380000), 0, 25, false, true));
  EXPECT_TRUE(nr_carrier_within_band(101, 1, from_nrarfcn(101, 1, 380000), 0, 24, false, false));
  EXPECT_FALSE(nr_carrier_within_band(101, 1, from_nrarfcn(101, 1, 380000), 0, 24, false, true));
  // 20 MHz does not fit into the 10 MHz of n101
  EXPECT_FALSE(nr_carrier_within_band(101, 1, from_nrarfcn(101, 1, 380000), 0, 51, false, false));
}

TEST(nr_band_n101, gscn_scan)
{
  // UE scanning a 10 MHz, 30 kHz carrier centered at 1905 MHz finds the whole 30 kHz raster of n101
  nr_gscn_info_t info[MAX_GSCN_BAND];
  const int n = get_scan_ssb_first_sc(1905e6, 24, 101, 1, info);
  ASSERT_GT(n, 0);
  for (int i = 0; i < n; i++) {
    EXPECT_GE(info[i].gscn, 4760);
    EXPECT_LE(info[i].gscn, 4764);
    EXPECT_EQ(info[i].ssRef, ssref_from_gscn(info[i].gscn));
  }
}

int main(int argc, char **argv)
{
  logInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
