#!/usr/bin/env python3
"""Community research routine — BACKLOG A9. PROPOSED-only output.

Reads four public sources, drops everything TIDE's design constraints already
exclude, dedups against the backlog and against a rejection memory, and prints
PROPOSED rows for Jeff to triage. It never decides product direction.

    python3 scripts/community-research.py                # all sources
    python3 scripts/community-research.py --source forum # one source
    python3 scripts/community-research.py --selftest     # offline, no network
    python3 scripts/community-research.py --dry-run      # show what it WOULD fetch

THE GUARDRAILS ARE STRUCTURAL, NOT POLICY. A9 lists them as hard rules, so they
are enforced by construction rather than by remembering:

  * `_get()` is the ONLY function in this file that touches the network, and it
    can only issue GET. There is no code path here that can post, vote, DM, or
    authenticate as a user -- not "we choose not to", but "there is nothing to
    call". Anyone adding a write path has to add the capability first, which is
    the point at which someone should say no.
  * Every request waits COURTESY_SECONDS since the previous one, process-wide.
  * The User-Agent identifies the project and links back here, so an operator
    who dislikes this traffic can find out who to tell.
  * https only. A plain-http URL raises.
  * Output is PROPOSED rows. This script cannot edit BACKLOG.md; it prints.

WHY A REJECTION MEMORY. Without it, every run re-proposes the same rejected
ideas and the routine trains Jeff to skim past its output -- the failure mode
A4's row calls "training the rubber-stamp reflex on trivia". docs/
community-research-rejected.md is that memory, and it is human-editable.
"""

import argparse
import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

UA = ("TideSynth-research/0.1 "
      "(+https://github.com/JeffMcClintock/TideSynth) read-only")
COURTESY_SECONDS = 1.5
TIMEOUT = 30

_last_request = [0.0]


# --------------------------------------------------------------------------
# the only network primitive
# --------------------------------------------------------------------------

def _get(url, accept="application/json"):
    """HTTP GET. The only network call in this file. Read-only by construction."""
    if not url.startswith("https://"):
        raise ValueError("https only, refusing: %s" % url)

    wait = COURTESY_SECONDS - (time.monotonic() - _last_request[0])
    if wait > 0:
        time.sleep(wait)
    _last_request[0] = time.monotonic()

    req = urllib.request.Request(url, method="GET")
    req.add_header("User-Agent", UA)
    req.add_header("Accept", accept)
    # A token only raises the rate limit; the endpoints used are all public and
    # work without one. Never send it anywhere but GitHub.
    token = os.environ.get("GH_TOKEN")
    if token and url.startswith("https://api.github.com/"):
        req.add_header("Authorization", "Bearer " + token)

    with urllib.request.urlopen(req, timeout=TIMEOUT) as fh:
        return fh.read()


def _get_json(url):
    return json.loads(_get(url).decode("utf-8"))


# --------------------------------------------------------------------------
# the auto-reject filter -- PLAN's constraints, encoded
# --------------------------------------------------------------------------
#
# A9 asks for "TIDE's product philosophy as an explicit auto-reject filter,
# Cardinal-style". PLAN.md already carries it, so this is a transcription, not
# a new opinion -- and every rule cites the constraint it comes from, so a
# rejection can always be argued with by arguing with PLAN instead of with a
# regex. Ordered most-specific first; the first match wins.

REJECT_RULES = [
    (r"\bplugin export|export as (a )?plugin|export.*\bvst\b", "commercial boundary — TIDE embeds patches, it does not export plugins"),
    (r"\basio\b|audio (device|interface) (selection|settings)|driver settings|midi device selection",
     "constraint 2 — the DAW owns I/O"),
    (r"\bstandalone (app|application|version)\b", "constraint 2 — TIDE is a plugin; the host supplies audio and MIDI"),
    (r"browse for|file (browser|dialog|picker)|load (a )?sample|sample library|wavetable file|user (folder|directory)",
     "constraint 3 — sandbox-safe; no arbitrary filesystem access"),
    (r"module (scan|scanning|cache|folder)|third.?party module|load(ing)? external module",
     "constraint 7 — fixed module set, compiled in; no scanning"),
    (r"\bskin(s|ning)?\b|custom theme|user theme|theme (folder|pack)", "constraint 8 — no user skins"),
    (r"dockable|floating window|multiple windows|separate editor window|\btabs?\b",
     "constraint 1 — one view, two depths; no tabs or dockable windows"),
    (r"modal dialog|popup dialog", "constraint 5 — minimal dialogs; panes, not dialogs"),
]

