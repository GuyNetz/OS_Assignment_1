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
  struct proc *cur = myproc();
  struct proc *target = 0;
  struct proc *p;

  // Getting Variables from the userspace (from the Trapframe)
  argint(0, &pid);
  argint(1, &value);

  // Check for input errors: pid illegal, self co_yielding, or non-positive value
  // (spec assumes value is always positive, rejecting invalid input defensively)
  if(pid <= 0 || pid == cur->pid || value <= 0){
    return -1;
  }

  acquire(&wait_lock);

  // Make sure our current process wasnt killed
  if(cur->killed){
    release(&wait_lock);
    return -1;
  }

  // Find the target process from the processes table
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->pid == pid && p->state != UNUSED && p->state != ZOMBIE){
      target = p;
      break;
    }
  }

  // Check for error: target process wasnt found or is killed
  if(target == 0 || target->killed){
    release(&wait_lock);
    return -1;
  }

  if(target->state == SLEEPING && target->chan == cur){
    // The target is waiting for us, perform direct control transfer and skip the scheduler!

    acquire(&target->lock);         // Lock the target process - must hold its lock before switching to it
    target->trapframe->a0 = value;  // Pass the value directly to its return register
    cur->state = SLEEPING;          // Current process goes to sleep
    cur->chan = target;             // Put the target address as our current sleep channel
    target->state = RUNNING;        // Set target to running (skipping RUNNABLE)
    mycpu()->proc = target;         // Update the CPU pointer
    release(&wait_lock);            // Release wait_lock as coordination is complete

    // Performing the context switch directly
    swtch(&cur->context, &target->context);

    // After we woke up:
    // We woke up because another process performed a swtch back to us.
    // The protocol dictates that whoever jumped to us holds cur->lock for us, so we must release it.
    cur->chan = 0;
    release(&cur->lock);

  } else {
    // To transition to the scheduler properly, we must hold our own process lock
    acquire(&cur->lock);

    // The target is not ready yet. Go to sleep and wait for someone to perform a direct handoff to us.
    cur->state = SLEEPING;
    cur->chan = target;

    release(&wait_lock);

    // Jump to the scheduler (it will ignore us because we are SLEEPING, not RUNNABLE)
    swtch(&cur->context, &mycpu()->context);

    // After we woke up:
    // We reach here only when the target process performs a Direct Handoff to us.
    // Again, the protocol dictates that the other process locked cur->lock for us before the swtch.
    cur->chan = 0;
    release(&cur->lock);
  }

  // After waking up - ensure we weren't killed while sleeping
  if(cur->killed) {
    return -1;
  }

  // Return the value placed in a0 for us by the process that yielded to us
  return cur->trapframe->a0;
}
