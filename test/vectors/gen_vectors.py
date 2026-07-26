#!/usr/bin/env python3
"""Generate conformance vectors for libbuhlmann from decompression theory.

This script is an INDEPENDENT implementation of the Buhlmann equations. It must
never import from, link against, or shell out to libbuhlmann — the whole point
of the vectors is that they are derived from the published formulas rather than
from the code they check. Expected values here say how the model SHOULD behave.

Where the current library deviates, the vector is still written with the
correct value and the deviation is listed in expected-failures.txt, so that
fixing a defect means deleting a line from that file rather than editing a
vector.

Run:  python3 test/vectors/gen_vectors.py
"""
import math
import pathlib
import re

HERE = pathlib.Path(__file__).resolve().parent
SRC = HERE.parent.parent / "src"

LN2 = math.log(2.0)
WATER_VAPOUR = 0.0627   # bar, saturated at 37 C (47 mmHg)
CO2 = 0.0534            # bar, alveolar (40 mmHg)
AIR_N2 = 0.78084        # dry atmospheric nitrogen fraction


# --------------------------------------------------------------------------
# The model, from the published equations. No library code involved.
# --------------------------------------------------------------------------

def ventilation(pamb, rq, ig_ratio):
    """Alveolar inert gas pressure (Buhlmann/Schreiner alveolar equation)."""
    return (pamb - WATER_VAPOUR + ((1.0 - rq) / rq) * CO2) * ig_ratio


def haldane(pt0, palv, t, half_time):
    """Constant-pressure loading: exact solution of dP/dt = k(Palv - P)."""
    k = LN2 / half_time
    return pt0 + (palv - pt0) * (1.0 - math.exp(-k * t))


def schreiner(pt0, palv0, rate, t, half_time):
    """Linear-ramp loading: exact solution with Palv(t) = palv0 + rate*t."""
    k = LN2 / half_time
    return palv0 + rate * (t - 1.0 / k) - (palv0 - pt0 - rate / k) * math.exp(-k * t)


def ceiling_weighted(n2_p, he_p, c):
    """Buhlmann combined-gas ceiling: blend a and b by partial-pressure share.

    This is the correct rule. The library's getCeiling() instead evaluates each
    gas independently and takes the maximum, which diverges on mixed loads.
    """
    total = n2_p + he_p
    if total <= 0.0:
        # No dissolved inert gas: fall back to the single-gas sentinel so the
        # value stays defined rather than 0/0.
        return max(-c["n2_a"] * c["n2_b"], -c["he_a"] * c["he_b"])
    a = (n2_p * c["n2_a"] + he_p * c["he_a"]) / total
    b = (n2_p * c["n2_b"] + he_p * c["he_b"]) / total
    return (total - a) * b


def tolerated_pressure(pt, a, b, gf):
    """Ambient pressure tolerated at gradient factor gf.

    P_tol(P_amb) = P_amb + gf*(M(P_amb) - P_amb),  M(P_amb) = a + P_amb/b
    inverted for P_amb.  At gf = 1 this reduces to (pt - a)*b.
    """
    return (pt - gf * a) / (1.0 - gf + gf / b)


def ndl_reference(c, pamb, n2_ratio, he_ratio, cap=100.0):
    """No-decompression limit by bisection on the ceiling crossing.

    Independent of the library's nodecotime(), which uses a geometric probe.
    """
    start = (ventilation(1.0, 1.0, AIR_N2), 0.0)   # (n2, he) at surface

    def ceiling_after(t):
        n2 = haldane(start[0], ventilation(pamb, 1.0, n2_ratio), t, c["n2_h"])
        he = haldane(start[1], ventilation(pamb, 1.0, he_ratio), t, c["he_h"])
        return ceiling_weighted(n2, he, c)

    if ceiling_after(cap) <= 1.0:
        return cap
    lo, hi = 0.0, cap
    for _ in range(200):
        mid = 0.5 * (lo + hi)
        if ceiling_after(mid) > 1.0:
            hi = mid
        else:
            lo = mid
    return lo


# --------------------------------------------------------------------------
# Constant tables, parsed from source so the vectors track the real values.
# Parsing (rather than retyping) is deliberate: a transcription error here
# would silently agree with itself.
# --------------------------------------------------------------------------

def parse_table(path, symbol, size):
    text = (SRC / path).read_text()
    start = text.index(symbol + "[")
    end = text.index("};", start)
    rows = []
    for line in text[start:end].split("\n"):
        s = line.strip()
        if not s.startswith("{"):
            continue
        nums = re.findall(r"\d+\.?\d*f?", s[: s.index("}")])
        vals = [float(x.rstrip("f")) for x in nums[:6]]
        rows.append(dict(zip(("n2_h", "n2_a", "n2_b", "he_h", "he_a", "he_b"), vals)))
    while len(rows) < size:                      # C zero-fills short initialisers
        rows.append(dict.fromkeys(("n2_h", "n2_a", "n2_b", "he_h", "he_a", "he_b"), 0.0))
    return rows


def write(name, header, rows, note):
    out = [f"# {note}", f"# {header}"]
    out += ["\t".join(f"{v:.10g}" if isinstance(v, float) else str(v) for v in r) for r in rows]
    (HERE / name).write_text("\n".join(out) + "\n")
    print(f"  {name:22} {len(rows):4d} vectors")


