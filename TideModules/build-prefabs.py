#!/usr/bin/env python3
"""Regenerate TIDE's rack prefabs (BACKLOG E2a).

The three .synthedit files in prefabs/ are BUILD OUTPUT, not hand-written
XML -- run this to reproduce them. Authoring them by hand was rejected: the
graph half (handles, <lines>, the IO Mods --containerise synthesises) is exactly
the part a human gets wrong silently, and SynthEditCL already builds it
correctly. What the CLI cannot do is place things on a panel, so this script
does the graph with the CLI and the LAYOUT itself, in that order.

Why each prefab is a Container with 'Visible' on: that is what makes it render
as a panel in the parent view -- the rack (PLAN constraint 1, and the Visible
pin is the container's GetPlug(2), asserted by name at CContainer.cpp:3301).

Usage:  python3 build-prefabs.py [--secl <path to SynthEditCL binary>]
"""
import argparse
import json
import pathlib
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

HERE = pathlib.Path(__file__).resolve().parent
DEFAULT_SECL = (HERE / ".." / "build" / "SynthEditCL" / "Release"
                / "SynthEditCL.app" / "Contents" / "MacOS" / "SynthEditCL").resolve()

# Panel geometry, in the container's OWN panel space -- origin 0,0.
#
# Two different rects, and confusing them is why the first cut of these prefabs
# loaded into the rack and drew nothing at all:
#
#   PanelWndPosition  the container's rect IN ITS PARENT, and what the rack
#                     actually draws (CContainer::getViewObRect returns exactly
#                     this for CF_PANEL_VIEW, CContainer.cpp:3332). SynthEditCL
#                     saves it as 0,0,0,0 because no panel window was ever
#                     opened at a position -- a zero-SIZE box, so the drop
#                     succeeded, the container existed, its properties showed,
#                     and the rack stayed empty. The drop path offsets the
#                     position; the size has to come from the file.
#   panel_rect        the container's own internal canvas.
#
# The reference is Controls/LED2.syntheditprefab, the shipped prefab of the same
# shape: PanelWndPosition 70,24..90,44 and panel_rect 0,0..93,45.
PP = 20                      # patch point edge length, as the view finalises it
SLOT_W, SLOT_H = 100, 160    # the rack slot a bare (jacks-only) module occupies
JACK_X = 40                  # jacks in a centred column
JACK_TOP = 40
JACK_PITCH = 46              # vertical spacing between jacks

# ---- faceplate metrics, for a prefab that has a background and captions ----
#
# THE SIZE IS NOT WHAT YOU WRITE, IT IS WHAT THE CHILDREN ADD UP TO. Measured
# 2026-08-19: one panel-view pass over the shipped MidiCv.synthedit rewrote its
# authored PanelWndPosition 0,0..100,160 to 0,0..20,158 -- exactly the union of
# its four jacks -- and saved that. SubView::measure (SubViewPanel.cpp:178)
# discards the size offered to it and returns the union of every visible
# non-connector child, and ViewBase::arrange (ViewBase.cpp:2342-2390) pushes the
# difference back through ResizeModule whenever it exceeds a 3px tolerance.
#
# So a faceplate is not decoration: the BACKGROUND RECT IS THE SIZING MECHANISM.
# Make it span the whole slot and the declared rect becomes self-consistent, which
# is why these prefabs get one and the bare four do not.
#
# On the rack grid, as ruled 2026-08-21 (BACKLOG E5): RackLayout is hpWidth 15
# (the VISUAL HP -- rail hole pitch), snapWidth 3 (the PLACEMENT pitch, and
# gcd(12,15), so SynthEdit's 12s and VCV's 15s both land exactly), rowHeight 384
# and railHeight 15. A slot is a multiple of 3 wide and up to 384 tall.
#
# 384, NOT 350: the rails are drawn as BACKGROUND by ViewBase::renderRack and
# the panel covers them, exactly as a real Eurorack panel covers the rails it
# is screwed to -- TIDE draws no mounting hardware. The old "380-2*15 = 350"
# here subtracted a rail allowance that does not exist, which is what made
# TIDE's own prefab measure as overlapping the rails.
# E15: FACE_W follows the panel, not the other way round. SE TiDE:Panel sizes
# ITSELF at RackUnits x 48 DIPs (TiDEPanelGui kRackUnitDips), and SubView's
# measure unions the children -- so an authored width the plate cannot be
# produces overhanging captions on the dark rack, which is what the first
# regeneration showed. 2 rack units = 96; labels end at 58 and jacks at 90,
# so everything sits on the plate. (The 3.2-HP-per-U mismatch with RackLayout's
# 15-DIP HP is E17's own open note, not this row's to settle.)
FACE_W, FACE_H = 96, 384
CAPTION_H = 18               # heading strip at the top
LABEL_W, LABEL_H = 52, 16
ROW_TOP = 44                 # first jack row, below the caption
ROW_PITCH = 70
LABEL_X = 6
FACE_JACK_X = 70             # jacks right-aligned, captions to their left


