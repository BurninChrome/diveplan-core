#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include <buhlmann.h>

/* OTU functions are not exposed in buhlmann.h */
extern double otu_const(double time, double o2_ratio);
extern double otu_descend(double time, double o2_ratio_i, double o2_ratio_f);

/* Floating-point comparison with tolerance */
#define EPSILON 1e-6

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_NEAR(expected, actual, eps, msg) do { \
    double _e = (expected), _a = (actual); \
    tests_run++; \
    if (fabs(_e - _a) < (eps)) { \
        tests_passed++; \
    } else { \
        fprintf(stderr, "  FAIL: %s: expected %.10f, got %.10f (diff %.2e)\n", \
                msg, _e, _a, fabs(_e - _a)); \
    } \
} while(0)

#define ASSERT_TRUE(cond, msg) do { \
    tests_run++; \
    if ((cond)) { \
        tests_passed++; \
    } else { \
        fprintf(stderr, "  FAIL: %s\n", msg); \
    } \
} while(0)

/* ========================================================================
 * ventilation() — alveolar partial pressure
 * palv = (pamb - WVP + ((1 - rq) / rq) * CO2P) * ig_ratio
 * ====================================================================== */
static void test_ventilation(void)
{
    double result;

    printf("test_ventilation\n");

    /* Surface, air, Bühlmann RQ=1.0: N2 */
    /* palv = (1.0 - 0.0627 + 0) * 0.78084 = 0.9373 * 0.78084 */
    result = ventilation(1.0, BUHLMANN_RQ, 0.78084);
    ASSERT_NEAR(0.9373 * 0.78084, result, EPSILON,
                "N2 at surface, RQ=1.0");

    /* Surface, air, Bühlmann RQ=1.0: He=0 → should be 0 */
    result = ventilation(1.0, BUHLMANN_RQ, 0.0);
    ASSERT_NEAR(0.0, result, EPSILON,
                "He=0 at surface");

    /* At 20m (3 bar), N2 in air */
    result = ventilation(3.0, BUHLMANN_RQ, 0.78084);
    ASSERT_NEAR((3.0 - 0.0627) * 0.78084, result, EPSILON,
                "N2 at 20m, RQ=1.0");

    /* Schreiner RQ=0.8, surface, N2 */
    /* palv = (1.0 - 0.0627 + ((1-0.8)/0.8) * 0.0534) * 0.78084 */
    double expected = (1.0 - 0.0627 + (0.2/0.8) * 0.0534) * 0.78084;
    result = ventilation(1.0, SCHREINER_RQ, 0.78084);
    ASSERT_NEAR(expected, result, EPSILON,
                "N2 at surface, RQ=0.8");

    /* Pressure proportionality: double pressure → double palv (with RQ=1.0) */
    double p1 = ventilation(2.0, BUHLMANN_RQ, 0.78084);
    double p2 = ventilation(4.0, BUHLMANN_RQ, 0.78084);
    /* p2/p1 should be (4.0-0.0627)/(2.0-0.0627) */
    ASSERT_NEAR((4.0 - 0.0627) / (2.0 - 0.0627), p2 / p1, EPSILON,
                "pressure proportionality");
}

/* ========================================================================
 * haldane() — constant-pressure gas loading
 * pt = pt0 + (palv0 - pt0) * (1 - exp(-k * t))  where k = ln2 / half_val
 * ====================================================================== */
