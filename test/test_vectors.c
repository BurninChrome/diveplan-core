/* Check libbuhlmann against the conformance vectors in test/vectors/.
 *
 * The vectors are generated from published decompression theory by
 * test/vectors/gen_vectors.py, which never touches this library. They state
 * how the model SHOULD behave, so some of them fail today: every such
 * deviation is a known defect listed in test/vectors/expected-failures.txt
 * with a pointer to its write-up.
 *
 * Exit status:
 *   0  every file within its allowed deviation count
 *   1  a file deviated more than allowed, or a file deviated less than allowed
 *      (a defect was fixed and expected-failures.txt needs updating)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <buhlmann.h>

#define TOL      1e-6
#define MAXLINE  512
#define MAXFILES 16

static const char *vector_dir = "vectors";

struct result { char file[64]; int checked, failed, skipped, unparsed; };
static struct result results[MAXFILES];
static int nresults;

static struct result *bucket(const char *file)
{
    int i;
    for (i = 0; i < nresults; i++)
        if (strcmp(results[i].file, file) == 0)
            return &results[i];
    if (nresults >= MAXFILES) {
        fprintf(stderr, "too many vector files (max %d)\n", MAXFILES);
        exit(2);
    }
    memset(&results[nresults], 0, sizeof results[0]);
    snprintf(results[nresults].file, sizeof results[0].file, "%s", file);
    return &results[nresults++];
}

/* A data line that does not parse is a corrupt vector file, not something to
   skip quietly: silence here is how a whole file can vanish and leave the
   suite green. */
static void unparsed(const char *file, const char *line)
{
    struct result *r = bucket(file);
    if (r->unparsed++ < 3)
        fprintf(stderr, "    %s: unparseable line: %.60s", file, line);
}

static void skipped(const char *file)
{
    bucket(file)->skipped++;
}

static const struct compartment_constants *table_of(const char *name, int *count)
{
    if (strcmp(name, "zh_l12") == 0)  { *count = ZH_L12_NR_COMPARTMENTS; return zh_l12;  }
    if (strcmp(name, "zh_l16C") == 0) { *count = ZH_L16_NR_COMPARTMENTS; return zh_l16C; }
    *count = 0;
    return NULL;
}

static FILE *open_vectors(const char *file)
{
    char path[256];
    FILE *f;
    snprintf(path, sizeof path, "%s/%s", vector_dir, file);
    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "  cannot open %s\n", path);
        exit(2);
    }
    return f;
}

/* Compare one value, recording the outcome against `file`. */
static void check(const char *file, double expected, double actual, double tol,
                  const char *what)
{
    struct result *r = bucket(file);
    r->checked++;
    if (!(fabs(expected - actual) < tol)) {
        r->failed++;
        if (r->failed <= 3)     /* keep the log readable */
            fprintf(stderr, "    %s: %s expected %.10f got %.10f (diff %.2e)\n",
                    file, what, expected, actual, fabs(expected - actual));
    }
}

/* ---------------------------------------------------------------- checkers */

static void check_alveolar(void)
{
    FILE *f = open_vectors("alveolar.tsv");
    char line[MAXLINE];
    double pamb, rq, ig, expected;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%lf %lf %lf %lf", &pamb, &rq, &ig, &expected) != 4)
            { unparsed("alveolar.tsv", line); continue; }
        check("alveolar.tsv", expected, ventilation(pamb, rq, ig), TOL, "palv");
    }
    fclose(f);
}

static void check_haldane(void)
{
    FILE *f = open_vectors("haldane.tsv");
    char line[MAXLINE];
    double pt0, palv, t, ht, expected;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%lf %lf %lf %lf %lf", &pt0, &palv, &t, &ht, &expected) != 5)
            { unparsed("haldane.tsv", line); continue; }
        check("haldane.tsv", expected, haldane(pt0, palv, t, ht), TOL, "pt");
    }
    fclose(f);
}

static void check_schreiner(void)
{
    FILE *f = open_vectors("schreiner.tsv");
    char line[MAXLINE];
    double pt0, palv0, rate, t, ht, expected;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%lf %lf %lf %lf %lf %lf", &pt0, &palv0, &rate, &t, &ht, &expected) != 6)
            { unparsed("schreiner.tsv", line); continue; }
        check("schreiner.tsv", expected, schreiner(pt0, palv0, rate, t, ht), TOL, "pt");
    }
    fclose(f);
}

static void check_ceiling(void)
{
    FILE *f = open_vectors("ceiling.tsv");
    char line[MAXLINE], tname[32];
    int idx, n;
    double n2_p, he_p, expected;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%31s %d %lf %lf %lf", tname, &idx, &n2_p, &he_p, &expected) != 5)
            { unparsed("ceiling.tsv", line); continue; }
        const struct compartment_constants *t = table_of(tname, &n);
        if (!t || idx >= n) continue;
        struct compartment_state s;
        s.n2_p = n2_p; s.he_p = he_p;
        check("ceiling.tsv", expected, getCeiling(&t[idx], &s), TOL, "ceiling");
    }
    fclose(f);
}

