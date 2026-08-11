# N1 — carrying the TIDE Rack rename through the tree

Status **TODO**, platform **any**. Lifted verbatim out of the
[BACKLOG.md](../BACKLOG.md) row by **A8**, 2026-08-12, when that file had
reached 76 KB and every run on three machines was reading all of it. The row
now carries the decision-shaped summary and points here; this file is the
detail. Wording below is unchanged from the row — only the line breaks are
new.

---

**Carry the TIDE Rack rename through the tree — PLAN naming ruling, decided
2026-08-08, reaffirmed by Jeff 2026-08-09.** The product is **TIDE Rack**;
**TIDE Synth is the organisation**, which may ship more than one plugin. The
domain stays `tidesynth.com` and so does the repo name. **The prose half is
DONE:** the website led with TIDE Rack from 2026-08-08, and `README.md` was
corrected 2026-08-09 — it had been titled "TIDE Synth" and describing the
product under it. Both now state the org/product relationship once, up front,
rather than in a footer. **What remains is the build-system half, and it is the
part that needs decisions rather than edits** — see (a) below. **Both names are
still in the tree in code and paths, and that is expected until that lands.**
Probably wants splitting once someone costs it — the surfaces are not equally
cheap. Cheap and safe: the docs under `docs/`, `README`, and the website copy
(the page is now the *organisation's* site carrying one product, which is a
small restructure, not a find-and-replace). Not cheap, and each needs a
decision before any edit: **(a)** the CMake targets `TIDE` / `TIDE_VST3` and
the artifacts they emit. The *form* is settled by (b)'s convention, and the two
halves differ: **targets stay underscored** (`TIDE_Rack` / `TIDE_Rack_VST3`)
because `SynthEditSem/CMakeLists.txt:90` builds them as
`${PROJECT_NAME}_${kind}`, so a dashed project name yields the mixed
`TIDE-Rack_VST3` in the one place you cannot avoid it; **the artifacts they
emit are dashed** (`TIDE-Rack.vst3`), via `OUTPUT_NAME`. Check where `:45`'s
`PROJECT_NAME` lands inside `gmpi_plugin.cmake` first — it likely feeds the
bundle name and possibly an Info.plist identifier, so `OUTPUT_NAME` alone may
not be enough. What is not settled is *when*: renaming churns every build doc,
every path in JOURNAL, and P4c's regression-test invocation, and the carve-out
is moving the same build files. **Do it after C7, not during** — C2–C7 already
touch `EditorLib/CMakeLists.txt` and a rename mid-carve-out doubles the
conflict surface for no benefit, since nothing has shipped under either name;
~~**(b)** the release asset names~~ — **answered 2026-08-08 and already applied
to [docs/distribution.md](distribution.md): three forms — display
`TIDE Rack` (space) for anything a person reads, shipped files `TIDE-Rack`
(dash) for anything with a path or URL, CMake targets `TIDE_Rack` (underscore,
internal only).** So `TIDE-Rack-Windows.exe`, `TIDE-Rack-macOS.pkg`,
`TIDE-Rack-Linux.tar.gz`, `TIDE-Rack.vst3`. **No spaces in any filename** is
the rule the other two serve: a space is `%20` in every
`releases/latest/download/` permalink forever, and R6's whole design is that
those permalinks never change, so **a space in a shipped filename cannot be
fixed later** — hence settling this before R2–R6 rather than during. Dash over
underscore for shipped files was a second pass on the same day: both stop
spaces equally, but these names become *visible link text* on the no-JS page
and underscores vanish under link underlining. **Never mix them in one
filename;** **(c)** the repo name `TideSynth`, which under the new split is
arguably *correct* as the organisation's repo — note this is the second naming
question answered "keep it", after `SynthEditLib`; **(d)** the Ko-fi handle is
already `TideRack` and the domain is already `tidesynth.com`, so the split is
live in public whether or not the code follows. **Do not start with a global
search-and-replace.** "TIDE" appears as a product name, an organisation name, a
CMake target, a 4-char vendor code (`TideApp::getVendor4charCode()` returns
`"TIDE"` — that one is a fixed-width field and must not grow) and a filename
prefix; they do not all move together. **Depends on P5** or supersedes it — see
that row.
