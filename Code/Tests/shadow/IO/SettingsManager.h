// SCAFFOLDING. RETIRED SHADOW - redirects to the real recovered header.
// Code/BF2/IO/SettingsManager.h now exists. One thing this stub knew that the real header did not:
// every recovered call site writes `io::VariableSet(0)`, which is why it modelled VariableSet as a
// class with an explicit int constructor. That observation is carried over into the real header;
// the functional-cast spelling works for an enum too, so both readings compile.
#include "../../../BF2/IO/SettingsManager.h"
