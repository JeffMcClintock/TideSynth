# X4 — why the `GIT_TAG origin/main` pins stay (WONTFIX)

Status **WONTFIX**, platform **any**. Lifted verbatim out of the
[BACKLOG.md](../BACKLOG.md) row by **A8**, 2026-08-12, when that file had
reached 76 KB and every run on three machines was reading all of it. The row
now carries the decision-shaped summary and points here; this file is the
detail. Wording below is unchanged from the row — only the line breaks are
new.

---

**Closed 2026-08-08 by Jeff: leave the `GIT_TAG`s alone. `FetchContent` is not
how these are meant to be developed against, so freezing them is not the
failure mode it looks like.** Do not re-file this. The reasoning, because it is
not obvious from the CMake: **the SynthEdit-family dependencies are not
third-party libraries that occasionally need a version bump — they implement a
large part of the application's own functionality and change daily.** The
intended local workflow is therefore to bypass `FetchContent` entirely and
point CMake at working copies you can edit, push and pull as you go. That is
what `SYNTHEDITLIB_FOLDER_OVERRIDE`, `GMPI_SDK_FOLDER_OVERRIDE`,
`GMPI_UI_FOLDER_OVERRIDE` and `GMPI_WRAPPER_FOLDER_OVERRIDE` are *for*; they
are the normal path on a dev box, not an optimisation. CI is the other case,
and it clones fresh, so `origin/main` there resolves to real current `main`.
Between the two there is no gap worth pinning six shared dependencies to close.
**The dependencies split cleanly, which is why this works:** the four that
change daily all have an override; the three that do not — `AudioUnitSDK`
(`SE16/CMakeLists.txt:184`), `clap` (`:200`), `clap-helpers` — are genuine
third-party SDKs that are stable enough for a frozen checkout to be harmless.
**So X3 was not really a freeze bug — it was a *missing override*.**
`SynthEditSem/CMakeLists.txt` had `GMPI_UI_FOLDER_OVERRIDE` set to the local
repo while `GMPI_WRAPPER_FOLDER_OVERRIDE` was left blank, so one sibling came
from disk and the other from a stale cached clone. **That asymmetry is the
thing to watch for, and CMake already announces it:** every one of these prints
either `Using local <X> folder` or `Fetching <X> from github` at configure
time. Read that banner — an unexpected "Fetching" on a SynthEdit-family repo
means you are building against whatever `main` looked like the first time this
tree was configured. If a build behaves impossibly and the source looks right,
confirm with `git log -1` in `build/_deps/<dep>-src` before believing your
tree. Original finding kept below for reference. **The `GIT_TAG origin/main`
pattern is in the shared `SE16/CMakeLists.txt`, six times** — `SynthEditLib`
(:120), `GMPI` (:137), `gmpi_ui` (:154), `AudioUnitSDK` (:189), `clap` (:205),
`clap-helpers` (:214). `origin/main` is a remote-tracking ref that already
resolves inside the cached FetchContent clone, so CMake's update step never
fetches and each checkout freezes at whatever `main` pointed to on the *first*
configure — permanently and invisibly. That is precisely how **X3** shipped a
Linux VST3 no host could load. The first three are masked on the dev boxes by
`*_FOLDER_OVERRIDE`; **the CLAP pair is masked by nothing and X1 needs CLAP**.
Pin explicit shas as X3 did for `GMPI_Wrappers`. `SE16/CMakeLists.txt` is a
**shared build file (GATED)** — it configures SynthEdit, SynthEditCL and the
mac/iOS targets, not just TIDE, so bumping six dependencies at once needs a box
that can rebuild those. Best done by Windows or macOS.