# The standing product hypothesis, CORRECTED 2026-08-20 (BACKLOG A28): no
# open-source EURORACK-STYLE RACK exists on iOS AUv3. The broader form this
# comment used to carry -- "no open-source modular" -- is false: plugdata is
# GPL-3.0, free, on the App Store and ships AUv3. It is a Pd patcher, not a
# rack, which is why the narrower claim survives. See docs/community-research.md
# and docs/competitive-review.md section 4.
#
# Anything touching that square is the highest-value signal this routine can
# surface, so it is flagged, never dropped -- even when a reject rule also
# matches.
#
# The named products split into two kinds, and conflating them is what this
# comment exists to prevent (Jeff, 2026-08-20: "plugdata is not really in the
# category of eurorack simulators. However mirack is an ios competitor"):
#
#   COMPETITORS -- they hold TIDE's actual square, an iOS Eurorack-style rack,
#   and all three are closed-source. `mirack` is the sharpest: AUv3, 800+
#   compiled-in modules, $14.99, and it is what the surviving hypothesis is a
#   claim ABOUT. `drambo` and `audulus` are the same square from other angles.
#
#   PRECEDENT -- `plugdata` is a Pd dataflow patcher, NOT a rack, so it is not
#   a competitor and must not be read as one. It is here because it has already
#   solved the iOS AUv3 packaging, distribution and App Store review problems
#   TIDE will hit, and because it is what refuted the broader hypothesis.
#
# This regex must stay in step with HYPOTHESIS_QUERIES below --
# source_hypothesis() drops any search hit whose TITLE this regex does not
# match, so a query with no matching term here returns nothing and looks like a
# quiet week. The reverse is fine: a term here with no query still catches
# mentions that the other sources surface.
HYPOTHESIS_RE = re.compile(
    r"\bios\b|\bauv3\b|\bipad\b|\biphone\b|audiobus|\baum\b|loopy pro|"
    r"\bmobile\b|app ?store|touch.?screen|plugdata|mirack|drambo|audulus",
    re.I)

REJECT_RES = [(re.compile(p, re.I), why) for p, why in REJECT_RULES]


def classify(title, body=""):
    """-> (verdict, reason). verdict is 'flag', 'reject' or 'keep'.

    THE CONSTRAINT RULES READ THE TITLE ONLY, and that is a correction made
    after the first live run rather than an arbitrary choice. Matching bodies
    auto-rejected a real Surge XT crash report -- "Surge XT CLAP crashes REAPER
    on load (SIGSEGV...)" -- because somewhere in the reporter's diagnostic
    notes was the phrase "The standalone app also works fine". An incidental
    mention in a bug report is not a feature request, and throwing away crash
    reports is the worst thing this filter could do.

    The hypothesis flag DOES read the body, because its failure mode is the
    opposite and much cheaper: a false positive merely flags an item for a
    human, and missing the one signal A9 exists to catch is the real cost.
    """
    if HYPOTHESIS_RE.search(title) or HYPOTHESIS_RE.search(body):
        return "flag", "matches the standing hypothesis (iOS / AUv3 / mobile)"
    for rx, why in REJECT_RES:
        if rx.search(title):
            return "reject", why
    return "keep", ""


# --------------------------------------------------------------------------
# sources
# --------------------------------------------------------------------------

