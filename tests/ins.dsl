uint64 inst_count = 0;
inst I where ((I.opcode) == Load) {
   before I {
       inst_count = inst_count + 1;
   }
}
exit{
   print(inst_count);
}
