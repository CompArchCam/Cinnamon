int global_bbs_exec = 0;
int next_global_id = 0;
dict<int, int> loop_bbs_exec;
dict<int, int> living_loop;
int num_loops = 0;
loop L {
   int loop_id = next_global_id;
   next_global_id = next_global_id + 1;
   entry L {
       living_loop[loop_id] = 1; // loop becomes alive
   }
   exit L {
       living_loop[loop_id] = 0; // loop no longer alive
   }
}
module M {
   num_loops = num_loops + (M.numLoops);
}
basicblock B {
    entry B {
        global_bbs_exec = global_bbs_exec + 1;
        for (int i=0; i<num_loops; i=i+1) {
           if ((living_loop[i]) == 1) {
             // update loop count for all alive loops
             loop_bbs_exec[i] = (loop_bbs_exec[i]) + 1;
           }
        }
    }
}
exit {
    int count;
    for (int i=0; i<num_loops; i=i+1) {
      count = ((loop_bbs_exec[i]) / global_bbs_exec) * 100;
      print(count);
    }
}
