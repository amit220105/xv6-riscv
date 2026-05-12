#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "israeli.h"

struct israeli_lock israeli_locks[NISRAELI];

void
israeli_init()
{
  for(int i = 0; i < NISRAELI; i++){
    initlock(&israeli_locks[i].lk, "israeli");
    israeli_locks[i].active = 0;
    israeli_locks[i].locked = 0;
    israeli_locks[i].queue_count = 0;
  }
}

int
israeli_create(int favoritism)
{
  if (favoritism < 0 || favoritism > 100) return -1;
  
  for(int i = 0; i < NISRAELI; i++){
    acquire(&israeli_locks[i].lk);
    if(israeli_locks[i].active == 0){
      israeli_locks[i].active = 1;
      israeli_locks[i].favoritism = favoritism;
      israeli_locks[i].locked = 0;
      israeli_locks[i].queue_count = 0;
      release(&israeli_locks[i].lk);
      return i;
    }
    release(&israeli_locks[i].lk);
  }
  return -1;
}

int
israeli_acquire(int lock_id)
{
  if (lock_id < 0 || lock_id >= NISRAELI) return -1;
  struct israeli_lock *il = &israeli_locks[lock_id];
  
  acquire(&il->lk);
  if (il->active == 0) {
    release(&il->lk);
    return -1;
  }

  struct proc *p = myproc();
  if (il->locked == 0) {
    il->locked = 1;
    il->owner_pid = p->pid;
    il->owner_gid = p->gid;
    release(&il->lk);
    return 0;
  }

  if (il->queue_count >= MAX_QUEUE) {
    release(&il->lk);
    return -1;
  }

  il->queue[il->queue_count++] = p;

  while(il->owner_pid != p->pid){
    if(killed(p)){
      // remove from queue (simplified)
      for (int i = 0; i < il->queue_count; i++) {
        if (il->queue[i] == p) {
          for (int j = i; j < il->queue_count - 1; j++) {
            il->queue[j] = il->queue[j+1];
          }
          il->queue_count--;
          break;
        }
      }
      release(&il->lk);
      return -1;
    }
    sleep(p, &il->lk);
  }
  
  release(&il->lk);
  return 0;
}

int
israeli_release(int lock_id)
{
  if (lock_id < 0 || lock_id >= NISRAELI) return -1;
  struct israeli_lock *il = &israeli_locks[lock_id];
  
  acquire(&il->lk);
  if (il->active == 0 || il->locked == 0 || il->owner_pid != myproc()->pid) {
    release(&il->lk);
    return -1;
  }

  if (il->queue_count == 0) {
    il->locked = 0;
    release(&il->lk);
    return 0;
  }

  int next_idx = 0;
  int G = myproc()->gid;
  int c = il->favoritism;

  for (int i = 0; i < il->queue_count; i++) {
    if (il->queue[i]->gid == G) {
      // Found the earliest waiting process with the same gid
      if (lcg_rand() % 100 < c) {
        next_idx = i;
      }
      // Whether we select it or not, we only consider the earliest one
      break;
    }
  }

  // Fallback to FIFO (first element) if no group matched or probability failed, which is next_idx = 0.
  
  struct proc *next_owner = il->queue[next_idx];
  
  for (int i = next_idx; i < il->queue_count - 1; i++) {
    il->queue[i] = il->queue[i+1];
  }
  il->queue_count--;

  il->owner_pid = next_owner->pid;
  il->owner_gid = next_owner->gid;
  
  wakeup(next_owner);
  release(&il->lk);
  return 0;
}

int
israeli_destroy(int lock_id)
{
  if (lock_id < 0 || lock_id >= NISRAELI) return -1;
  struct israeli_lock *il = &israeli_locks[lock_id];
  
  acquire(&il->lk);
  if (il->active == 0 || il->locked != 0) {
    release(&il->lk);
    return -1;
  }
  il->active = 0;
  release(&il->lk);
  return 0;
}
