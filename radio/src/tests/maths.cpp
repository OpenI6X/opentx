/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// Regression tests for the division routines rewritten by c7313187f
// ("Fast math for mixer", radio/src/maths.cpp + radio/src/opentx.h).
//
// Two layers:
//
//  * Layer 1 - TEST(FastMaths, ...) sweeps every input the mixer can feed the
//    changed routines and compares the fast STM32F0 paths against verbatim
//    copies of the generic implementations they replaced. This is the proof
//    that no mixer-computed value can change.
//
//  * Layer 2 - TEST_F(FastMathsMixerTest, ...) drives evalFlightModeMixes()
//    and applyLimits() end to end and checks the resulting chans[] /
//    channelOutputs values against expectations built from those same
//    reference implementations. This is the net that catches a future tweak
//    of one of the magic constants before it reaches the radio.

#include "gtests.h"

// ---------------------------------------------------------------------------
// Reference implementations.
//
// Verbatim copies of the generic (pre-"Fast math for mixer") code that used
// to live in opentx.h. They are kept here, deliberately unoptimised, so the
// tests compare the fast paths against the code they replaced instead of
// against themselves. Do not "improve" them.
// ---------------------------------------------------------------------------

// divRoundClosest() from opentx.h, before the STM32F0 fast path was added.
static inline int refDivRoundClosest(int n, int d)
{
  if (d == 0)
    return 0;
  else
    return ((n < 0) ^ (d < 0)) ? ((n - d/2)/d) : ((n + d/2)/d);
}

// div_and_round() from opentx.h, before the STM32F0 fast path was added.
static inline int refDivAndRound(int num, int den)
{
  if (den == 0) {
    return 0;
  }
  else if (num >= 0) {
    num += den / 2;
  }
  else {
    num -= den / 2;
  }
  return num / den;
}

static inline int refCalc100to256(int x)   { return refDivRoundClosest(x*256, 100); }
static inline int refCalc100toRESX(int x)  { return refDivRoundClosest(x*RESX, 100); }
static inline int refCalc1000toRESX(int x) { return refDivRoundClosest(x*RESX, 1000); }

// expou()/expo() from mixer.cpp with calc100to256() swapped for the
// reference implementation. EXTENDED_EXPO is not defined in this build, so
// the extended branches are omitted.
static uint32_t refExpou(uint32_t x, uint32_t k)
{
  k = (uint32_t)refCalc100to256((int)k);

  uint32_t value = x * x;
  value *= k;
  value >>= 8;
  value *= x;
  value >>= 12;
  value += (uint32_t)(256 - k) * x + 128;

  return value >> 8;
}

static int refExpo(int x, int k)
{
  if (k == 0) {
    return x;
  }

  int y;
  bool neg = (x < 0);

  if (neg) {
    x = -x;
  }
  if (x > (int)RESXu) {
    x = RESXu;
  }
  if (k < 0) {
    y = (int)RESXu - (int)refExpou(RESXu - (uint32_t)x, (uint32_t)(-k));
  }
  else {
    y = (int)refExpou((uint32_t)x, (uint32_t)k);
  }
  return neg ? -y : y;
}

// ---------------------------------------------------------------------------
// Layer 1 - primitive equivalence sweeps.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Bounds shared by the sweeps below.
// ---------------------------------------------------------------------------

// Channel feedback comes from the int32 chans[] accumulator and is not
// restricted to 16 bits. The signed division helpers are therefore checked
// across their whole defined int32 domain below rather than against a
// narrower, model-derived numerator bound.

// Dense sweeps cover the band the mixer works in every day. That alone is
// not enough: a subtly wrong magic constant still divides small inputs
// correctly and only diverges towards the top of the range - so the sweeps
// also sample the whole 32-bit range with this stride. It is prime, hence
// coprime with 2^32 and with 10/100/1000, so every residue class is hit.
#define DENSE_SWEEP_LIMIT  30000000
#define FULLRANGE_STRIDE   65521u

// div_and_round()/divRoundClosest() evaluate num +/- den/2 in signed int32
// before dividing. Within den/2 of the int32 limits that overflows (undefined
// behaviour) and the fast and the generic implementation happen to wrap
// differently: 11 / 101 / 1001 inputs for den = 10 / 100 / 1000. The
// full-range sweeps step over those inputs instead of trying to test UB.
static bool nearInt32Limit(int32_t v)
{
  return v > INT32_MAX - 512 || v < INT32_MIN + 512;
}

#if defined(STM32F0)

