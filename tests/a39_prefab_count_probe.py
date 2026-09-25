#!/usr/bin/env python3
"""BACKLOG A39 -- the rack gate's prefab expectation is DERIVED, and still has teeth.

A39 replaced `EXPECTED_PREFABS = <n>` in scripts/check-rack-populated.py with a
count read from `RackModules/`, the directory CMake stages into the bundle. The
row's Accept has three clauses and a warning, and this probe is all four:

  (a) add or delete a RackModules/*.synthedit with NO edit to the script, and
      the gate still passes;
  (b) make a staged prefab fail to seed, and the gate still FAILS -- "a negative
      control is mandatory, because a gate that derives its expectation from the
      subject passes vacuously and looks identical to a working one";
  (c) both arms recorded against the shipped `check()`.

CLAUSE (b) IS THE POINT OF THIS FILE. Deriving an expectation is only safe when
it is derived from the BUILD'S INPUT and never from the BUILD'S OUTPUT. Those
two are trivially confusable in code and utterly different in effect: one gate
catches a prefab that failed to stage, the other cannot fail at all. So the
vacuity arm below does not merely check that a mismatch fails -- it checks that
the captured log CANNOT MOVE THE EXPECTATION, by feeding one tree two logs
claiming different counts and requiring different verdicts.

    python3 tests/a39_prefab_count_probe.py        # exit 0 when all arms hold

No build, no network, no fixtures beyond the temporary trees it makes itself.
"""
import importlib.util
import pathlib
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent


