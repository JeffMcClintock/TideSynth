cmake_minimum_required(VERSION 3.30)

set(GMPI_WRAPPER_FOLDER_OVERRIDE "" CACHE PATH "path of a local GMPI WRAPPERs repo, or blank to fetch it automatically.")

# SE16 consumes TIDE by add_subdirectory of THIS directory, never loading the
# repo root -- so the S17 provenance commands used below must be included
# here as well. include_guard makes this a no-op in the standalone build,
# where the root already included it. Removing this include re-breaks every
# SE16-hosted configure with "Unknown CMake command" (issue #222).
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/S17DependencyProvenance.cmake)

# Same hosted-vs-standalone split for the path tracer (issue #222, second
# break behind the first): TideSynth's root adds modules/common -- which
# defines tide_render (linked at the bottom of this file) and the
# tide_render_strict_fp* helpers -- before adding this directory. SE16 adds
# neither, so add it here when the target is missing. modules/common is
# dependency-free by design, which is what makes this safe this early.
if(NOT TARGET tide_render)
  # Parity with TideSynth's own root, which force-disables the preview tool and
  # its image-regression test before adding this directory. Without this line,
  # hosted mode alone would build and run them -- and render_regression fails
  # on mac arm64 today for reasons unrelated to any build (S27: the committed
  # reference images do not reproduce across platforms).
  set(TIDE_RENDER_PREVIEW OFF CACHE BOOL "" FORCE)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../modules/common ${CMAKE_BINARY_DIR}/TideRender)
endif()

include(FetchContent)
include(${GMPI_SDK}/gmpi_plugin.cmake)

# GMPI WRAPPERS
if("${GMPI_WRAPPER_FOLDER_OVERRIDE}" STREQUAL "")
message(STATUS "Fetching GMPI WRAPPERS from github")
#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly generate projects for the GMPI examples)
FetchContent_Declare(
  gmpi_wrappers
  GIT_REPOSITORY https://github.com/JeffMcClintock/GMPI_Wrappers
  GIT_TAG origin/main
  SOURCE_SUBDIR Core
)
FetchContent_MakeAvailable(gmpi_wrappers)
set(GMPI_ADAPTORS ${gmpi_wrappers_SOURCE_DIR})
tide_report_dependency(GMPI_Wrappers "${GMPI_ADAPTORS}" "fetched")
else()
message(STATUS "Using local GMPI WRAPPERS folder")
set(GMPI_ADAPTORS ${GMPI_WRAPPER_FOLDER_OVERRIDE})
# S17 names this override as the original of the trap -- everything looks local
# and one dependency silently is not. Same check as the root's four.
tide_check_not_shadowed(gmpi_wrappers "${GMPI_WRAPPER_FOLDER_OVERRIDE}")
tide_report_dependency(GMPI_Wrappers "${GMPI_ADAPTORS}" "local override")
endif()

# N1a: the CMake project name is the TARGET form -- underscore, never shipped.
# Targets derive as ${PROJECT_NAME}_${kind} below, so a dashed project name
# would produce `TIDE-Rack_VST3`, mixing both separators in the one place you
# cannot avoid it (docs/distribution.md). Shipped filenames are dashed via
# OUTPUT_NAME instead, set in the format loop further down.
project(TIDE_Rack)

set( SYNTHEDITLIB_DIR ${syntheditlib_folder} )

# BACKLOG C7 (stage a), 2026-08-19 (linux). Carve-out stage 7 is "TIDE's CMake
# consumes public SynthEditLib only, with no reference to SE16", so every entry
# here that points into the private repo was measured rather than assumed. Four
# were dead and are gone; ONE is real and is named, because a private reference
# that is actually load-bearing is worth a line of explanation and a backlog id.
#
# Deleted, with what each measurement was:
#   EDITOR_DIR  (../SynthEdit)   -- set and never referenced anywhere in this
#   EDITOR2_DIR (../SynthEdit2)  -- file. Exactly the dead pair C6 removed from
#                                   EditorLib/CMakeLists.txt for the same reason.
#   ../Shared                    -- the directory DOES NOT EXIST in the tree.
#   ../SynthEdit                 -- holds only icons, skins and the .chm; no
#                                   headers. SynthEdit.rc includes resource.h
#                                   and windows.h only, so the Windows resource
#                                   compiler does not need it either.
#
# KEPT, and this is the whole of TIDE's remaining private-source dependency:
#   ../SynthEdit2  -- ONE consumer. TideAppStubs.cpp:31 includes "SynthEditApp.h",
#                     which resolves only in SE16/SynthEdit2. It is the same
#                     header BACKLOG C14 files for ApplySynthEditConfig.cpp, so
#                     the two want one fix -- C11's narrow-to-an-interface
#                     treatment -- not two. Do NOT delete this line before that
#                     lands.
#
#                     One correction for whoever takes C14, measured here rather
#                     than inherited: SynthEditApp.h's moonbasepp_Licensing.h
#                     include is INSIDE `#ifdef SE_MOONBASE_SUPPORT`
#                     (SynthEdit2/SynthEditApp.h:6-11), and that header is not
#                     tracked by git at all -- its own comment says to copy it in
#                     from SynthEdit_Azure. So "this header drags the licensing
#                     surface into the public repo" is true only for a moonbase
#                     build, which narrows C14 rather than blocking it. It is
#                     still a private header declaring a private app class; that,
#                     not the licensing include, is why it cannot simply move.
#
# C7c is CLOSED, and by removal rather than by a move: TIDE used to include
# ContainerThumbnail.h and link EditorScreenshot, both in SE16/EditorScreenshot,
# purely to draw breadcrumb thumbnails. Jeff removed the breadcrumb bar
# (2026-08-20, BACKLOG U3), so the dependency went with the feature and nothing
# had to leave the commercial repo. Full inventory: docs/c7-clean-clone.md in
# TideSynth.
#
# The dropped `PRIVATE` keyword was not a keyword: include_directories() is the
# DIRECTORY-scoped command and takes only paths, so CMake was adding a relative
# directory literally named "PRIVATE". Harmless because it does not exist, but
# it made this block read as if it were target_include_directories().
# C16, 2026-08-20: SE16_SYNTHEDIT2_DIR is gone. C7b parameterised the old
# `../SynthEdit2` because TideAppStubs.cpp still included SynthEditApp.h; C16
# deleted that include along with the three dead symbols it served, so TIDE's
# own source now resolves entirely against public repos. This target no longer
# reads anything from SE16, which is what C7d needs to configure standalone.
include_directories(
	${SYNTHEDITLIB_DIR}
	${VST3_SDK}
)

