# Rack-content fixtures — the negative controls for `check-rack-populated.py`

BACKLOG **M6**, third fixture added by **M8**, fourth by **E54**. Four
captured-diagnostic fixtures, every one of which the gate must FAIL. They exist because a gate
nobody has watched fail is not a gate — the same argument
[tests/hosts/](../hosts/README.md) makes for its silent negative control, and
for the same reason: a check that has only ever been run against a healthy
subject cannot distinguish "passes" from "does not look".

```bash
python3 scripts/check-rack-populated.py --log-file tests/rack-content/m5-empty-rack.log              # must exit 1
python3 scripts/check-rack-populated.py --log-file tests/rack-content/silent-empty-rack.log          # must exit 1
python3 scripts/check-rack-populated.py --log-file tests/rack-content/unresolved-resource-folder.log # must exit 1
python3 scripts/check-rack-populated.py --log-file tests/rack-content/lost-module-handle.log         # must exit 1
```

## `m5-empty-rack.log` — the state that shipped

The plugin's own words, from the M5 measurement of 2026-08-25, reproducing what
a Release AUv3 reported from at least 2026-08-23 while `auval` returned exit 0
and `AU VALIDATION SUCCEEDED` on it: four module-description XMLs unresolved, no
Prefabs folder, and no root MIDI-CV container. No control pins, an empty module
browser, no MIDI jacks.

This is the fixture that answers M6's Accept — *"would have caught M5 on
2026-08-23"*. The gate reports **12 failures** on it.

## `silent-empty-rack.log` — the case a negative grep would pass

**This is the load-bearing one.** Every message here is a *healthy* one; the
failure is a line that is ABSENT. When this fixture was captured,
`seedPrefabsFromBundle()` opened with

```cpp
const auto resourceFolder = BundleInfo::instance()->getResourceFolder();
if (resourceFolder.empty())
    return;                       // <-- no message of any kind
```

so an unresolved resource folder produced a rack with an empty module browser
and said nothing at all. The cheap version of this gate — "fail if any of the
known bad lines appears" — passes this fixture. Requiring the positive line
fails it.

**Keep this fixture even though M8 has since fixed the silence it models.** It
is no longer a picture of today's binary, and that is the point: it is the
picture of a binary that goes quiet for a reason nobody has thought of yet. The
positive assertion is what catches those, and this is the only fixture that
exercises it alone.

## `unresolved-resource-folder.log` — the same case, after M8

M8 put a `tideDiag` on that bare `return`, so an unresolvable resource folder
now announces itself:

```
TIDE: bundle resource folder did not resolve - the rack module browser will be empty
```

This fixture is `silent-empty-rack.log` plus that one line, and it is the
before/after pair for M8: the gate reports **1** failure on the silent one (an
absence, cause unknown) and **2** on this one, the first of which NAMES the
cause and the second of which says the cause was already named. Same defect,
same verdict, and now a reader of the log knows which of the three ways out of
`seedPrefabsFromBundle()` was taken.

It is not a hypothetical arrangement. `getResource()` reaches the embedded
Win32 resources before it ever consults `getResourceFolder()`, and under
`GMPI_IS_PLATFORM_JUCE` the folder lookup returns `{}` unconditionally — so the
four XMLs really can enrich perfectly while this one lookup comes back empty.

Keep all three. The first proves the gate catches the failure we know about;
the second proves it catches the shape of failure that made the first one
invisible for two days; the third proves that shape now names itself instead of
having to be inferred.

## `lost-module-handle.log` — the one every positive assertion passes

**BACKLOG E54, and it is the second load-bearing fixture** — for the opposite
reason to `silent-empty-rack.log`. There the failure is an absent line. Here
**every line the gate asserts positively is present and healthy**: four XMLs
enriched, five prefabs seeded, `default rack loaded, 25109 byte document`. The
rack is nonetheless missing a module, and the only evidence is one line the
gate did not read until E54:

```
SynthEdit: parameter names module handle 999999999, which this document does
not contain -- parameter left with no module.
```

So this fixture is the counterexample to reading M8's note as *"the positive
assertions are what matter, the fatal lines are belt-and-braces"*. Both halves
are load-bearing, in opposite directions, and this file is the proof of the
second.

**Why it exists at all is a story about a fix.** Before SynthEditLib `796bbc2`
(BACKLOG **E46**'s guard) that miss dereferenced `map::end()` and the process
died — so the gate caught this case by the ABSENT `default rack loaded` line,
without knowing anything about module handles. The guard made the same document
survive, degraded. **The bug got safer and the gate got blinder**, and nothing
would have noticed until a release shipped a rack with no MIDI input.

**The live case is not a crafted handle.**
[#491](https://github.com/JeffMcClintock/TideSynth/issues/491) was
`DefaultRack.synthedit` naming `MIDI In NL` **81 minutes** before SynthEditLib
had that module — a cross-repo straddle, which is a normal hazard here and not
a mistake anybody made. Captured 2026-08-27 (macos) from a real launch of a
`main` build, with the default rack's one `<param module=>` repointed at
`999999999`.

**This fixture's message comes from `SynthEditLib`, not `TideApp.cpp`** — it is
prefixed `SynthEdit:`, not `TIDE:`. That is why the check behind it is
`LOST_MODULE` rather than another `FATAL_LINES` entry: that loop requires the
`TIDE:` prefix, so an entry added there would have matched nothing and left the
check looking armed. Keeping this one in step means reading another repo.

**One limit, stated rather than implied:** this fixture and the check behind it
cover the `--standalone` and `--log-file` arms only. `--au3` reads os_log, and
this message is `std::cerr` from SynthEditLib, which does not mirror to it — so
**an AUv3 that lost a module is still invisible to the gate.** Closing that
means routing the library's diagnostics through the same channel, in another
repo.
