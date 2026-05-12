#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "israeli.h"

struct israeli_lock israeli_locks[NISRAELI];

static int
valid_lock_id(int lock_id)
{
  return lock_id >= 0 && lock_id < NISRAELI;
}

static int
queue_contains(struct israeli_lock *il, struct proc *p)
{
  for(int i = 0; i < il->qsize; i++){
    if(il->queue[i] == p)
      return 1;
  }
  return 0;
}

static int
enqueue(struct israeli_lock *il, struct proc *p)
{
  if(il->qsize >= ISRAELI_QUEUE)
    return -1;

  if(queue_contains(il, p))
    return 0;

  il->queue[il->qsize] = p;
  il->qsize++;
  return 0;
}

static struct proc*
remove_queue_index(struct israeli_lock *il, int index)
{
  struct proc *p = il->queue[index];

  for(int i = index; i < il->qsize - 1; i++)
    il->queue[i] = il->queue[i + 1];

  il->qsize--;
  return p;
}

static int
choose_next_index(struct israeli_lock *il, int releasing_gid)
{
  int same_gid_index = -1;

  for(int i = 0; i < il->qsize; i++){
    if(il->queue[i]->gid == releasing_gid){
      same_gid_index = i;
      break;
    }
  }

  if(same_gid_index == -1)
    return 0;

  if((lcg_rand() % 100) < il->favoritism)
    return same_gid_index;

  return 0;
}

void
israelilockinit(void)
{
  for(int i = 0; i < NISRAELI; i++){
    initlock(&israeli_locks[i].lock, "israeli");
    israeli_locks[i].active = 0;
    israeli_locks[i].locked = 0;
    israeli_locks[i].favoritism = 0;
    israeli_locks[i].owner = 0;
    israeli_locks[i].qsize = 0;
  }
}

int
israeli_create(int favoritism)
{
  if(favoritism < 0 || favoritism > 100)
    return -1;

  for(int i = 0; i < NISRAELI; i++){
    acquire(&israeli_locks[i].lock);

    if(israeli_locks[i].active == 0){
      israeli_locks[i].active = 1;
      israeli_locks[i].locked = 0;
      israeli_locks[i].favoritism = favoritism;
      israeli_locks[i].owner = 0;
      israeli_locks[i].qsize = 0;

      release(&israeli_locks[i].lock);
      return i;
    }

    release(&israeli_locks[i].lock);
  }

  return -1;
}

int
israeli_acquire(int lock_id)
{
  struct proc *p = myproc();

  if(!valid_lock_id(lock_id))
    return -1;

  struct israeli_lock *il = &israeli_locks[lock_id];

  acquire(&il->lock);

  if(il->active == 0){
    release(&il->lock);
    return -1;
  }

  if(il->locked == 0 && il->qsize == 0){
    il->locked = 1;
    il->owner = p;
    release(&il->lock);
    return 0;
  }

  if(enqueue(il, p) < 0){
    release(&il->lock);
    return -1;
  }

  while(il->active && il->owner != p){
    sleep(p, &il->lock);
  }

  if(il->active == 0){
    release(&il->lock);
    return -1;
  }

  release(&il->lock);
  return 0;
}

int
israeli_release(int lock_id)
{
  struct proc *p = myproc();

  if(!valid_lock_id(lock_id))
    return -1;

  struct israeli_lock *il = &israeli_locks[lock_id];

  acquire(&il->lock);

  if(il->active == 0 || il->locked == 0 || il->owner != p){
    release(&il->lock);
    return -1;
  }

  if(il->qsize == 0){
    il->locked = 0;
    il->owner = 0;
    release(&il->lock);
    return 0;
  }

  int next_index = choose_next_index(il, p->gid);
  struct proc *next = remove_queue_index(il, next_index);

  il->locked = 1;
  il->owner = next;

  wakeup(next);

  release(&il->lock);
  return 0;
}

int
israeli_destroy(int lock_id)
{
  if(!valid_lock_id(lock_id))
    return -1;

  struct israeli_lock *il = &israeli_locks[lock_id];

  acquire(&il->lock);

  if(il->active == 0){
    release(&il->lock);
    return -1;
  }

  il->active = 0;
  il->locked = 0;
  il->owner = 0;

  for(int i = 0; i < il->qsize; i++)
    wakeup(il->queue[i]);

  il->qsize = 0;

  release(&il->lock);
  return 0;
}