/*
 * Copyright 2002-2019 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <iostream>
#include "util.h"
using std::cerr;
using std::ofstream;
using std::ios;
using std::string;
using std::endl;

/*--- Callback Functions Start ---*/
/*--- Callback Functions End ---*/


/*--- BBL Event Starts ---*/

/*--- BBL Event Ends ---*/

/*--- INS Event Starts ---*/

/*--- INS Event Ends ---*/

/*--- IMG Event Starts ---*/

/*--- IMG Event Ends ---*/

/*--- RTN Event Starts ---*/

/*--- RTN Event Ends ---*/


VOID Fini(INT32 code, VOID *v)
{
    /*--- Exit Function Starts ---*/

    /*--- Exit Function Ends ---*/
}
VOID Init(VOID *v){
    /*--- Init Function Starts ---*/

    /*--- Init Function Ends ---*/

}


/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize pin
    PIN_InitSymbols();
    PIN_Init(argc, argv);

    /*--- Register Instrumentation Function Starts ---*/

    /*--- Register Instrumentation Function Ends ---*/
    
    // Register Fini to be called when the application exits
    PIN_AddApplicationStartFunction(Init, 0);
    PIN_AddFiniFunction(Fini, 0);
   
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