static void test_haldane(void)
{
    double result;

    printf("test_haldane\n");

    /* After t=0, tissue pressure unchanged */
    result = haldane(0.8, 1.5, 0.0, 5.0);
    ASSERT_NEAR(0.8, result, EPSILON,
                "t=0 → no change");

    /* After one half-time, should be halfway to equilibrium */
    /* pt = 0.0 + (1.0 - 0.0) * (1 - exp(-ln2)) = 1 * 0.5 = 0.5 */
    result = haldane(0.0, 1.0, 5.0, 5.0);
    ASSERT_NEAR(0.5, result, EPSILON,
                "one half-time → 50% equilibration");

    /* After two half-times → 75% */
    result = haldane(0.0, 1.0, 10.0, 5.0);
    ASSERT_NEAR(0.75, result, EPSILON,
                "two half-times → 75% equilibration");

    /* After three half-times → 87.5% */
    result = haldane(0.0, 1.0, 15.0, 5.0);
    ASSERT_NEAR(0.875, result, EPSILON,
                "three half-times → 87.5% equilibration");

    /* Very long time → converges to palv */
    result = haldane(0.0, 2.5, 1000.0, 5.0);
    ASSERT_NEAR(2.5, result, EPSILON,
                "infinite time → full equilibration");

    /* Off-gassing: tissue pressure decreases toward lower palv */
    result = haldane(2.0, 1.0, 5.0, 5.0);
    ASSERT_NEAR(1.5, result, EPSILON,
                "off-gassing: one half-time → halfway down");

    /* Compartment 1 (N2 h=2.65) at 20m for 5 min */
    double palv = (3.0 - WATER_VAPOR_PRESSURE) * 0.78084;
    double pt0 = (1.0 - WATER_VAPOR_PRESSURE) * 0.78084;
    double k = M_LN2 / 2.65;
    double expected = pt0 + (palv - pt0) * (1.0 - exp(-k * 5.0));
    result = haldane(pt0, palv, 5.0, 2.65);
    ASSERT_NEAR(expected, result, EPSILON,
                "compartment 1 N2 at 20m for 5 min");
}

/* ========================================================================
 * schreiner() — variable-pressure gas loading
 * k = ln2 / half_val
 * pt = palv0 + r*(t - 1/k) - (palv0 - pt0 - r/k) * exp(-k*t)
 * ====================================================================== */
static void test_schreiner(void)
{
    double result;

    printf("test_schreiner\n");

    /* With rate=0, schreiner should equal haldane */
    double pt0 = 0.8;
    double palv = 1.5;
    double t = 5.0;
    double half = 7.94;

    double hald = haldane(pt0, palv, t, half);
    result = schreiner(pt0, palv, 0.0, t, half);
    ASSERT_NEAR(hald, result, EPSILON,
                "rate=0 → equals haldane");

    /* At t=0, schreiner should return pt0 */
    /* pt = palv0 + r*(0 - 1/k) - (palv0 - pt0 - r/k) * exp(0) */
    /* pt = palv0 - r/k - palv0 + pt0 + r/k = pt0 */
    result = schreiner(0.8, 1.5, 2.0, 0.0, 5.0);
    ASSERT_NEAR(0.8, result, EPSILON,
                "t=0 → no change");

    /* Known calculation: compartment 1, descent from surface to 20m at 20m/min */
    /* Rate in pressure: 20m/min = 2 bar/min */
    double k = M_LN2 / 2.65;
    double r = 2.0 * 0.78084; /* N2 fraction of pressure rate */
    double palv0 = (1.0 - WATER_VAPOR_PRESSURE) * 0.78084;
    double expected = palv0 + r * (1.0 - 1.0/k) - (palv0 - 0.731881 - r/k) * exp(-k * 1.0);
    result = schreiner(0.731881, palv0, r, 1.0, 2.65);
    ASSERT_NEAR(expected, result, 1e-4,
                "compartment 1 N2 descent 1 min");
}

/* ========================================================================
 * getCeiling() — M-value ceiling for a single compartment
 * PStop = (p_tissue - a) * b  (for N2 and He separately, return max)
 * ====================================================================== */
