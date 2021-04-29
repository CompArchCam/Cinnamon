#ifndef __LIB_INST_H__
#define __LIB_INST_H__

#if !defined(LIB_EXPORT)
  #if defined(_MSC_VER)
    #define LIB_EXPORT __declspec(dllexport)
  #else
    #define LIB_EXPORT __attribute__((visibility ("default")))
  #endif
#endif

#include<map>
using namespace std;
/*--- Instrumentation Functions Prototype Start ---*/

/*--- Instrumentation Functions Prototype End ---*/
#endif