// All three fast_div*() must be exact floor divisions - div_and_round*() are
// built on top of them, so any error here would propagate into every mixer
// result. Unlike the signed helpers they have no overflow corner: the magic
// constants are exact for every uint32 input, which the full-range sweep
// below re-checks.
TEST(FastMaths, DivBy10_100_1000_MatchFloorDivision)
{
  int32_t bad;

  bad = 0;
  for (uint32_t n = 0; n <= (uint32_t)DENSE_SWEEP_LIMIT; n++) {
    if (fast_div10(n) != (int)(n / 10)) { bad = (int32_t)n; break; }
  }
  EXPECT_EQ(bad, 0) << "fast_div10 first mismatch at " << bad;

  bad = 0;
  for (uint32_t n = 0; n <= (uint32_t)DENSE_SWEEP_LIMIT; n++) {
    if (fast_div100(n) != (int)(n / 100)) { bad = (int32_t)n; break; }
  }
  EXPECT_EQ(bad, 0) << "fast_div100 first mismatch at " << bad;

  bad = 0;
  for (uint32_t n = 0; n <= (uint32_t)DENSE_SWEEP_LIMIT; n++) {
    if (fast_div1000(n) != (int)(n / 1000)) { bad = (int32_t)n; break; }
  }
  EXPECT_EQ(bad, 0) << "fast_div1000 first mismatch at " << bad;

  // The whole 32-bit range, strided - this is the sweep that notices a
  // magic constant which is only wrong for large numerators.
  bad = 0;
  for (uint64_t n = 0; n <= 0xFFFFFFFFull; n += FULLRANGE_STRIDE) {
    if (fast_div10((uint32_t)n) != (int)(n / 10) ||
        fast_div100((uint32_t)n) != (int)(n / 100) ||
        fast_div1000((uint32_t)n) != (int)(n / 1000)) {
      bad = (int32_t)n;
      break;
    }
  }
  EXPECT_EQ(bad, 0) << "strided fast_div* first mismatch at " << bad;

  // Check the top of uint32 densely as well; the strided sweep does not land
  // on UINT32_MAX and reciprocal-division errors often occur at the edge.
  for (uint64_t n = (uint64_t)UINT32_MAX - 1024; n <= UINT32_MAX; n++) {
    ASSERT_EQ(fast_div10((uint32_t)n), (int)(n / 10)) << n;
    ASSERT_EQ(fast_div100((uint32_t)n), (int)(n / 100)) << n;
    ASSERT_EQ(fast_div1000((uint32_t)n), (int)(n / 1000)) << n;
  }
}

#endif // STM32F0

// div_and_round(num, den) for the denominators the mixer actually uses:
//  - den = 10  : mixer.cpp:819, num = v * weight  (the widest input above)
//  - den = 1000: mixer.cpp:189, num = v * weight with |v| <= 1024 (the expo
//                line clamps its input to +-RESX), |weight| <= 1000
//  - den = 100 : special functions, generous bound
// plus one non-dispatched denominator to cover the untouched fallback.
TEST(FastMaths, DivAndRound_MatchesGeneric)
{
  int32_t bad;

  bad = 0;
  for (int32_t n = -DENSE_SWEEP_LIMIT; n <= DENSE_SWEEP_LIMIT; n++) {
    if (div_and_round(n, 10) != refDivAndRound(n, 10)) { bad = n; break; }
  }
  EXPECT_EQ(bad, 0) << "div_and_round(.,10) first mismatch at " << bad;

  bad = 0;
  for (int32_t n = -1024000; n <= 1024000; n++) {
    if (div_and_round(n, 1000) != refDivAndRound(n, 1000)) { bad = n; break; }
  }
  EXPECT_EQ(bad, 0) << "div_and_round(.,1000) first mismatch at " << bad;

  bad = 0;
  for (int32_t n = -1024000; n <= 1024000; n++) {
    if (div_and_round(n, 100) != refDivAndRound(n, 100)) { bad = n; break; }
  }
  EXPECT_EQ(bad, 0) << "div_and_round(.,100) first mismatch at " << bad;

  // Whole int32 range, strided (see FULLRANGE_STRIDE), stepping over the
  // overflow corners that nearInt32Limit() documents.
  bad = 0;
  for (uint64_t n = 0; n <= 0xFFFFFFFFull; n += FULLRANGE_STRIDE) {
    int32_t v = (int32_t)n;
    if (nearInt32Limit(v))
      continue;
    if (div_and_round(v, 10) != refDivAndRound(v, 10) ||
        div_and_round(v, 100) != refDivAndRound(v, 100) ||
        div_and_round(v, 1000) != refDivAndRound(v, 1000)) {
      bad = v;
      break;
    }
  }
  EXPECT_EQ(bad, 0) << "strided div_and_round first mismatch at " << bad;

  // Dense checks near both edges of the defined domain. A 1024 margin keeps
  // num +/- den/2 representable for all three optimized denominators.
  for (int32_t delta = 1024; delta < 2048; delta++) {
    int32_t positive = INT32_MAX - delta;
    int32_t negative = INT32_MIN + delta;
    ASSERT_EQ(div_and_round(positive, 10), refDivAndRound(positive, 10));
    ASSERT_EQ(div_and_round(positive, 100), refDivAndRound(positive, 100));
    ASSERT_EQ(div_and_round(positive, 1000), refDivAndRound(positive, 1000));
    ASSERT_EQ(div_and_round(negative, 10), refDivAndRound(negative, 10));
    ASSERT_EQ(div_and_round(negative, 100), refDivAndRound(negative, 100));
    ASSERT_EQ(div_and_round(negative, 1000), refDivAndRound(negative, 1000));
  }

  // denominators that are not dispatched to a fast path
  for (int32_t n = -50000; n <= 50000; n++) {
    ASSERT_EQ(div_and_round(n, 0), refDivAndRound(n, 0)) << n;
  }
  for (int32_t n = -50000; n <= 50000; n++) {
    ASSERT_EQ(div_and_round(n, 7), refDivAndRound(n, 7)) << n;
    ASSERT_EQ(div_and_round(n, -13), refDivAndRound(n, -13)) << n;
  }
}