def main():
    zh_l12 = parse_table("zh-l12.c", "zh_l12", 16)
    zh_l16c = parse_table("zh-l16.c", "zh_l16C", 17)
    tables = {"zh_l12": zh_l12, "zh_l16C": zh_l16c}
    print("generating conformance vectors from theory")

    # -- alveolar ----------------------------------------------------------
    rows = []
    for pamb in (1.0, 2.0, 3.0, 4.0, 6.0, 7.0):
        for rq in (1.0, 0.9, 0.8):
            for f in (AIR_N2, 0.79, 0.44, 0.35, 0.0):
                rows.append((pamb, rq, f, ventilation(pamb, rq, f)))
    write("alveolar.tsv", "pamb\trq\tig_ratio\texpected_palv", rows,
          "ventilation(): palv = (pamb - 0.0627 + ((1-rq)/rq)*0.0534) * ig_ratio")

    # -- haldane -----------------------------------------------------------
    rows = []
    for ht in (4.0, 12.5, 77.0, 635.0):
        for pt0, palv in ((0.7319, 2.29), (2.0, 0.7319), (0.5, 3.0)):
            for t in (0.0, 1.0, ht, 2 * ht, 3 * ht, 600.0):
                rows.append((pt0, palv, t, ht, haldane(pt0, palv, t, ht)))
    write("haldane.tsv", "pt0\tpalv\tt\thalf_time\texpected_pt", rows,
          "haldane(): pt = pt0 + (palv-pt0)*(1-exp(-ln2/half_time * t))")

    # -- schreiner ---------------------------------------------------------
    rows = []
    for ht in (4.0, 12.5, 77.0, 635.0):
        for rate in (0.0, 0.234, -0.156, 1.5618, -0.3):
            for t in (0.0, 1.0, 5.0, 20.0):
                pt0, palv0 = 0.7319, 0.7319
                rows.append((pt0, palv0, rate, t, ht, schreiner(pt0, palv0, rate, t, ht)))
    write("schreiner.tsv", "pt0\tpalv0\trate\tt\thalf_time\texpected_pt", rows,
          "schreiner(): pt = palv0 + r*(t-1/k) - (palv0-pt0-r/k)*exp(-k*t), k=ln2/half_time")

    # -- ceiling (correct combined-gas rule) -------------------------------
    rows = []
    for tname, tbl in tables.items():
        for idx in (0, 3, 8, len(tbl) - 1):
            for n2_p, he_p in ((2.0, 0.0), (1.5, 0.5), (1.0, 1.0), (0.5, 1.5), (0.0, 2.0), (3.0, 0.0)):
                rows.append((tname, idx, n2_p, he_p, ceiling_weighted(n2_p, he_p, tbl[idx])))
    write("ceiling.tsv", "table\tcompartment\tn2_p\the_p\texpected_ceiling", rows,
          "Buhlmann combined-gas ceiling: a and b blended by partial-pressure share")

    # -- gradient-factor tolerance ----------------------------------------
    rows = []
    for tname, tbl in tables.items():
        for idx in (0, 3, 8):
            c = tbl[idx]
            for pt in (1.5, 2.6, 4.0):
                for gf in (1.0, 0.85, 0.70, 0.55, 0.40, 0.30):
                    rows.append((tname, idx, pt, gf,
                                 tolerated_pressure(pt, c["n2_a"], c["n2_b"], gf)))
    write("gradient-tolerance.tsv", "table\tcompartment\tpt\tgf\texpected_tolerated_pamb", rows,
          "P_tol = (pt - gf*a) / (1 - gf + gf/b); at gf=1 equals (pt-a)*b")

    # -- NDL ---------------------------------------------------------------
    rows = []
    for tname, tbl in tables.items():
        for pamb in (2.0, 3.0, 4.0, 5.0, 6.0, 7.0):
            best = min(ndl_reference(c, pamb, AIR_N2, 0.0) for c in tbl if c["n2_h"] > 0)
            rows.append((tname, pamb, AIR_N2, 0.0, best))
        for pamb in (4.0, 6.0, 7.0):
            best = min(ndl_reference(c, pamb, 0.44, 0.35) for c in tbl if c["n2_h"] > 0)
            rows.append((tname, pamb, 0.44, 0.35, best))
    write("ndl.tsv", "table\tpamb\tn2_ratio\the_ratio\texpected_ndl_min", rows,
          "NDL by bisection on the ceiling crossing, minimum over compartments, capped at 100")

    # -- ZH-L16A coefficients against Buhlmann's derivation ----------------
    rows = []
    for i, c in enumerate(zh_l16c):
        t = c["n2_h"]
        if t <= 0:
            continue
        rows.append((i, t, 2.0 * t ** (-1.0 / 3.0), 1.005 - t ** -0.5))
    write("zh-l16-derivation.tsv", "compartment\tn2_half_time\ta_from_formula\tb_from_formula", rows,
          "Buhlmann derivation: a = 2/cbrt(t), b = 1.005 - 1/sqrt(t). Published b for "
          "compartments 4 and 5 deviates (0.7825 vs 0.7725, 0.8126 vs 0.8125).")


if __name__ == "__main__":
    main()