class Prefab:
    """One rack module: the CLI script that builds its graph, plus its jack order."""

    def __init__(self, name, filename, build_lines, jacks, caption, jack_labels,
                 select=None, post=None, background=None, labels=None,
                 caption_label=None):
        self.name = name
        self.filename = filename
        self.build_lines = build_lines
        self.jacks = jacks             # alias order, top to bottom on the panel
        self.caption = caption         # faceplate heading
        self.jack_labels = jack_labels # one per jack, same order
        # Both default to what the first four prefabs need, so those entries
        # are untouched.
        #   select: aliases to select before --containerise. Default ALL, which
        #           is right when every module belongs inside. The root-MIDI-CV
        #           prefab needs a SUBSET: it containerises only its jacks and
        #           leaves scaffolding outside, to be deleted afterwards.
        #   post:   extra CLI lines run AFTER --containerise, before the save --
        #           exactly that cleanup.
        self.select = select
        self.post = post or []
        # A prefab with a faceplate names its graphics, and lay_out_panel then
        # lays the whole module out instead of only the jack column:
        #   background:    alias of the rect that spans the slot (SIZING, see above)
        #   labels:        alias per jack, same order as `jacks`
        #   caption_label: alias of the heading label
        self.background = background
        self.labels = labels or []
        self.caption_label = caption_label

    @property
    def has_faceplate(self):
        return self.background is not None


