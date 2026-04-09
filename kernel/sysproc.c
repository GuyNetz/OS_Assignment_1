#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

extern struct proc proc[NPROC];
extern struct spinlock wait_lock;

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//returns the size of running process' memory in bytes
uint64
sys_memsize(void)
{
  struct proc *p = myproc();
  return p->sz;
}


uint64
sys_co_yield(void)
{
  int pid;
  int value;

  argint(0, &pid);
  argint(1, &value);

  struct proc *currProc = myproc();

  if(pid <= 0 || pid == currProc->pid){
    return -1;
  }

  // Scan the process table to find the target by PID
  struct proc *target = 0;
  struct proc *pp;    //temporary pointer
  for(pp = proc; pp < &proc[NPROC]; pp++){
    acquire(&pp->lock);
    if(pp->pid == pid && pp->state != UNUSED && pp->state != ZOMBIE){
      target = pp;
      release(&pp->lock);
      break;
    }
    release(&pp->lock);
  }

  // Target not found or not in a usable state
  if(target == 0)
    return -1;

  // Target was killed
  if(killed(target))
    return -1;

  // Store our outgoing value and target info in the trapframe.
  // We already read the arguments, so a0 and a1 are free to reuse.
  currProc->trapframe->a0 = value;   // the value we want to send
  currProc->trapframe->a1 = pid;     // who we are targeting

  acquire(&wait_lock);

  // TODO: step 30 will go here -- check if target is already waiting for us

  // Target is not ready yet. Sleep until the target yields to us.
  sleep(&currProc->pid, &wait_lock);

  // We woke up. Someone wrote a value into our trapframe->a0.
  // But first check: were we woken because we got killed?
  if(killed(currProc)){
    release(&wait_lock);
    return -1;
  }

  release(&wait_lock);

  // Return the value that our partner delivered to us
  return currProc->trapframe->a0;
}