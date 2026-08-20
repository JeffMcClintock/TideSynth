#!/usr/bin/env python3
"""BACKLOG A30 — build docs/lessons.md from every entry's **Learned** section.

Rotation (JOURNAL.md's own rules) moves entries into JOURNAL-<YYYY>-<MM>.md, and
no run reads the archive: the prompt's reading list names JOURNAL.md only. So a
lesson was load-bearing for about a day and then silently stopped being read.

This regenerates the digest from both files, so it cannot drift from them:

    python3 scripts/extract-lessons.py --write        # rewrite docs/lessons.md
    python3 scripts/extract-lessons.py --check        # CI: is it up to date?
    python3 scripts/extract-lessons.py --measure      # the size argument

WHY A HEADLINE INDEX AND NOT THE BULLETS THEMSELVES. Measured 2026-08-20 over
167 entries: the Learned sections total **223 KB**, 24% of the whole corpus.
Copying them verbatim into a file every run reads would be **worse than the
192 KB that triggered A8 in the first place** -- the same mistake A24 measured
and rejected for a 7-day journal floor. The journal's own convention saves this:
each bullet is written as a bold claim followed by its working, so the bold
lead-in alone is a real lesson, not a title. Keeping only those gives **56 KB /
599 lessons across 152 entries -- 4.1x smaller**, with every entry represented.

TWO COSTS, both stated rather than hidden:

  1. **The working is dropped.** A line tells you the claim and where to read the
     rest; it does not re-argue it.
  2. **This grows, and not slowly.** ~4 lessons per entry at ~90 bytes is roughly
     360 bytes an entry, and this fleet writes ~10 entries a day -- call it
     3.5 KB a day, ~100 KB a month. It is affordable now and will not be by
     October. Pruning is a real decision someone has to make; see the preamble.
"""
import argparse
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
SOURCES = ["JOURNAL.md", "JOURNAL-2026-08.md"]
OUT = REPO / "docs" / "lessons.md"

ENTRY = re.compile(r"\n(?=## )")
# "**Learned:**" and "**Learned - why the check belongs before the push:**" both
# occur; end at the next top-level bold heading that is not another Learned.
BLOCK = re.compile(r"^\*\*Learned\b.*?(?=^\*\*(?!Learned)[A-Z])", re.S | re.M)
ITEM = re.compile(r"^\s*(?:\d+\.|[-*])\s+(.*?)(?=^\s*(?:\d+\.|[-*])\s|\Z)", re.S | re.M)
LEAD = re.compile(r"\*\*(.+?)\*\*", re.S)
# Headings are "## <date> — <machine> — <topic>", but not all of them: interactive
# entries and a few early ones use other shapes. Match the date, then take the
# machine only when a second separator actually delimits one -- an over-strict
# pattern here silently dropped half the corpus (measured: 72 entries kept of
# 152 with lessons) and the digest looked complete while missing 80 entries.
HEADING = re.compile(r"^## (\d{4}-\d{2}-\d{2})\s*[—-]\s*(.*)$")


def split_heading(rest):
    """-> (machine, topic). Machine only if the first field looks like one."""
    parts = [p.strip() for p in rest.split("—", 1)]
    known = {"linux", "macos", "windows", "win", "mac"}
    if len(parts) == 2 and parts[0].split("(")[0].strip().lower() in known:
        return parts[0], parts[1]
    return "", rest.strip()


def first_claim(item):
    """The bold lead-in if there is one, else the first sentence.

    The fallback must not split inside inline code: `.pc` and `foo.cpp:31` are
    everywhere in this journal, and a naive split(".") truncated mid-token.
    """
    lead = LEAD.search(item)
    if lead:
        return lead.group(1)
    text = " ".join(item.split())
    out, ticks = [], False
    for i, ch in enumerate(text):
        if ch == "`":
            ticks = not ticks
        # A sentence ends at ". " outside code, not at every dot.
        if ch == "." and not ticks and (i + 1 >= len(text) or text[i + 1] == " "):
            break
        out.append(ch)
    return "".join(out)


LEARNED_HDR = re.compile(r"^\*\*Learned\b(.*?)\*\*", re.S)


def claims(block):
    """One line per lesson in a **Learned** block.

    THREE SHAPES occur in this journal and only the first is a markdown list, so
    an ITEM-only reader silently kept 72 of the 152 entries that have lessons --
    it looked complete and dropped 80 entries. Measured 2026-08-20:

      1. "**Learned:**" followed by "1." / "-" list items      (72 entries)
      2. "**Learned:** <one prose paragraph>"                  }  the other 80
      3. "**Learned - headline.** **1. Claim...** ..."         }
    """
    out = []
    hdr = LEARNED_HDR.match(block)
    tail = block[hdr.end():] if hdr else block

    # Shape 3's headline is itself a claim; "Learned:" alone is not.
    if hdr:
        head = " ".join(hdr.group(1).split()).lstrip("—- ").rstrip(":. ")
        if len(head) >= 12:
            out.append(head)

    items = ITEM.findall(tail)
    if items:
        for item in items:
            out.append(first_claim(item))
    else:
        bolds = [b for b in LEAD.findall(tail)]
        if bolds:
            out.extend(bolds)
        else:
            out.append(first_claim(tail))

    seen, keep = set(), []
    for text in out:
        text = " ".join(text.split()).rstrip(":").strip()
        # Strip a leading enumerator left by shape 3's "**1. Claim**".
        text = re.sub(r"^\d+\.\s*", "", text)
        # A stray bold phrase mid-sentence is not a lesson.
        if len(text) < 12 or text.lower() in seen:
            continue
        seen.add(text.lower())
        keep.append(text)
    return keep


