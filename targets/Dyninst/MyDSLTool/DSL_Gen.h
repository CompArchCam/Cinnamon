#ifndef _DSL_GEN_
#define _DSL_GEN_

// DyninstAPI includes
#include "BPatch.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_point.h"

#include "BPatch_object.h"
#include "BPatch_module.h"

//template<typename T> void
//insertSnippetAt(BPatch_binaryEdit* app, T * curComp, BPatch_function* action, int trigger, bool attach_data, int data);
template<typename T> void
insertSnippetAt(BPatch_addressSpace* app, T * curComp, BPatch_function* action, int trigger, bool attach_data, BPatch_Vector < BPatch_snippet * >instArgs);

template<typename T> void
insertSnippetAtCFG(BPatch_addressSpace* app, BPatch_flowGraph* CFG, T* curComp, BPatch_function* action, int trigger, bool attach_data, BPatch_Vector < BPatch_snippet * >instArgs);
#endif
