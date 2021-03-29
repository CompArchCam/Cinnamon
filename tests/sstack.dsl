dict<int, addr> sstack;
int top = 0;
inst I where ((I.opcode) == Call) {
    before I {
     // return address of call
      addr fall_addr = I.nextaddr;
      sstack[top] = fall_addr;
      top = top + 1;
     }
}
inst I where ((I.opcode) == Return) {
    before I {
        if (top > 0 && (sstack[top-1]) == (I.trgaddr)) {
            top = top - 1;
        } else{
            print("ERROR");
        }
    }
}
