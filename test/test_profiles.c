/* Drive real and synthetic dive profiles through libbuhlmann and pin the
 * resulting tissue state.
 *
 * This is a REGRESSION check, not a conformance check: the expected values in
 * profiles/expected.tsv are recorded from this library and therefore include
 * its known defects. Its job is to make change visible. What the model SHOULD
 * do is in vectors/ — see vectors/README.md for why the two are separate.
 *
 * Unlike the shell baseline it replaces, this calls the library directly. It
 * does not run src/dive, does not shell out to Python, and does not depend on
 * the 36-field stdio format, so a failure points at a compartment rather than
 * at "something in three layers". It also runs every profile against BOTH
 * constant tables, including the ZH-L16C that the demo never loads.
 *
 *   ./test_profiles            check against profiles/expected.tsv
 *   ./test_profiles --record   regenerate it (deliberately, alongside an
 *                              approved model change)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>

#include <buhlmann.h>

#define MAXSTEPS   8192
#define MAXPROF     128
#define TOL        1e-9

struct sample { double t, p, o2, he; };

struct summary {
    int    steps;
    double max_ceiling;     /* deepest ceiling seen over the whole profile */
    double min_ndl;         /* tightest NDL seen over the whole profile    */
    double n2_first, he_first;
    double n2_mid,   he_mid;
    double n2_last,  he_last;
    double n2_sum,   he_sum;   /* catches drift in any compartment at all */
};

static char profile_dir[256] = "profiles";

static int load_profile(const char *path, struct sample *s, int max)
{
    char line[256];
    int n = 0;
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (n >= max) {          /* truncating and then RECORDING the truncation
                                    would bake a short profile into the baseline */
            fprintf(stderr, "  %s: more than %d samples; raise MAXSTEPS\n", path, max);
            fclose(f);
            exit(2);
        }
        if (sscanf(line, "%lf %lf %lf %lf",
                   &s[n].t, &s[n].p, &s[n].o2, &s[n].he) != 4) {
            fprintf(stderr, "  %s:%d: unparseable sample\n", path, n + 1);
            fclose(f);
            exit(2);
        }
        n++;
    }
    fclose(f);
    return n;
}

/* Replay a profile against one constant table. Mirrors what a caller of this
   library must do today, which is itself the argument for the missing
   primitives — see .okf/domain/ascent-and-stop-scheduling.md */
static struct summary replay(const struct compartment_constants *tbl, int ncomp,
                             const struct sample *s, int nsteps)
{
    struct compartment_state st[ZH_L16_NR_COMPARTMENTS];
    struct summary out;
    double lastt, lastp;
    int i, k;

    memset(&out, 0, sizeof out);
    out.steps = nsteps;
    out.max_ceiling = -1e9;
    out.min_ndl = 1e9;

    for (i = 0; i < ncomp; i++) {
        st[i].he_p = ventilation(1.0, BUHLMANN_RQ, 0.0);
        st[i].n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);
    }
    lastt = s[0].t;
    lastp = s[0].p;

    for (k = 0; k < nsteps; k++) {
        double dt = s[k].t - lastt;
        double dp = s[k].p - lastp;
        double n2r = 1.0 - s[k].o2 - s[k].he;
        double ceiling = -1e9, ndl = 1e9;

        if (dt < 0.0) continue;

        for (i = 0; i < ncomp; i++) {
            double c, d;
            if (dt > 0.0)
                compartment_descend(&tbl[i], &st[i], &st[i], lastp,
                                    dp / dt, dt, BUHLMANN_RQ, n2r, s[k].he);
            c = getCeiling(&tbl[i], &st[i]);
            if (c > ceiling) ceiling = c;
            /* Evaluate the NDL at the depth actually reached, not the one just
               left — see .okf/findings/dive-passes-wrong-pressure-to-ndl.md */
            d = nodecotime(&tbl[i], &st[i], s[k].p, n2r, s[k].he);
            if (d < ndl) ndl = d;
        }
        if (ceiling > out.max_ceiling) out.max_ceiling = ceiling;
        if (ndl < out.min_ndl) out.min_ndl = ndl;

        lastt = s[k].t;
        lastp = s[k].p;
    }

    out.n2_first = st[0].n2_p;            out.he_first = st[0].he_p;
    out.n2_mid   = st[ncomp / 2].n2_p;    out.he_mid   = st[ncomp / 2].he_p;
    out.n2_last  = st[ncomp - 1].n2_p;    out.he_last  = st[ncomp - 1].he_p;
    for (i = 0; i < ncomp; i++) {
        out.n2_sum += st[i].n2_p;
        out.he_sum += st[i].he_p;
    }
    return out;
}

#define NAMELEN 256