static void check_gradient_tolerance(void)
{
    FILE *f = open_vectors("gradient-tolerance.tsv");
    char line[MAXLINE], tname[32];
    int idx, n;
    double pt, gf, expected;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%31s %d %lf %lf %lf", tname, &idx, &pt, &gf, &expected) != 5)
            { unparsed("gradient-tolerance.tsv", line); continue; }
        const struct compartment_constants *t = table_of(tname, &n);
        if (!t || idx >= n) continue;
        /* There is no GF-aware entry point in the library. At gf == 1 the
           tolerated pressure is exactly the raw ceiling, so those rows can be
           checked against getCeiling(); the rest cannot be checked at all
           until the primitive exists. */
        if (fabs(gf - 1.0) > 1e-12) {
            /* Not a deviation — nothing is measured. There is no GF-aware
               entry point to call, so these rows are unreachable rather than
               wrong. Counting them as failures overstated what the suite
               tests and made the ratchet structurally unable to fire. */
            skipped("gradient-tolerance.tsv");
            continue;
        }
        struct compartment_state s;
        s.n2_p = pt; s.he_p = 0.0;
        check("gradient-tolerance.tsv", expected, getCeiling(&t[idx], &s), TOL, "P_tol(gf=1)");
    }
    fclose(f);
}

static void check_ndl(const char *file)
{
    FILE *f = open_vectors(file);
    char line[MAXLINE], tname[32];
    int n, i;
    double pamb, n2r, her, expected;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%31s %lf %lf %lf %lf", tname, &pamb, &n2r, &her, &expected) != 5)
            { unparsed(file, line); continue; }
        const struct compartment_constants *t = table_of(tname, &n);
        if (!t) continue;
        double surf_n2 = ventilation(1.0, BUHLMANN_RQ, 0.78084);
        double best = 100.0;
        for (i = 0; i < n; i++) {
            struct compartment_state s;
            double v;
            if (t[i].n2_h <= 0.0) continue;         /* skip zero-filled rows */
            s.n2_p = surf_n2; s.he_p = 0.0;
            v = nodecotime(&t[i], &s, pamb, n2r, her);
            if (v < best) best = v;
        }
        check(file, expected, best, 0.5, "min NDL");
    }
    fclose(f);
}

static void check_zh_l16a_derivation(void)
{
    FILE *f = open_vectors("zh-l16a-derivation.tsv");
    char line[MAXLINE];
    int idx;
    double ht, a_formula, b_formula;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%d %lf %lf %lf", &idx, &ht, &a_formula, &b_formula) != 4)
            { unparsed("zh-l16a-derivation.tsv", line); continue; }
        if (idx >= ZH_L16_NR_COMPARTMENTS) continue;
        /* 1e-4, not tighter: the tables store `f`-suffixed literals in double
           fields, so 38.3 reads back as 38.2999992371. Separate cleanup. */
        check("zh-l16a-derivation.tsv", ht, zh_l16A[idx].n2_h, 1e-4, "A half-time");
        check("zh-l16a-derivation.tsv", a_formula, zh_l16A[idx].n2_a, 1e-3, "A a = 2/cbrt(t)");
        /* The 18.5 min compartment deviates from the b formula in the
           published table itself (0.7825 against a computed 0.77250). Keyed on
           the half-time, not the index: zh_l16A omits the 1b row, so 18.5 min
           sits at index 3 there and index 4 in zh_l16C. */
        if (fabs(ht - 18.5) > 1e-6)
            check("zh-l16a-derivation.tsv", b_formula, zh_l16A[idx].n2_b, 1e-3,
                  "A b = 1.005 - 1/sqrt(t)");
    }
    fclose(f);
}

static void check_zh_l16c_published(void)
{
    FILE *f = open_vectors("zh-l16c-published.tsv");
    char line[MAXLINE];
    int idx;
    double ht, a, b, hht, ha, hb;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%d %lf %lf %lf %lf %lf %lf",
                   &idx, &ht, &a, &b, &hht, &ha, &hb) != 7)
            { unparsed("zh-l16c-published.tsv", line); continue; }
        if (idx >= ZH_L16_NR_COMPARTMENTS) continue;
        check("zh-l16c-published.tsv", ht,  zh_l16C[idx].n2_h, 1e-4, "C N2 half-time");
        check("zh-l16c-published.tsv", a,   zh_l16C[idx].n2_a, 1e-4, "C N2 a");
        check("zh-l16c-published.tsv", b,   zh_l16C[idx].n2_b, 1e-4, "C N2 b");
        check("zh-l16c-published.tsv", hht, zh_l16C[idx].he_h, 1e-3, "C He half-time");
        check("zh-l16c-published.tsv", ha,  zh_l16C[idx].he_a, 1e-4, "C He a");
        check("zh-l16c-published.tsv", hb,  zh_l16C[idx].he_b, 1e-4, "C He b");
    }
    fclose(f);
}

