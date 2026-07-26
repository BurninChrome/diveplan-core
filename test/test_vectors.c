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

struct result { char file[64]; int checked, failed; };
static struct result results[MAXFILES];
static int nresults;

static struct result *bucket(const char *file)
{
    int i;
    for (i = 0; i < nresults; i++)
        if (strcmp(results[i].file, file) == 0)
            return &results[i];
    snprintf(results[nresults].file, sizeof results[0].file, "%s", file);
    return &results[nresults++];
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
    while (fgets(line, sizeof line, f))
        if (line[0] != '#' && sscanf(line, "%lf %lf %lf %lf", &pamb, &rq, &ig, &expected) == 4)
            check("alveolar.tsv", expected, ventilation(pamb, rq, ig), TOL, "palv");
    fclose(f);
}

static void check_haldane(void)
{
    FILE *f = open_vectors("haldane.tsv");
    char line[MAXLINE];
    double pt0, palv, t, ht, expected;
    while (fgets(line, sizeof line, f))
        if (line[0] != '#' && sscanf(line, "%lf %lf %lf %lf %lf", &pt0, &palv, &t, &ht, &expected) == 5)
            check("haldane.tsv", expected, haldane(pt0, palv, t, ht), TOL, "pt");
    fclose(f);
}

static void check_schreiner(void)
{
    FILE *f = open_vectors("schreiner.tsv");
    char line[MAXLINE];
    double pt0, palv0, rate, t, ht, expected;
    while (fgets(line, sizeof line, f))
        if (line[0] != '#' &&
            sscanf(line, "%lf %lf %lf %lf %lf %lf", &pt0, &palv0, &rate, &t, &ht, &expected) == 6)
            check("schreiner.tsv", expected, schreiner(pt0, palv0, rate, t, ht), TOL, "pt");
    fclose(f);
}

static void check_ceiling(void)
{
    FILE *f = open_vectors("ceiling.tsv");
    char line[MAXLINE], tname[32];
    int idx, n;
    double n2_p, he_p, expected;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#') continue;
        if (sscanf(line, "%31s %d %lf %lf %lf", tname, &idx, &n2_p, &he_p, &expected) != 5)
            continue;
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
        if (line[0] == '#') continue;
        if (sscanf(line, "%31s %d %lf %lf %lf", tname, &idx, &pt, &gf, &expected) != 5)
            continue;
        const struct compartment_constants *t = table_of(tname, &n);
        if (!t || idx >= n) continue;
        /* There is no GF-aware entry point in the library. At gf == 1 the
           tolerated pressure is exactly the raw ceiling, so those rows can be
           checked against getCeiling(); the rest cannot be checked at all
           until the primitive exists. */
        if (fabs(gf - 1.0) > 1e-12) {
            bucket("gradient-tolerance.tsv")->checked++;
            bucket("gradient-tolerance.tsv")->failed++;
            continue;
        }
        struct compartment_state s;
        s.n2_p = pt; s.he_p = 0.0;
        check("gradient-tolerance.tsv", expected, getCeiling(&t[idx], &s), TOL, "P_tol(gf=1)");
    }
    fclose(f);
}

static void check_ndl(void)
{
    FILE *f = open_vectors("ndl.tsv");
    char line[MAXLINE], tname[32];
    int n, i;
    double pamb, n2r, her, expected;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#') continue;
        if (sscanf(line, "%31s %lf %lf %lf %lf", tname, &pamb, &n2r, &her, &expected) != 5)
            continue;
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
        check("ndl.tsv", expected, best, 0.5, "min NDL");
    }
    fclose(f);
}

static void check_zh_l16_derivation(void)
{
    FILE *f = open_vectors("zh-l16-derivation.tsv");
    char line[MAXLINE];
    int idx;
    double ht, a_formula, b_formula;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#') continue;
        if (sscanf(line, "%d %lf %lf %lf", &idx, &ht, &a_formula, &b_formula) != 4)
            continue;
        if (idx >= ZH_L16_NR_COMPARTMENTS) continue;
        /* 1e-4, not tighter: the tables store `f`-suffixed literals in double
           fields, so every value is float-rounded then widened (38.3 reads
           back as 38.2999992371). That is its own cleanup, tracked separately;
           conflating it with wrong coefficients would muddle the counts. */
        check("zh-l16-derivation.tsv", ht, zh_l16C[idx].n2_h, 1e-4, "half-time");
        check("zh-l16-derivation.tsv", a_formula, zh_l16C[idx].n2_a, 1e-3, "a from formula");
        /* Compartments 4 and 5 deviate from the b formula in the published
           table itself; the generator's header records the values. */
        if (idx != 4 && idx != 5)
            check("zh-l16-derivation.tsv", b_formula, zh_l16C[idx].n2_b, 1e-3, "b from formula");
    }
    fclose(f);
}

/* ------------------------------------------------------- expected failures */

struct allowance { char file[64]; int allowed; char reason[160]; };
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
        a = &allowances[nallowances];
        if (sscanf(line, "%63s %d %159[^\n]", a->file, &a->allowed, a->reason) == 3)
            nallowances++;
    }
    fclose(f);
}

static int allowed_for(const char *file, const char **reason)
{
    int i;
    for (i = 0; i < nallowances; i++)
        if (strcmp(allowances[i].file, file) == 0) {
            *reason = allowances[i].reason;
            return allowances[i].allowed;
        }
    *reason = NULL;
    return 0;
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
    check_ndl();
    check_zh_l16_derivation();

    printf("\n%-26s %8s %8s %8s   %s\n", "file", "checked", "failed", "allowed", "verdict");
    for (i = 0; i < nresults; i++) {
        const char *reason = NULL;
        int allowed = allowed_for(results[i].file, &reason);
        const char *verdict;

        total  += results[i].checked;
        failed += results[i].failed;

        if (results[i].failed == 0 && allowed == 0) {
            verdict = "ok";
        } else if (results[i].failed == allowed) {
            verdict = "known deviation";
        } else if (results[i].failed > allowed) {
            verdict = "REGRESSION"; status = 1;
        } else {
            verdict = "FIXED - update expected-failures.txt"; status = 1;
        }
        printf("%-26s %8d %8d %8d   %s\n",
               results[i].file, results[i].checked, results[i].failed, allowed, verdict);
        if (reason && results[i].failed)
            printf("%-26s %26s   %s\n", "", "", reason);
    }

    printf("\n%d vectors checked, %d deviating\n", total, failed);
    printf("%s\n", status ? "=== FAIL ===" : "=== OK ===");
    return status;
}
