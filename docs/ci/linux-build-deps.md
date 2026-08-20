# The Linux CI package set — measured, not inferred

**Status: the step below is NOT installed.** It belongs in
`.github/workflows/build.yml`, which a scheduled agent may not write — a
workflow file executes with repository-secret access on the branch that pushes
it, and the fleet's token carries no `workflow` scope. Installing it is a
manual step for Jeff, exactly like [verify.yml](verify.yml) beside it.

Measured 2026-08-20 on the linux box (Ubuntu 24.04.4, the same release
`ubuntu-latest` currently pins) against a clean `git clone` of this repo.

## The step

```yaml
      - name: Install Linux deps
        if: matrix.platform == 'linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y build-essential cmake ninja-build \
            libx11-dev libxext-dev libgl1-mesa-dev libfreetype-dev \
            libfontconfig1-dev libasound2-dev libharfbuzz-dev \
            libdbus-1-dev libpng-dev
```

Three packages added to what `build.yml` installs today: **`libxext-dev`,
`libharfbuzz-dev`, `libdbus-1-dev`. `libpng-dev` is a fourth, added for a
different reason** — see "libpng" below.

## Why three, when the CI log names one

`GMPI_Wrappers/wrapper/VST3/CMakeLists.txt:249-263` runs seven
`pkg_check_modules(... REQUIRED)` probes on Linux. **`REQUIRED` fails fast**, so
a run reports the first missing module and never reaches the rest — the log on
[#190](https://github.com/JeffMcClintock/TideSynth/issues/190) names only
`xext`, and fixing that alone would have produced another red run naming
`harfbuzz`, and then a third naming `dbus-1`.

So the chain was **measured by walking it**, restoring one `.pc` file at a time
into a pkg-config path that mirrors the runner, and re-configuring:

| step | configure result | missing module | probe |
|---|---|---|---|
| 1 | rc=1 | `xext` | `VST3/CMakeLists.txt:257` |
| 2 | rc=1 | `harfbuzz` | `VST3/CMakeLists.txt:260` |
| 3 | rc=1 | `dbus-1` | `VST3/CMakeLists.txt:264` |
| 4 | **rc=0** | — | — |

Step 1 reproduces the CI failure exactly: same error, same file and line, same
package name.

## pkg-config module → Debian package

Read off this box with `dpkg -S` on each `.pc` file, rather than guessed from
the module name:

| pkg-config module | `.pc` file | Debian package | in `build.yml` today? |
|---|---|---|---|
| `x11` | `x11.pc` | `libx11-dev` | yes |
| `xext` | `xext.pc` | **`libxext-dev`** | **no — fatal** |
| `fontconfig` | `fontconfig.pc` | `libfontconfig-dev` | yes, via `libfontconfig1-dev` |
| `freetype2` | `freetype2.pc` | `libfreetype-dev` | yes |
| `harfbuzz` | `harfbuzz.pc` | **`libharfbuzz-dev`** | **no** |
| `libpng` | `libpng.pc` | `libpng-dev` | no — but preinstalled on the runner |
| `dbus-1` | `dbus-1.pc` | **`libdbus-1-dev`** | **no** |

Two notes on that table.

**`libfontconfig1-dev` is a transitional dummy package** on 24.04 — it is
`Depends: libfontconfig-dev` and nothing else. It works, and it is what the
workflow already installs, so it is left alone here rather than churned.

**`libpng` is the one that is not currently broken and is still worth pinning.**
The runner image ships it, so the probe passes today by luck of the base image
rather than by anything this repo states. A base-image change would turn it into
the next `xext`, with the same one-name-at-a-time diagnosis cost.

## The optional three

`wayland-client`, `xkbcommon` and `libdecor-0` (`libwayland-dev`,
`libxkbcommon-dev`, `libdecor-0-dev`) **degrade gracefully** and are deliberately
not in the step above. Without them configure prints:

```
-- VST3 wrapper: Wayland support off (need wayland-client, xkbcommon, libdecor-0
   and a VST3 SDK of 3.8.0 or later); X11 editor only.
-- gmpi_plugin(TIDE): STANDALONE skipped -- Standalone_Wrapper cannot be built here
```

and carries on to a working X11 VST3. Adding them is a product choice — whether
CI also builds the Wayland editor and `TIDE_STANDALONE` — not a build fix. Note
`libpipewire-0.3` is also missing on the runner and is only needed by the
standalone.

## Making the probe optional instead would ship a broken plugin

`GMPI_Wrappers` is on the weekly prompt's ALLOWED list, so a run could relax the
X11 probe the way the Wayland one already is and turn CI green without Jeff.
That was declined on macOS as papering over a real dependency; on Linux it is
now **measured** rather than argued. The linked binary genuinely uses all three:

| library | undefined symbols in `TIDE_VST3.so` |
|---|---|
| `libXext` | **5** (`XShm*` — the MIT-SHM path the CMake comment describes) |
| `libharfbuzz` | **50** (`hb_*`) |
| `libdbus-1` | **27** (`dbus_*`) |

Relaxing the probe would not produce a Linux VST3 with a degraded editor. It
would produce one that fails to resolve those symbols.

## What passes once the step is installed

The full CI recipe, run on a clean clone with only these packages visible:

```
git clone https://github.com/JeffMcClintock/TideSynth      158 files
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release             rc=0
cmake --build build --config Release --parallel            rc=0, 0 error lines
  -> TIDE.gmpi        7,378,536 bytes  ELF 64-bit x86-64
  -> TIDE_VST3.so     8,494,704 bytes  ELF 64-bit x86-64
  -> TIDE_VST3.vst3/Contents/x86_64-linux/TIDE_VST3.so
350 objects; zero SE16 references in the configure log, build log or CMakeCache
```

**One defect this build found is NOT fixed by the step above** — TIDE's
resources, prefabs included, are staged outside the Linux bundle. That is
BACKLOG **S21**, filed separately; it does not affect whether CI goes green.