# E2a, 2026-08-18, Jeff's call in an interactive session. STANDALONE is a
# DEVELOPER target, not a product: it builds TIDE-Rack.app, which stays
# in the build tree (nothing copies it to a Plug-Ins folder, unlike GMPI/VST3),
# and it carries the GMPI standalone wrapper's command channel -- a unix socket
# an MCP server drives, so the rack can be screenshotted, clicked and rendered
# to WAV from a script. That is what makes E2a's prefabs verifiable at all.
#
# MEASURED before adopting it, because the channel is a local IPC endpoint and
# the question was whether it lands in the shipped plugin. It does not:
# Standalone_Wrapper is linked PRIVATE into the _STANDALONE executable only
# (GMPI/gmpi_plugin.cmake:373), and `nm` on the Release binaries counts 25
# IpcServer/CommandDispatcher symbols in the standalone against ZERO in both
# TIDE-Rack.vst3 and TIDE-Rack.gmpi.
#
# THE ONE FOOTGUN: GMPI_STANDALONE_COMMAND_CHANNEL defaults ON. If TIDE ever
# ships a standalone to users, that release must configure with
# -DGMPI_STANDALONE_COMMAND_CHANNEL=OFF, which removes the code rather than
# merely declining to start it (GMPI_Wrappers/wrapper/Standalone/CMakeLists.txt:92).
# BACKLOG R4 -- CLAP is here because every installer row needs it, not just
# the Linux one. docs/distribution.md lists CLAP as a shipped format on all
# three platforms, and R2/R3/R4 each say "the VST3 and CLAP bundles" -- but
# CLAP was absent from this list, so NO platform was building one. Enabling it
# here is the shared half of that work; the per-platform packaging is not.
#
# It cost one word. GMPI already had the CLAP wrapper wired, and the format
# picks up N1a's OUTPUT_NAME for free, so this produces TIDE-Rack.clap with
# no other change. Verified on macOS/arm64 2026-08-22: builds rc=0 with zero
# errors, correct bundle layout, and `nm` finds exactly one exported entry
# point (clap_entry) -- see the journal entry for the commands.
# AU joined the list 2026-08-22 (BACKLOG M1/M3), and only after four separate
# things were fixed -- it is worth knowing it was never one switch:
#
#   1. a header shadowing, because THIS project's root include_directories()
#      put SynthEditLib's modules/shared ahead of GMPI_Wrappers' wrapper/common
#      for every target, including plist_util, which is AU-only (S17's class)
#   2. Apple's AudioUnitSDK needs C++23 (std::expected), and the exception has
#      to live inside the format loop below or it is silently overwritten
#   3. two "here to satisfy linker" fallbacks in AU2_Wrapper.cpp were strong
#      symbols and collided with EditorLib/SynthEditLib (GMPI_Wrappers#11)
#   4. plist_util DERIVED CFBundleExecutable as "<stem>_AU", which is wrong the
#      moment OUTPUT_NAME is set -- the component then built fine and macOS
#      silently refused to register it (GMPI#8; issue #271's class, third site)
#
# Verified before enabling, not after: the component registers
# (aumu Drck Dsyh) and `auval` reports AU VALIDATION SUCCEEDED.
set(FORMATS_LIST GMPI VST3 CLAP AU3 STANDALONE)