// divRoundClosest() dispatches to div_and_round*() for d = 10/100/1000 and
// falls through to the untouched generic path for anything else.  The widest
// numerator any calc*to*() helper can hand it is x*RESX with |x| <= 4096.
TEST(FastMaths, DivRoundClosest_MatchesGeneric)
{
  int32_t bad;

  bad = 0;
  for (int32_t n = -4200000; n <= 4200000; n++) {
    if (divRoundClosest(n, 10) != refDivRoundClosest(n, 10)) { bad = n; break; }
  }
  EXPECT_EQ(bad, 0) << "divRoundClosest(.,10) first mismatch at " << bad;

  bad = 0;
  for (int32_t n = -4200000; n <= 4200000; n++) {
    if (divRoundClosest(n, 100) != refDivRoundClosest(n, 100)) { bad = n; break; }
  }
  EXPECT_EQ(bad, 0) << "divRoundClosest(.,100) first mismatch at " << bad;

  bad = 0;
  for (int32_t n = -4200000; n <= 4200000; n++) {
    if (divRoundClosest(n, 1000) != refDivRoundClosest(n, 1000)) { bad = n; break; }
  }
  EXPECT_EQ(bad, 0) << "divRoundClosest(.,1000) first mismatch at " << bad;

  // Whole int32 range, strided, skipping the overflow corners that
  // nearInt32Limit() documents.
  bad = 0;
  for (uint64_t n = 0; n <= 0xFFFFFFFFull; n += FULLRANGE_STRIDE) {
    int32_t v = (int32_t)n;
    if (nearInt32Limit(v))
      continue;
    if (divRoundClosest(v, 10) != refDivRoundClosest(v, 10) ||
        divRoundClosest(v, 100) != refDivRoundClosest(v, 100) ||
        divRoundClosest(v, 1000) != refDivRoundClosest(v, 1000)) {
      bad = v;
      break;
    }
  }
  EXPECT_EQ(bad, 0) << "strided divRoundClosest first mismatch at " << bad;

  // widest calc*to*() numerator (4096 * RESX), both signs
  ASSERT_EQ(divRoundClosest(4096 * RESX, 1000), refDivRoundClosest(4096 * RESX, 1000));
  ASSERT_EQ(divRoundClosest(-4096 * RESX, 1000), refDivRoundClosest(-4096 * RESX, 1000));

  // divisors that must keep taking the generic path
  for (int32_t n = -20000; n <= 20000; n++) {
    ASSERT_EQ(divRoundClosest(n, 0), refDivRoundClosest(n, 0)) << n;
    ASSERT_EQ(divRoundClosest(n, 7), refDivRoundClosest(n, 7)) << n;
    ASSERT_EQ(divRoundClosest(n, -100), refDivRoundClosest(n, -100)) << n;
    ASSERT_EQ(divRoundClosest(n, RESX), refDivRoundClosest(n, RESX)) << n;
  }
}

