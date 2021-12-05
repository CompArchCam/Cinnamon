uint64 inst_count = 0;
select inst I where ((I.opcode) == Load) {
   before I {
       inst_count = inst_count + 1;
   }
}
exit{
   print_u64(inst_count);
}