PREFABS = [
    # Output first: nothing renders without an egress, and it is the one prefab
    # every other E1 case needs. Sound Out's input pin is IO_AUTODUPLICATE
    # (ug_soundcard_out.cpp:20), so the second --connect makes a second channel
    # rather than stealing the first -- that is why L and R both target :Out.
    Prefab(
        "TIDE Output", "Output.synthedit",
        [
            '--add-module "SE Patch Point in" 60,80  --as inL',
            '--add-module "SE Patch Point in" 60,200 --as inR',
            '--add-module "Sound Out"         360,120 --as snd',
            '--connect $inL:Output $snd:Out',
            '--connect $inR:Output $snd:Out',
        ],
        ["inL", "inR"], "OUTPUT", ["L", "R"],
    ),
    # Envelope. THIS IS AN ENVELOPE *AND A VCA*, and that is deliberate: E2a's
    # three prefabs have to chain into a working voice or they do not serve the
    # V1 acceptance test they exist for. A bare ADSR emits control voltage and
    # has no audio path at all, so osc -> env -> out would be silent no matter
    # how correctly each part worked. Multiply is the VCA.
    #
    # Overall Level is set to 1 (its default is 10) so the envelope's output is
    # 0..1 and Multiply is unity-gain -- at the default the VCA would multiply
    # audio by ten and clip hard.
    #
    # The Gate jack defaults OPEN (10). An unpatched Eurorack VCA passes signal;
    # more to the point, the minimal three-module rack has nothing to patch a
    # gate from, so a default of 0 would make V1's own test render silence.
    # Patch a gate in and it behaves as an envelope.
    Prefab(
        "TIDE Envelope", "Envelope.synthedit",
        [
            '--add-module "SE Patch Point in"  60,80   --as audio',
            '--add-module "SE Patch Point in"  60,240  --as gate',
            '--add-module "ADSR"               260,240 --as adsr',
            '--add-module "Multiply"           520,80  --as vca',
            '--add-module "SE Patch Point out" 760,80  --as out',
            '--connect $gate:Output       $adsr:Gate',
            '--connect $audio:Output      $vca:"Input 1"',
            '--connect $adsr:"Signal Out" $vca:"Input 2"',
            '--connect $vca:Output        $out:Input',
            '--set-pin $adsr:"Overall Level" 1',
            '--set-pin $gate:Input 10',
        ],
        ["audio", "gate", "out"], "ENVELOPE", ["IN", "GATE", "OUT"],
    ),
    # Oscillator. The classic statically-registered "Oscillator"
    # (ug_oscillator2.cpp:60, INIT_STATIC_FILE at UgDatabase.cpp:1224) -- NOT
    # "OscillatorNaive", which S8 correctly measured as absent from the binary
    # because it is a separately-loaded .gmpi module, not a static one.
    Prefab(
        "TIDE Oscillator", "Oscillator.synthedit",
        [
            '--add-module "SE Patch Point in"  60,80   --as pitch',
            '--add-module "Oscillator"         260,80  --as osc',
            '--add-module "SE Patch Point out" 520,80  --as out',
            '--connect $pitch:Output      $osc:Pitch',
            '--connect $osc:"Audio Out"   $out:Input',
            # 1 V/octave, 5 V = middle A. An unpatched jack outputs its Input
            # default, so without this the module free-runs at 13.75 Hz -- below
            # hearing, and indistinguishable from "the oscillator is broken".
            '--set-pin $pitch:Input 5',
        ],
        ["pitch", "out"], "OSC", ["PITCH", "OUT"],
    ),
    # Filter. Added 2026-08-19 for BACKLOG E2b -- the first module beyond E2a's
    # three, and the one that makes the set musical rather than merely audible:
    # oscillator -> filter -> envelope -> out is the canonical subtractive voice.
    #
    # `1 Pole LP` is REGISTER_MODULE_1 in ug_filter_1pole_lp.cpp:21, category
    # Filters, and it is CORE -- so TIDE links it. Verified the way E12's entry
    # recommends, by the static-init SYMBOL rather than by the module-id string:
    # `nm` finds se_static_library_init_ug_filter_1pole in the TIDE binary. The
    # same test reports SVFilter4, ButterworthHP and OscillatorNaive ABSENT, so
    # it discriminates rather than always saying yes. Do NOT reach for those
    # three: they are separately-built modules, not core, and a prefab naming
    # one places at the right size and draws nothing (S8's shape exactly).
    #
    # Pins are Signal / Pitch / Output, i.e. the Oscillator prefab's shape with
    # an audio input added. Pitch is the CUTOFF and is 1 V/octave on the same
    # scale as the oscillator's: 5 V = 440 Hz.
    #
    # THE FREQ JACK DEFAULTS WIDE OPEN (10 V = 14 kHz), and that is the same
    # rule the Envelope's GATE default follows: an unpatched jack takes the
    # value that lets the module PASS SIGNAL, because a rack that does not patch
    # everything must still sound. A default of 0 would make a dropped-in filter
    # render silence and look broken.
    #
    # Measured 2026-08-19 on a 440 Hz source, recording $filt:Output, which is
    # also why this module needs no faceplate to prove itself -- it has a real
    # audio path, unlike Output and MIDI-CV (see tests/README.md):
    #
    #   cutoff 10 V (14 kHz)  peak  -6.3 dBFS   <- effectively open
    #   cutoff  8 V (3.5 kHz) peak  -6.6 dBFS
    #   cutoff  5 V (440 Hz)  peak  -9.5 dBFS   <- -3 dB AT cutoff, textbook 1-pole
    #   cutoff  2 V (55 Hz)   peak -22.2 dBFS   <- ~6 dB/octave beyond it
    Prefab(
        "TIDE Filter", "Filter.synthedit",
        [
            '--add-module "SE Patch Point in"  60,80   --as audio',
            '--add-module "SE Patch Point in"  60,240  --as cutoff',
            '--add-module "1 Pole LP"          260,80  --as filt',
            '--add-module "SE Patch Point out" 520,80  --as out',
            '--connect $audio:Output  $filt:Signal',
            '--connect $cutoff:Output $filt:Pitch',
            '--connect $filt:Output   $out:Input',
            '--set-pin $cutoff:Input 10',
        ],
        ["audio", "cutoff", "out"], "FILTER", ["IN", "FREQ", "OUT"],
    ),
    # MIDI. Added 2026-08-18 for BACKLOG V3 -- PLAN's v0.1 acceptance test asks for
    # a rack the user can "play from the DAW's MIDI", and until this prefab there
    # was nothing on the rack MIDI could arrive through: the other three sound
    # continuously off their pin defaults, which is deliberate but means a MIDI
    # failure is invisible to them.
    #
    # THIS IS THE MONOPHONIC VERSION AND IT WORKS. Measured on a saved project
    # with one middle-C note, on at 0.500 s and off at 1.200 s:
    #   0.05-0.45 s silent | 0.60-1.10 s 440.0 Hz | 1.35-1.95 s silent
    #
    # `SE MIDItoGate2` is monophonic, so it does NOT make its container a voice
    # container -- and polyphony cannot escape a container, which is E7. Its
    # output therefore crosses a patch cable where `SE MIDI to CV 2`'s does not.
    #
    # WHAT IT COSTS: no PITCH. MIDItoGate2 has none, so a rack played through this
    # sounds at whatever the Oscillator's own PITCH jack defaults to (5 V =
    # 440 Hz). Notes start and stop; every note is the same pitch. `SE MIDI to CV 2`
    # is the module you actually want and is kept as PROBE C for the day E7 lands.
    #
    # Gate and Trigger are datatype="bool", not audio-rate float. That connection
    # only carries anything because TIDE now links the type converters: the DSP
    # graph auto-inserts "SE BoolToVolts" (ug_base.cpp:1751), and until
    # Converters.cpp joined SynthEditSem's source list that lookup missed and the
    # connection was SILENTLY abandoned -- `assert(false); return;` with the assert
    # compiled out in Release. Do not remove that source-list entry.
    Prefab(
        "TIDE MIDI", "Midi.synthedit",
        [
            '--add-module "MIDI In"            60,80   --as midiin',
            '--add-module "SE MIDItoGate2"     260,80  --as mtg',
            '--add-module "SE Patch Point out" 520,80  --as gate',
            '--add-module "SE Patch Point out" 520,200 --as trig',
            '--connect $midiin:"MIDI Data" $mtg:"MIDI In"',
            '--connect $mtg:Gate    $gate:Input',
            '--connect $mtg:Trigger $trig:Input',
        ],
        ["gate", "trig"], "MIDI", ["GATE", "TRIG"],
    ),
    # ROOT MIDI-CV -- V3, Jeff's design, and the one prefab that is NOT
    # self-contained: it is a FACADE. It holds nothing but jacks, and every jack
    # is fed from OUTSIDE the container by the real `SE MIDI to CV 2` that TIDE
    # places at the document ROOT (TideApp.cpp, right after OnNewDocument).
    #
    # WHY. `SE MIDI to CV 2` is polyphonicSource/cloned, so wherever it sits
    # becomes a voice container -- and polyphony cannot escape a container (E7).
    # Put it inside a rack module and its Gate is correct internally and worth
    # nothing outside. Put it at the ROOT and the root is the voice context, so
    # it works polyphonically; this container then only has to carry the CV
    # inward, which is an ordinary signal connection, not a polyphonic escape.
    #
    # HOW THE IO WORKS, measured rather than assumed. Connections that cross a
    # container boundary go via an `IO Mod` inside and the container's own pins
    # outside. `--containerise` synthesises both -- but only for connections that
    # ALREADY cross the selection boundary, which is why this recipe builds
    # throwaway `SE Patch Point in` scaffolding at root, wires it to each jack,
    # containerises ONLY the jacks, and then deletes the scaffolding. What is
    # left is a container with one outer `Input` pin per jack and nothing else.
    #
    # THE PIN MAPPING TIDE RELIES ON, verified for 1, 2 and 4 jacks:
    #   container outer input pin = 7 + jack index      (jack order = below)
    #   pin 7 = PITCH, 8 = GATE, 9 = VEL, 10 = TRIG
    # A Container's pins 0..6 are its own built-ins (pin 2 is Visible), so 7 is
    # the first synthesised IO pin. TideApp hard-codes that mapping; if the jack
    # list here changes, that code changes with it.
    #
    # 2026-08-19: this one now has a FACEPLATE -- a background rect and a caption
    # per jack -- which is what makes it a rack module rather than four floating
    # jacks. Three things about that are load-bearing:
    #
    #  1. THE BACKGROUND IS THE SIZE. See FACE_W/FACE_H above: the container's
    #     declared rect is overwritten by the union of its visible children, so
    #     the rect spanning the slot is what gives the module its footprint.
    #  2. THE CAPTIONS ARE BLACK, so the faceplate is LIGHT. `SE Label` has no
    #     colour pin; its colour comes from the skin font style and the fallback
    #     is opaque black. TIDE's bundle ships no skin, so light-on-dark is not
    #     available and a dark faceplate would render invisible text -- in the
    #     DAW, while looking fine in a SynthEditCL screenshot, because the CLI
    #     resolves an external skin folder the bundle does not have.
    #  3. THE SCAFFOLDING'S Y ORDER STILL DECIDES THE PIN NUMBERS. OnEditContain
    #     sorts crossing input lines by the source module's structure-view top
    #     (CContainer.cpp:2053-2065) and allocates one outer pin each, so
    #     0/60/120/180 below is not cosmetic -- it is the 7=PITCH, 8=GATE, 9=VEL,
    #     10=TRIG contract TideApp.cpp hard-codes. Adding inert graphics CANNOT
    #     move it (PlugSortOrderOK sorts every IO plug after every non-IO plug,
    #     plug4.cpp:510-529, so 7 is always the first synthesised IO pin) but
    #     adding another CROSSING connection would.
    Prefab(
        "TIDE MIDI-CV", "MidiCv.synthedit",
        [
            '--add-module "SE Patch Point in"  0,0   --as scaffold_pitch',
            '--add-module "SE Patch Point in"  0,60   --as scaffold_gate',
            '--add-module "SE Patch Point in"  0,120   --as scaffold_vel',
            '--add-module "SE Patch Point in"  0,180   --as scaffold_trig',
            '--add-module "SE Patch Point out" 400,0 --as pitch',
            '--add-module "SE Patch Point out" 400,60 --as gate',
            '--add-module "SE Patch Point out" 400,120 --as vel',
            '--add-module "SE Patch Point out" 400,180 --as trig',
            '--connect $scaffold_pitch:Output $pitch:Input',
            '--connect $scaffold_gate:Output $gate:Input',
            '--connect $scaffold_vel:Output $vel:Input',
            '--connect $scaffold_trig:Output $trig:Input',
            # Faceplate: TIDE's own panel (E15). Its look is compile-time --
            # Jeff's 2026-08-19 ruling, the appearance is TIDE's and not the
            # patch author's -- so unlike the SE Rectangle XP it replaces there
            # are no colour pins to set here. Its Text caption stays empty: the
            # heading Label below already carries the name, and this row swaps
            # the mechanism while keeping the look.
            '--add-module "SE TiDE:Panel" 700,0 --as bg',
            # Width is the ONE pin the swap sets: it is per-instance geometry,
            # not appearance (those are compile-time by the 2026-08-19 ruling).
            '--set-pin $bg:"Rack Units" 2',
            '--add-module "SE Label" 700,60  --as heading',
            '--add-module "SE Label" 700,120 --as lbl_pitch',
            '--add-module "SE Label" 700,180 --as lbl_gate',
            '--add-module "SE Label" 700,240 --as lbl_vel',
            '--add-module "SE Label" 700,300 --as lbl_trig',
            '--set-pin $heading:Text "MIDI-CV"',
            '--set-pin $lbl_pitch:Text PITCH',
            '--set-pin $lbl_gate:Text GATE',
            '--set-pin $lbl_vel:Text VEL',
            '--set-pin $lbl_trig:Text TRIG',
        ],
        ["pitch", "gate", "vel", "trig"], "MIDI-CV", ["PITCH", "GATE", "VEL", "TRIG"],
        # --add-module leaves what it inserted SELECTED, so without an explicit
        # deselect the graphics would be swept into the container along with the
        # jacks -- which is wanted here, but by accident. build() now emits
        # --deselect-all before this list, so the selection is exactly this.
        select=["pitch", "gate", "vel", "trig",
                "bg", "heading", "lbl_pitch", "lbl_gate", "lbl_vel", "lbl_trig"],
        post=[
            '--delete $scaffold_pitch',
            '--delete $scaffold_gate',
            '--delete $scaffold_vel',
            '--delete $scaffold_trig',
        ],
        background="bg",
        labels=["lbl_pitch", "lbl_gate", "lbl_vel", "lbl_trig"],
        caption_label="heading",
    ),
]

