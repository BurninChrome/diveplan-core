---
type: Bug Report
title: visoutput.py drops the last helium column and mis-renders the NDL
description: An asymmetric slice bound loses compartment 16's helium, and the decimal-minutes-to-seconds conversion multiplies by 100 instead of 60.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/tools/visoutput.py
tags: [bug, tooling, visualisation, unfixed]
severity: low
status: reported-awaiting-approval
timestamp: '2026-07-25T09:30:00Z'
---

Both defects are in [`tools/visoutput.py`](/tooling/visoutput.md), a display
tool outside the [algorithm integrity policy](/decisions/algorithm-integrity-policy.md).
They are written up rather than fixed only to keep this bundle's authoring pass
read-only.

# 1. The last helium column is dropped

```python
for i in range(2, len(toks)-2, 2):   # N2  → 16 values ✓
    histline.append(toks[i])
...
for i in range(3, len(toks)-3, 2):   # He  → 15 values ✗
    histline.append(toks[i])
```

With the 36-field [output format](/interfaces/dive-stdio-format.md):

```
N2 indices: 2, 4, …, 32   → 16 values
He indices: 3, 5, …, 31   → 15 values     index 33 never read
```

The helium bound is `-3` where it should be `-2`. Compartment 16's helium
loading is silently discarded from `compHe`, and from the `maxpressure`
calculation that scales the y-axis.

**Fix:** change `range(3, len(toks)-3, 2)` to `range(3, len(toks)-2, 2)`.

Currently invisible, because the helium bar chart is commented out (line 74) and
`compHe` is never plotted. It becomes a live off-by-one the moment anyone
uncomments it — which is the natural thing to do when investigating trimix, and
the resulting chart would be short one bar with no error.

# 2. Decimal minutes rendered as if sexagesimal

```python
mins = int(nodectime[time])
secs = int((nodectime[time] - mins) * 100)      # ← should be * 60
if secs >= 60:
    mins = mins + 1
    secs = secs - 60
```

`nodectime` is decimal minutes. Multiplying the fractional part by 100 treats
`.5` as 50 seconds rather than 30.

| Actual NDL | Displayed | Correct |
|---|---|---|
| 7.50 min | 7 m 50 s | 7 m 30 s |
| 7.70 min | 8 m 10 s | 7 m 42 s |
| 7.99 min | 8 m 39 s | 7 m 59 s |

The `if secs >= 60` block is a symptom, not a fix: it exists only to mop up the
overflow the ×100 creates, and it does so by rolling into the next minute, so
the displayed value can exceed the true one by nearly 40 seconds.

**Fix:** `secs = int(round((nodectime[time] - mins) * 60))`, after which the
overflow block can go — or be kept to handle `round()` landing on 60.

Compounding: the underlying value is itself
[1.5–1.7× too large](/findings/nodecotime-overestimates-ndl.md), so the
displayed NDL is wrong twice over, both times in the optimistic direction.

# 3. Minor — the label unit is ambiguous

```python
ax.text(2, 6, "NoDec   : " + "{0} m {1} s".format(mins, secs), fontsize=15)
```

`m` means minutes here, directly below `"Ceiling : %.1f m"` where `m` means
metres. Suggest `min`.

# 4. Minor — `maxpressure` compares strings

```python
histline.append(toks[i])                       # str, not float
if (float(max(histline)) > maxpressure):       # max() over strings
```

`max()` on a list of strings is lexicographic: `"0.9" > "10.0"`. It works today
only because tissue pressures stay single-digit with consistent `%lf`
formatting. A compartment above 10 bar — reachable below about 90 m — would
scale the y-axis to the wrong value.

**Fix:** append `float(toks[i])` and drop the inner `float()` calls.

# Recommendation

Fix 1 and 2 together; they are four lines and both are wrong in ways a user
would trust. 3 and 4 are cosmetic and can ride along. None of it affects the C
library or the [output contract](/interfaces/dive-stdio-format.md).
