#define NISRAELI 15
#define ISRAELI_QUEUE 16

struct israeli_lock {
  struct spinlock lock;

  int active;
  int locked;
  int favoritism;

  struct proc *owner;

  struct proc *queue[ISRAELI_QUEUE];
  int qsize;
};

extern struct israeli_lock israeli_locks[NISRAELI];