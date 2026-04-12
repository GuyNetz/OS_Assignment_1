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
  // Local variables
  int pid;
  int value;
  struct proc *p;
  struct proc *cur = myproc();

  // Getting Variables from the userspace (from the Trapframe)
  argint(0, &pid);
  argint(1, &value);

  // Check for errors: pid illegal, value illegal, or self co_yielding
  if(pid <= 0 || value <= 0 || pid == cur->pid){
    return -1;
  }

  acquire(&wait_lock);  // A lock to synchronize the sleep/wakeup protocol between the processes

  for(;;){
    struct proc *target = 0;

    // Make sure our current process wasnt killed
    if(cur->killed){
      release(&wait_lock);
      return -1;
    }

    // Find the target process from the processes table
    for(p = proc; p < &proc[NPROC]; p++){
      acquire(&p->lock);
      if(p->pid == pid && p->state != UNUSED && p->state != ZOMBIE){
        target = p;
        break;
      }
      release(&p->lock);
    }

    // Check for error: target process wasnt found
    if(target == 0){
      release(&wait_lock);
      return -1;
    }
    // Check for error: target process is killed
    if(target->killed){
      release(&target->lock);
      release(&wait_lock);
      return -1;
    }

    // A matching coroutine call is already waiting for us. Consume the
    // value it stored in a1, publish our value as its return value in a0,
    // and let the normal scheduler run it later.
    if(target->state == SLEEPING && target->chan == cur){
      int received = target->trapframe->a1;
      target->trapframe->a0 = value;
      target->state = RUNNABLE;
      release(&target->lock);
      release(&wait_lock);
      return received;
    }

    // No partner is waiting yet. Remember our outgoing value and sleep on
    // the target's address until that target yields back to us.
    cur->trapframe->a0 = -1;
    cur->trapframe->a1 = value;
    release(&target->lock);
    sleep(target, &wait_lock);

    // If we were woken by a successful handoff, a0 now contains the value
    // provided by the partner. If the target died, a0 is still -1.
    if(cur->trapframe->a0 >= 0 || cur->killed){
      int received = cur->trapframe->a0;
      release(&wait_lock);
      return received;
    }
  }
}
