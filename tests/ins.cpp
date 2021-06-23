#include <stdint.h>
extern uint64_t inst_count = 0;
void func_1(){
    inst_count = inst_count + 1;
}
