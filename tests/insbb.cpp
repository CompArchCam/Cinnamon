#include <stdint.h>
extern uint64_t inst_count = 0;
void func_1(uint64_t var0){
    inst_count = inst_count + var0;
}