def source_forum(limit=30):
    """VCV Community (Discourse). Public JSON, no key."""
    base = "https://community.vcvrack.com"
    data = _get_json(base + "/latest.json?order=created")
    out = []
    for t in data.get("topic_list", {}).get("topics", [])[:limit]:
        out.append({
            "source": "vcv-forum",
            "id": str(t.get("id")),
            "title": t.get("title", ""),
            "text": t.get("title", "") + " " + (t.get("excerpt") or ""),
            "url": "%s/t/%s/%s" % (base, t.get("slug", ""), t.get("id")),
            "when": (t.get("created_at") or "")[:10],
            "extra": "replies=%s views=%s" % (t.get("posts_count", 0) - 1, t.get("views", 0)),
            "score": (t.get("posts_count", 0) - 1) * 3 + (t.get("views", 0)) // 100,
        })
    return out


def source_issues(repo, limit=30):
    """Cardinal / Surge XT issue trackers, via the public GitHub REST API."""
    url = ("https://api.github.com/repos/%s/issues"
           "?state=open&sort=created&direction=desc&per_page=%d" % (repo, limit))
    out = []
    for it in _get_json(url):
        if "pull_request" in it:      # the endpoint returns PRs too
            continue
        out.append({
            "source": repo,
            "id": str(it.get("number")),
            "title": it.get("title", ""),
            "text": (it.get("title", "") + " " + (it.get("body") or ""))[:4000],
            "url": it.get("html_url", ""),
            "when": (it.get("created_at") or "")[:10],
            "extra": "comments=%s" % it.get("comments", 0),
            "score": it.get("comments", 0) * 3,
        })
    return out


def source_library():
    """VCV Library manifests — ecosystem composition, not a feed.

    Deliberately ONE request for the timestamp cache plus one per plugin would
    be 553 requests, which is not courteous. So this reads the cache only, and
    reports ecosystem shape; a full tag census wants a shallow clone and is a
    separate, occasional job rather than part of a routine run.
    """
    url = ("https://raw.githubusercontent.com/VCVRack/library"
           "/v2/manifests-cache.json")
    data = json.loads(_get(url).decode("utf-8"))
    plugins = len(data)
    modules = sum(len(p.get("modules") or {}) for p in data.values())
    newest = sorted(
        ((p.get("creationTimestamp") or 0, slug) for slug, p in data.items()),
        reverse=True)[:5]
    return [{
        "source": "vcv-library",
        "id": "census",
        "title": "VCV Library census: %d plugins, %d modules" % (plugins, modules),
        "text": "ecosystem size census",
        "url": "https://github.com/VCVRack/library",
        "when": "",
        "extra": "newest plugins: " + ", ".join(s for _, s in newest),
        "score": 0,
    }]


# Keep in step with HYPOTHESIS_RE -- see the two kinds described there.
# `plugdata` (precedent) and `mirack` (competitor) added by A28; miRack is the
# one the surviving hypothesis is actually a claim about, so someone announcing
# an open-source answer to it is the single highest-value thing this routine
# could surface.
HYPOTHESIS_QUERIES = ["ios auv3", "ipad", "iphone modular", "plugdata", "mirack"]