static void test_getCeiling(void)
{
    printf("test_getCeiling\n");

    struct compartment_state s;

    /* Surface-saturated tissue: ceiling should be below surface (< 1.0 bar) */
    s.n2_p = 0.731881;
    s.he_p = 0.0;
    double ceiling = getCeiling(&zh_l12[0], &s);
    ASSERT_TRUE(ceiling < 1.0,
                "surface-saturated → ceiling below surface");

    /* Highly loaded tissue: ceiling should be positive */
    s.n2_p = 3.0;
    s.he_p = 0.0;
    ceiling = getCeiling(&zh_l12[0], &s);
    /* (3.0 - 2.200) * 0.820 = 0.656 */
    ASSERT_NEAR((3.0 - zh_l12[0].n2_a) * zh_l12[0].n2_b, ceiling, EPSILON,
                "compartment 1, N2=3.0 bar");

    /* He-loaded tissue */
    s.n2_p = 0.0;
    s.he_p = 2.0;
    ceiling = getCeiling(&zh_l12[0], &s);
    ASSERT_NEAR((2.0 - zh_l12[0].he_a) * zh_l12[0].he_b, ceiling, EPSILON,
                "compartment 1, He=2.0 bar");

    /* Both gases: ceiling is max of N2 and He ceilings */
    s.n2_p = 3.0;
    s.he_p = 2.0;
    double n2_ceil = (s.n2_p - zh_l12[0].n2_a) * zh_l12[0].n2_b;
    double he_ceil = (s.he_p - zh_l12[0].he_a) * zh_l12[0].he_b;
    ceiling = getCeiling(&zh_l12[0], &s);
    ASSERT_NEAR(fmax(n2_ceil, he_ceil), ceiling, EPSILON,
                "compartment 1, both gases → max");

    /* Verify across multiple compartments: faster compartments have higher M-values */
    s.n2_p = 2.5;
    s.he_p = 0.0;
    double ceil_cmp1 = getCeiling(&zh_l12[0], &s);
    double ceil_cmp8 = getCeiling(&zh_l12[7], &s);
    ASSERT_TRUE(ceil_cmp1 < ceil_cmp8,
                "fast compartment has lower ceiling than slow at same loading");
}

/* ========================================================================
 * compartment_stagnate() — constant-depth integration via haldane
 * ====================================================================== */
static void test_compartment_stagnate(void)
{
    printf("test_compartment_stagnate\n");

    struct compartment_state cur, end;

    /* Surface-saturated tissue stays the same at surface */
    cur.n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
    cur.he_p = ventilation(1.0, BUHLMANN_RQ, 0.0);
    compartment_stagnate(&zh_l12[0], &cur, &end, 1.0, 100.0,
                         BUHLMANN_RQ, 0.78084, 0.0);
    ASSERT_NEAR(cur.n2_p, end.n2_p, EPSILON,
                "equilibrated at surface → no N2 change");
    ASSERT_NEAR(0.0, end.he_p, EPSILON,
                "no He → stays 0");

    /* On-gassing: tissue at surface, ambient at 20m → N2 increases */
    cur.n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
    cur.he_p = 0.0;
    compartment_stagnate(&zh_l12[0], &cur, &end, 3.0, 5.0,
                         BUHLMANN_RQ, 0.78084, 0.0);
    ASSERT_TRUE(end.n2_p > cur.n2_p,
                "on-gassing: N2 increases at depth");

    /* Verify against direct haldane call */
    double expected_n2 = haldane(cur.n2_p,
                                 ventilation(3.0, BUHLMANN_RQ, 0.78084),
                                 5.0, zh_l12[0].n2_h);
    ASSERT_NEAR(expected_n2, end.n2_p, EPSILON,
                "stagnate N2 matches direct haldane");
}

/* ========================================================================
 * compartment_descend() — variable-depth integration via schreiner
 * ====================================================================== */
