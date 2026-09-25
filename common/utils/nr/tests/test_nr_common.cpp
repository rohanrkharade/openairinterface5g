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
    check_ssb_raster(ssref_from_gscn(gscn), 100, 0, false);
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(2302), 100, 0, false), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(2308), 100, 0, false), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(2305), 100, 1, false), "Couldn't find band");
  // absoluteFrequencySSB of gnb.sa.band100.25prb.usrpb205mini.conf
  check_ssb_raster(from_nrarfcn(100, 0, 184370), 100, 0, false);
}

TEST(nr_band_n101, ssb_raster)
{
  // 38.101-1 Table 5.4.3.3-1: n101, 15 kHz 4754 - <1> - 4768, 30 kHz 4760 - <1> - 4764
  for (int gscn = 4754; gscn <= 4768; gscn++)
    check_ssb_raster(ssref_from_gscn(gscn), 101, 0, false);
  for (int gscn = 4760; gscn <= 4764; gscn++)
    check_ssb_raster(ssref_from_gscn(gscn), 101, 1, false);
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(4753), 101, 0, false), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(4769), 101, 0, false), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(4759), 101, 1, false), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(4765), 101, 1, false), "does not belong to GSCN range");
  // absoluteFrequencySSB of the band101 usrpb205mini configs
  check_ssb_raster(from_nrarfcn(101, 0, 380450), 101, 0, false);
  check_ssb_raster(from_nrarfcn(101, 1, 380910), 101, 1, false);
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

// SSB reference frequency (Hz) of a GSCN of the 3 MHz raster, 38.101-1 Table 5.4.3.1-2
static uint64_t ssref_3mhz_from_gscn(int gscn)
{
  const int n = gscn - 26638; // 3N + (M - 3) / 2
  const int M = 3 + 2 * (((n % 3) + 1) % 3 - 1);
  const int N = (n - (M - 3) / 2) / 3;
  return (uint64_t)N * 600000 + M * 50000 + 300000;
}

TEST(nr_3mhz, bands)
{
  // 38.101-1 Table 5.3.5-1 (Rel.18)
  for (int band : {26, 28, 31, 72, 85, 100, 106})
    EXPECT_TRUE(nr_band_supports_3mhz(band)) << "band " << band;
  for (int band : {1, 8, 78, 101})
    EXPECT_FALSE(nr_band_supports_3mhz(band)) << "band " << band;
}

TEST(nr_3mhz, channel_bandwidth)
{
  // 38.101-1 Table 5.3.2-1 (Rel.18): 3 MHz is 15 PRB, 15 kHz only
  EXPECT_TRUE(nr_is_3mhz_carrier(0, FR1, 15));
  EXPECT_FALSE(nr_is_3mhz_carrier(1, FR1, 15));
  EXPECT_FALSE(nr_is_3mhz_carrier(0, FR1, 25));
  EXPECT_FALSE(nr_is_3mhz_carrier(2, FR2, 15));
  EXPECT_EQ(get_nr_channel_bw_mhz(0, FR1, 15), 3);
  EXPECT_EQ(get_nr_channel_bw_mhz(0, FR1, 25), 5);
  EXPECT_EQ(get_nr_channel_bw_mhz(1, FR1, 24), 10);
  EXPECT_EQ(get_nr_channel_bw_mhz(1, FR1, 273), 100);
  // the bandwidth index used for the UE capability and power tables is unchanged
  EXPECT_EQ(get_supported_band_index(0, FR1, 15), -1);
  EXPECT_EQ(get_supported_band_index(0, FR1, 25), 0);
  EXPECT_DEATH(get_nr_channel_bw_mhz(1, FR1, 15), "not a supported channel bandwidth");
}

TEST(nr_3mhz, sample_rate)
{
  double sample_rate, tx_bw, rx_bw;
  get_samplerate_and_bw(0, 15, 0, &sample_rate, &tx_bw, &rx_bw);
  EXPECT_EQ(sample_rate, 7.68e6);
  EXPECT_EQ(tx_bw, 3e6);
  EXPECT_EQ(rx_bw, 3e6);
  get_samplerate_and_bw(0, 15, 1, &sample_rate, &tx_bw, &rx_bw);
  EXPECT_EQ(sample_rate, 5.76e6);
}

TEST(nr_3mhz, ssb_raster)
{
  // 38.101-1 Table 5.4.3.1-2 examples: GSCN 31240 = 920.85 MHz, 31253 = 923.35 MHz
  EXPECT_EQ(ssref_3mhz_from_gscn(31240), 920850000);
  EXPECT_EQ(ssref_3mhz_from_gscn(31253), 923350000);
  // 38.101-1 Table 5.4.3.3-2: n100 31240 - <1> - 31242, 31244 - <1> - 31253
  for (int gscn = 31240; gscn <= 31253; gscn++) {
    if (gscn == 31243)
      EXPECT_DEATH(check_ssb_raster(ssref_3mhz_from_gscn(gscn), 100, 0, true), "does not belong to GSCN range");
    else
      check_ssb_raster(ssref_3mhz_from_gscn(gscn), 100, 0, true);
  }
  EXPECT_DEATH(check_ssb_raster(ssref_3mhz_from_gscn(31239), 100, 0, true), "does not belong to GSCN range");
  EXPECT_DEATH(check_ssb_raster(ssref_3mhz_from_gscn(31254), 100, 0, true), "does not belong to GSCN range");
  // n28 30432 - <1> - 30644
  check_ssb_raster(ssref_3mhz_from_gscn(30432), 28, 0, true);
  check_ssb_raster(ssref_3mhz_from_gscn(30644), 28, 0, true);
  // the two rasters do not overlap
  EXPECT_DEATH(check_ssb_raster(ssref_from_gscn(2305), 100, 0, true), "not on the 3 MHz synchronization raster");
  EXPECT_DEATH(check_ssb_raster(ssref_3mhz_from_gscn(31245), 100, 0, false), "not on the synchronization raster");
  // no 3 MHz in n101 or n8
  EXPECT_DEATH(check_ssb_raster(ssref_3mhz_from_gscn(31245), 101, 0, true), "Couldn't find band");
  EXPECT_DEATH(check_ssb_raster(ssref_3mhz_from_gscn(31245), 8, 0, true), "Couldn't find band");
}