# Diagnostics, NOT product. Generated only with --diagnostics, so the shipped
# set stays at four; kept here because the experiment they ran is the most
# useful thing V3 produced and it should be repeatable rather than retold.
#
# Each sends MIDI-CV 2's Gate and Pitch to a Sound Out INSIDE its own container,
# so no patch cable is in the path and the only question left is whether MIDI
# becomes a note. Sound Out's input is IO_AUTODUPLICATE, so two connects give
# L = Gate, R = Pitch.
#
# MEASURED 2026-08-18, one note (middle C) on at 0.500 s, off at 1.200 s:
#
#   PROBE A (with MIDI In)     Gate 0 0 0 0 0 1 1 1 1 1 1 1 0 0 0 ...  <-- tracks the note
#   PROBE B (without MIDI In)  Gate 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ...  <-- never opens
#
# So the MIDI In module is REQUIRED and the shipped prefab is right to have it.
# The theory it was testing -- that leaving MIDI-CV 2's MIDI In pin unconnected
# lets CreateMidiRedirector feed it (keyboard.xml:21) -- is WRONG for a plugin,
# and B is the negative control that shows it.
#
# Pitch reads as a clipped 1.0 in both: CV is volts and 10 V is full scale, so
# a raw pitch voltage saturates a WAV. That is the probe's limitation, not a
# defect -- read the Gate channel, not the Pitch one.
DIAGNOSTIC_PREFABS = [
    # PROBE D -- the variable PROBE A and PROBE B left tangled, spotted by Jeff:
    # "did we prove that MIDI exits the MIDI In module?" We had not.
    #
    # A (module present + cabled) gates; B (module absent) does not. That changed
    # TWO things at once, so it showed only that "MIDI In plus its cable"
    # delivers MIDI -- not that the MIDI travels through the CABLE. The live
    # alternative was that the module's mere PRESENCE makes the container a MIDI
    # destination and ug_container's redirector feeds MIDI-CV 2 directly
    # (ug_base.cpp:2859 scans for the first DT_MIDI2 input pin).
    #
    # D holds the module present and leaves MIDI-CV 2's MIDI In pin UNCONNECTED.
    # MEASURED, one middle-C note, gate mean per 100 ms:
    #
    #   A  present + cabled    0 0 0 0 0 1 1 1 1 1 1 1 0 ...
    #   D  present, uncabled   0 0 0 0 0 0 0 0 0 0 0 0 0 ...
    #   B  absent              0 0 0 0 0 0 0 0 0 0 0 0 0 ...
    #
    # A and D differ ONLY in the cable and only A gates, so MIDI travels through
    # the patch cable: it does exit the MIDI Data output. The redirector
    # alternative is dead, and SE MIDItoGate2 -- wired the same way -- IS being
    # fed MIDI, so its silence is internal to that module.
    Prefab(
        "PROBE D uncabled", "ProbeD.synthedit",
        [
            '--add-module "MIDI In"            60,80   --as midiin',
            '--add-module "SE MIDI to CV 2"    260,80  --as mcv',
            '--add-module "Sound Out"          520,80  --as snd',
            # deliberately NO --connect from $midiin to $mcv
            '--connect $mcv:Gate  $snd:Out',
            '--connect $mcv:Pitch $snd:Out',
            '--add-module "SE Patch Point in"  60,300  --as unused',
        ],
        ["unused"], "PROBE D", ["-"],
    ),
    # PROBE C -- the design the shipped MIDI prefab should return to once E7 is
    # fixed. `SE MIDI to CV 2` gives PITCH as well as Gate, which MIDItoGate2 does
    # not; it is not shipped because it is polyphonicSource/cloned, so its
    # container is a voice container and polyphony cannot escape a container. Its
    # Gate is measurably correct INSIDE its own container (PROBE A) and worth
    # nothing outside it. Place this, cable PITCH and GATE into the Oscillator and
    # Envelope, and it reproduces E7 in one render.
    Prefab(
        "PROBE C midicv", "ProbeC.synthedit",
        [
            '--add-module "MIDI In"            60,80   --as midiin',
            '--add-module "SE MIDI to CV 2"    260,80  --as mcv',
            '--add-module "SE Patch Point out" 520,80  --as pitch',
            '--add-module "SE Patch Point out" 520,200 --as gate',
            '--add-module "SE Patch Point out" 520,320 --as vel',
            '--connect $midiin:"MIDI Data" $mcv:"MIDI In"',
            '--connect $mcv:Pitch          $pitch:Input',
            '--connect $mcv:Gate           $gate:Input',
            '--connect $mcv:Velocity       $vel:Input',
        ],
        ["pitch", "gate", "vel"], "PROBE C", ["PITCH", "GATE", "VEL"],
    ),
    Prefab(
        "PROBE A midiin", "ProbeA.synthedit",
        [
            '--add-module "MIDI In"            60,80   --as midiin',
            '--add-module "SE MIDI to CV 2"    260,80  --as mcv',
            '--add-module "Sound Out"          520,80  --as snd',
            '--connect $midiin:"MIDI Data" $mcv:"MIDI In"',
            '--connect $mcv:Gate  $snd:Out',
            '--connect $mcv:Pitch $snd:Out',
            '--add-module "SE Patch Point in"  60,300  --as unused',
        ],
        ["unused"], "PROBE A", ["-"],
    ),
    Prefab(
        "PROBE B redirect", "ProbeB.synthedit",
        [
            '--add-module "SE MIDI to CV 2"    260,80  --as mcv',
            '--add-module "Sound Out"          520,80  --as snd',
            '--connect $mcv:Gate  $snd:Out',
            '--connect $mcv:Pitch $snd:Out',
            '--add-module "SE Patch Point in"  60,300  --as unused',
        ],
        ["unused"], "PROBE B", ["-"],
    ),
]