static void test_compartment_descend(void)
{
    printf("test_compartment_descend\n");

    struct compartment_state cur, end;

    /* With rate=0, should match stagnate */
    cur.n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
    cur.he_p = 0.0;

    struct compartment_state end_stag, end_desc;
    compartment_stagnate(&zh_l12[0], &cur, &end_stag, 3.0, 5.0,
                         BUHLMANN_RQ, 0.78084, 0.0);
    compartment_descend(&zh_l12[0], &cur, &end_desc, 3.0, 0.0, 5.0,
                        BUHLMANN_RQ, 0.78084, 0.0);
    ASSERT_NEAR(end_stag.n2_p, end_desc.n2_p, EPSILON,
                "rate=0 descend matches stagnate for N2");
    ASSERT_NEAR(end_stag.he_p, end_desc.he_p, EPSILON,
                "rate=0 descend matches stagnate for He");

    /* Descent at 20m/min for 1 min: loading should increase */
    cur.n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
    cur.he_p = 0.0;
    compartment_descend(&zh_l12[0], &cur, &end, 1.0, 2.0, 1.0,
                        BUHLMANN_RQ, 0.78084, 0.0);
    ASSERT_TRUE(end.n2_p > cur.n2_p,
                "descent → N2 loading increases");
}

/* ========================================================================
 * nodecotime() — no-decompression time for a compartment
 * ====================================================================== */
static void test_nodecotime(void)
{
    printf("test_nodecotime\n");

    struct compartment_state s;

    /* Surface-saturated tissue: NDL should be large (near 100 min cap) */
    s.n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
    s.he_p = 0.0;
    double ndl = nodecotime(&zh_l12[0], &s, 1.0, 0.78084, 0.0);
    ASSERT_NEAR(100.0, ndl, 1.0,
                "surface-saturated → NDL near max");

    /* At 50m (6 bar): fast compartment (h=2.65, a=2.2) reaches its M-value
     * ceiling in ~7.5 min from surface saturation — well within the 100 min cap */
    s.n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
    s.he_p = 0.0;
    ndl = nodecotime(&zh_l12[0], &s, 6.0, 0.78084, 0.0);
    ASSERT_TRUE(ndl > 0.0 && ndl < 100.0,
                "at 50m → finite NDL for fast compartment");

    /* Deeper → shorter NDL (both within finite range at these depths) */
    double ndl_50m = nodecotime(&zh_l12[0], &s, 6.0, 0.78084, 0.0);
    double ndl_60m = nodecotime(&zh_l12[0], &s, 7.0, 0.78084, 0.0);
    ASSERT_TRUE(ndl_60m < ndl_50m,
                "deeper → shorter NDL");
}

/* ========================================================================
 * otu_const() — OTU for constant depth
 * otu = time * pow(0.5 / (O2 - 0.5), -5.0/6.0)
 * ====================================================================== */
static void test_otu_const(void)
{
    printf("test_otu_const\n");

    /* O2 <= 0.5: no toxicity */
    double result = otu_const(60.0, 0.21);
    ASSERT_NEAR(0.0, result, EPSILON,
                "O2=0.21 → no OTU");

    result = otu_const(60.0, 0.50);
    ASSERT_NEAR(0.0, result, EPSILON,
                "O2=0.50 → no OTU");

    /* O2=1.0 (pure O2): otu = time * pow(0.5/0.5, -5/6) = time * 1.0 = time */
    result = otu_const(60.0, 1.0);
    ASSERT_NEAR(60.0, result, EPSILON,
                "O2=1.0 → OTU equals time");

    /* O2=0.8: known calculation */
    double expected = 60.0 * pow(0.5 / (0.8 - 0.5), -5.0/6.0);
    result = otu_const(60.0, 0.8);
    ASSERT_NEAR(expected, result, EPSILON,
                "O2=0.8, 60 min");

    /* Linearity in time */
    double otu_30 = otu_const(30.0, 0.8);
    double otu_60 = otu_const(60.0, 0.8);
    ASSERT_NEAR(2.0, otu_60 / otu_30, EPSILON,
                "OTU linear in time");
}

/* ========================================================================
 * gradient_factor_slope() and gradient_factor()
 * ====================================================================== */
