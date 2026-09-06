-- E19 linux VST3 cell, phase 2: MEASURE.
--
-- Opens the project prepare.lua saved, floats the plug-in's editor, rolls the
-- transport, and logs GetPlayState()/GetPlayPosition() for the whole window.
--
-- That transport log is not decoration. BACKLOG E19's own note, and the
-- 2026-08-28 linux entry that first ran this: "the plug-in is frozen" and
-- "nothing is being processed" are indistinguishable from inside the plug-in,
-- so a run without this control cannot tell which of the two it measured.

local SCRATCH = os.getenv("E19_SCRATCH")
local WINDOW = tonumber(os.getenv("E19_WINDOW") or "75")

local log = assert(io.open(SCRATCH .. "/measure.log", "w"))
local function say(s) log:write(s .. "\n"); log:flush() end

-- See prepare.lua: never File:Quit on Windows, where a dirty project raises an
-- undismissable save modal. The driver waits for this sentinel and kills us.
local function bail()
  say("done")
  log:close()
  if not reaper.GetOS():find("Win") then reaper.Main_OnCommand(40004, 0) end
end

say("measure: start, window " .. WINDOW .. " s")
-- E19_PROJ so the same driver serves the CLAP run (E78) as well as the VST3
-- one; prepare.lua already honoured it and this did not, which is how the
-- two halves of one harness drift apart.
local PROJ = os.getenv("E19_PROJ") or (SCRATCH .. "/proj/e19.rpp")
say("opening " .. PROJ)
reaper.Main_openProject("noprompt:" .. PROJ)

local tr = reaper.GetTrack(0, 0)
if not tr then say("FAIL: no track after open"); bail(); return end

local n = reaper.TrackFX_GetCount(tr)
say("fx count " .. n)
if n < 1 then say("FAIL: no fx after open"); bail(); return end

local _, nm = reaper.TrackFX_GetFXName(tr, 0, "")
say("fx name: " .. tostring(nm))
local _, chunk = reaper.TrackFX_GetNamedConfigParm(tr, 0, "vst_chunk")
say("restored vst_chunk b64 len=" .. tostring(chunk and #chunk or 0))

-- WHICH BINARY. Same reason as prepare.lua, and it has to be re-asked here:
-- the project names the plug-in, not the file, so a re-scan between the two
-- phases can hand this phase a different bundle than the one that was minted.
local _, ident = reaper.TrackFX_GetNamedConfigParm(tr, 0, "fx_ident")
say("fx_ident: " .. tostring(ident))

reaper.TrackFX_Show(tr, 0, 3)   -- 3 = float the FX window
say("editor floated")

-- An EMPTY project has length 0 and REAPER stops the transport the instant it
-- reaches the end -- measured on Windows 7.78 as playstate 1 -> 0 inside one
-- second with pos never leaving 0.000, which reads exactly like a wedged
-- plug-in. A silent MIDI item gives the project a length to play through. It
-- feeds the plug-in nothing, so it cannot be mistaken for the signal under
-- test. The linux fixture had items and so never met this.
local item = reaper.CreateNewMIDIItemInProj(tr, 0, WINDOW + 60, false)
say("length item created: " .. tostring(item ~= nil))

reaper.CSurf_OnPlay()
local t0 = reaper.time_precise()
say("play requested")

-- One line a second is enough to prove the engine never stalled and keeps the
-- log readable; the defer loop itself runs at REAPER's UI rate.
local last = -1
local function pump()
  local now = reaper.time_precise() - t0
  if math.floor(now) > last then
    last = math.floor(now)
    say(string.format("t=%.1f playstate=%d pos=%.3f", now, reaper.GetPlayState(), reaper.GetPlayPosition()))
  end
  -- Re-issue play if the transport ever drops, and SAY so on the line. A window
  -- that silently restarted is not the same measurement as one that never
  -- stopped, and the difference has to be readable afterwards.
  if reaper.GetPlayState() == 0 and now < WINDOW - 2 then
    say(string.format("t=%.1f transport had stopped -- re-issuing play", now))
    reaper.CSurf_OnPlay()
  end
  if now < WINDOW then
    reaper.defer(pump)
  else
    say("window complete")
    reaper.CSurf_OnStop()
    bail()
  end
end

pump()