#  HAS_VST3
gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP HAS_GUI
	FORMATS_LIST ${FORMATS_LIST}
    SOURCE_FILES
	  # BACKLOG E14 -- TIDE's OWN two modules, and the only ones in this list that
	  # are not borrowed from SynthEditLib. They live in this repo's modules/
	  # tree, which also builds them as loadable .gmpi bundles for authoring in
	  # SynthEditCL; that build is the AUTHORING path and this is the SHIPPING
	  # one. TIDE cannot load a bundle at all -- PLAN constraint 7 is a fixed,
	  # statically registered set and S1a deleted the scan -- so the class only
	  # reaches the product by being compiled in here. E2c set the precedent.
	  #
	  # Neither needs a _tide_xmls entry: both register with Register<>::withXml
	  # carrying their own pin descriptions inline (TiDEknob.cpp:23,
	  # TiDEPanelGui.cpp:2678), which is the `SE Label` case rather than the
	  # `SE Rectangle XP` one. TiDEknobGui.cpp attaches to the same id with
	  # withId, so the GUI half needs no separate registration either.
	  ${CMAKE_CURRENT_SOURCE_DIR}/../modules/TiDEknob/TiDEknob.cpp
	  ${CMAKE_CURRENT_SOURCE_DIR}/../modules/TiDEknob/TiDEknobGui.cpp
	  ${CMAKE_CURRENT_SOURCE_DIR}/../modules/TiDEPanel/TiDEPanelGui.cpp

	  SynthEdit.cpp
	  SynthEditGui.cpp
	  $<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/TideCursorMac.mm>  # S24 cross cursor
	  SynthEditController.cpp
	  TideApp.h
	  TideApp.cpp
	  ${GMPI_UI_SDK}/helpers/Timer.h
	  ${GMPI_UI_SDK}/helpers/Timer.cpp

	  ${SYNTHEDITLIB_DIR}/modules/se_sdk3_hosting/ConnectorView.h
	  ${SYNTHEDITLIB_DIR}/modules/se_sdk3_hosting/ConnectorView.cpp

	  # BACKLOG U2d -- TIDE's fixed module set (PLAN constraint 7), modern half.
	  # These TUs are compiled into SynthEditLib ONLY inside IF(SE2JUCE), so a
	  # GMPI/VST3 product never links them and their static registrations never
	  # run: GetById("SE List Entry") returned null and ModuleViewPanel rendered
	  # empty -- silently, in Release (see the diagnostic in ModuleView.cpp).
	  # Listed as direct target sources because archive members can be
	  # dead-stripped but target sources cannot. Grow this list deliberately as
	  # TIDE's module set grows; do NOT fix this by defining
	  # SE_GRAPHICS_SUPPORT for the whole lib -- that wakes unrelated dormant
	  # code (Controller.cpp, MpParameter.cpp) and double-registers modules in
	  # the scanning editor.
	  # Landed first: SE Background Image (modern GMPI PluginEditor, no legacy
	  # widget layer) -- registered and VERIFIED drawing in REAPER. The classic
	  # SDK3 controls (SE List Entry / SE Text Entry, ControlsXp) also register
	  # and construct once listed here, but their widget layer assumes skin
	  # bitmaps/fonts exist and crashes in initialize/measure/arrange when
	  # widgets never build (three distinct SIGSEGVs, stacks in the U2d row).
	  # Add them back WITH their deps (ClassicControlGuiBase,
	  # sharedLegacyWidgets/*, shared/ImageCache+ImageMetadata+FontCache) once
	  # that layer is fixed.
	  # U2e: the classic SDK3 controls and their widget/resource deps. Their
	  # widget layer needs GmpiResourceManager's Image folder seeded (see
	  # TideApp::InitInstance) -- without it these crashed, which is why U2d
	  # landed with them excluded.
	  ${SYNTHEDITLIB_DIR}/modules/shared/ImageCache.cpp
	  ${SYNTHEDITLIB_DIR}/modules/shared/ImageMetadata.cpp
	  ${SYNTHEDITLIB_DIR}/modules/shared/FontCache.cpp
	  ${SYNTHEDITLIB_DIR}/modules/sharedLegacyWidgets/Widget.cpp
	  ${SYNTHEDITLIB_DIR}/modules/sharedLegacyWidgets/ListWidget.cpp
	  ${SYNTHEDITLIB_DIR}/modules/sharedLegacyWidgets/TextWidget.cpp
	  ${SYNTHEDITLIB_DIR}/modules/sharedLegacyWidgets/BitmapWidget.cpp
	  ${SYNTHEDITLIB_DIR}/modules/sharedLegacyWidgets/TextEditWidget.cpp
	  ${SYNTHEDITLIB_DIR}/modules/ControlsXp/ClassicControlGuiBase.cpp
	  ${SYNTHEDITLIB_DIR}/modules/ControlsXp/ListEntry.cpp
	  ${SYNTHEDITLIB_DIR}/modules/ControlsXp/ListEntryGui.cpp
	  ${SYNTHEDITLIB_DIR}/modules/ControlsXp/TextEntry.cpp
	  ${SYNTHEDITLIB_DIR}/modules/ControlsXp/TextEntryGui.cpp
	  ${SYNTHEDITLIB_DIR}/modules/Controls/PlainImageGui.cpp
	  # BACKLOG E15 -- SE Rectangle XP is gone: the rack faceplate is TIDE's own
	  # SE TiDE:Panel now (modules/TiDEPanel, static via TIDE_STATIC_EXTRAS), so
	  # RectangleGui.cpp and the SubControlsXp.xml staging both came off this
	  # list. The two-halves rule its comment taught lives on at MIDItoGate2
	  # below.

	  # The rack prefabs' captions. `SE Label` is the ONE-LINE case: it registers
	  # with Register<>::withXml, so it carries its own pin descriptions and needs
	  # no entry in _tide_xmls below -- unlike RectangleGui.cpp above. Its skin
	  # dependency degrades safely: the load is guarded on findResourceUri(
	  # "global.txt") succeeding, and SkinMetadata::getFont never returns null
	  # (it falls back to a default Arial). gmpi_helper::SkinMetadata::Serialise
	  # (const char*) is already a defined symbol in libSynthEditLib -- checked
	  # with nm, not assumed -- via SynthEditLib's own helpers/ImageMetadata.cpp,
	  # so there is nothing else to add.
	  #
	  # NOTE THE COLOUR TRAP: SE Label has NO colour pin. Its colour comes from
	  # the skin font style, and with no skin in the bundle that default is
	  # OPAQUE BLACK. Captions are therefore only legible on a LIGHT faceplate,
	  # which is why build-prefabs.py paints one rather than a dark one. A
	  # screenshot taken with SynthEditCL is NOT evidence here -- the CLI
	  # resolves an external skin folder the plugin bundle does not have, so
	  # text can come out white there and black in the DAW.
	  ${SYNTHEDITLIB_DIR}/modules/Controls/LabelGui.cpp

	  # BACKLOG V3/E7 -- SE MIDItoGate2, a MONOPHONIC MIDI-to-gate. Same two-halves
	  # rule as RectangleGui.cpp above: this .cpp for the classes, and its pins
	  # merged from MidiPlayer2.xml (staged below).
	  #
	  # It needs adding here for a reason worth writing down, because it is NOT
	  # the reason it first looked like. MidiToGate.cpp is listed in
	  # SynthEditLib/CMakeLists.txt, but in a source list belonging to a SEPARATE
	  # `MidiPlayer2` target -- it is built as a loadable module, not compiled into
	  # the SynthEditLib static library. So no INIT_STATIC_FILE can rescue it:
	  # adding one to UgDatabase.cpp fails to link outright, because the object is
	  # not in the library. `modules/OscillatorNaive/OscillatorNaive.cpp` sits in
	  # the same list, which is what S8 measured.
	  #
	  # TIDE links statically and has no scan (S1a), so a loadable module is
	  # invisible to it until its source joins a target TIDE actually links --
	  # which is this one.
	  ${SYNTHEDITLIB_DIR}/modules/MidiPlayer2/MidiToGate.cpp

	  # BACKLOG E7 -- THE TYPE CONVERTERS, and this is not a nice-to-have.
	  #
	  # The DSP graph auto-inserts a converter module whenever two connected pins
	  # have different datatypes: ug_base.cpp:1751 builds an id like
	  # "SE <From>To<To>" and calls ModuleFactory()->GetById(). If that lookup
	  # misses, the code does `assert(false); return;` -- so in a RELEASE build the
	  # assert compiles out and **the connection is silently abandoned**. The
	  # editor still shows the cable; the DSP simply never carries it.
	  #
	  # TIDE had none of these, because Converters.cpp builds into a separate
	  # `Converters` target as a loadable Converters.sem, and TIDE links statically
	  # with no scan (S1a). That is why SE MIDItoGate2's BoolOutPin gate never
	  # reached the Envelope: bool -> audio-rate needs "SE BoolToVolts"
	  # (Converters.cpp:137), which did not exist in TIDE.
	  #
	  # So this is a whole CLASS of silent failure, not one module's bug: any
	  # mixed-datatype cable a user draws in TIDE would go quietly dead. Staged
	  # with Converters.xml below for its pins, same two-halves rule as the rest.
	  ${SYNTHEDITLIB_DIR}/modules/Converters/Converters.cpp
	  # Converters.cpp instantiates SimpleConverter<From,To> for every pair, and
	  # each one calls myTypeConvert<From,To>() -- whose specialisations live in a
	  # separate translation unit. Without it the link fails on a wall of
	  # undefined myTypeConvert<...> symbols.
	  ${SYNTHEDITLIB_DIR}/modules/Converters/my_type_convert.cpp

	  # BACKLOG E2c -- TIDE's two GO-TO MVP MODULES, named by Jeff 2026-08-19.
	  #
	  # Same shape as MidiToGate.cpp and Converters.cpp above, and for the same
	  # reason: both of these build into SEPARATE loadable targets (`VaFilters`,
	  # `OscillatorHD`) rather than into the SynthEditLib static library, so no
	  # INIT_STATIC_FILE can reach them -- UgDatabase.cpp already HAS
	  # `INIT_STATIC_FILE(SVFilter4)` at :1114 and the class was still absent from
	  # TIDE. That list is SynthEditLib's; TIDE links a subset of it, so it is not
	  # a menu of what TIDE can instantiate.
	  #
	  # Measured before and after with the check that discriminates -- the
	  # static-init SYMBOL, not the module-id string, which gives false positives
	  # via the rename table at CUG.cpp:301 (see build-prefabs.py:514).
	  #
	  # SE SV Filter4 ("StateVar Filter"): Signal / Pitch / Resonance / Strength /
	  # Mode / Output. Its pins live in VaFilters.xml, staged below -- without that
	  # the class links but has no pins and the browser does not list it, exactly
	  # the trap MidiPlayer2.xml records.
	  ${SYNTHEDITLIB_DIR}/modules/VaFilters/SvFilter.cpp

	  # SE Oscillator4 ("Oscillator HD"): Pitch / Pulse Width / Waveform / Sync ...
	  # NEEDS NO XML -- it registers with Register<>::withXml(R"XML(...)") inline in
	  # its own .cpp (Oscillator.cpp:1054), the way MidiToCv2 does, so there is
	  # nothing to stage for it. Do not add an OscillatorHD entry to _tide_xmls
	  # looking for one; there is no such file.
	  ${SYNTHEDITLIB_DIR}/modules/OscillatorHD/Oscillator.cpp
	  # Oscillator HD builds its mipmapped wavetables with an FFT, and realft()
	  # lives in a separate translation unit -- the same second-TU rule
	  # my_type_convert.cpp records for Converters. Without it the link fails on
	  # exactly one symbol: `realft(float*, unsigned int, int)`, referenced from
	  # MipMapCalculator::generateWavetable in Oscillator.o.
	  ${SYNTHEDITLIB_DIR}/modules/shared/real_fft.cpp

	  # TideAppStubs.cpp supplies the three symbols EditorLib references from
	  # SynthEditApp -- theApp, SafeMessagebox, and two licensing predicates.
	  # It replaces compiling the whole of SynthEdit2/SynthEditApp.cpp, which
	  # brought SynthEditApp::InitInstance() (a LoadOrScanModuleData() call site,
	  # a filesystem-watcher thread and the settings-file writes) into the
	  # plugin binary even though TIDE never calls it. BACKLOG S1b; see the
	  # header comment in that file for why behaviour is unchanged. It also
	  # replaces platform_plugin.cpp's stubs -- that file's SafeMessagebox would
	  # collide, and its GmpiResourceManager::Instance() is superseded by
	  # EditorLib's GmpiResourceManager_editor.cpp.
	  AboutPane.h
	  TideAppStubs.cpp
)