static int by_name(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

static int collect(char names[][NAMELEN], int max)
{
    DIR *d = opendir(profile_dir);
    struct dirent *e;
    int n = 0;
    if (!d) { fprintf(stderr, "cannot open %s\n", profile_dir); exit(2); }
    while ((e = readdir(d))) {
        size_t len = strlen(e->d_name);
        if (len <= 4 || strcmp(e->d_name + len - 4, ".txt") != 0) continue;
        if (n >= max || len >= NAMELEN) {
            fprintf(stderr, "too many or too-long profile names; raise MAXPROF/NAMELEN\n");
            exit(2);
        }
        memcpy(names[n++], e->d_name, len + 1);
    }
    closedir(d);
    /* readdir order is filesystem-dependent; sort so the recorded baseline is
       reproducible across machines. */
    qsort(names, (size_t)n, NAMELEN, by_name);
    return n;
}

#define FMT "%s\t%s\t%d\t%.12g\t%.12g\t%.12g\t%.12g\t%.12g\t%.12g\t%.12g\t%.12g\t%.12g\t%.12g\n"
#define ARGS(name, tname, s) \
    name, tname, (s).steps, (s).max_ceiling, (s).min_ndl, \
    (s).n2_first, (s).he_first, (s).n2_mid, (s).he_mid, \
    (s).n2_last, (s).he_last, (s).n2_sum, (s).he_sum

struct table_ref { const char *name; const struct compartment_constants *t; int n; };

int main(int argc, char **argv)
{
    char names[MAXPROF][NAMELEN];
    char path[512], expected_path[512];
    struct sample *samples;
    int record = 0, nprof, i, k, failures = 0, checked = 0;
    FILE *out = NULL;
    struct table_ref tables[2];

    tables[0].name = "zh_l12";  tables[0].t = zh_l12;  tables[0].n = ZH_L12_NR_COMPARTMENTS;
    tables[1].name = "zh_l16C"; tables[1].t = zh_l16C; tables[1].n = ZH_L16_NR_COMPARTMENTS;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--record") == 0) record = 1;
        else snprintf(profile_dir, sizeof profile_dir, "%s", argv[i]);
    }
    if (!record && getenv("srcdir") && strcmp(profile_dir, "profiles") == 0)
        snprintf(profile_dir, sizeof profile_dir, "%s/profiles", getenv("srcdir"));

    snprintf(expected_path, sizeof expected_path, "%s/expected.tsv", profile_dir);
    samples = malloc(sizeof *samples * MAXSTEPS);
    if (!samples) return 2;

    nprof = collect(names, MAXPROF);
    printf("=== profile replay (%s): %d profiles x %d tables ===\n\n",
           profile_dir, nprof, (int)(sizeof tables / sizeof tables[0]));

    if (record) {
        out = fopen(expected_path, "w");
        if (!out) { fprintf(stderr, "cannot write %s\n", expected_path); return 2; }
        fprintf(out, "# Regression baseline: tissue state after replaying each profile.\n");
        fprintf(out, "# Recorded from this library, so it includes its known defects.\n");
        fprintf(out, "# Regenerate with ./test_profiles --record, only alongside an\n");
        fprintf(out, "# approved model change, in the same commit.\n");
        fprintf(out, "# profile\ttable\tsteps\tmax_ceiling\tmin_ndl\tn2_first\the_first"
                     "\tn2_mid\the_mid\tn2_last\the_last\tn2_sum\the_sum\n");
    }

    for (i = 0; i < nprof; i++) {
        int nsteps;
        snprintf(path, sizeof path, "%s/%s", profile_dir, names[i]);
        nsteps = load_profile(path, samples, MAXSTEPS);
        if (nsteps <= 0) continue;

        for (k = 0; k < 2; k++) {
            struct summary s = replay(tables[k].t, tables[k].n, samples, nsteps);
            if (record) {
                fprintf(out, FMT, ARGS(names[i], tables[k].name, s));
            } else {
                /* look the row up in expected.tsv */
                FILE *ef = fopen(expected_path, "r");
                char line[1024];
                int found = 0;
                if (!ef) { fprintf(stderr, "no %s; run --record\n", expected_path); return 2; }
                while (fgets(line, sizeof line, ef)) {
                    char pn[NAMELEN], tn[32];
                    struct summary e;
                    if (line[0] == '#') continue;
                    if (sscanf(line, "%255s %31s %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
                               pn, tn, &e.steps, &e.max_ceiling, &e.min_ndl,
                               &e.n2_first, &e.he_first, &e.n2_mid, &e.he_mid,
                               &e.n2_last, &e.he_last, &e.n2_sum, &e.he_sum) != 13)
                        continue;
                    if (strcmp(pn, names[i]) || strcmp(tn, tables[k].name)) continue;
                    found = 1;
                    checked++;
                    #define CMP(field) \
                        if (fabs(e.field - s.field) > TOL) { \
                            if (failures < 10) \
                                fprintf(stderr, "  %s [%s] %s: expected %.12g got %.12g\n", \
                                        names[i], tables[k].name, #field, e.field, s.field); \
                            failures++; \
                        }
                    CMP(max_ceiling) CMP(min_ndl)
                    CMP(n2_first) CMP(he_first) CMP(n2_mid) CMP(he_mid)
                    CMP(n2_last)  CMP(he_last)  CMP(n2_sum) CMP(he_sum)
                    #undef CMP
                    if (e.steps != s.steps) {
                        fprintf(stderr, "  %s [%s] steps: expected %d got %d\n",
                                names[i], tables[k].name, e.steps, s.steps);
                        failures++;
                    }
                    break;
                }
                fclose(ef);
                if (!found) {
                    fprintf(stderr, "  %s [%s]: no recorded baseline\n",
                            names[i], tables[k].name);
                    failures++;
                }
            }
        }
    }

    free(samples);
    if (record) {
        fclose(out);
        printf("recorded %d profiles x 2 tables to %s\n", nprof, expected_path);
        printf("commit this together with the change that made it necessary\n");
        return 0;
    }

    printf("%d comparisons, %d deviating\n", checked * 11, failures);
    printf("%s\n", failures ? "=== MODEL OUTPUT CHANGED ===" : "=== unchanged ===");
    if (failures)
        printf("\nIf intended, re-run with --record and commit the result alongside\n"
               "the change that caused it. If not, this is a regression.\n");
    return failures ? 1 : 0;
}