// calc100to256() receives two GVar precision domains from mixer.cpp:770:
//  - precision 0.1: every integer through +-GVAR_MAX
//  - precision 1  : multiples of 10 through +-GV_RANGELARGE*10
// expou() (mixer.cpp:104) supplies a plain +-100 value, covered by the first.
TEST(FastMaths, Calc100to256_MatchesGeneric)
{
  for (int32_t x = -GVAR_MAX; x <= GVAR_MAX; x++) {
    ASSERT_EQ(calc100to256(x), refCalc100to256(x)) << x;
  }
  for (int32_t k = -GV_RANGELARGE; k <= GV_RANGELARGE; k++) {
    int32_t x = 10 * k;
    ASSERT_EQ(calc100to256(x), refCalc100to256(x)) << x;
  }
}

// calc100toRESX() receives the same two domains from mix offsets at
// mixer.cpp:824. The arbitrary +-GVAR_MAX sweep also contains every other
// caller: expo offsets (+-1000), trims/logical-switch thresholds (+-512),
// safety channels (+-150), swash ring values (+-100), and int8 curve points.
TEST(FastMaths, Calc100toRESX_MatchesGeneric)
{
  for (int32_t x = -GVAR_MAX; x <= GVAR_MAX; x++) {
    ASSERT_EQ(calc100toRESX(x), refCalc100toRESX(x)) << x;
  }
  for (int32_t k = -GV_RANGELARGE; k <= GV_RANGELARGE; k++) {
    int32_t x = 10 * k;
    ASSERT_EQ(calc100toRESX(x), refCalc100toRESX(x)) << x;
  }
}

// calc1000toRESX() call sites:
//  - mixer.cpp:339   trim sources are multiples of 8 through
//                    +-8*TRIM_EXTENDED_MAX
//  - myeeprom.h      plain limits are arbitrary values inside +-2013
//  - myeeprom.h      precision-0.1 GVar limits stay inside +-GVAR_MAX;
//                    precision-1 GVar limits are multiples of 10 through
//                    +-10*GVAR_MAX
TEST(FastMaths, Calc1000toRESX_MatchesGeneric)
{
  for (int32_t x = -(GV_RANGELARGE + 1000); x <= GV_RANGELARGE + 1000; x++) {
    ASSERT_EQ(calc1000toRESX(x), refCalc1000toRESX(x)) << x;
  }
  for (int32_t k = -TRIM_EXTENDED_MAX; k <= TRIM_EXTENDED_MAX; k++) {
    int32_t x = 8 * k;
    ASSERT_EQ(calc1000toRESX(x), refCalc1000toRESX(x)) << x;
  }
  for (int32_t k = -GVAR_MAX; k <= GVAR_MAX; k++) {
    int32_t x = 10 * k;
    ASSERT_EQ(calc1000toRESX(x), refCalc1000toRESX(x)) << x;
  }
}

// The fast paths are deliberately bounded. Simulator assertions use these
// same predicates at every call site; the exhaustive tests above prove exact
// equivalence over each accepted domain.
#if defined(STM32F0)
TEST(FastMaths, FastConversionDomainsAreExplicit)
{
  EXPECT_TRUE(isFastCalc100InputValid(GVAR_MAX));
  EXPECT_TRUE(isFastCalc100InputValid(-GVAR_MAX));
  EXPECT_TRUE(isFastCalc100InputValid(10 * GV_RANGELARGE));
  EXPECT_TRUE(isFastCalc100InputValid(-10 * GV_RANGELARGE));
  EXPECT_FALSE(isFastCalc100InputValid(GVAR_MAX + 1));
  EXPECT_FALSE(isFastCalc100InputValid(10 * (GV_RANGELARGE + 1)));

  EXPECT_TRUE(isFastCalc1000InputValid(GV_RANGELARGE + 1000));
  EXPECT_TRUE(isFastCalc1000InputValid(-GV_RANGELARGE - 1000));
  EXPECT_TRUE(isFastCalc1000InputValid(8 * TRIM_EXTENDED_MAX));
  EXPECT_TRUE(isFastCalc1000InputValid(-8 * TRIM_EXTENDED_MAX));
  EXPECT_TRUE(isFastCalc1000InputValid(10 * GVAR_MAX));
  EXPECT_TRUE(isFastCalc1000InputValid(-10 * GVAR_MAX));
  EXPECT_FALSE(isFastCalc1000InputValid(GV_RANGELARGE + 1001));
  EXPECT_FALSE(isFastCalc1000InputValid(8 * (TRIM_EXTENDED_MAX + 1)));
  EXPECT_FALSE(isFastCalc1000InputValid(10 * (GVAR_MAX + 1)));
}
#endif

// ---------------------------------------------------------------------------
// Layer 2 - end-to-end mixer goldens.
//
// Every expectation is computed with the reference implementations from the
// top of this file, never with the fast path, so a regression in the fast
// path shows up as a mixer-value failure.
// ---------------------------------------------------------------------------

class FastMathsMixerTest : public OpenTxTest {};

