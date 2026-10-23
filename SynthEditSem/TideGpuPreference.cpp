// TIDE presents through gmpi_ui's GPU renderer by default (Jeff, 2026-10-24).
//
// gmpi_ui keeps that renderer opt-in while it is young: gpugfx::wanted() is
// false unless the host states a preference or GMPI_UI_GPU is set. A static
// initialiser is the earliest point a host can state it, ahead of the first
// window presenting, and it needs no hook in the standalone wrapper's main().
// GMPI_UI_GPU=0 in the environment still forces the CPU path, and a machine
// that cannot initialise the renderer falls back to it regardless.
//
// Compiled into the standalone target only, and only with TIDE_GPU_BACKEND.

#include "backends/GpuGfx.h"

namespace
{
const bool tidePrefersGpu = []
{
    gmpi::gpugfx::hostPreference() = gmpi::gpugfx::HostPreference::Gpu;
    return true;
}();
}
