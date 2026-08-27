#!/bin/bash
# Exercises the two gates added to package-macos.sh against representative
# `codesign -dvv` output. Proves they FAIL on the real bug before trusting
# that they pass on the fix.
check() {
  local name="$1" auth="$2" rc=0
  case "$auth" in
    *"Authority=Developer ID Application"*) ;;
    *) echo "  $name -> REJECTED (no Developer ID authority)"; return 1;;
  esac
  case "$auth" in
    *Timestamp=*) ;;
    *) echo "  $name -> REJECTED (no secure timestamp)"; return 1;;
  esac
  echo "  $name -> accepted"
}

ADHOC='Executable=/x/TIDE-Rack.appex/Contents/MacOS/TIDE-Rack
Identifier=com.synthedit.tiderack
Signature=adhoc
Info.plist entries=22
TeamIdentifier=not set'

DEVID='Executable=/x/TIDE-Rack.appex/Contents/MacOS/TIDE-Rack
Identifier=com.synthedit.tiderack
Authority=Developer ID Application: SynthEdit Limited (36SNPLRFK3)
Authority=Developer ID Certification Authority
Authority=Apple Root CA
Timestamp=27 Aug 2026 at 15:04:11
TeamIdentifier=36SNPLRFK3
Runtime Version=15.0.0'

DEVID_NOTS='Executable=/x/TIDE-Rack.appex/Contents/MacOS/TIDE-Rack
Authority=Developer ID Application: SynthEdit Limited (36SNPLRFK3)
TeamIdentifier=36SNPLRFK3'

fails=0
echo "the bug as Apple saw it (ad-hoc appex):"
check "adhoc" "$ADHOC" && { echo "  ** GATE DID NOT FIRE **"; fails=1; }
echo "Developer ID, no timestamp:"
check "devid-no-timestamp" "$DEVID_NOTS" && { echo "  ** GATE DID NOT FIRE **"; fails=1; }
echo "properly signed:"
check "devid" "$DEVID" || { echo "  ** FALSE POSITIVE **"; fails=1; }
echo
[ $fails -eq 0 ] && echo "PROBE OK: rejects both bad shapes, accepts the good one" || echo "PROBE FAILED"
exit $fails