TEST(nr_3mhz, gscn_scan)
{
  // 3 MHz carrier in n100 from 920 MHz (PointA) to 922.7 MHz: the punctured SSB (12 RB = 2.16 MHz) fits for an SSREF
  // between 921.08 and 921.62 MHz, i.e. GSCN 31242 (921.35 MHz) and 31244 (921.55 MHz), 31243 is not a 3 MHz GSCN of n100
  nr_gscn_info_t info[MAX_GSCN_BAND];
  const int n = get_scan_ssb_first_sc(921.35e6, 15, 100, 0, info);
  ASSERT_EQ(n, 2);
  const int expected[] = {31242, 31244};
  for (int i = 0; i < n; i++) {
    EXPECT_EQ(info[i].gscn, expected[i]);
    EXPECT_EQ(info[i].ssRef, ssref_3mhz_from_gscn(info[i].gscn));
    // first of the 240 subcarriers relative to PointA: the SSB after puncturing starts 48 subcarriers later
    EXPECT_EQ(info[i].ssbFirstSC, (int)((info[i].ssRef - 920e6) / 15e3 - 120));
    EXPECT_GE(info[i].ssbFirstSC + 48, 0);
    EXPECT_LE(info[i].ssbFirstSC + 192, 15 * 12);
  }
  // not a 3 MHz band: normal raster, the 20 RB SSB does not fit in 15 PRB
  EXPECT_EQ(get_scan_ssb_first_sc(1902.7e6, 15, 101, 0, info), 0);
}

TEST(nr_3mhz, carrier_within_band)
{
  // 3 MHz channel (15 PRB) in n100: pointA 920 MHz, carrier center 921.35 MHz, channel 919.85 - 922.85 MHz
  EXPECT_TRUE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 184000), 0, 15, false, true));
  // pointA at the lower band edge: the RBs fit, the 3 MHz channel does not
  EXPECT_TRUE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 183880), 0, 15, false, false));
  EXPECT_FALSE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 183880), 0, 15, false, true));
  // UL: pointA 875 MHz, center 876.35 MHz
  EXPECT_TRUE(nr_carrier_within_band(100, 0, from_nrarfcn(100, 0, 175000), 0, 15, true, true));
}

TEST(nr_power_class, power_class_1_bands)
{
  // 38.101-1 Table 6.2.1-1 (Rel.18), n14 excluded (MPR of Table 6.2.2-5 not implemented)
  for (int band : {7, 25, 31, 40, 41, 66, 71, 72, 77, 78, 85, 100, 101})
    EXPECT_TRUE(nr_band_supports_power_class_1(band)) << "band " << band;
  for (int band : {1, 3, 8, 14, 28, 79, 106})
    EXPECT_FALSE(nr_band_supports_power_class_1(band)) << "band " << band;
}

TEST(nr_channel_raster, carrier_centre)
{
  // 38.101-1 5.4.2: the RF reference frequency (PointA + N_RB * 6 subcarriers) has to be on the channel raster
  // n100 5 MHz: 919.75 + 2.25 = 922.0 MHz (DL), 877.0 MHz (UL), 100 kHz raster
  EXPECT_TRUE(nr_carrier_on_channel_raster(100, 0, from_nrarfcn(100, 0, 183950), 0, 25, false));
  EXPECT_TRUE(nr_carrier_on_channel_raster(100, 0, from_nrarfcn(100, 0, 174950), 0, 25, true));
  // n100 3 MHz: 920.05 + 1.35 = 921.4 MHz
  EXPECT_TRUE(nr_carrier_on_channel_raster(100, 0, from_nrarfcn(100, 0, 184010), 0, 15, false));
  // n101: 1900.45 + 2.25 = 1902.7 MHz, 1900.68 + 4.32 = 1905.0 MHz
  EXPECT_TRUE(nr_carrier_on_channel_raster(101, 0, from_nrarfcn(101, 0, 380090), 0, 25, false));
  EXPECT_TRUE(nr_carrier_on_channel_raster(101, 1, from_nrarfcn(101, 1, 380136), 0, 24, false));
  // n78, 106 PRB at 30 kHz (gnb.sa.band78.106prb.rfsim.yaml): 3300.6 + 19.08 = 3319.68 MHz, 30 kHz raster
  EXPECT_TRUE(nr_carrier_on_channel_raster(78, 1, from_nrarfcn(78, 1, 620040), 0, 106, false));
  // n66, 25 PRB at 15 kHz from 2150 MHz: 2152.25 MHz is not on the 100 kHz raster
  EXPECT_FALSE(nr_carrier_on_channel_raster(66, 0, from_nrarfcn(66, 0, 430000), 0, 25, false));
  // same carrier shifted by 50 kHz: 2152.3 MHz is
  EXPECT_TRUE(nr_carrier_on_channel_raster(66, 0, from_nrarfcn(66, 0, 430010), 0, 25, false));
  // offsetToCarrier moves the carrier by whole RBs (180 kHz): 922.0 + 0.18 is not on the raster
  EXPECT_FALSE(nr_carrier_on_channel_raster(100, 0, from_nrarfcn(100, 0, 183950), 1, 25, false));
}

int main(int argc, char **argv)
{
  logInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
