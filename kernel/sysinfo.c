#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"


uint64 
sys_sysinfo(void){
  uint64 addr; // user pointer to struct sysinfo
  if(argaddr(0, &addr) < 0 )
    return -1;
  struct sysinfo info;
  info.nproc=proc_num();  
  info.freemem=freememsize();
  struct proc *p = myproc();
  if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
      return -1;
  return 0;
}