dict<addr,int> freed;
dict<addr,addr> base_table;
int size;
inst I where ((I.opcode) == Call && (I.trgname) == "malloc@plt") {
   before I {
       // size of malloc allocation
       size = I.arg1;
   }
   after I {
        // base address of allocation
        addr base_addr = I.rtnval;
        for (addr i=base_addr; i<base_addr+size; i=i+1) {
          base_table[i] = base_addr;
        }
        freed[base_addr] = 0;
    }
}
inst I where ((I.opcode) == Call && (I.trgname) == "free@plt") {
    before I {
     // address to be freed
      addr freed_addr = I.arg1;
      freed[freed_addr] = 1;
   }
}
inst I where ((I.opcode) == Load || (I.opcode) == Store) {
    before I {
        // address being read/written
        addr acc_addr = I.memaddr;
        addr base_addr;
        if ((base_table[acc_addr]) != NULL) {
            base_addr = base_table[acc_addr];
            if ((freed[base_addr]) == 1) {
                print("ERROR: use after free access");
            }
       }
   }
}
