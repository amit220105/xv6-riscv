#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

static struct spinlock randlock;
static uint rand_state = 1;

void
randinit(void)
{
  initlock(&randlock, "rand");
}

void
lcg_srand(uint seed)
{
  acquire(&randlock);
  rand_state = seed;
  release(&randlock);
}

uint
lcg_rand(void)
{
  uint result;

  acquire(&randlock);
  rand_state = rand_state * 1664525 + 1013904223;
  result = rand_state;
  release(&randlock);

  return result;
}