static void test_gradient_factor(void)
{
    printf("test_gradient_factor\n");

    /* GF slope = (gfhi - gflow) / (final_stop - first_stop) */
    double slope = gradient_factor_slope(0.85, 0.30, 3.0, 30.0);
    ASSERT_NEAR((0.85 - 0.30) / (3.0 - 30.0), slope, EPSILON,
                "GF slope 30/85");

    /* Same stops → slope = 0 */
    slope = gradient_factor_slope(0.85, 0.30, 10.0, 10.0);
    ASSERT_NEAR(0.0, slope, EPSILON,
                "same stop depths → slope=0");

    /* GF at current depth = slope * depth + gfhi */
    double gf = gradient_factor(-0.02, 15.0, 0.85);
    ASSERT_NEAR(-0.02 * 15.0 + 0.85, gf, EPSILON,
                "GF at 15m");

    /* GF at depth=0 → gfhi */
    gf = gradient_factor(-0.02, 0.0, 0.85);
    ASSERT_NEAR(0.85, gf, EPSILON,
                "GF at surface → gfhi");
}

/* ========================================================================
 * Integration test: full dive simulation
 * ====================================================================== */
static void test_integration_surface_equilibrium(void)
{
    printf("test_integration_surface_equilibrium\n");

    /* All 16 compartments at surface equilibrium should have no ceiling */
    struct compartment_state s[ZH_L12_NR_COMPARTMENTS];
    int i;
    double max_ceiling = 0.0;

    for (i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
        s[i].n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
        s[i].he_p = ventilation(1.0, BUHLMANN_RQ, 0.0);
        double c = getCeiling(&zh_l12[i], &s[i]);
        if (c > max_ceiling) max_ceiling = c;
    }
    ASSERT_TRUE(max_ceiling < 1.0,
                "surface equilibrium → all ceilings below surface");
}

static void test_integration_dive_loading(void)
{
    printf("test_integration_dive_loading\n");

    /* Simulate 10 min at 30m: fast compartments load more than slow ones */
    struct compartment_state s[ZH_L12_NR_COMPARTMENTS];
    int i;

    for (i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
        s[i].n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
        s[i].he_p = 0.0;
    }

    double initial_n2 = s[0].n2_p;

    for (i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
        compartment_stagnate(&zh_l12[i], &s[i], &s[i], 4.0, 10.0,
                             BUHLMANN_RQ, 0.78084, 0.0);
    }

    /* Fast compartment (0) loads more than slow compartment (15) */
    ASSERT_TRUE(s[0].n2_p > s[15].n2_p,
                "fast compartment loads more than slow");

    /* All compartments increased from initial */
    ASSERT_TRUE(s[0].n2_p > initial_n2,
                "compartment 0 N2 increased");
    ASSERT_TRUE(s[15].n2_p > initial_n2,
                "compartment 15 N2 increased");

    /* After long dive at depth, ceiling should be above surface for some compartments */
    for (i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
        s[i].n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
        s[i].he_p = 0.0;
    }

    /* 30 min at 40m (5 bar) */
    for (i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
        compartment_stagnate(&zh_l12[i], &s[i], &s[i], 5.0, 30.0,
                             BUHLMANN_RQ, 0.78084, 0.0);
    }

    double max_ceiling = 0.0;
    for (i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
        double c = getCeiling(&zh_l12[i], &s[i]);
        if (c > max_ceiling) max_ceiling = c;
    }
    ASSERT_TRUE(max_ceiling > 1.0,
                "30 min at 40m → deco obligation (ceiling > surface)");
}

/* ========================================================================
 * compartment_mvalue() — weighted M-value across N2 and He
 * a = (n2_p*n2_a + he_p*he_a) / (n2_p + he_p)
 * b = (n2_p*n2_b + he_p*he_b) / (n2_p + he_p)
 * mvalue = ((n2_p + he_p) - a) * b
 * ====================================================================== */