def load_gate():
    """Import scripts/check-rack-populated.py -- its name is not an identifier."""
    path = ROOT / "scripts" / "check-rack-populated.py"
    spec = importlib.util.spec_from_file_location("check_rack_populated", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def healthy_log(prefabs):
    """A capture in which everything the gate asserts is present and healthy.

    Built from the real message strings in TideApp.cpp, so a rename there breaks
    this probe rather than silently making it assert nothing.
    """
    lines = [
        "TIDE: ControlsXp.xml enriched 2 of 18 described class(es)",
        "TIDE: MidiPlayer2.xml enriched 2 of 7 described class(es)",
        "TIDE: Converters.xml enriched 26 of 70 described class(es)",
        "TIDE: VaFilters.xml enriched 2 of 7 described class(es)",
        "TIDE: %d rack prefab(s) seeded from the bundle" % prefabs,
        "TIDE: default rack loaded, 25109 byte document",
    ]
    return "\n".join(lines) + "\n"


def tree(names):
    """A throwaway repo root holding RackModules/<names>."""
    root = pathlib.Path(tempfile.mkdtemp(prefix="a39-"))
    (root / "RackModules").mkdir()
    for n in names:
        (root / "RackModules" / n).write_text("<!-- probe -->\n", encoding="utf-8")
    return root


FAILURES = []


def expect(label, condition, detail=""):
    status = "ok  " if condition else "FAIL"
    suffix = ("  -- " + detail) if detail and not condition else ""
    print("  %s %s%s" % (status, label, suffix))
    if not condition:
        FAILURES.append(label)


def main():
    gate = load_gate()

    print("A39 probe -- the expectation is derived from the staging input\n")

    live = gate.count_staged_prefabs()
    print("RackModules/ on this checkout holds %d prefab(s)\n" % live)

    # ---- clause (a): the count follows the directory, with no script edit ---
    print("(a) adding and deleting a prefab needs no edit to the script")
    for names in (["A.synthedit", "B.synthedit", "C.synthedit"],
                  ["A.synthedit", "B.synthedit", "C.synthedit", "D.synthedit"],
                  ["A.synthedit", "B.syntheditprefab"]):
        root = tree(names)
        n = gate.count_staged_prefabs(root)
        failures, _ = gate.check(healthy_log(n), n)
        expect("%d file(s) -> expects %d, healthy capture passes" % (len(names), n),
               n == len(names) and not failures, "; ".join(failures))

    # Sub-directories count, because seedPrefabsFromBundle() recurses.
    root = tree(["A.synthedit"])
    (root / "RackModules" / "group").mkdir()
    (root / "RackModules" / "group" / "B.synthedit").write_text("x", encoding="utf-8")
    got = gate.count_staged_prefabs(root)
    expect("a prefab in a sub-folder is counted (the app recurses)",
           got == 2, "got %d" % got)

    # Non-prefab files do NOT count, because the app filters on extension.
    root = tree(["A.synthedit"])
    (root / "RackModules" / "README.md").write_text("x", encoding="utf-8")
    (root / "RackModules" / "notes.txt").write_text("x", encoding="utf-8")
    got = gate.count_staged_prefabs(root)
    expect("a README beside the prefabs is not counted (extension filter)",
           got == 1, "got %d" % got)

    # ---- clause (b): THE TEETH. A prefab that failed to stage still fails ---
    print("\n(b) negative control -- a prefab that fails to stage still FAILS")
    root = tree(["A.synthedit", "B.synthedit", "C.synthedit", "D.synthedit"])
    n = gate.count_staged_prefabs(root)               # 4 staged
    failures, _ = gate.check(healthy_log(n - 1), n)   # only 3 seeded
    expect("4 in RackModules/, 3 seeded -> gate fails",
           any("rack prefab(s) seeded" in f for f in failures),
           "failures were %r" % failures)
    expect("...and the failure names BOTH numbers",
           any("3 rack prefab(s) seeded" in f and "holds 4" in f for f in failures),
           "failures were %r" % failures)

    # The other direction too: MORE seeded than staged is also wrong (a stale
    # build tree serving a deleted prefab -- BACKLOG E40's exact shape).
    failures, _ = gate.check(healthy_log(n + 1), n)
    expect("4 in RackModules/, 5 seeded (stale bundle, E40) -> gate fails",
           any("rack prefab(s) seeded" in f for f in failures),
           "failures were %r" % failures)

    # ---- the vacuity control: the SUBJECT cannot move the EXPECTATION ------
    print("\n(b') vacuity control -- the captured log cannot set its own bar")
    root = tree(["A.synthedit", "B.synthedit", "C.synthedit"])
    n = gate.count_staged_prefabs(root)
    ok_failures, _ = gate.check(healthy_log(3), n)
    bad_failures, _ = gate.check(healthy_log(9), n)
    expect("same tree, log says 3 -> pass; log says 9 -> fail",
           not ok_failures and any("rack prefab(s) seeded" in f for f in bad_failures),
           "3->%r  9->%r" % (ok_failures, bad_failures))

    # ---- refusals: a zero or absent source is an error, never a pass -------
    print("\n(c) a missing or empty source refuses, rather than expecting zero")
    empty = pathlib.Path(tempfile.mkdtemp(prefix="a39-empty-"))
    (empty / "RackModules").mkdir()
    try:
        gate.count_staged_prefabs(empty)
        expect("empty RackModules/ raises", False, "it returned a count")
    except SystemExit as e:
        expect("empty RackModules/ raises rather than expecting 0",
               "vacuous" in str(e), str(e))

    missing = pathlib.Path(tempfile.mkdtemp(prefix="a39-missing-"))
    try:
        gate.count_staged_prefabs(missing)
        expect("absent RackModules/ raises", False, "it returned a count")
    except SystemExit as e:
        expect("absent RackModules/ raises with an actionable message",
               "--expect-prefabs" in str(e), str(e))

    # ---- the four shipped negative controls are untouched by this change ---
    print("\n(d) the shipped fixtures still fail, for their own reasons")
    for name, why in (("m5-empty-rack.log", "the M5 shape"),
                      ("silent-empty-rack.log", "a pure absence"),
                      ("unresolved-resource-folder.log", "M8's named cause"),
                      ("lost-module-handle.log", "E54's degraded rack")):
        text = (ROOT / "tests" / "rack-content" / name).read_text(errors="replace")
        # 5 is the count these captures were taken at; passing it explicitly
        # keeps each control testing ITS OWN failure and not a count mismatch.
        failures, _ = gate.check(text, 5)
        expect("%s still fails (%s)" % (name, why), bool(failures))

    print()
    if FAILURES:
        print("%d arm(s) failed: %s" % (len(FAILURES), ", ".join(FAILURES)))
        return 1
    print("every arm held -- the expectation is derived, and the gate still has teeth.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
