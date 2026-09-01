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

say("measure: start, window " .. WINDOW .. " s")
-- E19_PROJ so the same driver serves the CLAP run (E78) as well as the VST3
-- one; prepare.lua already honoured it and this did not, which is how the
-- two halves of one harness drift apart.
local PROJ = os.getenv("E19_PROJ") or (SCRATCH .. "/proj/e19.rpp")
say("opening " .. PROJ)
reaper.Main_openProject("noprompt:" .. PROJ)

local tr = reaper.GetTrack(0, 0)
if not tr then say("FAIL: no track after open"); log:close(); reaper.Main_OnCommand(40004, 0); return end

local n = reaper.TrackFX_GetCount(tr)
say("fx count " .. n)
if n < 1 then say("FAIL: no fx after open"); log:close(); reaper.Main_OnCommand(40004, 0); return end

local _, nm = reaper.TrackFX_GetFXName(tr, 0, "")
say("fx name: " .. tostring(nm))
local _, chunk = reaper.TrackFX_GetNamedConfigParm(tr, 0, "vst_chunk")
say("restored vst_chunk b64 len=" .. tostring(chunk and #chunk or 0))

reaper.TrackFX_Show(tr, 0, 3)   -- 3 = float the FX window
say("editor floated")

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
  if now < WINDOW then
    reaper.defer(pump)
  else
    say("window complete")
    reaper.CSurf_OnStop()
    log:close()
    reaper.Main_OnCommand(40004, 0)
  end
end

pump()