def source_hypothesis():
    """Actively SEARCH for the standing hypothesis, rather than wait for it.

    The standing hypothesis, as corrected by A28 on 2026-08-20, is that no
    open-source EURORACK-STYLE RACK exists on iOS AUv3, and A9 says to watch for
    anyone moving into that gap. The broader form -- "no open-source modular" --
    was refuted by plugdata on 2026-08-18; docs/community-research.md carries
    the evidence and what follows from it. **Passive scanning cannot do that**, and this is
    measured, not assumed: across 48 real forum topics and Cardinal issues from
    a live run, the hypothesis filter matched **zero**. An iPad thread appears
    on that forum roughly once a year, so a routine reading the 30 newest
    topics would essentially never see one -- it would look like a working
    watch that had simply found nothing.

    Searching finds them immediately: this query surfaces "VCV Rack for iPad -
    2025?" and "VCV Rack on iOS/Android devices?" on the first call. Every hit
    here is flagged by construction, since matching the query IS the signal.
    """
    base = "https://community.vcvrack.com"
    seen, out = set(), []
    for q in HYPOTHESIS_QUERIES:
        data = _get_json("%s/search.json?q=%s" % (base, urllib.parse.quote(q)))
        for t in data.get("topics", []):
            tid = str(t.get("id"))
            if tid in seen:
                continue
            # THE MATCH MUST BE IN THE TITLE. Discourse search matches post
            # bodies, so without this the top hits are the forum's giant
            # megathreads -- "What are you listening to?" (6,135 replies) and
            # "Member Introductions" -- which merely contain the word "iPad"
            # somewhere in thousands of posts. Requiring the title keeps
            # "VCV Rack for iPad - 2025?" and "VCV Rack on iOS/Android
            # devices?", which are the actual signal, and drops the rest.
            if not HYPOTHESIS_RE.search(t.get("title", "")):
                continue
            seen.add(tid)
            out.append({
                "source": "vcv-forum-watch",
                "id": tid,
                "title": t.get("title", ""),
                # the query matched, so the hypothesis verdict is not in doubt
                "text": t.get("title", "") + " ios auv3",
                "url": "%s/t/%s/%s" % (base, t.get("slug", ""), tid),
                "when": (t.get("created_at") or "")[:10],
                "extra": "matched %r · replies=%s" % (q, max(t.get("posts_count", 1) - 1, 0)),
                # score 0 on purpose: these rank by RECENCY, not engagement.
                # The hypothesis is about someone moving into the gap *now*, so
                # a busy 2018 thread is worth less than a quiet 2026 one.
                "score": 0,
            })
    return out


SOURCES = {
    "forum":    ("VCV Community forum",   lambda: source_forum()),
    "cardinal": ("Cardinal issues",       lambda: source_issues("DISTRHO/Cardinal")),
    "surge":    ("Surge XT issues",       lambda: source_issues("surge-synthesizer/surge")),
    "library":  ("VCV Library census",    lambda: source_library()),
    "watch":    ("iOS/AUv3 hypothesis watch", lambda: source_hypothesis()),
}

# Deliberately absent, and why -- so nobody "helpfully" adds them:
#   Audiobus / Loopy Pro forums : no usable API; A9 makes them a QUARTERLY
#                                 HUMAN skim, not an automated source. This is
#                                 where the iOS AUv3 audience actually lives,
#                                 so the omission is a known gap, not an
#                                 oversight.
#   Reddit / KVR                : demoted by A9 -- low yield and ToS-hostile to
#                                 bulk fetch.


# --------------------------------------------------------------------------
# dedup: the backlog, and the rejection memory
# --------------------------------------------------------------------------

REJECTED_DOC = os.path.join(ROOT, "docs", "community-research-rejected.md")


def load_rejected():
    """Keys previously rejected by a human. One `- key` bullet per line."""
    if not os.path.exists(REJECTED_DOC):
        return set()
    keys = set()
    with open(REJECTED_DOC, encoding="utf-8") as fh:
        for line in fh:
            m = re.match(r"\s*-\s+`([^`]+)`", line)
            if m:
                keys.add(m.group(1).strip())
    return keys


def load_backlog_text():
    text = ""
    for name in ("BACKLOG.md", "BACKLOG-DONE.md"):
        p = os.path.join(ROOT, name)
        if os.path.exists(p):
            with open(p, encoding="utf-8") as fh:
                text += fh.read()
    return text


def already_known(item, backlog_text):
    """True if the backlog already cites this exact URL."""
    return bool(item["url"]) and item["url"] in backlog_text


def key_of(item):
    return "%s#%s" % (item["source"], item["id"])


# --------------------------------------------------------------------------
# output
# --------------------------------------------------------------------------