// EdgeTX writes the filtered analog values that anaIn() reads; in this test
// harness analogs are stored directly in anaInValues[] (see gtests.cpp).
static void anaSetFiltered(uint8_t chan, int32_t val)
{
  anaInValues[chan] = (uint16_t)val;
}

// Full reset for every iteration of the parameterised loops below -
// OpenTxTest::SetUp() only runs once per TEST_F.
#define FASTMATHS_RESET()                      \
  do {                                         \
    SYSTEM_RESET();                            \
    MODEL_RESET();                             \
    MIXER_RESET();                             \
    setModelDefaults();                        \
  } while (0)

// mixer.cpp:770 (calc100to256 on the weight) + mixer.cpp:819
// (div_and_round(v * weight, 10)), with v = MIXSRC_MAX = 1024 so no analog
// input, trim or channel feedback is involved.
TEST_F(FastMathsMixerTest, MixWeightMatchesReference)
{
  static const int weights[] = { 1, 47, 100, 483, -483, 500, 1013, -1013 };

  for (unsigned i = 0; i < sizeof(weights)/sizeof(weights[0]); i++) {
    FASTMATHS_RESET();

    g_model.mixData[0].destCh = 0;
    g_model.mixData[0].mltpx = MLTPX_ADD;
    g_model.mixData[0].srcRaw = MIXSRC_MAX;
    g_model.mixData[0].weight = weights[i];
    g_model.mixData[0].carryTrim = TRIM_OFF;

    evalFlightModeMixes(e_perout_mode_normal, 0);

    int32_t weight = refCalc100to256(10 * weights[i]);
    int32_t expected = refDivAndRound(1024 * weight, 10);
    EXPECT_EQ(chans[0], expected) << "weight " << weights[i];
  }
}

// Channel feedback is int32, not int16. Four high-weight additive lines make
// a source above INT16_MAX; the following line verifies the wider numerator
// still takes the exact fast-division path.
TEST_F(FastMathsMixerTest, ChainedChannelFeedbackMatchesReference)
{
  FASTMATHS_RESET();

  for (uint8_t i = 0; i < 4; i++) {
    g_model.mixData[i].destCh = 0;
    g_model.mixData[i].mltpx = MLTPX_ADD;
    g_model.mixData[i].srcRaw = MIXSRC_MAX;
    g_model.mixData[i].weight = GV_RANGELARGE;
    g_model.mixData[i].carryTrim = TRIM_OFF;
  }
  g_model.mixData[4].destCh = 1;
  g_model.mixData[4].mltpx = MLTPX_ADD;
  g_model.mixData[4].srcRaw = MIXSRC_CH1;
  g_model.mixData[4].weight = GV_RANGELARGE;
  g_model.mixData[4].carryTrim = TRIM_OFF;

  evalFlightModeMixes(e_perout_mode_normal, 0);

  int32_t weight = refCalc100to256(10 * GV_RANGELARGE);
  int32_t oneLine = refDivAndRound(1024 * weight, 10);
  int32_t feedback = (4 * oneLine) >> 8;
  ASSERT_GT(feedback, INT16_MAX);
  EXPECT_EQ(chans[0], 4 * oneLine);
  EXPECT_EQ(chans[1], refDivAndRound(feedback * weight, 10));
}

// mixer.cpp:824 - dv += div_and_round(calc100toRESX(10*offset), 10) << 8,
// added on top of the weight term from the test above.
TEST_F(FastMathsMixerTest, MixOffsetMatchesReference)
{
  // The mix offset editor limits the raw field to +-GV_RANGELARGE_OFFSET.
  static const int offsets[] = { 1, 50, -50, 100, -100, 500, -500 };

  for (unsigned i = 0; i < sizeof(offsets)/sizeof(offsets[0]); i++) {
    FASTMATHS_RESET();

    g_model.mixData[0].destCh = 0;
    g_model.mixData[0].mltpx = MLTPX_ADD;
    g_model.mixData[0].srcRaw = MIXSRC_MAX;
    g_model.mixData[0].weight = 100;
    g_model.mixData[0].offset = offsets[i];
    g_model.mixData[0].carryTrim = TRIM_OFF;

    evalFlightModeMixes(e_perout_mode_normal, 0);

    int32_t expected = refDivAndRound(1024 * refCalc100to256(10 * 100), 10);
    expected += refDivAndRound(refCalc100toRESX(10 * offsets[i]), 10) << 8;
    EXPECT_EQ(chans[0], expected) << "offset " << offsets[i];
  }
}