static void test_compartment_mvalue(void)
{
    printf("test_compartment_mvalue\n");

    struct compartment_state s;

    /* Pure N2 loading, ZH-L16C compartment 1 (idx 0):
     * a = n2_a = 1.2599, b = n2_b = 0.5240
     * mvalue = (2.0 - 1.2599) * 0.5240 = 0.7401 * 0.5240 */
    s.n2_p = 2.0;
    s.he_p = 0.0;
    double expected = (2.0 - zh_l16C[0].n2_a) * zh_l16C[0].n2_b;
    ASSERT_NEAR(expected, compartment_mvalue(&zh_l16C[0], &s), EPSILON,
                "ZH-L16C cpt1, pure N2: mvalue matches ceiling formula");

    /* Pure He loading, ZH-L16C compartment 1:
     * a = he_a = 1.7424, b = he_b = 0.4245 */
    s.n2_p = 0.0;
    s.he_p = 2.0;
    expected = (2.0 - zh_l16C[0].he_a) * zh_l16C[0].he_b;
    ASSERT_NEAR(expected, compartment_mvalue(&zh_l16C[0], &s), EPSILON,
                "ZH-L16C cpt1, pure He: mvalue matches ceiling formula");

    /* Mixed N2+He: weighted average of a and b coefficients */
    s.n2_p = 1.5;
    s.he_p = 0.5;
    double total = s.n2_p + s.he_p;
    double a = (s.n2_p * zh_l16C[0].n2_a + s.he_p * zh_l16C[0].he_a) / total;
    double b = (s.n2_p * zh_l16C[0].n2_b + s.he_p * zh_l16C[0].he_b) / total;
    expected = (total - a) * b;
    ASSERT_NEAR(expected, compartment_mvalue(&zh_l16C[0], &s), EPSILON,
                "ZH-L16C cpt1, mixed N2+He: weighted mvalue");

    /* Higher total inert gas pressure → higher mvalue */
    struct compartment_state s_low = { .n2_p = 1.0, .he_p = 0.0 };
    struct compartment_state s_high = { .n2_p = 3.0, .he_p = 0.0 };
    ASSERT_TRUE(compartment_mvalue(&zh_l16C[0], &s_high) >
                compartment_mvalue(&zh_l16C[0], &s_low),
                "higher loading → higher mvalue");
}

/* ========================================================================
 * otu_descend() — OTU during descent (changing O2 partial pressure)
 * otu = (3/11 * time) / (o2_f - o2_i) * (pow(...f) - pow(...i))
 * ====================================================================== */
static void test_otu_descend(void)
{
    printf("test_otu_descend\n");

    /* Both ppO2 <= 0.5: no toxicity */
    double result = otu_descend(10.0, 0.21, 0.40);
    ASSERT_NEAR(0.0, result, EPSILON,
                "both ppO2 <= 0.5 → no OTU");

    /* Both sides > 0.5: toxicity calculated */
    result = otu_descend(10.0, 0.6, 1.0);
    ASSERT_TRUE(result > 0.0,
                "descent within hyperoxia → positive OTU");

    /* o2_i < 0.5, o2_f > 0.5: ascent into hyperoxia — OTU from threshold */
    result = otu_descend(10.0, 0.4, 1.0);
    ASSERT_TRUE(result > 0.0,
                "descent into hyperoxia → positive OTU");

    /* o2_i > 0.5, o2_f < 0.5: descent out of hyperoxia — clamp o2_f */
    result = otu_descend(10.0, 0.8, 0.4);
    ASSERT_TRUE(result > 0.0,
                "ascent out of hyperoxia → positive OTU");

    /* Both sides > 0.5: toxicity calculated */
    result = otu_descend(60.0, 0.6, 1.0);
    ASSERT_TRUE(result > 0.0,
                "descent within hyperoxia → positive OTU");

    /* Linearity in time */
    double otu_30 = otu_descend(30.0, 0.6, 1.0);
    double otu_60 = otu_descend(60.0, 0.6, 1.0);
    ASSERT_NEAR(2.0, otu_60 / otu_30, EPSILON,
                "OTU_descend linear in time");

    /* Known calculation: o2_i=0.6, o2_f=1.0, t=60 */
    double expected = ((3.0/11.0) * 60.0) / (1.0 - 0.6) *
                      (pow((1.0 - 0.5)/0.5, 11.0/6.0) -
                       pow((0.6 - 0.5)/0.5, 11.0/6.0));
    result = otu_descend(60.0, 0.6, 1.0);
    ASSERT_NEAR(expected, result, EPSILON,
                "otu_descend known calculation");
}