def render(items, top=0):
    if not items:
        return "No new items survived the filter. Nothing to propose."
    withheld = 0
    if top and len(items) > top:
        withheld = len(items) - top
        items = items[:top]
    out = ["PROPOSED rows — triage these; the routine decides nothing.", ""]
    for it in items:
        flag = "**[HYPOTHESIS]** " if it["verdict"] == "flag" else ""
        out.append("- %s%s" % (flag, it["title"].strip()))
        out.append("  - source: `%s`  ·  %s  ·  %s" %
                   (key_of(it), it["when"] or "n/d", it["extra"]))
        out.append("  - provenance: %s" % it["url"])
        out.append("")
    if withheld:
        # NO SILENT CAPS. A routine that quietly truncates reads as "this is
        # everything" when it is not, and that is worse than a long list.
        out.append("%d further item(s) passed the filter and were withheld by "
                   "--top; re-run with --top 0 to see them all." % withheld)
        out.append("")
    out.append("To reject one permanently, add its `source#id` key as a bullet")
    out.append("in docs/community-research-rejected.md with a one-line reason.")
    return "\n".join(out)


def run(selected, verbose=False):
    backlog = load_backlog_text()
    rejected = load_rejected()
    kept, stats = [], {"fetched": 0, "constraint": 0, "remembered": 0, "known": 0}

    for name in selected:
        label, fn = SOURCES[name]
        try:
            items = fn()
        except (urllib.error.URLError, urllib.error.HTTPError, ValueError) as e:
            print("  ! %-22s FAILED: %s" % (label, e), file=sys.stderr)
            continue
        print("  · %-22s %d item(s)" % (label, len(items)), file=sys.stderr)
        stats["fetched"] += len(items)

        for it in items:
            if key_of(it) in rejected:
                stats["remembered"] += 1
                continue
            if already_known(it, backlog):
                stats["known"] += 1
                continue
            verdict, why = classify(it["title"], it["text"])
            if verdict == "reject":
                stats["constraint"] += 1
                if verbose:
                    print("    rejected: %s\n      -> %s" % (it["title"][:70], why),
                          file=sys.stderr)
                continue
            it["verdict"] = verdict
            kept.append(it)

    # Rank: hypothesis matches first, then engagement, then recency. All
    # descending -- the first live run sorted ASCENDING by date and buried the
    # newest items under two-year-old ones, which is worth naming because it
    # looked like a fetch bug and was not: the API returns newest-first.
    kept.sort(key=lambda i: (i["verdict"] == "flag", i.get("score", 0), i["when"]),
              reverse=True)
    print("\n%d fetched · %d auto-rejected by a constraint · %d already rejected "
          "before · %d already in the backlog · %d proposed\n"
          % (stats["fetched"], stats["constraint"], stats["remembered"],
             stats["known"], len(kept)), file=sys.stderr)
    return kept


# --------------------------------------------------------------------------
# selftest -- offline, so it can run anywhere and in CI
# --------------------------------------------------------------------------

