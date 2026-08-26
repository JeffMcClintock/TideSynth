# Rack-content fixtures — the negative controls for `check-rack-populated.py`

BACKLOG **M6**, third fixture added by **M8**. Three captured-diagnostic
fixtures, every one of which the gate must FAIL. They exist because a gate
nobody has watched fail is not a gate — the same argument
[tests/hosts/](../hosts/README.md) makes for its silent negative control, and
for the same reason: a check that has only ever been run against a healthy
subject cannot distinguish "passes" from "does not look".

```bash
python3 scripts/check-rack-populated.py --log-file tests/rack-content/m5-empty-rack.log              # must exit 1
python3 scripts/check-rack-populated.py --log-file tests/rack-content/silent-empty-rack.log          # must exit 1
python3 scripts/check-rack-populated.py --log-file tests/rack-content/unresolved-resource-folder.log # must exit 1
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