/* ------------------------------------------------------- expected failures */

struct allowance { char file[64]; int expect_checks, expect_skips, allowed; char reason[160]; };
static struct allowance allowances[MAXFILES];
static int nallowances;

static void load_allowances(void)
{
    char path[256], line[MAXLINE];
    FILE *f;
    snprintf(path, sizeof path, "%s/expected-failures.txt", vector_dir);
    f = fopen(path, "r");
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        struct allowance *a;
        if (line[0] == '#' || line[0] == '\n') continue;
        if (nallowances >= MAXFILES) {
            fprintf(stderr, "too many allowance entries (max %d)\n", MAXFILES);
            exit(2);
        }
        a = &allowances[nallowances];
        if (sscanf(line, "%63s %d %d %d %159[^\n]",
                   a->file, &a->expect_checks, &a->expect_skips,
                   &a->allowed, a->reason) == 5)
            nallowances++;
        else
            fprintf(stderr, "  malformed allowance line: %.60s", line);
    }
    fclose(f);
}

static const struct allowance *allowance_for(const char *file)
{
    int i;
    for (i = 0; i < nallowances; i++)
        if (strcmp(allowances[i].file, file) == 0)
            return &allowances[i];
    return NULL;
}

int main(int argc, char **argv)
{
    int i, status = 0, total = 0, failed = 0;

    if (argc > 1) vector_dir = argv[1];
    else if (getenv("srcdir")) {
        static char buf[256];
        snprintf(buf, sizeof buf, "%s/vectors", getenv("srcdir"));
        vector_dir = buf;
    }

    printf("=== conformance vectors (%s) ===\n\n", vector_dir);
    load_allowances();

    check_alveolar();
    check_haldane();
    check_schreiner();
    check_ceiling();
    check_gradient_tolerance();
    check_ndl("ndl-air.tsv");
    check_ndl("ndl-trimix.tsv");
    check_zh_l16a_derivation();
    check_zh_l16c_published();

    printf("\n%-26s %8s %8s %8s %8s   %s\n",
           "file", "checked", "expect", "failed", "allowed", "verdict");
    for (i = 0; i < nresults; i++) {
        const struct allowance *a = allowance_for(results[i].file);
        int allowed = a ? a->allowed : 0;
        int expect  = a ? a->expect_checks : -1;
        const char *verdict;

        total  += results[i].checked;
        failed += results[i].failed;

        if (results[i].unparsed) {
            verdict = "CORRUPT - unparseable lines"; status = 1;
        } else if (!a) {
            verdict = "NO ALLOWANCE ENTRY"; status = 1;
        } else if (results[i].checked != expect) {
            verdict = "WRONG CHECK COUNT - data missing or changed"; status = 1;
        } else if (a && results[i].skipped != a->expect_skips) {
            /* Skipped rows are unreachable, not wrong — but they are still
               data, and 83% of gradient-tolerance.tsv is skipped. Without this
               those rows could be deleted with no signal. */
            verdict = "WRONG SKIP COUNT - data missing or changed"; status = 1;
        } else if (results[i].failed == allowed) {
            verdict = allowed ? "known deviation" : "ok";
        } else if (results[i].failed > allowed) {
            verdict = "REGRESSION"; status = 1;
        } else {
            verdict = "FIXED - update expected-failures.txt"; status = 1;
        }
        printf("%-26s %8d %8d %8d %8d   %s\n", results[i].file,
               results[i].checked, expect, results[i].failed, allowed, verdict);
        if (results[i].skipped || (a && a->expect_skips))
            printf("%-26s %8s %8d of %d rows unreachable (no library entry point)\n",
                   "", "skipped:", results[i].skipped, a ? a->expect_skips : 0);
        if (a && a->reason[0] && results[i].failed)
            printf("%-26s %26s   %s\n", "", "", a->reason);
    }

    /* A file listed in expected-failures.txt that produced no results at all
       never ran — deleted, renamed, or emptied. Without this the suite reports
       success on a corpus that has silently evaporated. */
    for (i = 0; i < nallowances; i++) {
        int j, seen = 0;
        for (j = 0; j < nresults; j++)
            if (strcmp(results[j].file, allowances[i].file) == 0) seen = 1;
        if (!seen) {
            printf("%-26s %8s %8d %8s %8s   MISSING - no vectors ran\n",
                   allowances[i].file, "-", allowances[i].expect_checks, "-", "-");
            status = 1;
        }
    }

    printf("\n%d vectors checked, %d deviating\n", total, failed);
    printf("%s\n", status ? "=== FAIL ===" : "=== OK ===");
    return status;
}