/* ========================================================================
 * ZH-L16C specific tests — the algorithm used in dive computers
 * This is the most safety-critical constant table in the library.
 * ====================================================================== */
static void test_zh_l16c_constants(void)
{
    printf("test_zh_l16c_constants (ZH-L16C — dive computer algorithm)\n");

    /* ZH-L16C has exactly 17 compartments (1, 1b, 2..16) */
    /* Compartment 1 (idx 0): N2 half-time = 4.0 min */
    ASSERT_NEAR(4.0,    zh_l16C[0].n2_h,  EPSILON, "cpt1  N2 half-time = 4.0");
    ASSERT_NEAR(1.2599, zh_l16C[0].n2_a,  1e-4,    "cpt1  N2 a = 1.2599");
    ASSERT_NEAR(0.5240, zh_l16C[0].n2_b,  1e-4,    "cpt1  N2 b = 0.5240");
    ASSERT_NEAR(1.51,   zh_l16C[0].he_h,  1e-4,    "cpt1  He half-time = 1.51");

    /* Compartment 1b (idx 1) is unique to ZH-L16C vs A/B */
    ASSERT_NEAR(5.0,    zh_l16C[1].n2_h,  EPSILON, "cpt1b N2 half-time = 5.0");
    ASSERT_NEAR(1.6189, zh_l16C[1].he_a,  1e-4,    "cpt1b He a = 1.6189");

    /* Last compartment (idx 16): N2 half-time = 635 min */
    ASSERT_NEAR(635.0,  zh_l16C[16].n2_h, EPSILON, "cpt16 N2 half-time = 635.0");
    ASSERT_NEAR(0.2327, zh_l16C[16].n2_a, 1e-4,    "cpt16 N2 a = 0.2327");
    ASSERT_NEAR(0.9653, zh_l16C[16].n2_b, 1e-4,    "cpt16 N2 b = 0.9653");

    /* Half-times must increase monotonically (faster → slower compartments) */
    int i;
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS - 1; i++) {
        ASSERT_TRUE(zh_l16C[i].n2_h < zh_l16C[i+1].n2_h,
                    "N2 half-times increase monotonically");
        ASSERT_TRUE(zh_l16C[i].he_h < zh_l16C[i+1].he_h,
                    "He half-times increase monotonically");
    }

    /* He half-times must be shorter than N2 (He diffuses faster) */
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        ASSERT_TRUE(zh_l16C[i].he_h < zh_l16C[i].n2_h,
                    "He half-time < N2 half-time in every compartment");
    }

    /* All b coefficients must be in (0, 1) — physical constraint */
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        ASSERT_TRUE(zh_l16C[i].n2_b > 0.0 && zh_l16C[i].n2_b < 1.0,
                    "N2 b in (0,1)");
        ASSERT_TRUE(zh_l16C[i].he_b > 0.0 && zh_l16C[i].he_b < 1.0,
                    "He b in (0,1)");
    }

    /* N2 a coefficients decrease with compartment number */
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS - 1; i++) {
        ASSERT_TRUE(zh_l16C[i].n2_a > zh_l16C[i+1].n2_a,
                    "N2 a decreases with compartment index");
    }

    /* N2 b coefficients increase with compartment number */
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS - 1; i++) {
        ASSERT_TRUE(zh_l16C[i].n2_b < zh_l16C[i+1].n2_b,
                    "N2 b increases with compartment index");
    }
}

static void test_zh_l16c_surface_equilibrium(void)
{
    printf("test_zh_l16c_surface_equilibrium\n");

    struct compartment_state s[ZH_L16_NR_COMPARTMENTS];
    int i;

    /* All 17 ZH-L16C compartments at surface saturation → no deco obligation */
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        s[i].n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
        s[i].he_p = 0.0;
    }

    double max_ceiling = 0.0;
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        double c = getCeiling(&zh_l16C[i], &s[i]);
        if (c > max_ceiling) max_ceiling = c;
    }
    ASSERT_TRUE(max_ceiling < 1.0,
                "ZH-L16C: surface equilibrium → all ceilings below surface");
}

