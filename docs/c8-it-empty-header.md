# C8 — `SynthEditLib/it_empty.h`: why it exists, and whether to keep it

Audited 2026-08-11 on the macOS box. C8 asks for a **deliberate keep or a
deliberate delete**, and offers "or find out why it exists" as the alternative
to deleting blind. This document does the finding-out. It does **not** delete
the file — see [The one thing this run did not do](#the-one-thing-this-run-did-not-do).

**Recommendation: delete.** The evidence below is unusually clean for a
dead-code question — the file has no includers, is in no build, is exported by
nothing, and its template was last instantiated in 2022.

---

## What the file is

A 30-line MFC-era header containing one class template:

```cpp
template <class T, class DERIVED_FROM> class EmptyIterator : public DERIVED_FROM
{
    virtual void First() {}
    virtual void Next()  {}
    virtual bool IsDone()       { return true; }
    virtual T*   CurrentItem()  { return 0; }
};
```

An iterator that is always immediately done — the null object of the old
hand-rolled `it_*` iterator hierarchy, from before range-`for` and the standard
iterators made that hierarchy unnecessary.

**Its own comment is wrong about its contents.** Line 1 reads
`// it_empty.h: interface for the it_empty class` — but there is no `it_empty`
class in it, and has not been for as long as git can see. That mismatch is the
first hint that the file was left behind rather than maintained.

**It was created 2002-01-10.** Not inferred from git, which only goes back to
the 2013 engine import — read out of the ClassWizard header guard itself:

```
AFX_IT_EMPTY_H__E50CDB53_05FA_11D6_AF04_000103170662__INCLUDED_
```

`E50CDB53-05FA-11D6-...` is a version-1 UUID, and its embedded timestamp decodes
to **2002-01-10 18:50:19 UTC**. The `AFX_` prefix dates it to Visual C++'s MFC
ClassWizard. The file is 24 years old and predates every repo it now sits in.

## Why it still exists — three dead-code passes that each missed it

Reconstructed with `git log -S` across `SE16`. Each step removed something real
and left the header one degree more orphaned:

| When | Commit | What went | What was left |
|---|---|---|---|
| 2022-03-03 | `27f28b54e` *chore(se) : remove dead code and comments* | The last **live instantiation**: `class it_visual_ob_list_empty : public EmptyIterator<CVisualOb, it_visual_ob>` | The template, now uninstantiated |
| 2025-01-24 | `176c6c26f` *chore(se) : remove unused files* | The archived V1 copies under `OtherProjects/SynthEdit_1.0/` — `return new EmptyIterator<generic_parameter, it_parameter>` and another `it_visual_ob_list_empty` | The template, now uninstantiated anywhere in history's reach |
| 2026-04-13 | `671457fc5` *remove dead code* | `SynthEdit2/it_empty.cpp` — **the header's last includer anywhere** | The header, now included by nothing |
| 2026-08-08 | `c3a4f9fac` / `6e49dbf` (C2) | Relocated `SynthEdit2/it_empty.h` → `SynthEditLib/it_empty.h` | The orphan, now in a public repo |

The 2026-04-13 step is the one that matters, and it is quietly absurd: the
`it_empty.cpp` it deleted had **every line of its body commented out**. Its
entire live content was

```cpp
#include "it_empty.h"
```

So for its last four months the header's sole reason to exist was an empty file
that existed only to include it. Removing that file was correct and left the
header with nothing.

**C2 then moved it for a mechanical reason, not a considered one.** The
carve-out's file list is `SE16/EditorLib/CMakeLists.txt`, and `it_empty.h` is on
it at line 74. C2 moved everything on that list. A CMake source list is a
build-system inventory, not a dependency graph — it happily carries a header no
translation unit includes, because listing a header contributes nothing to
compilation and so nothing ever complains. C2's own journal entry flags that it
noticed and deliberately declined to judge, which is why C8 exists.

## Evidence that deleting it is safe

Every row was run for this audit, not carried over from C2.

| Check | Command | Result |
|---|---|---|
| Includers, tracked files, 8 repos | `git grep -nIE 'it_empty\|EmptyIterator'` over `SynthEdit`, `SynthEditLib`, `gmpi_ui`, `GMPI_Wrappers`, `GMPI`, `GMPI-plugins`, `GMPI_Adaptors`, `TideSynth` | **Zero `#include`.** Only hits: the file itself, `EditorLib/CMakeLists.txt:74`, and TideSynth's own prose |
| Includers, working trees incl. build dirs | `grep -rIl --exclude-dir=.git` over the same eight | Same. The one extra hit is a stale `.claude/worktrees/…/SynthEdit2/it_empty.h` — a pre-C2 copy in an abandoned scratch worktree, not a reference |
| Built by its own repo? | `grep it_empty SynthEditLib/CMakeLists.txt` | **No.** And neither is any other C2 file — `SynthEditLib`'s CMake lists none of `it_doc_ob.cpp`, `imbedded_file.cpp`, `checkpoint.cpp`, `it_plug_destinations.cpp`. The C2 files physically live there but are still compiled only by `EditorLib`, which reaches across via `${SYNTHEDITLIB_DIR}`. That is expected until **C6** moves the list |
| Swept in by a glob? | `grep -rnE 'file *\( *GLOB\|GLOB_RECURSE'` over all of `SynthEditLib`'s CMake | **None anywhere in the repo.** No wildcard can pick it up |
| Exported as public API? | `grep -nE 'install *\(\|export *\(\|PUBLIC_HEADER\|FILE_SET'` | **None.** `SynthEditLib` has no install or export rules at all — nothing is packaged, so nothing is public in the API sense |
| Hand-maintained projects | `grep -rIl it_empty --include='*.vcxproj' --include='*.filters' --include='*.pbxproj' --include='*.xcconfig' --include='*.yml'` | **None.** Unlike `FuzzyMatch.h`, which C2 had to repoint in three such files |

Two of these are worth stating as conclusions rather than rows:

- **A header with zero `#include` directives naming it cannot affect any
  translation unit.** That is a proof, not a sampling. It is also why no build
  was run for this audit: a build can only fail to find a problem the grep has
  already excluded by construction, and building `SE16` on this box has known
  live traps (P6, and the half-overridden `GMPI_WRAPPER_FOLDER_OVERRIDE`
  recorded in the P7 entry). A green build here would be weaker evidence than
  the grep, not stronger.
- **"Public API surface", the phrase in the C8 row, overstates it.** The file is
  *visible* in a public repo, which is real — anyone can read it and, post-ISC,
  legally copy it. But it is not exported, not installed, not built, and not
  reachable by `#include` from anything the repo ships. Deleting it breaks no
  consumer because there is no mechanism by which a consumer could have one.

## Recommendation

**Delete**, for three reasons in descending weight:

1. It is dead by every measure available, and has been since 2022. Keeping it
   preserves nothing.
2. It is actively misleading. The comment names a class the file does not
   contain, and the `AFX_` guard advertises an MFC dependency it does not have.
   A reader of the newly-public repo meets this file with no way to tell it is
   an orphan.
3. The carve-out's whole purpose is to make `SynthEditLib` a repo someone can
   read and build against. Shipping 2002's dead iterator scaffolding as part of
   the first public impression is a small cost, but it is all cost.

The counter-argument, stated fairly: it is 30 bytes of nothing, deleting it has
zero functional benefit, and `git` remembers it anyway if it is ever wanted.
That argues for *keep* only if one expects the old iterator hierarchy to come
back, and 2022's removal of the last instantiation is evidence it is going the
other way.

**Two files change, and both must change together:**

| File | Change |
|---|---|
| `SynthEditLib/it_empty.h` | delete |
| `SE16/EditorLib/CMakeLists.txt:74` | remove the `${SYNTHEDITLIB_DIR}/it_empty.h` line |

Deleting the file without removing the CMake line leaves a source list naming a
file that does not exist. CMake tolerates that for headers, so it would not fail
the build — it would just sit there being wrong, which is how this file got here
in the first place.

## The one thing this run did not do

**The deletion, because both files are in GATED paths and C8 is not a C1–C7
stage.**

STEP 5 of the run prompt gates `the SynthEditLib repo` and `SE16/EditorLib/`
explicitly, with one exception: *"unless your item is an approved carve-out
stage (C1-C7) and BACKLOG shows C0 as approved."* C0 is approved. C8 is not
C1–C7 — it is numbered outside the range, and it is a cleanup rather than a
stage. Both files this change needs are behind that gate.

The prompt's own remedy — *"do the TIDE-side part, then file the gated part as
its own BACKLOG item naming the exact file and why"* — is already satisfied and
cannot be applied again: C8 **is** that item, filed by C2, naming the exact
file. There is no TIDE-side part to do. So the remedy terminates in a question
rather than an action, and the question is a one-liner:

> Does an agent taking C8 have authority to make this deletion, or does C8 need
> Jeff to either make it or say "go"?

That is filed as a `PROPOSED:` entry in [decisions.md](decisions.md). It is
deliberately narrow — it asks about C8, not about relaxing the gate — because
the gate is load-bearing and G3 is the precedent for asking rather than
assuming. The audit above is the expensive half and it is complete; whichever
way the ruling goes, the execution is one commit in each of two repos.

**Do not read this as the work being blocked on ceremony.** The C8 row itself
says the outcome "should be a deliberate keep or a deliberate delete", and C2
span it off precisely so the call would not be made as a side effect of a file
move. A run deciding on its own authority to delete from the commercial-adjacent
shared repo would be making it a side effect again, one layer up.
