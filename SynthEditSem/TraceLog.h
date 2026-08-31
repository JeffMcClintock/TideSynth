#pragma once
// BACKLOG E73 -- give a HOSTED plug-in's trace somewhere to land.
//
// THE PROBLEM, measured 2026-08-31 (macos) while taking E19's mac AU3 cell.
// Every diagnostic this project has -- TIDE's own `TIDE: ...` lines, the rack
// adaptor's RACK_ADAPTOR_TRACE counters (`display-state capture #N`, `first
// nonzero light`, the `apply expect=/sum=` pair), SynthEditLib's -- is written
// to stderr. That works for the standalone, where a shell owns the process.
//
// An audio-unit extension does not work that way. It runs OUT OF PROCESS under
// the system, so nothing it writes to stderr reaches the host, and grepping
// REAPER's stderr for `TIDE:` or `RackProcessor` after loading the AUv3 returns
// exactly nothing -- verified, with the strings present in the appex binary and
// the plug-in demonstrably running. E19's animation, int/bool/enum and
// pixel-diff clauses all read those counters, so on macOS AU3 they could not be
// read at all, however long anybody watched.
//
// WHY THIS IS ONE freopen AND NOT SIXTEEN EDITS. The obvious fix is to convert
// each fprintf(stderr, ...) into a file write, the way E65 did for the panel.
// It is the wrong shape here for two reasons:
//
//   * The sites are spread across repos with different rules. Sixteen of them
//     are in SynthEdit_Rack_Adaptor, which is on NEITHER of STEP 5's lists and
//     is therefore GATED by default; more are in SynthEditLib, which is GATED
//     outright. Redirecting the stream captures all of them from TIDE's own
//     ALLOWED code, and reaches sites nobody has written yet.
//   * The defect is not in any of those calls. They are correct; the process
//     they run in has no stderr worth writing to. Fixing the stream fixes the
//     class.
//
// WHERE THE FILE GOES, and why the default is the useful one. An AUv3 appex is
// sandboxed (`com.apple.security.app-sandbox`), so its TMPDIR is its OWN
// container -- `~/Library/Containers/<extension-bundle-id>/Data/tmp` -- which is
// writable from inside and, crucially, READABLE FROM OUTSIDE. That is what makes
// the log collectable by a harness that never gets to set the appex's
// environment: a host launches an extension through the system, so environment
// variables set for the host do NOT propagate to it. TIDE_TRACE_LOG_PATH is
// still honoured, because it works for the standalone and the VST3 where they
// do propagate, and it lets a harness choose the directory there.
//
// ARMED AT COMPILE TIME, off by default -- the same shape as E65's
// TIDE_PANEL_TRACE_LOG, and for the same reason: a shipped plug-in must not
// silently write files, and an environment variable cannot arm it in the one
// configuration that needs it most.
//
//   cmake -B <dir> -DTIDE_TRACE_LOG=ON ...
//
// std::cerr rides along without any extra work: the default
// sync_with_stdio(true) means the C++ stream writes through the C stderr FILE*,
// so redirecting the FILE* redirects both. TIDE's own lines use std::cerr and
// the rack adaptor's use fprintf, and both land in the same file in order.

#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>

#ifdef _WIN32
  #include <process.h>   // _getpid
#else
  #include <unistd.h>    // getpid
#endif

namespace tide {
namespace trace {

#ifndef TIDE_TRACE_LOG
#define TIDE_TRACE_LOG 0
#endif

#if TIDE_TRACE_LOG

// Returns the path actually opened, or an empty string when nothing was
// redirected. Safe to call from any number of threads and any number of
// objects; only the first call does anything.
inline const std::string& redirectStderrOnce()
{
	static std::string opened;
	static std::once_flag once;

	std::call_once(once, []
	{
		const char* explicitPath = std::getenv("TIDE_TRACE_LOG_PATH");
#ifdef _WIN32
		const char* tmp = std::getenv("TEMP");
		const char sep = '\\';
#else
		// POSIX sets TMPDIR, not TEMP. E65 shipped the other way round and
		// wrote a file literally named `.\TiDEPanel.log` into the process's
		// working directory on mac and linux -- created, so nothing failed and
		// nothing said so. Same trap, avoided here rather than rediscovered.
		const char* tmp = std::getenv("TMPDIR");
		const char sep = '/';
#endif
		const std::string path = explicitPath
			? std::string(explicitPath)
			: (tmp ? std::string(tmp) : std::string(".")) + sep + "TideTrace.log";

		// PROBE BEFORE REDIRECTING. freopen that fails closes the stream, so a
		// bad path would not merely fail to help -- it would destroy the stderr
		// the standalone still depends on. Open it normally first; only when
		// that works is the redirect safe.
		if (std::FILE* probe = std::fopen(path.c_str(), "w"))
		{
			std::fclose(probe);

			if (std::freopen(path.c_str(), "w", stderr))
			{
				opened = path;
				// Unbuffered: an assert or a crash mid-investigation must not
				// eat the lines that explain it. This is a diagnostic build.
				std::setvbuf(stderr, nullptr, _IONBF, 0);

				// Self-identifying, because the interesting case is a file
				// found later in a container directory with no idea which
				// process wrote it or whether it is this run's.
				std::fprintf(stderr,
					"TIDE: trace log opened by pid %d -> %s\n",
					static_cast<int>(
#ifdef _WIN32
						_getpid()
#else
						getpid()
#endif
					), path.c_str());
			}
		}
	});

	return opened;
}

#else

inline const std::string& redirectStderrOnce()
{
	static const std::string none;
	return none;
}

#endif // TIDE_TRACE_LOG

}} // namespace tide::trace
