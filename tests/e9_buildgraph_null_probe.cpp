// E9 probe — does SeAudioMaster::BuildDspGraph survive an EMPTY document?
//
// It does not. This reproduces SeAudioMaster.cpp:403-413 verbatim against the
// document SynthRuntime::prepareToPlay would be holding if TIDE ever prepared
// before a chunk arrived: currentDspXml.Parse("") on the empty "dsp.se.xml"
// bundle resource, because TIDE — unlike an exported SynthEdit plugin — bakes
// no graph into its bundle.
//
// The point of the two positive controls is that a NULL result here is only
// meaningful if the probe can also produce non-NULL ones. The middle case is
// the one the existing `if (!pElem)` guard was actually written for, and it
// passes; the empty case dereferences one line earlier than that guard.
//
// Reasoning and the full call chain: docs/e9-sample-rate.md.
//
// Build (from SynthEditLib/tinyxml, which supplies the parser under test):
//
//   clang++ -std=c++17 -I. -o /tmp/e9_probe \
//     ../TideSynth/tests/e9_buildgraph_null_probe.cpp \
//     tinyxml.cpp tinyxmlparser.cpp tinyxmlerror.cpp
//
#include <cstdio>
#include "tinyxml.h"

static const char* NN(const void* p) { return p ? "non-null" : "NULL"; }

static void probe(const char* label, const char* xml)
{
    TiXmlDocument currentDspXml;
    currentDspXml.Parse(xml);

    printf("--- %s ---\n", label);
    printf("  Parse error         : %s\n", currentDspXml.Error() ? "yes" : "no");
    printf("  RootElement()       : %s\n", NN(currentDspXml.RootElement()));

    TiXmlHandle hDoc(&currentDspXml);
    TiXmlElement* document_xml = hDoc.FirstChildElement("Document").Element();
    printf("  document_xml (:409) : %s\n", NN(document_xml));

    if (!document_xml) {
        printf("  -> SeAudioMaster.cpp:410 would dereference this NULL pointer.\n");
        printf("     The guard on the NEXT line (:413 `if (!pElem) return;`)\n");
        printf("     is one line too late to save it.\n");
        return;
    }
    TiXmlElement* pElem = document_xml->FirstChildElement("DSP");
    printf("  pElem   (:410)      : %s  -> guard at :413 %s\n",
           NN(pElem), pElem ? "passes" : "returns cleanly");
}

int main()
{
    probe("EMPTY document  (TIDE with no chunk pushed)", "");
    probe("POSITIVE CONTROL: <Document> with no <DSP>", "<Document/>");
    probe("POSITIVE CONTROL: <Document><DSP/>",         "<Document><DSP/></Document>");
    return 0;
}
