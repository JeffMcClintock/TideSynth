# U1 — what TIDE's UI actually is today, measured

BACKLOG **U1**. Produced 2026-08-16 on the macOS box, against `SE16` at
`origin/master` and a Release build of the `TIDE` target.

U1's own first instruction is *"re-measure against current
`TideApp`/`SynthEditSem` state before doing anything else here"*, because the
findings in its row predate the **rack-mode pivot** of 2026-08-13. This is that
re-measure. **It is an audit only — nothing here changes behaviour.**

---

## The headline

**Rack mode is not started, and it is also not a from-scratch build.** Both
halves are measured, and the second is the useful one:

| | state | evidence |
|---|---|---|
| TIDE's default view | **structure view**, as before the pivot | `TideApp.cpp:63` |
| A panel/rack renderer | **exists**, in the public repo TIDE already links | `ContainerView.h:15` |
| …compiled? | **yes** — 13 symbols in `ContainerView.o` | `nm` on the archive member |
| …linked into TIDE? | **no** — 0 symbols in the shipped binary | `nm` on `TIDE.gmpi` |

So the gap between today and constraint 1's rack view is **not "write a panel
renderer"**. The renderer is written, compiled, and sitting in
`libSynthEditLib.a`. Nothing constructs it, so the linker never extracts it.

---

## 1. TIDE opens the structure view

`TideApp::OpenView` builds a `ContainerViewStruct` and hands the presenter the
structure-view flag:

```cpp
// SE16/SynthEditSem/TideApp.cpp:37-69
SE2::ContainerViewStruct* TideApp::OpenView(gmpi::api::IUnknown* host)
    auto* viewOb = new SE2::ContainerViewStruct( ... );
    const int view_flag = CF_STRUCTURE_VIEW;
    auto presenter = new MfcDocPresenter(Document()->MasterContainer, view_flag);
    mod->offsetViewObRect(CF_STRUCTURE_VIEW, 24, 0);
```

There is no branch, no setting, and no second code path. **The pivot changed
PLAN; it has not yet changed a line of TIDE.** That is expected — the ruling is
three days old — but it means [docs/design-notes.md](design-notes.md)'s
"one-view UX" section and [state-of-the-prototype.md](state-of-the-prototype.md)
§6 describe the *current* code accurately even though both are labelled stale
relative to the pivot. Their descriptions are right; only their *aspiration* is
out of date.

## 2. The panel view exists, and is closer than the row assumes

`SynthEditLib` — the **public** repo, already on TIDE's link line — defines a
sibling to the structure view:

```cpp
// SynthEditLib/modules/se_sdk3_hosting/ContainerView.h:15,24
class ContainerViewPanel : public TopView
    ... getViewType() { return CF_PANEL_VIEW; }
```

Same base class (`TopView`) as `ContainerViewStruct`. `CF_PANEL_VIEW` is a
first-class view type throughout the shared code —
`MfcDocPresenter.h:356,421`, `CUG.cpp:822,843,900,1270` — not a stub.

## 3. It is compiled but dead-stripped, which is measured, not inferred

This is the part worth recording, because "the class exists" and "the class
ships" are different claims and only one of them was true.

| binary | `ContainerViewPanel` | `ContainerViewStruct` | `ModuleViewPanel` | bogus name |
|---|---|---|---|---|
| `TIDE.gmpi` (shipped) | **0** | 15 | 25 | 0 |
| `ContainerView.o` (archive member) | **13** | — | — | — |

The last column is the negative control and the second the positive one, so the
zero in column two is a real absence rather than a broken `nm` invocation.

**Why:** `ContainerView.cpp` is on `SynthEditLib/CMakeLists.txt:535`, so it
compiles into `libSynthEditLib.a`. Nothing in TIDE references
`ContainerViewPanel`, so the linker never pulls that archive member in. **The
same static-library behaviour C12e documented** for `Dialogs_editor2.obj` — an
object with no referenced symbol is simply never extracted.

**Note `ModuleViewPanel` is present with 25 symbols.** The *per-module* panel
renderer already ships in TIDE; only the *top-level container* panel does not.
That asymmetry is the strongest single piece of evidence that rack mode is a
wiring job before it is a rendering job.

## 4. The affordances constraint 1 names

| Affordance | State | Evidence |
|---|---|---|
| Breadcrumb bar | **absent** — no reference in `TideApp.cpp` | grep |
| Properties pane | **constructible, not placed** — `TideApp::OpenPropertiesBrowser` exists (`:88`) | unchanged from the P2 finding |
| Module browser | **constructible** — `TideApp::OpenModuleBrowser` (`:79`), SynthEdit's browser | unchanged from the P2 finding |

So two of the P2-era findings **still hold after the pivot** and one (the view
type) is now the item's central question rather than a detail. The canvas-offset
and dead-strip findings in `state-of-the-prototype.md` §6 were **not** re-measured
here — they need a running host, which this audit did not do.

---

## What this means for splitting U1

U1's row says it *"probably wants splitting once someone costs it."* On this
evidence the natural split is three items, in dependency order:

1. **Switch the default view to the panel/rack view.** Construct
   `ContainerViewPanel` instead of `ContainerViewStruct` in `TideApp::OpenView`,
   with `CF_PANEL_VIEW`. **Scope is one ALLOWED file.** The honest unknown is
   what it *looks like* when it renders — the class exists, but nothing has ever
   linked it in TIDE, so "it compiles and links" is the first acceptance bar and
   "it draws something sane" is the second. **Expect this to surface bugs rather
   than finish the job**, and treat a crash here as information, not failure.
2. **The breadcrumb bar**, which constraint 1 requires for moving between
   depths and which [about-pane.md](about-pane.md) now also depends on — it is
   the only persistent chrome the about pane can hang from. Blocked on (1) in
   practice, since there is nothing to navigate between until the rack renders.
3. **Rack styling and snapping** — the Eurorack case, modules snapping to rows.
   This is the part that is genuinely *not* written, and the only part of U1
   that is a from-scratch build.

**Do not fold these together.** (1) is small and mechanical and its result
determines whether (3) is a small styling job on a working panel view or a
larger one.

---

## What this audit did not do

- **It did not run TIDE in a host.** Every claim above is from source and from
  `nm` on the built binary. The visual findings in
  [state-of-the-prototype.md](state-of-the-prototype.md) §6 — canvas offset, the
  dead strip on the right — are therefore **still unverified against the pivot**
  and are not repeated here as if they were.
- **It did not change any behaviour.** `TideApp.cpp` is untouched; switching the
  view flag is item (1) above, deliberately not done inside an audit.
- **It did not re-measure the module browser's two-column layout or the missing
  collapse control**, both P2-era. They are UI-visible and want the same running
  host as the point above.