static void test_zh_l16c_dive_loading(void)
{
    printf("test_zh_l16c_dive_loading\n");

    struct compartment_state s[ZH_L16_NR_COMPARTMENTS];
    int i;

    /* Initialize to surface saturation */
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        s[i].n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
        s[i].he_p = 0.0;
    }

    /* Simulate 30 min at 40m (5 bar) with air */
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        compartment_stagnate(&zh_l16C[i], &s[i], &s[i], 5.0, 30.0,
                             BUHLMANN_RQ, 0.78084, 0.0);
    }

    /* Fast compartment loads more than slow */
    ASSERT_TRUE(s[0].n2_p > s[16].n2_p,
                "ZH-L16C: fast cpt loads more than slow after 30min@40m");

    /* After 30 min at 40m, there should be a deco obligation */
    double max_ceiling = 0.0;
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        double c = getCeiling(&zh_l16C[i], &s[i]);
        if (c > max_ceiling) max_ceiling = c;
    }
    ASSERT_TRUE(max_ceiling > 1.0,
                "ZH-L16C: 30min@40m → deco obligation");

    /* NDL at surface after this dive: should be 0 (we're in deco) */
    double min_ndl = 100.0;
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        double ndl = nodecotime(&zh_l16C[i], &s[i], 5.0, 0.78084, 0.0);
        if (ndl < min_ndl) min_ndl = ndl;
    }
    ASSERT_TRUE(min_ndl < 5.0,
                "ZH-L16C: NDL near zero after 30min@40m");
}

static void test_zh_l16c_trimix(void)
{
    printf("test_zh_l16c_trimix (He loading)\n");

    struct compartment_state s[ZH_L16_NR_COMPARTMENTS];
    int i;

    /* Initialize to surface saturation (no He) */
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        s[i].n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
        s[i].he_p = 0.0;
    }

    /* 20 min at 60m (7 bar) on trimix 21/35 (21% O2, 35% He, 44% N2) */
    double n2_ratio = 0.44;
    double he_ratio = 0.35;
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        compartment_stagnate(&zh_l16C[i], &s[i], &s[i], 7.0, 20.0,
                             BUHLMANN_RQ, n2_ratio, he_ratio);
    }

    /* He should now be loaded in all compartments */
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        ASSERT_TRUE(s[i].he_p > 0.0,
                    "ZH-L16C trimix: He loaded in all compartments");
    }

    /* Fast He compartment loads more He than slow */
    ASSERT_TRUE(s[0].he_p > s[16].he_p,
                "ZH-L16C trimix: fast cpt has more He than slow");

    /* Ceiling should exist — trimix at 60m generates deco */
    double max_ceiling = 0.0;
    for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        double c = getCeiling(&zh_l16C[i], &s[i]);
        if (c > max_ceiling) max_ceiling = c;
    }
    ASSERT_TRUE(max_ceiling > 1.0,
                "ZH-L16C trimix 20min@60m → deco obligation");
}

/* ======================================================================== */

int main(void)
{
    printf("=== Bühlmann library unit tests ===\n\n");

    test_ventilation();
    test_haldane();
    test_schreiner();
    test_getCeiling();
    test_compartment_stagnate();
    test_compartment_descend();
    test_nodecotime();
    test_otu_const();
    test_gradient_factor();
    test_integration_surface_equilibrium();
    test_integration_dive_loading();

    printf("\n--- ZH-L16C (dive computer algorithm) ---\n\n");
    test_zh_l16c_constants();
    test_zh_l16c_surface_equilibrium();
    test_zh_l16c_dive_loading();
    test_zh_l16c_trimix();

    printf("\n--- Additional function coverage ---\n\n");
    test_compartment_mvalue();
    test_otu_descend();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