def run_cli(secl, lines):
    """Run one SynthEditCL script; return the list of parsed JSON result lines."""
    script = "\n".join(lines) + "\n"
    proc = subprocess.run([str(secl), "--script", "-"], input=script,
                          capture_output=True, text=True)
    results = []
    for ln in proc.stdout.splitlines():
        ln = ln.strip()
        if ln.startswith("{"):
            try:
                results.append(json.loads(ln))
            except json.JSONDecodeError:
                pass
    failures = [r for r in results if r.get("ok") is False]
    if failures:
        for f in failures:
            print(f"  CLI ERROR: {f}", file=sys.stderr)
        raise SystemExit(f"SynthEditCL reported {len(failures)} failure(s)")
    return results


def container_handle(path):
    """The prefab's one container: the single <module> under master_container."""
    root = ET.parse(path).getroot()
    master = root.find("master_container")
    mods = master.find("modules")
    containers = [m for m in mods.findall("module") if m.get("type") == "Container"]
    if len(containers) != 1:
        raise SystemExit(f"{path}: expected exactly 1 container, found {len(containers)}")
    return containers[0].get("handle")


def set_rect(elem, tag, l, t, w, h):
    r = elem.find(tag)
    if r is None:
        r = ET.SubElement(elem, tag)
    r.set("l", str(l)); r.set("r", str(l + w))
    r.set("t", str(t)); r.set("b", str(t + h))