#if defined(GVARS)
// Exercise the two domains produced by GET_GVAR_PREC1(): arbitrary values for
// 0.1-precision GVars and multiples of 10 for integer-precision GVars. The
// integer case includes 9240, which exposed the old calc100toRESX bias.
TEST_F(FastMathsMixerTest, MixGVarPrecisionDomainsMatchReference)
{
  struct GVarCase {
    uint8_t precision;
    int16_t weightValue;
    int16_t offsetValue;
    int32_t weightInput;
    int32_t offsetInput;
  };

  static const GVarCase cases[] = {
    { 0,  GVAR_MAX,  924,  GV_RANGELARGE * 10,  9240 },
    { 0, -GVAR_MAX, -924, -GV_RANGELARGE * 10, -9240 },
    { 1,  GVAR_MAX, 1023,  GVAR_MAX,             1023 },
    { 1, -GVAR_MAX,-1023, -GVAR_MAX,            -1023 },
  };

  for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
    FASTMATHS_RESET();

    g_model.gvars[0].prec = cases[i].precision;
    g_model.gvars[1].prec = cases[i].precision;
    setGVarValue(0, cases[i].weightValue, 0);
    setGVarValue(1, cases[i].offsetValue, 0);

    MixData * md = &g_model.mixData[0];
    md->destCh = 0;
    md->mltpx = MLTPX_ADD;
    md->srcRaw = MIXSRC_MAX;
    md->weight = GV_CALC_VALUE_IDX_POS(0, GV1_LARGE);
    md->offset = GV_CALC_VALUE_IDX_POS(1, GV1_LARGE);
    md->carryTrim = TRIM_OFF;

    int32_t weightInput = GET_GVAR_PREC1(MD_WEIGHT(md), GV_RANGELARGE_NEG, GV_RANGELARGE, 0);
    int32_t offsetInput = GET_GVAR_PREC1(MD_OFFSET(md), GV_RANGELARGE_NEG, GV_RANGELARGE, 0);
    ASSERT_EQ(weightInput, cases[i].weightInput);
    ASSERT_EQ(offsetInput, cases[i].offsetInput);

    evalFlightModeMixes(e_perout_mode_normal, 0);

    int32_t expected = refDivAndRound(1024 * refCalc100to256(weightInput), 10);
    expected += refDivAndRound(refCalc100toRESX(offsetInput), 10) << 8;
    EXPECT_EQ(chans[0], expected)
      << "precision " << (int)cases[i].precision
      << " weight " << cases[i].weightValue
      << " offset " << cases[i].offsetValue;
  }
}
#endif

// mixer.cpp:339 - a trim source is calc1000toRESX(8 * trim). 512 is the
// interesting end of the range: it is one step away from the first value
// where the fast and generic implementations disagree (see the tripwire
// test above).
TEST_F(FastMathsMixerTest, TrimSourceMatchesReference)
{
  static const int trims[] = { 1, 100, -100, 512, -512 };

  for (unsigned i = 0; i < sizeof(trims)/sizeof(trims[0]); i++) {
    FASTMATHS_RESET();

    setTrimValue(0, 0, trims[i]);  // flight mode 0, rudder trim

    g_model.mixData[0].destCh = 0;
    g_model.mixData[0].mltpx = MLTPX_ADD;
    g_model.mixData[0].srcRaw = MIXSRC_FIRST_TRIM;
    g_model.mixData[0].weight = 100;
    g_model.mixData[0].carryTrim = TRIM_OFF;

    evalFlightModeMixes(e_perout_mode_normal, 0);

    int32_t v = refCalc1000toRESX(8 * trims[i]);
    int32_t expected = refDivAndRound(v * refCalc100to256(10 * 100), 10);
    EXPECT_EQ(chans[0], expected) << "trim " << trims[i];
  }
}

