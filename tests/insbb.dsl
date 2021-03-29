uint64 inst_count = 0;
basicblock B {
    uint64 local_inst_count = 0;
    inst I where ((I.opcode) == Load) {
            local_inst_count = local_inst_count + 1;
    }
    entry B where( local_inst_count > 0) {
        inst_count = inst_count + local_inst_count;
    }
}
exit{
   print(inst_count);
}
