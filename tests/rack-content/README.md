# Rack-content fixtures — the negative controls for `check-rack-populated.py`

BACKLOG **M6**. Two captured-diagnostic fixtures, both of which the gate must
FAIL. They exist because a gate nobody has watched fail is not a gate — the same
argument [tests/hosts/](../hosts/README.md) makes for its silent negative
control, and for the same reason: a check that has only ever been run against a
healthy subject cannot distinguish "passes" from "does not look".

```bash
python3 scripts/check-rack-populated.py --log-file tests/rack-content/m5-empty-rack.log     # must exit 1
python3 scripts/check-rack-populated.py --log-file tests/rack-content/silent-empty-rack.log # must exit 1
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

**This is the more important of the two.** Every message here is a *healthy*
one; the failure is a line that is ABSENT. `seedPrefabsFromBundle()` opens with

```cpp
const auto resourceFolder = BundleInfo::instance()->getResourceFolder();
if (resourceFolder.empty())
    return;                       // <-- no message of any kind
```

so an unresolved resource folder produces a rack with an empty module browser
and says nothing at all. The cheap version of this gate — "fail if any of the
known bad lines appears" — passes this fixture. Requiring the positive line
fails it.

Keep both. The first proves the gate catches the failure we know about; the
second proves it catches the shape of failure that made the first one invisible
for two days.
