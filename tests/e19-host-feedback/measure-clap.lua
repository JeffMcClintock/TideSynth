-- E78 phase 2: MEASURE the hosted CLAP.
--
-- Staged, unlike measure.lua, because opening the editor is the risky step here
-- and the interesting evidence comes BEFORE it: on Linux the CLAP timer is
-- registered by guiSetParent and unregistered by guiDestroy (Editor_CLAP.cpp
-- :269, :213), so a hosted CLAP with no editor open has NO UI-thread tick at
-- all. Everything Controller_CLAP::onTimer does -- both directions of the
-- parameter channel -- is therefore dead until a window exists, and the
-- no-editor window is worth logging rather than skipping past.

local SCRATCH = os.getenv("E19_SCRATCH")
local WINDOW  = tonumber(os.getenv("E19_WINDOW") or "60")
local SHOWMODE = tonumber(os.getenv("E78_SHOWMODE") or "3")  -- 3 float, 1 chain
local PREROLL = tonumber(os.getenv("E78_PREROLL") or "8")

local log = assert(io.open(SCRATCH .. "/measure-clap.log", "w"))
local function say(s) log:write(s .. "\n"); log:flush() end

say("measure-clap: start, window " .. WINDOW .. " s, showmode " .. SHOWMODE)
reaper.Main_openProject("noprompt:" .. (os.getenv("E19_PROJ") or (SCRATCH .. "/proj/e78.rpp")))

local tr = reaper.GetTrack(0, 0)
if not tr then say("FAIL: no track"); log:close(); reaper.Main_OnCommand(40004,0); return end
local _, nm = reaper.TrackFX_GetFXName(tr, 0, "")
say("fx name: " .. tostring(nm))
local _, chunk = reaper.TrackFX_GetNamedConfigParm(tr, 0, "clap_chunk")
say("restored clap_chunk b64 len=" .. tostring(chunk and #chunk or 0))

reaper.CSurf_OnPlay()
local t0 = reaper.time_precise()
say("play requested")

local shown = false
local last = -1
local function pump()
  local now = reaper.time_precise() - t0
  if math.floor(now) > last then
    last = math.floor(now)
    say(string.format("t=%.1f playstate=%d pos=%.3f%s", now,
        reaper.GetPlayState(), reaper.GetPlayPosition(),
        shown and " editor=open" or " editor=CLOSED"))
  end
  -- The pre-roll is the control: it says what a rolling transport does with no
  -- editor, so the lines after the show are attributable to the editor and not
  -- to the transport having only just started.
  if not shown and now >= PREROLL then
    say("about to TrackFX_Show mode " .. SHOWMODE .. " -- if the log stops here, that call killed the host")
    reaper.TrackFX_Show(tr, 0, SHOWMODE)
    shown = true
    say("TrackFX_Show returned")
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
