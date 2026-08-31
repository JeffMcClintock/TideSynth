-- E19 linux VST3 cell, phase 1: MINT the project.
--
-- Adds a TIDE-Rack VST3 to a fresh project, pushes the prepared rack in as the
-- instance's vst_chunk, saves the project, and quits. REAPER writes its OWN
-- plug-in token that way, so nothing here has to know the Linux VST3 UID byte
-- order that BACKLOG E29 records as platform-divergent -- which is exactly the
-- trap scripts/make-host-fixture.py carries a hard-coded Windows constant for.
--
-- Phase 2 (measure.lua) then OPENS the saved project, which is the path E59
-- was about and the only one that answers this cell.

local SCRATCH = os.getenv("E19_SCRATCH")
local OUT = os.getenv("E19_PROJ") or (SCRATCH .. "/proj/e19.rpp")
local log = assert(io.open(SCRATCH .. "/prepare.log", "w"))
local function say(s) log:write(s .. "\n"); log:flush() end

say("prepare: start")

local tr = reaper.GetTrack(0, 0)
if not tr then
  reaper.InsertTrackAtIndex(0, false)
  tr = reaper.GetTrack(0, 0)
end
say("track ok")

local fx = reaper.TrackFX_AddByName(tr, "TIDE-Rack", false, -1)
say("TrackFX_AddByName -> " .. tostring(fx))
if fx < 0 then
  fx = reaper.TrackFX_AddByName(tr, "TiDE Rack", false, -1)
  say("retry 'TiDE Rack' -> " .. tostring(fx))
end
if fx < 0 then
  say("FAIL: no TIDE-Rack VST3 found in the scan")
  log:close()
  reaper.Main_OnCommand(40004, 0)
  return
end

local _, nm = reaper.TrackFX_GetFXName(tr, fx, "")
say("fx name: " .. tostring(nm))

-- What a DEFAULT instance stores, so the framing this run writes can be
-- compared against REAPER's own rather than assumed.
local ok0, def = reaper.TrackFX_GetNamedConfigParm(tr, fx, "vst_chunk")
say("default vst_chunk ok=" .. tostring(ok0) .. " len=" .. tostring(def and #def or 0))
local f0 = io.open(SCRATCH .. "/default_chunk.b64", "w")
if f0 then f0:write(def or ""); f0:close() end

-- The prepared rack, framed by frame_chunk.py into the same shape. On the
-- FIRST pass this file does not exist yet: the run dumps the default chunk
-- above, learns REAPER's framing from it, and comes back.
local fh = io.open(SCRATCH .. "/prepared_chunk.b64", "r")
if not fh then
  say("no prepared_chunk.b64 -- dump pass only")
  log:close()
  reaper.Main_OnCommand(40004, 0)
  return
end
local b64 = fh:read("*a"):gsub("%s+", "")
fh:close()
say("prepared chunk b64 len=" .. #b64)

local ok = reaper.TrackFX_SetNamedConfigParm(tr, fx, "vst_chunk", b64)
say("SetNamedConfigParm -> " .. tostring(ok))

local _, back = reaper.TrackFX_GetNamedConfigParm(tr, fx, "vst_chunk")
say("read back len=" .. tostring(back and #back or 0))
say("read back == written: " .. tostring(back == b64))

reaper.Main_SaveProjectEx(0, OUT, 0)
say("wrote " .. OUT)
say("saved")
log:close()
reaper.Main_OnCommand(40004, 0)  -- File: Quit REAPER