# Ids that TIDE links STATICALLY but SynthEditCL only ever sees as a scanned
# module, so the check below cannot tell them apart from "not linked at all".
#
# The `class=` attribute a saved document carries means "statically registered in
# the app that saved it". SynthEditCL scans `build/modules/*.sem`, so a module
# that lives in a loadable bundle resolves by id at load time and gets NO class
# attribute -- even though the same module is compiled straight into TIDE via
# SynthEditSem/CMakeLists.txt's source list. Without this allowlist the check
# fires on exactly the modules E2a's two-halves rule was invented to add.
#
# Every entry here MUST have its .cpp in SynthEditSem/CMakeLists.txt. That file
# is the claim; this is only the acknowledgement, and the authoritative check
# remains placing the prefab in TIDE and looking at it (docs/e2a-prefabs.md 9.2).
TIDE_STATIC_EXTRAS = {
    # modules/MidiPlayer2/MidiToGate.cpp -- both classes it registers
    "SE MIDItoGate",
    "SE MIDItoGate2",
# E15 retired "SE Rectangle XP" here: the MIDI-CV faceplate is SE TiDE:Panel
    # now, no prefab names the Rectangle any more, and RectangleGui.cpp came
    # off the TIDE source list with it.
    # modules/Controls/LabelGui.cpp -- the faceplate captions. Verified linked at
    # the symbol level rather than by strings/nm on an id:
    #   nm TIDE-Rack (the VST3 binary) | grep __GLOBAL__sub_I_LabelGui.cpp   -> present
    # That symbol names the FILE, so the legacy rename table cannot fake it. Both
    # of these live in source lists belonging to SEPARATE CMake targets in
    # SynthEditLib, which is why they need naming in SynthEditSem's own list --
    # the same trap as Converters.cpp and MidiToGate.cpp.
    "SE Label",
    # BACKLOG E14 -- TIDE's own two modules. Unlike every entry above, these are
    # not borrowed from SynthEditLib: they live in this repo's modules/ tree and
    # are built TWICE on purpose. modules/CMakeLists.txt builds each as a
    # loadable .gmpi so SynthEditCL can scan and place it -- that is how a prefab
    # using them gets authored at all -- while SynthEditSem/CMakeLists.txt
    # compiles the same .cpp files straight into the plugin, which is the only
    # way a class reaches TIDE (PLAN constraint 7; S1a deleted the scan).
    #
    # So SynthEditCL sees them as SCANNED and TIDE has them STATIC, which is
    # exactly the mismatch this allowlist exists for: a saved document's
    # `class=` attribute reflects the editor's registration, not TIDE's, so the
    # check below would otherwise refuse a prefab that works perfectly.
    "SE TiDE:knob",
    "SE TiDE:Panel",
}