// mixer.cpp:189 (div_and_round by 1000), mixer.cpp:193 (calc100toRESX +
// div_and_round by 10) and mixer.cpp:104 (calc100to256 inside expou), all in
// one go: an input line with a curve, a weight and an offset, read back by a
// mix line.
TEST_F(FastMathsMixerTest, ExpoCurveWeightAndOffsetMatchReference)
{
  static const int curves[] = { 1, 30, 50, 100, -50, -100 };
  static const int weights[] = { 0, 37, 100, -100 };
  static const int offsets[] = { 0, 50, -50, 100 };
  static const int inputs[] = { 1024, -1024, 512, -512, 0 };

  for (unsigned c = 0; c < sizeof(curves)/sizeof(curves[0]); c++) {
    for (unsigned w = 0; w < sizeof(weights)/sizeof(weights[0]); w++) {
      for (unsigned o = 0; o < sizeof(offsets)/sizeof(offsets[0]); o++) {
        for (unsigned n = 0; n < sizeof(inputs)/sizeof(inputs[0]); n++) {
          FASTMATHS_RESET();
#if defined(STICK_DEAD_ZONE)
          // The dead zone rescales the analog value on its way in; it has
          // nothing to do with the division routines, so turn it off to keep
          // the expected value below exact.
          g_eeGeneral.stickDeadZone = 0;
#endif
          anaSetFiltered(CONVERT_MODE(RUD_STICK), inputs[n]);

          ExpoData * ed = &g_model.expoData[0];
          ed->mode = 3;                 // active for both signs
          ed->chn = 0;
          ed->srcRaw = MIXSRC_Rud;
          ed->weight = weights[w];
          ed->offset = offsets[o];
          ed->carryTrim = TRIM_OFF;
          ed->curve.type = CURVE_REF_EXPO;
          ed->curve.value = curves[c];

          g_model.mixData[0].destCh = 0;
          g_model.mixData[0].mltpx = MLTPX_ADD;
          g_model.mixData[0].srcRaw = MIXSRC_FIRST_INPUT;
          g_model.mixData[0].weight = 100;
          g_model.mixData[0].carryTrim = TRIM_OFF;

          evalFlightModeMixes(e_perout_mode_normal, 0);

          // applyCurve() does GET_GVAR_PREC1(value, -100, 100, fm) / 10,
          // which is just the raw value again for a non-GVAR curve param.
          int32_t v = refExpo(inputs[n], curves[c]);
          v = refDivAndRound(v * (10 * weights[w]), 1000);
          if (10 * offsets[o]) {
            v += refDivAndRound(refCalc100toRESX(10 * offsets[o]), 10);
          }
          int32_t expected = refDivAndRound(v * refCalc100to256(10 * 100), 10);

          EXPECT_EQ(chans[0], expected)
            << "curve " << curves[c] << " weight " << weights[w]
            << " offset " << offsets[o] << " input " << inputs[n];
        }
      }
    }
  }
}

// Replica of applyLimits() from mixer.cpp:227-287, with the LIMIT_*_RESX
// macros expanded through the reference calc1000toRESX(). LIMIT_OFS/MAX/MIN
// are retained so this reference also covers both GVar precision modes.
static int16_t refApplyLimits(uint8_t channel, int32_t value, const LimitData *lim)
{
  int16_t ofs   = refCalc1000toRESX(LIMIT_OFS(lim));
  int16_t lim_p = refCalc1000toRESX(LIMIT_MAX(lim));
  int16_t lim_n = refCalc1000toRESX(LIMIT_MIN(lim));

  if (ofs > lim_p) ofs = lim_p;
  if (ofs < lim_n) ofs = lim_n;

  value = limit(int32_t(-RESXl*256), value, int32_t(RESXl*256));

#if defined(PPM_LIMITS_SYMETRICAL)
  if (value) {
    int16_t tmp;
    if (lim->symetrical)
      tmp = (value > 0) ? (lim_p) : (-lim_n);
    else
      tmp = (value > 0) ? (lim_p - ofs) : (-lim_n + ofs);
    value = (int32_t) value * tmp;
    tmp = (value + (value < 0 ? (1<<17)-1 : (1<<17))) >> 18;
    ofs += tmp;
  }
#else
  if (value) {
    int16_t tmp = (value > 0) ? (lim_p - ofs) : (-lim_n + ofs);
    value = (int32_t) value * tmp;
    tmp = (value + (value < 0 ? (1<<17)-1 : (1<<17))) >> 18;
    ofs += tmp;
  }
#endif

  if (ofs > lim_p) ofs = lim_p;
  if (ofs < lim_n) ofs = lim_n;

  if (lim->revert) ofs = -ofs;

#if defined(OVERRIDE_CHANNEL_FUNCTION)
  if (safetyCh[channel] != OVERRIDE_CHANNEL_UNDEFINED) {
    ofs = refCalc100toRESX(safetyCh[channel]);
  }
#endif

  return ofs;
}

