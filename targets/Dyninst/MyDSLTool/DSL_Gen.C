
/*
 *  Template functions to add instrumentations/snippets at the specified 
 *  points in the bianry.
 */

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

// Command line parsing
#include <getopt.h>

// DyninstAPI includes
#include "DSL_Gen.h"


using namespace Dyninst;

enum{
        BEFORE=1,
        AFTER,
        AT,
        ENTRY,
        EXIT,
        ITER,
        ITER_END
};

template<> void
insertSnippetAt<BPatch_basicBlock>(BPatch_addressSpace* app, BPatch_basicBlock * curComp, BPatch_function* action, int trigger, bool attach_data, BPatch_Vector < BPatch_snippet * >instArgs)
{
        BPatch_point *trigger_point;
        BPatch_callWhen when;
        
        switch(trigger){
            //case ENTRY:
            case BEFORE:
                trigger_point = (curComp)->findEntryPoint();
                when = BPatch_callBefore;
            break;
            //case EXIT:
            case AFTER:
                trigger_point = (curComp)->findExitPoint();
                when = BPatch_callAfter;
            break;
        }
        if (NULL == trigger_point) {
            //cerr << "Failed to find entry for basic block at 0x" << hex << address
            cerr << "Failed to find entry/exit for basic block at 0x"<< endl;
            return;
        }
        BPatch_funcCallExpr actionCallExpr(*action, instArgs);
        
        BPatchSnippetHandle *handle =
            app->insertSnippet (actionCallExpr, *trigger_point, when,
                    BPatch_lastSnippet);
        if (!handle) {
            //cerr << "Failed to insert instrumention in basic block at 0x" << hex <<
              //  address << endl;
            cerr << "Failed to insert instrumention in basic block" << endl;
            return;
        }

}
template<> void
insertSnippetAt<BPatch_point>(BPatch_addressSpace* app, BPatch_point* curComp, BPatch_function* action, int trigger, bool attach_data, BPatch_Vector < BPatch_snippet * >instArgs){

    BPatch_callWhen when;
    /* Find the instrumentation points */
    if (NULL == curComp) {
        cerr << "Failed to find trigger point for Inst " << endl;
        return;
    }
    switch(trigger){
        case BEFORE:
        case AT:
          when = BPatch_callBefore;
        break;
        case AFTER:
          when = BPatch_callAfter;
        break;
    }
    /* Create a vector of arguments to the action funciton */
    BPatch_funcCallExpr actionCallExpr(*action, instArgs);
    /* Insert the snippet before or after the instruction */
    BPatchSnippetHandle *handle =
        app->insertSnippet (actionCallExpr, *curComp, when,
                BPatch_lastSnippet);
    if (!handle) {
        cerr << "Failed to insert instrumention at function of"
            << endl;
    }
}
template<> void
insertSnippetAt<BPatch_basicBlockLoop>(BPatch_addressSpace* app, BPatch_basicBlockLoop* curComp, BPatch_function* action, int trigger, bool attach_data, BPatch_Vector < BPatch_snippet * >instArgs){

    vector < BPatch_point * >*trigger_point;
    BPatch_callWhen when;
    BPatch_flowGraph* CFG = curComp->getFlowGraph();
    /* Find the instrumentation points */
    switch(trigger){
        case ENTRY:
          cout<<"Loop Entry"<<endl;
          trigger_point = CFG->findLoopInstPoints(BPatch_locLoopEntry, curComp);
          when = BPatch_callBefore;
        break;
        case EXIT:
          cout<<"Loop Exit"<<endl;
          trigger_point = CFG->findLoopInstPoints(BPatch_locLoopExit, curComp);
          when = BPatch_callAfter;
        break;
        case ITER:
          cout<<"Loop Iter"<<endl;
          trigger_point = CFG->findLoopInstPoints(BPatch_locLoopStartIter, curComp);
          when = BPatch_callBefore;
        break;
        case ITER_END:
          trigger_point = CFG->findLoopInstPoints(BPatch_locLoopEndIter, curComp);
          when = BPatch_callBefore;
        break;
    }
    if (NULL == trigger_point) {
        cerr << "Failed to find trigger point for function " << endl;
        return;
    }
    
    
    BPatch_funcCallExpr actionCallExpr(*action, instArgs);
    /* Insert the snippet at function entry/exit */
    BPatchSnippetHandle *handle =
      app->insertSnippet (actionCallExpr, *trigger_point);
/*    BPatchSnippetHandle *handle =
        app->insertSnippet (actionCallExpr, *trigger_point, when,
                BPatch_lastSnippet);*/
    if (!handle) {
        cerr << "Failed to insert instrumention at function of "
            << endl;
    }

}
template<> void
insertSnippetAtCFG<BPatch_basicBlockLoop>(BPatch_addressSpace* app, BPatch_flowGraph* CFG, BPatch_basicBlockLoop* curComp, BPatch_function* action, int trigger, bool attach_data, BPatch_Vector < BPatch_snippet * >instArgs){

    vector < BPatch_point * >*trigger_point;
    BPatch_callWhen when;

    /* Find the instrumentation points */
    switch(trigger){
        case ENTRY:
          cout<<"Loop Entry"<<endl;
          trigger_point = CFG->findLoopInstPoints(BPatch_locLoopEntry, curComp);
          when = BPatch_callBefore;
        break;
        case EXIT:
          cout<<"Loop Exit"<<endl;
          trigger_point = CFG->findLoopInstPoints(BPatch_locLoopExit, curComp);
          when = BPatch_callAfter;
        break;
        case ITER:
          cout<<"Loop Iter"<<endl;
          trigger_point = CFG->findLoopInstPoints(BPatch_locLoopStartIter, curComp);
          when = BPatch_callBefore;
        break;
        case ITER_END:
          trigger_point = CFG->findLoopInstPoints(BPatch_locLoopEndIter, curComp);
          when = BPatch_callBefore;
        break;
    }
    if (NULL == trigger_point) {
        cerr << "Failed to find trigger point for function " << endl;
        return;
    }
    
    /* Create a vector of arguments to the action funciton */
    
    BPatch_funcCallExpr actionCallExpr(*action, instArgs);
    /* Insert the snippet at function entry/exit */
    BPatchSnippetHandle *handle =
      app->insertSnippet (actionCallExpr, *trigger_point);
/*    BPatchSnippetHandle *handle =
        app->insertSnippet (actionCallExpr, *trigger_point, when,
                BPatch_lastSnippet);*/
    if (!handle) {
        cerr << "Failed to insert instrumention at function of "
            << endl;
    }
}
template<> void
insertSnippetAt<BPatch_function>(BPatch_addressSpace* app, BPatch_function* curComp, BPatch_function* action, int trigger, bool attach_data, BPatch_Vector < BPatch_snippet * >instArgs){

    vector < BPatch_point * >*trigger_point;
    BPatch_callWhen when;

    /* Find the instrumentation points */
    switch(trigger){
        case ENTRY:
            trigger_point = curComp->findPoint (BPatch_entry);
            when = BPatch_callBefore;
            break;
        case EXIT:
            trigger_point = curComp->findPoint (BPatch_exit);
            when = BPatch_callAfter;
            break;
    }
    if (NULL == trigger_point) {
        cerr << "Failed to find trigger point for function " << endl;
        return;
    }

    /* Create a vector of arguments to the action funciton */
    BPatch_funcCallExpr actionCallExpr(*action, instArgs);
    /* Insert the snippet at function entry/exit */
    /*BPatchSnippetHandle *handle =
      app->insertSnippet (actionCallExpr, *trigger_point);*/
    BPatchSnippetHandle *handle =
        app->insertSnippet (actionCallExpr, *trigger_point, when,
                BPatch_lastSnippet);
    if (!handle) {
        cerr << "Failed to insert instrumention at function of " << endl;
    }
}