def assert_all_modules_linked(path):
    """Every module a prefab uses must be a class TIDE actually links.

    The saved <PluginList> records `class="1"` (DSP) or `class="2"` (GUI) for a
    module with a real registered class behind it, and NO class attribute for an
    XML-only entry -- one the scanning editor knows about from a .sem's XML but
    which TIDE, with its fixed static module set (PLAN constraint 7), cannot
    instantiate. A prefab containing one of those is not partially broken: the
    container places at the right size, reports itself correctly in the
    properties pane, and draws nothing whatsoever.

    IMPORTANT LIMIT, so this is not trusted for more than it proves: the
    attribute reflects SYNTHEDITCL's registration at save time, not TIDE's.
    SynthEditCL scans .sem files, so it can carry classes TIDE does not link,
    and can equally record an XML-only entry for a module whose .cpp TIDE DOES
    link (SE Rectangle XP is exactly that case now). So this catches the crude
    mistake -- naming a module nothing knows how to instantiate -- but a clean
    pass is NOT proof the module works in TIDE. The authoritative check is
    placing the prefab in TIDE and looking at it.

    Checking the binary with strings/nm is worse than either: "SE Rectangle XP"
    appears there via the legacy rename table at CUG.cpp:301 while having no
    registration, so it reports a false positive.
    """
    root = ET.parse(path).getroot()
    plugins = root.find("PluginList")
    if plugins is None:
        return
    unlinked = [p.get("id") for p in plugins.findall("Plugin")
                if p.get("class") is None and p.get("id") not in TIDE_STATIC_EXTRAS]
    for p in plugins.findall("Plugin"):
        if p.get("class") is None and p.get("id") in TIDE_STATIC_EXTRAS:
            print(f"    note: '{p.get('id')}' is scanned by SynthEditCL and STATIC in "
                  f"TIDE -- allowlisted, see TIDE_STATIC_EXTRAS")
    if unlinked:
        raise SystemExit(
            f"{path}: these modules have no linked class and would render nothing "
            f"in TIDE: {', '.join(unlinked)}")


def lay_out_panel(path, prefab, handles):
    """Place the jacks in a column and give the container its rack slot.

    The CLI has no verb that moves a module, so every panel rect it finalises
    lands on the canvas centre -- all of them, on top of each other. Layout is
    therefore done here rather than pretending the CLI did it.
    """
    tree = ET.parse(path)
    root = tree.getroot()
    master = root.find("master_container")
    container = [m for m in master.find("modules").findall("module")
                 if m.get("type") == "Container"][0]

    container.set("name", prefab.name)
    modules_el = container.find("modules")
    by_handle = {m.get("handle"): m for m in modules_el.findall("module")}

    def place(alias, x, y, w, h):
        mod = by_handle.get(str(handles[alias]))
        if mod is None:
            raise SystemExit(f"{path}: '{alias}' (handle {handles[alias]}) not in container")
        set_rect(mod, "panelRect", x, y, w, h)
        return mod

    if prefab.has_faceplate:
        # Faceplate layout: heading strip, then one row per jack with its caption
        # to the left of the jack.
        place(prefab.background, 0, 0, FACE_W, FACE_H)
        if prefab.caption_label:
            place(prefab.caption_label, LABEL_X, 4, FACE_W - 2 * LABEL_X, CAPTION_H)
        for i, alias in enumerate(prefab.jacks):
            y = ROW_TOP + i * ROW_PITCH
            place(alias, FACE_JACK_X, y, PP, PP)
            if i < len(prefab.labels):
                place(prefab.labels[i], LABEL_X, y + 2, LABEL_W, LABEL_H)

        slot_w, slot_h = FACE_W, FACE_H

        # Z-ORDER, and it is counter-intuitive: CContainer::AddSorted push_FRONTs
        # everything except lines (CContainer.cpp:3195-3213), so the file's order
        # is REVERSED into BaseList, and ViewBase::renderChildrenLayer iterates
        # forward -- so the LAST module in the file is drawn FIRST, i.e. behind.
        # Moving the background to the end of the list is therefore what puts it
        # underneath. Measured both ways: background first in file covers the
        # jacks completely; background last renders correctly.
        bg = by_handle[str(handles[prefab.background])]
        modules_el.remove(bg)
        modules_el.append(bg)
    else:
        for i, alias in enumerate(prefab.jacks):
            place(alias, JACK_X, JACK_TOP + i * JACK_PITCH, PP, PP)
        slot_w, slot_h = SLOT_W, SLOT_H

    # What the rack draws. Position is nominal -- the drop path moves it to
    # wherever the user dropped -- but the SIZE is what stops it being invisible.
    #
    # For a faceplate prefab this rect is also SELF-CONSISTENT rather than merely
    # declared: it equals the background's rect, which is the union of the visible
    # children, which is what the view recomputes it to anyway. The bare prefabs
    # keep their old 100x160, which the view still rewrites to their jack column's
    # 20x158 the first time a panel view opens -- harmless, and left alone here
    # rather than changed as a drive-by.
    set_rect(container, "PanelWndPosition", 0, 0, slot_w, slot_h)
    # The container's own canvas, origin-relative so the rects above are in the
    # same space.
    set_rect(container, "panel_rect", 0, 0, slot_w, slot_h)

    tree.write(path, encoding="UTF-8", xml_declaration=True)


