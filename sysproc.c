#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_shutdown(void)
{
  /* Either of the following will work. Does not harm to put them together. */
  outw(0xB004, 0x0|0x2000); // working for old qemu
  outw(0x604, 0x0|0x2000); // working for newer qemu
  
  return 0;
}

extern int sched_trace_enabled;
extern int sched_trace_counter;
int sys_enable_sched_trace(void)
{
  if (argint(0, &sched_trace_enabled) < 0)
  {
    cprintf("enable_sched_trace() failed!\n");
  }
  
  sched_trace_counter = 0;

  return 0;
}

extern int child_first_scheduling;
int sys_fork_winner(void) {
  
  int w;

  if (argint(0, &w) < 0) {
    cprintf("error: no winner specified\n");
    return -1;
  }
  
  child_first_scheduling = w;
  
  return 0;
}


extern int stride_scheduling;
int sys_set_sched(void) {
  
  int p;

  if (argint(0, &p) < 0) {
    cprintf("error: no policy specified\n");
    return -1;
  }

  stride_scheduling = p;

  if (stride_scheduling)
    redistribute_tickets();
  else
    reset_procs();
  
  return 0;
}

int sys_tickets_owned(void) {
  
  int pid, ret;
  
  if (argint(0, &pid) < 0) {
    cprintf("error: no PID specified\n");
    return -1;
  }

  // helper function in proc.c (because it accesses ptable)
  ret = tickets_owned_helper(pid);

  if (ret < 0) {
    cprintf("error: no process with exists with PID %d\n", pid);
    return -1;
  }

  return ret;
  
}

int sys_transfer_tickets(void) {
  
  int recipient_pid, tickets, ret;

  if (argint(0, &recipient_pid) < 0) {
    cprintf("error: no recipient PID specified\n");
    return -4;
  }

  if (argint(1, &tickets) < 0) {
    cprintf("error: tickets not specified\n");
    return -4;
  }

  // helper function in proc.c (because it accesses ptable)
  ret = transfer_tickets_helper(recipient_pid, tickets);

  if (ret == -3) {
    cprintf("error: recipient process does not exist. pid=%d\n", recipient_pid);
    return ret;
  }

  // If the number of tickets requested to transfer is smaller than 0, return -1.
  if (ret == -1) {
    cprintf("error: number of tickets requested to transfer is smaller than 0\n");
    return ret;
  }

  // If the number of tickets requested to transfer is larger than ticket p−1, return -2.
  if (ret == -2) {
    cprintf("error: number of tickets requested to transfer is larger than (ticket_p − 1)\n");
    return ret;
  }

  // completed successfully; no need to print any message
  return ret;
}