def harvest():
    """[(date, machine, topic, [headline, ...]), ...] newest first, source order.

    Deduplicated across the whole corpus, newest wins -- a digest that repeats
    itself is just the journal again. Measured 2026-08-20: only 2 lines collide,
    so this is about the invariant, not the bytes.
    """
    out = []
    seen = set()
    for name in SOURCES:
        path = REPO / name
        if not path.exists():
            continue
        for chunk in ENTRY.split(path.read_text(encoding="utf-8")):
            if not chunk.startswith("## 2026-"):
                continue
            head = chunk.split("\n", 1)[0]
            m = HEADING.match(head)
            if not m:
                continue
            date = m.group(1)
            machine, topic = split_heading(m.group(2))
            lines = []
            for block in BLOCK.findall(chunk):
                for text in claims(block):
                    key = "".join(ch for ch in text.lower() if ch.isalnum() or ch == " ")
                    if key in seen:
                        continue
                    seen.add(key)
                    lines.append(text)
            if lines:
                out.append((date, machine, topic, lines))
    return out


PREAMBLE = """# Lessons — the standing digest

**Every run reads this. Rotation cannot age it out — that is the whole point.**

`JOURNAL.md` is rotated to stay under 60 KB (**A8**, floored by **A24**), and no
run reads `JOURNAL-<YYYY>-<MM>.md` — the prompt's reading list names the live
journal only. So until **A30** a lesson was load-bearing for about a day and then
silently stopped being read, while the entry that carried it sat in an archive
nobody opens.

**This file is GENERATED. Do not hand-edit it** — run
`python3 scripts/extract-lessons.py --write`. It is rebuilt from both journal
files, so it cannot drift from them, and adding a lesson means writing a
**Learned** bullet in your entry exactly as you already do.

**One line per bullet: the claim, not its working.** The journal's convention is
that each Learned bullet opens with a bold claim and then argues it; this keeps
the claim and drops the argument. Measured 2026-08-20: the Learned sections are
**223 KB** across 167 entries, so copying them whole into a file every run reads
would be worse than the 192 KB that triggered A8. This is **4.1x smaller** and
still represents **every** entry that has a lesson — 152 of them, none dropped.

**To read the working**, find the entry by its date and machine — in
[JOURNAL.md](../JOURNAL.md) if recent, else
[JOURNAL-2026-08.md](../JOURNAL-2026-08.md).

**This file GROWS, and someone will have to prune it.** ~4 lessons an entry at
~90 bytes is ~360 bytes per entry, and this fleet writes ~10 entries a day —
roughly **3.5 KB a day, ~100 KB a month**. Affordable today at 56 KB; not by
October. Stated here rather than discovered later, because that is exactly how
A8 happened.

**Pruning means fixing the SOURCE, never editing this file or the archive.** It
is regenerated from the journals, so a lesson that no longer holds is corrected
by a newer entry saying so. The lever nobody has pulled yet: drop `SOURCES`'
archive file once its lessons are genuinely spent, which halves this at a
stroke — that is a judgement call and belongs to Jeff, not to a run.

"""


def render(data):
    body = [PREAMBLE]
    n = 0
    current = None
    for date, machine, topic, lines in data:
        if date != current:
            body.append(f"## {date}\n")
            current = date
        label = f"{machine} — {topic}" if machine else topic
        body.append(f"**{label}**\n")
        for text in lines:
            body.append(f"- {text}")
            n += 1
        body.append("")
    return "\n".join(body).rstrip() + "\n", n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--measure", action="store_true")
    args = ap.parse_args()

    data = harvest()
    text, n = render(data)

    if args.measure:
        raw = 0
        for name in SOURCES:
            path = REPO / name
            if not path.exists():
                continue
            for chunk in ENTRY.split(path.read_text(encoding="utf-8")):
                if chunk.startswith("## 2026-"):
                    raw += sum(len(b) for b in BLOCK.findall(chunk))
        print(f"entries with lessons : {len(data)}")
        print(f"lesson headlines     : {n}")
        print(f"Learned sections     : {raw:,} bytes")
        print(f"this digest          : {len(text):,} bytes")
        print(f"ratio                : {raw / max(len(text), 1):.1f}x smaller")
        return 0

    if args.check:
        if not OUT.exists():
            print(f"{OUT.relative_to(REPO)} does not exist -- run --write", file=sys.stderr)
            return 1
        if OUT.read_text(encoding="utf-8") != text:
            print(f"{OUT.relative_to(REPO)} is stale -- run --write", file=sys.stderr)
            return 1
        print(f"docs/lessons.md up to date -- {n} lessons from {len(data)} entries")
        return 0

    if args.write:
        OUT.write_text(text, encoding="utf-8")
        print(f"wrote {OUT.relative_to(REPO)} -- {n} lessons from {len(data)} entries, {len(text):,} bytes")
        return 0

    ap.print_help()
    return 1


if __name__ == "__main__":
    sys.exit(main())