# BACKLOG M1 -- plist_util is an AU-ONLY helper, and TIDE's own global include
# path breaks it.
#
# THE MECHANISM, measured 2026-08-22 rather than guessed. gmpi_plugin() builds
# plist_util (GMPI/gmpi_plugin.cmake:377) only when the format list contains AU
# or AU3, and compiles GMPI's Hosting/dynamic_linking.cpp into it. That file
# does `using namespace wrapper::JmUnicodeConversions;`, which needs
# GMPI_Wrappers/wrapper/common/unicode_conversion.h. THREE headers share that
# name and they do not agree:
#
#   gmpi_wrappers  wrapper/common/unicode_conversion.h   namespace wrapper::JmUnicodeConversions   <- wanted
#   syntheditlib   modules/shared/unicode_conversion.h   namespace JmUnicodeConversions            <- found
#   gmpi_ui        helpers/unicode_conversion.h          namespace gmpi::unicode                   (unrelated)
#
# plist_util DOES ask for wrapper/common -- but as a TARGET-level PRIVATE dir,
# and this project's ROOT CMakeLists.txt:269 adds SynthEditLib's modules/shared
# with the DIRECTORY-scoped include_directories(), which CMake places FIRST.
# Measured order on the failing compile line: syntheditlib/modules/shared at
# position 3, gmpi_wrappers/wrapper/common at position 9. So the wrong header
# wins by six places and the AU build fails with "no namespace named
# 'JmUnicodeConversions' in 'wrapper'".
#
# This is S17's dependency-shadowing class, and it is why that row exists.
# The narrow fix is here rather than in GMPI (PR-GATED) because the offending
# path is OURS: we put it on every target in the project, including targets we
# do not define. BEFORE puts the right directory ahead of the inherited ones.
#
# S40 -- the AUv3 containing app is a SHIPPED, USER-VISIBLE bundle: it installs
# into /Applications and Finder shows its name. gmpi_plugin.cmake names it after
# the CMake target, giving "TIDE_Rack_AU3App" -- underscored, which
# docs/distribution.md reserves for internal target names and is not something to
# put in front of a customer.
#
# "TIDE-Rack-AUv3" rather than plain "TIDE-Rack" because the STANDALONE format
# already produces TIDE-Rack.app in this same directory, and two bundles cannot
# share a name. The suffix also tells a user what the app is for, which matters
# because its only job is to carry the extension: macOS registers the AUv3 from
# it automatically, with no launch required (measured 2026-08-22).
if(APPLE AND TARGET ${PROJECT_NAME}_AU3App)
    set_target_properties(${PROJECT_NAME}_AU3App PROPERTIES
        OUTPUT_NAME "TIDE-Rack-AUv3"
        MACOSX_BUNDLE_BUNDLE_NAME "TIDE Rack (AUv3)"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.tidesynth.tiderack.au3app")
    message(STATUS "S40: AUv3 containing app named TIDE-Rack-AUv3.app")
endif()

# NOT guarded silently. N1a lost a session to `if(TARGET ...)` skipping without
# a word, so this says so out loud when the target is absent.
if(APPLE)
    if(TARGET plist_util)
        target_include_directories(plist_util BEFORE PRIVATE
            "${GMPI_ADAPTORS}/wrapper/common")
        message(STATUS "M1: plist_util include path corrected (wrapper/common first)")
    elseif("AU" IN_LIST FORMATS_LIST OR "AU3" IN_LIST FORMATS_LIST)
        message(FATAL_ERROR
            "M1: FORMATS_LIST asks for AU but gmpi_plugin() created no plist_util "
            "target. Either gmpi_plugin.cmake changed or the format name is wrong "
            "-- do not let this skip silently.")
    endif()
endif()

# BACKLOG U2e -- the classic controls' pin descriptions, registered at startup
# by TideApp::InitInstance (the S1a design removed the scan that used to load
# them). Staged from SynthEditLib's copy so it cannot drift. Contents/Resources
# is the P6-approved home for bundle data (codesign refuses data under MacOS/).
# Windows reads getResource via the exe resource first, then this same file --
# the win staging equivalent is follow-up, noted in the U2e row.
# D6 — the about pane's clipboard write ("Copy link"). Objective-C++, so it is
# Apple-only; other platforms fall back to the stub in AboutPaneStub.cpp and the
# pane simply omits the button rather than offering one that does nothing.
#
# E2a — iterate the format targets rather than naming them. This block used to
# name TIDE_Rack_VST3 and TIDE_Rack, so adding STANDALONE to FORMATS_LIST above linked an
# app whose only two undefined symbols were these. gmpi_plugin() derives every
# target name from PROJECT_NAME the same way, so deriving them here keeps the
# two lists from drifting the next time a format is added.
if(APPLE)
  set(_tide_clipboard_src ${CMAKE_CURRENT_SOURCE_DIR}/AboutPaneClipboard.mm)
else()
  set(_tide_clipboard_src ${CMAKE_CURRENT_SOURCE_DIR}/AboutPaneStub.cpp)
endif()
foreach(_fmt ${FORMATS_LIST})
  if(_fmt STREQUAL "GMPI")
    # N1a: derive from PROJECT_NAME, never spell the target out. These two
    # loops hardcoded `TIDE` / `TIDE_${_fmt}`, and because every use of
    # _tide_target sits behind an `if(TARGET ...)` guard, a rename did not
    # fail -- it SILENTLY SKIPPED the body: no tide_render link, no resource
    # staging, no error. Found by exactly that during the rename.
    set(_tide_target ${PROJECT_NAME})   # the GMPI format target is the bare name
  else()
    set(_tide_target ${PROJECT_NAME}_${_fmt})
  endif()
  if(TARGET ${_tide_target})
    target_sources(${_tide_target} PRIVATE ${_tide_clipboard_src})
    # E14 -- TiDEPanelGui.cpp includes TidePathTracer.h and marches its distance
    # field, so every format target needs the library.
    target_link_libraries(${_tide_target} PRIVATE tide_render)
  endif()
endforeach()

# E14 -- match the tracer's floating point model on the ONE file that compiles a
# distance field, and nowhere else.
#
# TIDE sets /fp:fast on MSVC and a reassociating subset on Apple
# (../CMakeLists.txt), which is hostile to a sphere tracer: reassociated
# rounding decides whether a ray stops at a surface or steps through it, and the
# tracer's own comment says so. Applying the opt-out to the whole plugin target
# would change the DSP engine's float model to fix a renderer -- far more than
# E14 should touch -- so it is applied per source file instead.
#
# On Linux neither fast-math block above is active, so this is currently a
# no-op here; it is not a no-op on Windows or macOS, and it was NOT verified
# there. Whoever builds those first should confirm the panel still renders.
tide_render_strict_fp_sources(${CMAKE_CURRENT_SOURCE_DIR}/../modules/TiDEPanel/TiDEPanelGui.cpp)

# Bundle data, staged into EVERY format target.
#
# This block used to be `if(APPLE)` around a single add_custom_command naming
# the VST3 target, which meant the GMPI build never got ControlsXp.xml and no format got it
# on Windows -- the gap U2e's row flags. E2a's prefabs would inherit exactly the
# same gap, so both are staged together here rather than repeating the mistake.
#
# BundleInfo::getResourceFolder() (SynthEditLib/modules/se_sdk3_hosting/
# BundleInfo.cpp:258) is what reads these back, and on every platform where the
# plugin is a BUNDLE it answers "<...>/Contents/Resources". That is the one
# layout this block produces.
#
# KNOWN LIMIT, stated rather than hidden: a non-bundle target on Windows (the
# standalone .exe) resolves resources through getImbeddedFileFolder() instead,
# so it needs the resources embedded rather than copied. Nothing does that yet;
# on Windows the standalone will report the missing folder on stderr and run
# with an empty Prefabs group rather than misbehaving.
foreach(_fmt ${FORMATS_LIST})
  if(_fmt STREQUAL "GMPI")
    set(_tide_target ${PROJECT_NAME})
  else()
    set(_tide_target ${PROJECT_NAME}_${_fmt})
  endif()
  if(NOT TARGET ${_tide_target})
    continue()
  endif()

  if(APPLE)
    set(_tide_resources "$<TARGET_BUNDLE_CONTENT_DIR:${_tide_target}>/Resources")
  elseif(UNIX)
    # Linux gets its own branch because TARGET_FILE_DIR is NOT inside the bundle
    # here: gmpi_plugin.cmake links a bare .so into the target directory and
    # assembles the bundle around it in a POST_BUILD copy afterwards. The
    # sibling rule below therefore lands two directories away from the plugin --
    # measured 2026-08-20, resources in build/Resources/ while the bundle held
    # only Contents/x86_64-linux/, so a Linux TIDE reported "no Prefabs folder
    # in bundle resources" and loaded none of the pin XMLs. BACKLOG S21.
    #
    # The two formats want DIFFERENT answers, and getBundleContentsFolder()
    # (BundleInfo.cpp:204) is why: it walks the loaded module's path for a
    # "Contents" element and falls back to parent_path() when there is none.
    #   VST3  <name>.vst3/Contents/<arch>-linux/<name>.so -> Contents/Resources
    #   GMPI  a bare <name>.gmpi, no Contents at all      -> Resources beside it
    #
    # The VST3 path is spelled from CMAKE_CURRENT_BINARY_DIR rather than from
    # TARGET_FILE_DIR so it matches, character for character, the expression
    # gmpi_plugin.cmake builds the bundle at. It uses TARGET_FILE_BASE_NAME,
    # not the target name, so that the bundle directory tracks OUTPUT_NAME and
    # matches the .so inside it (issue #271) -- this expression MUST be changed
    # in lockstep with that one, or the .so and the Resources land in two
    # differently-named bundles and the loadable one has no prefabs at all.
    if(_fmt STREQUAL "VST3")
      set(_tide_resources "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_BASE_NAME:${_tide_target}>.vst3/Contents/Resources")
    else()
      set(_tide_resources "$<TARGET_FILE_DIR:${_tide_target}>/Resources")
    endif()
  else()
    # Windows bundle layout: the binary sits in Contents/<arch>/, so Resources
    # is its sibling. Left exactly as it was on purpose -- S21 was found on
    # Linux, and Windows is not verifiable from there, so it gets its own branch
    # rather than a shared expression tuned for one of the two.
    set(_tide_resources "$<TARGET_FILE_DIR:${_tide_target}>/../Resources")
  endif()

  # U2e — the classic controls' pin descriptions; E2a — the sub-controls'.
  # Merge-only enrichment for classes TIDE already links (see the loop in
  # TideApp::InitInstance), so an entry whose .cpp is not in the source list
  # above is skipped rather than becoming a phantom browser entry.
  # MidiPlayer2.xml joined for BACKLOG V3/E7: SE MIDItoGate2 registers with
  # sesdk::Register<>::withId, so its pins live in that file rather than inline
  # in its .cpp the way MidiToCv2's do -- without it the class is linked but has
  # no pins, and TIDE's browser does not list it at all.
  # ONE list, used by BOTH staging blocks below. It used to be a loop here and
  # a hand-enumerated pair of copies for the installed bundle, which meant
  # adding an XML got it into the build tree and NOT into the plugin the DAW
  # loads -- silently, and indistinguishable from the module not being linked.
  # Cost a debugging cycle on V3; do not split them again.
  set(_tide_xmls
    ControlsXp/ControlsXp.xml
    MidiPlayer2/MidiPlayer2.xml
    Converters/Converters.xml
    # E2c -- SE SV Filter4's pins. Oscillator HD is NOT here on purpose: its XML
    # is inline in its own .cpp, so it needs no staging.
    VaFilters/VaFilters.xml
  )
  foreach(_tide_xml ${_tide_xmls})
    get_filename_component(_tide_xml_leaf ${_tide_xml} NAME)
    add_custom_command(TARGET ${_tide_target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SYNTHEDITLIB_DIR}/modules/${_tide_xml}"
        "${_tide_resources}/${_tide_xml_leaf}"
      VERBATIM
    )
  endforeach()

  # E2a — the rack prefabs. copy_directory_if_different so adding a prefab to
  # TideModules/ needs no CMake edit; TideApp::seedPrefabsFromBundle enumerates
  # whatever lands here.
  add_custom_command(TARGET ${_tide_target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/../TideModules/prefabs"
      "${_tide_resources}/Prefabs"
    COMMENT "Staging rack prefabs (${_tide_target})"
    VERBATIM
  )

  # ...and again into the INSTALLED bundle, because of POST_BUILD ordering.
  #
  # gmpi_plugin()'s copy_plugin() copies the whole bundle to ~/Library/Audio/
  # Plug-Ins as its own POST_BUILD step, and POST_BUILD steps run in the order
  # they were added. gmpi_plugin() is called far above this block, so its copy
  # has already happened by the time the resources above are staged: the build
  # tree ends up correct and the plugin the DAW actually loads does not. That
  # is invisible until you wonder why a freshly built TIDE shows an empty
  # Prefabs group in REAPER while the standalone shows three.
  #
  # Re-staging into the install destination is the narrow fix. The broad one is
  # for copy_plugin to run last, which is a GMPI change affecting every plugin
  # and is not this row's to make.
  if(APPLE AND _fmt STREQUAL "VST3")
    set(_tide_installed "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3/Contents/Resources")
  elseif(APPLE AND _fmt STREQUAL "GMPI")
    set(_tide_installed "$ENV{HOME}/Library/Audio/Plug-Ins/GMPI/TIDE-Rack.gmpi/Contents/Resources")
  else()
    set(_tide_installed "")
  endif()

  if(_tide_installed)
    set(_tide_install_cmds "")
    foreach(_tide_xml ${_tide_xmls})
      get_filename_component(_tide_xml_leaf ${_tide_xml} NAME)
      list(APPEND _tide_install_cmds
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${SYNTHEDITLIB_DIR}/modules/${_tide_xml}"
          "${_tide_installed}/${_tide_xml_leaf}")
    endforeach()
    add_custom_command(TARGET ${_tide_target} POST_BUILD
      ${_tide_install_cmds}
      # copy_directory_if_different COPIES but never PRUNES, so a prefab deleted
      # from TideModules/prefabs lingers in the installed bundle and keeps
      # showing up in the browser. Bit V3 with its two throwaway probes.
      COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../TideModules/prefabs"
        "${_tide_installed}/Prefabs"
      COMMENT "Staging bundle data into the installed ${_tide_target}"
      VERBATIM
    )
  endif()
endforeach()

#	  ${VST3_SDK}/pluginterfaces/base/funknown.h
#	  ${VST3_SDK}/pluginterfaces/base/funknown.cpp
#	  ${VST3_SDK}/pluginterfaces/base/coreiids.cpp
#	  ${VST3_SDK}/base/source/fdebug.h
#	  ${VST3_SDK}/base/source/fdebug.cpp
#	  ${VST3_SDK}/base/source/fstring.h
#	  ${VST3_SDK}/base/source/fstring.cpp
#	  ${VST3_SDK}/base/source/fbuffer.h
#	  ${VST3_SDK}/base/source/fbuffer.cpp
#	  ${VST3_SDK}/base/source/baseiids.cpp
#	  ${VST3_SDK}/public.sdk/source/vst/vstinitiids.cpp

foreach(kind IN LISTS FORMATS_LIST)
#	set( SUB_PROJECT_NAME ${PROJECT_NAME}_${kind} )

    if(${kind} STREQUAL "GMPI")
        set(SUB_PROJECT_NAME ${PROJECT_NAME})
    else()
        set(SUB_PROJECT_NAME ${PROJECT_NAME}_${kind})
    endif()

    # gmpi_plugin() may decline a format, so the target named here need not
    # exist -- the same guard the two loops above already carry, and missing
    # only here. GMPI 03dd218 made gmpi_plugin() run the Standalone wrapper's
    # dependency probe and drop STANDALONE from its OWN parsed copy of the
    # list when a dependency is absent; FORMATS_LIST above is a different
    # variable and still reads GMPI VST3 STANDALONE afterwards. On Linux with
    # no libpipewire-0.3 that made this line a hard configure error against a
    # target that was never created, which cost the whole tree rather than the
    # standalone app the decline exists to trade away. Invisible on Windows and
    # macOS because dependencies.cmake only has anything to say on Linux.
    if(NOT TARGET ${SUB_PROJECT_NAME})
        continue()
    endif()

    # N1a -- SHIPPED FILENAMES ARE DASHED, and every format gets the same one:
    # TIDE-Rack.gmpi / TIDE-Rack.vst3 / TIDE-Rack.app. The extension already
    # says which format it is, so the target's `_VST3` suffix must not reach
    # the filename (docs/distribution.md's table names `TIDE-Rack.vst3`
    # explicitly, and R2/R4 install exactly that).
    #
    # Identity is NOT affected and that was checked before renaming: the
    # plugin's id, name and vendor come from the <Plugin> element in
    # SynthEdit.cpp, not from PROJECT_NAME, so a host still resolves saved
    # sessions. What DOES follow PROJECT_NAME is the STANDALONE's bundle id
    # (gmpi_plugin.cmake sets com.gmpi.standalone.${PROJECT_NAME}), which is a
    # developer tool rather than a shipped artifact.
    #
    # THE SIDE-CHANNEL ARTIFACTS MUST STAY DISTINCT, and this is not tidiness:
    # OUTPUT_NAME alone renamed the .pdb and the import .lib too, and all three
    # format targets build into the SAME directory on Windows. Only the module
    # itself is disambiguated by its extension (.gmpi / .vst3 / .exe); the PDB
    # and the import library are not, so GMPI, VST3 and STANDALONE all linked
    # against `Release/TIDE-Rack.pdb` at once and MSBuild's parallel link died
    # with `LNK1201: error writing to program database` -- a message that reads
    # like a disk-space problem and is really a name collision.
    #
    # Invisible on macOS and Linux, which is why CI showed green there and red
    # on Windows only: neither emits a PDB, and neither emits an import library
    # for a MODULE. Keyed off the TARGET name, which is unique by construction
    # (`${PROJECT_NAME}_${kind}`), rather than a hand-written per-format string.
    # BACKLOG R8 -- give every bundle a CFBundleIdentifier that TIDE owns.
    #
    # Until now NONE of the plug-in formats had a usable one. CMake's stock
    # MacOSXBundleInfo.plist.in substitutes MACOSX_BUNDLE_GUI_IDENTIFIER, and
    # nothing set it for a plug-in target -- GMPI sets it only for STANDALONE
    # (gmpi_plugin.cmake:775) -- so the key was emitted PRESENT AND EMPTY:
    #
    #     <key>CFBundleIdentifier</key>
    #     <string></string>
    #
    # codesign then invents one from the executable name plus a hash, observed as
    # "TIDE-Rack-a330dda6894e6d221c12682d5774c181b4845f8b". Signing still
    # SUCCEEDS, which is why this stayed invisible -- but that string is the unit
    # Gatekeeper, notarization tickets and any future update logic key on, and it
    # matches no convention TIDE controls.
    #
    # Scheme: reverse-DNS on tidesynth.com, the product domain, with a per-format
    # suffix. Distinct per format ON PURPOSE -- all four bundles can be installed
    # side by side, and macOS treats the identifier as the unit of identity, so
    # sharing one across them is the same class of collision R9 just removed from
    # the plug-in id. The standalone keeps the bare identifier because it is the
    # application rather than one of the plug-in formats.
    #
    # This is TIDE overriding a GMPI default in TIDE's own file, which is why it
    # lands here rather than upstream. THE AU IS NOT COVERED: its plist is written
    # by plist_util, not by CMake, so it needs --bundle-id passed from
    # gmpi_plugin.cmake (PR-GATED). Tracked on R8.
    string(TOLOWER "${kind}" _tide_format_suffix)
    if(kind STREQUAL "STANDALONE")
        set(_tide_bundle_id "com.tidesynth.tiderack")
    else()
        set(_tide_bundle_id "com.tidesynth.tiderack.${_tide_format_suffix}")
    endif()

    set_target_properties(${SUB_PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "TIDE-Rack"
        PDB_NAME "${SUB_PROJECT_NAME}"
        COMPILE_PDB_NAME "${SUB_PROJECT_NAME}"
        ARCHIVE_OUTPUT_NAME "${SUB_PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "TIDE Rack"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${_tide_bundle_id}")

	# U3 — EditorScreenshot is gone from this list along with the breadcrumb
	# bar it existed for. That was TIDE's last link-time dependency on a
	# SE16-only target, and closing it is what C7c resolved into.
	target_link_libraries(${SUB_PROJECT_NAME}
		 PRIVATE SynthEditLib
		 PRIVATE EditorLib
	)
	target_compile_definitions(
		${SUB_PROJECT_NAME} PRIVATE 
		IS_SYNTHEDIT_SEM=1
		GMPI_DISABLE_FACTORY=1
	)

	if(WIN32)
	target_link_libraries(${SUB_PROJECT_NAME}
			 PRIVATE Version.lib
			 PRIVATE winmm.lib
			 PRIVATE Rpcrt4.lib
	)
	endif()

	# Emit a PDB for Release too. A plugin only crashes in the host, so the only
	# crash reports that ever arrive are against an optimised build; without this
	# a minidump gives a bare fault RVA and nothing else (BACKLOG P4 -- the
	# resize crash was symbolisable only because a Debug repro existed).
	#
	# /Zi and /DEBUG add symbols without changing codegen, but /DEBUG turns
	# /OPT:REF and /OPT:ICF back off, so restore them explicitly -- otherwise
	# this quietly stops stripping unreferenced code and the Release binary
	# grows. The PDB sits beside the artifact and is not shipped.
	if(MSVC)
		target_compile_options(${SUB_PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/Zi>)
		target_link_options(${SUB_PROJECT_NAME} PRIVATE
			$<$<CONFIG:Release>:/DEBUG>
			$<$<CONFIG:Release>:/OPT:REF>
			$<$<CONFIG:Release>:/OPT:ICF>
		)
	endif()

	# BACKLOG M1 -- the AU target needs C++23; everything else stays at 20.
	#
	# Apple's AudioUnitSDK uses std::expected / std::unexpected (AUUtility.h,
	# AUBuffer.h), which are C++23. Verified on this toolchain rather than
	# assumed: a three-line program using std::expected compiles at -std=c++23
	# and fails at -std=c++20 with "no member named 'expected' in namespace
	# 'std'".
	#
	# THIS LINE IS WHY THE OBVIOUS FIX DOES NOT WORK. Setting CXX_STANDARD on
	# the AU target anywhere earlier in this file is silently undone here, one
	# loop iteration later -- the property is set, the configure message prints,
	# and the compile still says -std=gnu++20. Measured by reading the failing
	# compile line, not by trusting the property.
	#
	# Raised per-target rather than moving CMAKE_CXX_STANDARD, so one
	# dependency's requirement does not change the language every other target
	# is checked against.
	if(SUB_PROJECT_NAME MATCHES "_AU$")
		set_property(TARGET ${SUB_PROJECT_NAME} PROPERTY CXX_STANDARD 23)
		set_property(TARGET ${SUB_PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
	else()
		set_property(TARGET ${SUB_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
	endif()
endforeach()

add_subdirectory(${GMPI_ADAPTORS} adaptors)