def selftest():
    cases = [
        # (text, expected verdict) -- the constraint filter
        ("Please add ASIO device selection to the settings", "reject"),
        ("Add a standalone application version", "reject"),
        ("Browse for a sample to load into the sampler", "reject"),
        ("Support scanning a third-party module folder", "reject"),
        ("Custom skins would be great, a theme folder please", "reject"),
        ("Make the editor dockable with multiple windows", "reject"),
        ("Add plugin export so I can ship my patch as a VST", "reject"),
        # the hypothesis wins over a reject rule, on purpose
        ("iOS AUv3 build with a module scan folder", "flag"),
        ("Any chance of an iPad version?", "flag"),
        ("Does this run in AUM or Loopy Pro?", "flag"),
        # A28: plugdata is the closest open-source precedent for the iOS AUv3
        # problems TIDE has ahead of it, so it is worth surfacing by name. This
        # case is `reject`-baited on purpose -- "standalone" would normally be
        # rejected under constraint 2, and the hypothesis has to win.
        ("How does plugdata ship a standalone AND an AUv3?", "flag"),
        ("plugdata 0.9.3 released", "flag"),
        # miRack is the COMPETITOR half -- see HYPOTHESIS_RE. Someone shipping an
        # open-source answer to it is the highest-value item this routine can
        # find, so a bare mention with no other hypothesis term must still flag.
        ("miRack 4.6 adds a new sequencer", "flag"),
        # Negative controls for both halves. Without these, "everything is
        # flagged" would pass every case above and nobody would know.
        ("Pure Data style dataflow patching in a rack", "keep"),
        ("Rack-style sequencer module request", "keep"),
        # ordinary signal survives
        ("Polyphonic oscillator aliasing above 8 kHz", "keep"),
        ("Cables are hard to see against a dark background", "keep"),
        ("Feature request: undo for cable deletion", "keep"),
    ]
    # (title, body, expected) -- the title/body split, pinned with the REAL
    # incident from the first live run rather than an invented example.
    body_cases = [
        ("Surge XT CLAP crashes REAPER on load (SIGSEGV in JUCE repaint)",
         "the crash appears specific to the CLAP build/wrapper. "
         "The standalone app also works fine.",
         "keep"),   # was 'reject' before the title-only fix -- a lost crash report
        ("Improve initial template Welcome message text",
         "mentions the standalone app in passing", "keep"),
        # a genuine feature request in the TITLE is still rejected
        ("Please add a standalone application version", "", "reject"),
        # the hypothesis still reads the body, deliberately
        ("Cables render oddly", "this only happens on my iPad in AUM", "flag"),
        # A28: every HYPOTHESIS_QUERIES term must also match a TITLE, because
        # source_hypothesis() drops hits whose title HYPOTHESIS_RE misses. A
        # query with no matching term returns nothing and looks like a quiet
        # week -- the failure shape docs/community-research.md is built around.
        ("plugdata adds AUv3 effects", "", "flag"),
    ]

    # A28: assert that coupling directly, rather than trusting the cases above
    # to notice if someone adds a query later. The positive control for THIS
    # assertion has to be a term that will never legitimately be added -- an
    # earlier draft used "drambo", which became a real regex term hours later
    # and would have silently disarmed the control.
    assert not HYPOTHESIS_RE.search("zzq-not-a-product"), \
        "the uncoupled-query control needs a term HYPOTHESIS_RE does not match"
    for q in HYPOTHESIS_QUERIES:
        if not HYPOTHESIS_RE.search(q):
            print("FAIL query %r matches no HYPOTHESIS_RE term -- "
                  "source_hypothesis() would discard every hit for it" % q)
            bad_queries = True
            break
    else:
        bad_queries = False

    bad = 1 if bad_queries else 0
    for title, body, want in body_cases:
        got, why = classify(title, body)
        if got != want:
            bad += 1
            print("FAIL body-case %-44s want=%-6s got=%-6s (%s)"
                  % (title[:44], want, got, why))

    for text, want in cases:
        got, why = classify(text)
        if got != want:
            bad += 1
            print("FAIL %-52s want=%-6s got=%-6s (%s)" % (text[:52], want, got, why))
    # the guardrail itself is testable
    try:
        _get("http://example.com/insecure")
        print("FAIL http:// was not refused")
        bad += 1
    except ValueError:
        pass
    # rejection-memory parsing
    keys = load_rejected()
    print("%d classification case(s), %d failed; rejection memory holds %d key(s)"
          % (len(cases) + len(body_cases), bad, len(keys)))
    return 1 if bad else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--source", action="append", choices=sorted(SOURCES),
                    help="limit to one source (repeatable); default is all")
    ap.add_argument("--selftest", action="store_true", help="offline checks, no network")
    ap.add_argument("--dry-run", action="store_true", help="list sources and exit")
    ap.add_argument("--top", type=int, default=12, metavar="N",
                    help="show only the N highest-ranked items (0 = all). "
                         "The count withheld is always reported.")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="show each auto-rejected item and the constraint that did it")
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    selected = args.source or sorted(SOURCES)
    if args.dry_run:
        print("would read, GET-only, >=%.1fs apart, as %r:" % (COURTESY_SECONDS, UA))
        for n in selected:
            print("  %-10s %s" % (n, SOURCES[n][0]))
        return 0

    print("reading %d source(s), GET-only, >=%.1fs apart"
          % (len(selected), COURTESY_SECONDS), file=sys.stderr)
    print(render(run(selected, args.verbose), args.top))
    return 0


if __name__ == "__main__":
    sys.exit(main())
