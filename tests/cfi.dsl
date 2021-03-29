set<addr> vtable;
file outfile;
init {
    line l;
    outfile.open("fAddr.txt");
    while(getline(outfile,l)){
       vtable.insert(stoul(l));
    }
    outfile.close();
}
func F {
    outfile.open("fAddr.txt");
    writeToFile(outfile, F.startAddr);
    outfile.close();
}
inst I where ((I.opcode) == Call) {
  before I {
       if ( (vtable.count(I.trgaddr)) == 0) {
           print("ERROR");
       }
   }
}
