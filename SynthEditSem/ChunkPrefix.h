#pragma once
#include <cstdint>
#include <cstring>

// The 4-byte tag on the front of every chunk (parameter 1) payload, telling
// the PROCESSOR why the bytes arrived - because that is the one thing it
// cannot otherwise know, and the difference decides the most expensive
// operation it can perform.
//
//   Build  the document's DSP structure changed (a module added or deleted).
//          The rack is stale: tear it down and rebuild. Sent by
//          serviceDocumentSync's push, which is already debounced and
//          shape-gated on the editor side.
//
//   Sync   nothing structural happened; this is a SAVE-TIME REFRESH of the
//          persistent chunk so the host serialises current values
//          (IController::syncState - the VST3Adaptor's pattern, see
//          GMPI_Adaptors/VST3Adaptor/ControllerWrapper.cpp). The running rack
//          must NOT rebuild: the standalone autosaves moments after every
//          knob tweak, and a rebuild per autosave is an audio glitch per
//          edit. A Sync chunk only builds a rack that does not exist yet -
//          which is exactly the restore-after-restart case, where the
//          wrapper re-seeds the retained parameter into a fresh processor.
//
//   Legacy no tag (bytes begin "<?xm"): a chunk saved before the tag
//          existed. Treated as Build, which is what every chunk meant then.
//
// WHY IN-BAND: the processor may live in another process (AUv3), so nothing
// out-of-band can accompany the parameter. And the tag is fixed-size at a
// fixed offset - classifying it is a 4-byte compare, deliberately NOT a
// string comparison, because this runs on the audio thread (Jeff's rule:
// "the Processor has important real-time stuff to do, not comparing huge
// strings").
//
// The saved format gains these 4 bytes. Ruled acceptable 2026-08-26: the
// plugin XML above the processor already records that nothing has shipped
// ("none of this has ever shipped, so there is nothing to be compatible
// with"), and Legacy keeps every session.xml written before today loading.
namespace tideChunk
{

inline constexpr char tagBuild[4] = { 'T', 'D', 'b', '1' };
inline constexpr char tagSync[4]  = { 'T', 'D', 's', '1' };
inline constexpr size_t tagSize = 4;

enum class Kind { Build, Sync, Legacy };

inline Kind classify(const uint8_t* data, size_t size)
{
	if (size >= tagSize)
	{
		if (0 == memcmp(data, tagBuild, tagSize)) return Kind::Build;
		if (0 == memcmp(data, tagSync,  tagSize)) return Kind::Sync;
	}
	return Kind::Legacy;
}

// The document bytes, tag excluded (Legacy has none to exclude).
inline const uint8_t* payload(const uint8_t* data, Kind kind)
{
	return kind == Kind::Legacy ? data : data + tagSize;
}
inline size_t payloadSize(size_t size, Kind kind)
{
	return kind == Kind::Legacy ? size : size - tagSize;
}

} // namespace tideChunk
