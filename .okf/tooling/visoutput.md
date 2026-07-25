---
type: Tool
title: tools/visoutput.py — interactive visualiser
description: Matplotlib viewer for dive output — depth profile with ceiling overlay and a time-scrubbable per-compartment loading histogram.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/tools/visoutput.py
tags: [tooling, python, matplotlib, visualisation]
timestamp: '2026-07-25T09:30:00Z'
---

# Usage

```bash
cd tools && python3 -m venv venv && . venv/bin/activate
pip install -r requirements.txt          # matplotlib>=3.5, numpy>=1.21

python3 test/gen_dive.py -d 20 -t 5 | src/dive | python3 tools/visoutput.py
```

Exits immediately if stdin is a TTY, so it is pipeline-only. Requires a display;
there is no headless/save-to-file mode.

# What it draws

Left panel — depth against time, y-axis inverted, with the ceiling overlaid.
Ceilings at or below 1.0 bar are clamped to 0 so no-obligation stretches sit
flat on the surface line. A red marker tracks the scrubber position, and two
text overlays show the ceiling and NDL at that instant.

Right panel — a 16-bar histogram of nitrogen loading per compartment at the
scrubbed time.

A `Slider` along the bottom scrubs through samples. `update()` clears and
redraws both axes on every movement, which is why scrubbing a long profile
feels sluggish.

# Reads the output format positionally

See [the stdio contract](/interfaces/dive-stdio-format.md). It splits on a
single space, takes index 0 as time, index 1 as pressure, `len-2` as ceiling and
`len-1` as NDL, and slices the middle for compartments. **Any change to the
field layout breaks it silently** — a shifted index produces a plausible-looking
wrong plot, not an exception.

# Known defects

Four, written up in full with fixes in
[the finding](/findings/visoutput-he-column.md):

- The **last helium column is dropped** — an asymmetric slice bound reads 15
  helium values where nitrogen reads 16.
- The **NDL display multiplies the fractional minute by 100 instead of 60**, so
  7.5 minutes renders as "7 m 50 s".
- The label unit `m` means minutes, directly below a ceiling label where it
  means metres.
- **`maxpressure` compares unparsed strings**, which is lexicographic and would
  break above 10 bar.

Two further limitations that are design gaps rather than bugs:

**16 compartments are hard-coded** in `compList` and `np.arange(16)`. A switch
to [ZH-L16C](/findings/dive-uses-zh-l12-not-zh-l16c.md) would need this changed
to 17.

**Helium is never plotted.** The `compHe` series is built (short by one) and the
bar call is commented out. There is no way to see helium loading, which makes
the tool much less useful for the trimix cases the library supports — and it is
why the dropped column has gone unnoticed.

# `tools/test.py`

An unrelated matplotlib smoke test — plots a sine wave to `test.png`. It
verifies the virtualenv works and nothing more. Not part of any test suite.
