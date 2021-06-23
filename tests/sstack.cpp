#include <map>
extern map<int,uintptr_t> sstack;
extern int top = 0;
void func_1(uint64_t var0){
    uintptr_t fall_addr = var0;
    sstack[top] = fall_addr;
    top = top + 1;
}
void func_2(uint64_t var0){
     if( top > 0 && sstack[top - 1] == var0 )
    {
        top = top - 1;
    }
    else {
        print("ERROR");
    }
}