def build(secl, prefab, outdir):
    out = outdir / prefab.filename
    print(f"  {prefab.name} -> {out.name}")

    # Pass 1: graph. --containerise wraps the selection; the Visible pin (2) is
    # what makes the result a rack panel rather than an ordinary sub-patch.
    #
    # A MODULE NEEDS BOTH HALVES to work in TIDE -- its .cpp in
    # SynthEditSem/CMakeLists.txt's source list (the class and its registration)
    # AND its pin descriptions, because TIDE has no module scan to supply them
    # (S1a). Either alone fails, and differently: XML-only is an insertable
    # phantom with no class behind it; .cpp-only is a class with no pins, which
    # takes the whole enclosing container's widget layer down with it -- a blank
    # rack, not one missing module. For legacy SDK3 modules the second half is an
    # XML merged in TideApp::InitInstance (MIDItoGate2: MidiToGate.cpp plus
    # MidiPlayer2.xml -- SE Rectangle XP was the original example until E15
    # replaced the faceplate with SE TiDE:Panel); for modern Register<>::withXml
    # modules it comes with the .cpp (SE Label and the TiDE pair need no XML
    # entry).
    #
    # THE TWO OPEN QUESTIONS THAT KEPT THESE JACKS-ONLY ARE BOTH ANSWERED as of
    # 2026-08-19, and the MIDI-CV prefab now carries a faceplate:
    #   * z-order -- the file's order is REVERSED into BaseList (AddSorted
    #     push_FRONTs non-lines, CContainer.cpp:3195-3213) and rendering iterates
    #     forward, so LAST in the file draws FIRST, i.e. behind. lay_out_panel
    #     moves the background to the end. Measured both ways.
    #   * a caption module -- SE Label (modules/Controls/LabelGui.cpp), now in
    #     TIDE's source list. Its Text is a pin, so captions are set by --set-pin
    #     rather than patched into the XML. SE Text Entry, which was considered,
    #     is a patch-memory text FIELD (pin 0 is "patchValue"), not a label.
    # The remaining four prefabs stay bare on purpose: rack styling as a whole is
    # BACKLOG E5 and the visual language is Jeff's to set, so the faceplate went
    # on the one module he asked for rather than all five as a drive-by.
    aliases = re.findall(r"--as (\w+)", "\n".join(prefab.build_lines))
    lines = ["--new"] + prefab.build_lines
    # --add-module leaves what it inserted SELECTED, so the explicit --select list
    # below is only exact if the accumulated selection is cleared first. Without
    # this, a prefab that deliberately containerises a SUBSET (the MIDI-CV one)
    # gets whatever --add-module happened to leave behind as well, and it looks
    # like it worked.
    lines += ["--deselect-all"]
    lines += [f"--select ${a}" for a in (prefab.select or aliases)]
    lines += ["--containerise --as cont", "--set-pin $cont:2 1"]
    lines += prefab.post
    lines += [f'--save-as "{out}"']
    results = run_cli(secl, lines)

    handles = {r["as"]: r["handle"] for r in results
               if r.get("cmd") == "add-module" and "as" in r}

    # Pass 2: rects. A module's rect is finalised by the view that draws it, so
    # the INNER modules need the container's own views opened -- which needs the
    # container's literal handle, because --screenshot's --container sub-arg does
    # not resolve $aliases the way --connect does.
    h = container_handle(out)
    run_cli(secl, [
        f'--load "{out}"',
        f'--screenshot /tmp/e2a_{prefab.filename}_struct.png --view structure --container {h}',
        f'--screenshot /tmp/e2a_{prefab.filename}_panel.png  --view panel     --container {h}',
        f'--save-as "{out}"',
    ])

    # Pass 3: layout, then the invariant that matters most.
    lay_out_panel(out, prefab, handles)
    assert_all_modules_linked(out)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--secl", default=str(DEFAULT_SECL))
    ap.add_argument("--outdir", default=str(HERE / "prefabs"))
    ap.add_argument("--diagnostics", action="store_true",
                    help="also build the V3 MIDI probes (see DIAGNOSTIC_PREFABS). "
                         "They are NOT product -- delete them from outdir before "
                         "a shipping build, or they get staged into the bundle.")
    args = ap.parse_args()

    secl = pathlib.Path(args.secl)
    if not secl.exists():
        raise SystemExit(f"SynthEditCL not found at {secl}\n"
                         f"Build it: cmake --build build --config Release --target SynthEditCL")

    outdir = pathlib.Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    todo = list(PREFABS) + (DIAGNOSTIC_PREFABS if args.diagnostics else [])
    print(f"Building {len(todo)} rack prefabs into {outdir}")
    for p in todo:
        build(secl, p, outdir)
    if args.diagnostics:
        print("NOTE: the two PROBE prefabs are diagnostics, not product -- delete")
        print("      them from the output folder before a shipping build.")
    print("done")


if __name__ == "__main__":
    main()