// myeeprom.h LIMIT_MAX_RESX / LIMIT_MIN_RESX / LIMIT_OFS_RESX feed
// calc1000toRESX() three times per channel.
TEST_F(FastMathsMixerTest, ApplyLimitsMatchesReference)
{
  LimitData * lim = limitAddress(0);

#if defined(OVERRIDE_CHANNEL_FUNCTION)
  for (uint32_t i = 0; i < MAX_OUTPUT_CHANNELS; i++) {
    safetyCh[i] = OVERRIDE_CHANNEL_UNDEFINED;
  }
#endif

  static const int16_t limVals[] = { 0, 1, -1, 500, -500, 1013, -1013 };
  static const int32_t inVals[]  = { 0, 1, -1, 131072, -131072, 262144, -262144, 1 << 20 };
  const unsigned nLim = sizeof(limVals)/sizeof(limVals[0]);
  const unsigned nIn  = sizeof(inVals)/sizeof(inVals[0]);

  for (unsigned revert = 0; revert < 2; revert++) {
    for (unsigned sym = 0; sym < 2; sym++) {
      for (unsigned a = 0; a < nLim; a++) {
        for (unsigned b = 0; b < nLim; b++) {
          for (unsigned c = 0; c < nLim; c++) {
            lim->min = limVals[a];
            lim->max = limVals[b];
            lim->offset = limVals[c];
            lim->revert = revert;
            lim->symetrical = sym;
            lim->curve = 0;

            for (unsigned v = 0; v < nIn; v++) {
              int16_t got = applyLimits(0, inVals[v]);
              int16_t expected = refApplyLimits(0, inVals[v], lim);
              EXPECT_EQ(got, expected)
                << "min " << lim->min << " max " << lim->max
                << " offset " << lim->offset << " revert " << revert
                << " symetrical " << sym << " value " << inVals[v];
            }
          }
        }
      }
    }
  }
}

#if defined(GVARS)
TEST_F(FastMathsMixerTest, ApplyLimitsGVarPrecisionDomainsMatchReference)
{
  static const int32_t inVals[] = { 0, 65536, -65536 };

  for (uint8_t precision = 0; precision <= 1; precision++) {
    FASTMATHS_RESET();
#if defined(OVERRIDE_CHANNEL_FUNCTION)
    for (uint32_t i = 0; i < MAX_OUTPUT_CHANNELS; i++) {
      safetyCh[i] = OVERRIDE_CHANNEL_UNDEFINED;
    }
#endif

    g_model.gvars[0].prec = precision;
    g_model.gvars[1].prec = precision;
    g_model.gvars[2].prec = precision;
    setGVarValue(0, -GVAR_MAX, 0);
    setGVarValue(1,  GVAR_MAX, 0);
    setGVarValue(2,  precision ? 1023 : 924, 0);

    LimitData * lim = limitAddress(0);
    lim->min = GV_CALC_VALUE_IDX_POS(0, GV1_LARGE);
    lim->max = GV_CALC_VALUE_IDX_POS(1, GV1_LARGE);
    lim->offset = GV_CALC_VALUE_IDX_POS(2, GV1_LARGE);
    lim->revert = 0;
    lim->symetrical = 0;
    lim->curve = 0;

    int32_t expectedMin = precision ? -GVAR_MAX : -10 * GVAR_MAX;
    int32_t expectedMax = precision ?  GVAR_MAX :  10 * GVAR_MAX;
    int32_t expectedOfs = precision ? 1023 : 9240;
    ASSERT_EQ(LIMIT_MIN(lim), expectedMin);
    ASSERT_EQ(LIMIT_MAX(lim), expectedMax);
    ASSERT_EQ(LIMIT_OFS(lim), expectedOfs);

    for (unsigned v = 0; v < sizeof(inVals)/sizeof(inVals[0]); v++) {
      int16_t got = applyLimits(0, inVals[v]);
      int16_t expected = refApplyLimits(0, inVals[v], lim);
      EXPECT_EQ(got, expected)
        << "precision " << (int)precision << " value " << inVals[v];
    }
  }
}
#endif

// mixer.cpp:282 - an active safety channel replaces the limit offset with
// calc100toRESX(safetyCh[channel]).
#if defined(OVERRIDE_CHANNEL_FUNCTION)
TEST_F(FastMathsMixerTest, ApplyLimitsSafetyChannelMatchesReference)
{
  LimitData * lim = limitAddress(0);
  lim->min = -1013;
  lim->max = 1013;
  lim->offset = 700;
  lim->revert = 0;
  lim->symetrical = 0;
  lim->curve = 0;

  static const int8_t safetyValues[] = { 0, 1, -1, 50, -50, 100, -100, 127, -127 };
  static const int32_t inVals[] = { 0, 131072, -131072 };

  for (unsigned s = 0; s < sizeof(safetyValues)/sizeof(safetyValues[0]); s++) {
    for (unsigned v = 0; v < sizeof(inVals)/sizeof(inVals[0]); v++) {
      safetyCh[0] = safetyValues[s];
      int16_t got = applyLimits(0, inVals[v]);
      int16_t expected = refApplyLimits(0, inVals[v], lim);
      EXPECT_EQ(got, expected)
        << "safety " << (int)safetyValues[s] << " value " << inVals[v];
      safetyCh[0] = OVERRIDE_CHANNEL_UNDEFINED;
    }
  }
}
#endif
