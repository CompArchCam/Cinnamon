
/*
 *  A binary profiling/monitoring tool using DyninstAPI
 *
 *  The file contains placeholders for registering initialisation and exit functions, 
 *  inspection/analysis of the code, adding instrumenation/mutators etc.
 */

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

// Command line parsing
#include <getopt.h>

// DyninstAPI includes
#include "BPatch.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_point.h"

#include "BPatch_object.h"
#include "BPatch_module.h"
#include "DSL_Gen.h"
#include "DSLutil.h"

using namespace Dyninst;

static const char *USAGE = " [-m] [-o] [-p] <inbinary>\n \
                            -m: modes are static, dynamic and cdynamic\n \
                            -o: output binary (needed for static mode)\n \
                            -p: pid (needed for dynamic mode)\n";

static const char *OPT_STR = "m:o:p:";

/* Every Dyninst mutator needs to declare one instance of BPatch */
BPatch bpatch;
typedef enum {
    create,
    attach,
    open
} accessType_t;
// configuration options
char *inBinary = NULL;
char *outBinary = NULL;
accessType_t mode = open; //default is static /open
int pid = -1;
/*----- Global Var Decl Start -----*/
/*----- Global Var Decl End -----*/

bool parseArgs (int argc, char *argv[])
{
    int c;
    char *mvalue = NULL;
    while ((c = getopt (argc, argv, OPT_STR)) != -1) {
        switch ((char) c) {
            case 'm':
                mvalue = optarg;
                if(strcmp(mvalue, "static")==0){
                   mode = open;
                }
                else if(strcmp(mvalue,"dynamic")==0){
                   mode = attach;
                }
                else if(strcmp(mvalue,"cdynamic")==0){
                   mode = create;
                }
                else{
                    cerr << "Usage: " << argv[0] << USAGE;
                    return false;
                }
                break;
            case 'p':
                pid = atoi(optarg);
                break;
            case 'o':
                outBinary = optarg;
                break;
            default:
                cerr << "Usage: " << argv[0] << USAGE;
                return false;
        }
    }

    int endArgs = optind;

    if (endArgs >= argc) {
        cerr << "Input binary not specified." << endl
            << "Usage: " << argv[0] << USAGE;
        return false;
    }
    /* Input Binary */
    inBinary = argv[endArgs];

    if(mode == open && outBinary == NULL){
        cerr << "Output binary not specified for static rewriting." << endl
            << "Usage: " << argv[0] << USAGE;
        return false;

    }
    if(mode == attach && pid == -1){
        cerr << "Invalid pid or no pid specified for dynamic rewriting." << endl
            << "Usage: " << argv[0] << USAGE;
        return false;
    }
    /*
    // Rewritten Binary
    outBinary = argv[endArgs];*/

    return true;
}

BPatch_function *findFuncByName (BPatch_image * appImage, char *funcName)
{
    /* fundFunctions returns a list of all functions with the name 'funcName' in the binary */
    BPatch_Vector < BPatch_function * >funcs;
    if (NULL == appImage->findFunction (funcName, funcs) || !funcs.size ()
            || NULL == funcs[0]) {
        cerr << "Failed to find " << funcName <<
            " function in the instrumentation library" << endl;
        return NULL;
    }
    return funcs[0];
}

BPatch_addressSpace* open_create_inBinary(const char* name, int pid, const char* argv[]){

    BPatch_addressSpace* handle = NULL;
    switch(mode) {
        case create:
            handle = bpatch.processCreate(name, argv);
            if (!handle) { fprintf(stderr, "processCreate failed\n"); }
            break;
        case attach:
            handle = bpatch.processAttach(name, pid);
            if (!handle) { fprintf(stderr, "processAttach failed\n"); }
            break;
        case open:
            // Open the binary file and all dependencies
            handle = bpatch.openBinary(name);
            //handle = bpatch.openBinary(name, false);
            //handle = bpatch.openBinary(name, true);
            if (!handle) { fprintf(stderr, "openBinary failed\n"); }
            break;
    }
    return handle;

}
void close_create_outBinary(BPatch_addressSpace* app, const char* out_name){

    BPatch_process* appProc = dynamic_cast<BPatch_process*>(app);
    BPatch_binaryEdit* appBin = dynamic_cast<BPatch_binaryEdit*>(app);

    if (appProc) { /*Dynamic Rewrite*/
        if (!appProc->continueExecution()) {
            cerr << "continue Execution failed"<< endl;
            return;
        }
        while (!appProc->isTerminated()) {
            bpatch.waitForStatusChange();
        }
    } else if (appBin) { /*Static Rewrite*/
        if (!appBin->writeFile(out_name)) {
            cerr << "Failed to write output file: " << out_name << endl;
            return;
        }
    }
}

int main (int argc, char *argv[])
{
    if (!parseArgs (argc, argv))
        return EXIT_FAILURE;
   
   // const char* pargv[] = {inBinary, "test.sgf", NULL};
    const char* pargv[] = {inBinary, "-h", NULL};

    /* Open the specified binary for binary rewriting. 
     * When the second parameter is set to true, all the library dependencies 
     * as well as the binary are opened */
    BPatch_addressSpace* app = open_create_inBinary(inBinary, pid, pargv); 
    if (app == NULL) {
        cerr << "Failed to open binary" << endl;
        return EXIT_FAILURE;
    }

    /* Open the instrumentation library.
     * loadLibrary loads the instrumentation library into the binary image and
     * adds it as a new dynamic dependency in the rewritten library */
    const char *instLibrary = "libInst.so";
    if (!app->loadLibrary (instLibrary)) {
        cerr << "Failed to open instrumentation library" << endl;
        return EXIT_FAILURE;
    }

    BPatch_image *appImage = app->getImage ();
    /*--- Action Functions Loading Start ---*/

    /*--- Action Functions Loading End ---*/ 
    BPatch_function *init_func = 
       findFuncByName (appImage, (char *) "init_func");
    BPatch_Vector < BPatch_snippet * >instInitArgs;
    /*---------Insert Args - Init Func Start ---- */

    /*---------Insert Args - Init Func End ---- */
    BPatch_funcCallExpr instInitExpr (*init_func, instInitArgs);
    
   

   /*--- Insert Instrumentation Code Start ---*/

    /*--- Insert Instrumentation Code End ---*/
    
    BPatch_function *exit_func = 
        findFuncByName (appImage, (char *) "exit_func");
    BPatch_Vector < BPatch_snippet * >instExitArgs;
    /*---------Insert Args - Exit Func Start ---- */

    /*---------Insert Args - Exit Func End ---- */
    
    BPatch_funcCallExpr instExitExpr (*exit_func, instExitArgs);
    
    
    
    BPatch_Vector<BPatch_function*> funcs;
    appImage->findFunction("_fini", funcs);
    if (funcs.size()) {
        for (auto fIter : funcs) {
            BPatch_module * mod = fIter->getModule();
             if (!mod->isSharedLib ()) {
                mod->getObject()->insertFiniCallback(instExitExpr);
            }
        }
    } 
    //exit point main.
    /*BPatch_function *main_func = findFuncByName (appImage, (char *) "main");
    std::vector<BPatch_point*> *exitPoint =main_func->findPoint(BPatch_exit);
    BPatchSnippetHandle *handle = app->insertSnippet(instExitExpr, exitPoint[0] ,BPatch_callAfter,BPatch_lastSnippet);
*/
    /* Output the instrumented binary */
    close_create_outBinary(app, outBinary);

    return EXIT_SUCCESS;
}
