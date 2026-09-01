-- E78, phase 1: MINT the CLAP project. The clap_chunk twin of prepare.lua.
--
-- Same shape and same reason: adding the plug-in BY NAME and setting its state
-- parm makes REAPER write its own plug-in token, so no fixture can carry the
-- wrong one (BACKLOG E29). What differs is the parm name and the framing --
-- `clap_chunk`, and the raw preset XML with one trailing zero byte rather than
-- VST3's four-int header. See frame_clap_chunk.py.

local SCRATCH = os.getenv("E19_SCRATCH")
local OUT = os.getenv("E19_PROJ") or (SCRATCH .. "/proj/e78.rpp")
local log = assert(io.open(SCRATCH .. "/prepare-clap.log", "w"))
local function say(s) log:write(s .. "\n"); log:flush() end

say("prepare-clap: start")

local tr = reaper.GetTrack(0, 0)
if not tr then reaper.InsertTrackAtIndex(0, false); tr = reaper.GetTrack(0, 0) end

-- REAPER's own naming, taken from the instantiated FX rather than guessed:
-- "CLAPi: TiDE Rack (TiDE Synth)". The "i" is the instrument marker, and the
-- prefix is what stops AddByName matching the VST3 of the same name.
local fx = reaper.TrackFX_AddByName(tr, "CLAPi: TiDE Rack", false, -1)
say("TrackFX_AddByName -> " .. tostring(fx))
if fx < 0 then
  say("FAIL: no TiDE Rack CLAP in the scan")
  log:close(); reaper.Main_OnCommand(40004, 0); return
end

local _, nm = reaper.TrackFX_GetFXName(tr, fx, "")
say("fx name: " .. tostring(nm))

local ok0, def = reaper.TrackFX_GetNamedConfigParm(tr, fx, "clap_chunk")
say("default clap_chunk ok=" .. tostring(ok0) .. " len=" .. tostring(def and #def or 0))

local fh = io.open(SCRATCH .. "/prepared_clap_chunk.b64", "r")
if not fh then say("no prepared_clap_chunk.b64 -- dump pass only")
  local f0 = io.open(SCRATCH .. "/clap_default_chunk.b64", "w")
  if f0 then f0:write(def or ""); f0:close() end
  log:close(); reaper.Main_OnCommand(40004, 0); return
end
local b64 = fh:read("*a"):gsub("%s+", "")
fh:close()
say("prepared chunk b64 len=" .. #b64)

local ok = reaper.TrackFX_SetNamedConfigParm(tr, fx, "clap_chunk", b64)
say("SetNamedConfigParm -> " .. tostring(ok))

-- Read back and SAY THE LENGTH. Asking for a parm this plug-in format does not
-- have returns ok=false and an empty string rather than an error, so a run that
-- does not print this can mint a project carrying nothing and never notice.
local _, back = reaper.TrackFX_GetNamedConfigParm(tr, fx, "clap_chunk")
say("read back len=" .. tostring(back and #back or 0))

reaper.Main_SaveProjectEx(0, OUT, 0)
say("wrote " .. OUT)
log:close()
reaper.Main_OnCommand(40004, 0)
