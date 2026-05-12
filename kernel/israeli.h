#ifndef ISRAELI_H
#define ISRAELI_H

#define NISRAELI 15
#define MAX_QUEUE 16

struct israeli_lock {
  uint active;
  int favoritism;
  struct spinlock lk;
  
  int locked;
  int owner_pid;
  int owner_gid;
  
  struct proc *queue[MAX_QUEUE];
  int queue_count;
};

extern struct israeli_lock israeli_locks[NISRAELI];

#endif