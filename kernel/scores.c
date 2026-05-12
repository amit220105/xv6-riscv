#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

#define MAX_TEAMS 10

static struct spinlock scorelock;
static int scores[MAX_TEAMS];

void
scoresinit(void)
{
  initlock(&scorelock, "scores");

  for(int i = 0; i < MAX_TEAMS; i++)
    scores[i] = 0;
}

int
score_reset(void)
{
  acquire(&scorelock);

  for(int i = 0; i < MAX_TEAMS; i++)
    scores[i] = 0;

  release(&scorelock);
  return 0;
}

int
score_inc(int team_id)
{
  int result;

  if(team_id < 0 || team_id >= MAX_TEAMS)
    return -1;

  acquire(&scorelock);
  scores[team_id]++;
  result = scores[team_id];
  release(&scorelock);

  return result;
}

int
score_get(int team_id)
{
  int result;

  if(team_id < 0 || team_id >= MAX_TEAMS)
    return -1;

  acquire(&scorelock);
  result = scores[team_id];
  release(&scorelock);

  return result;
}

int
score_winner(int target)
{
  int winner = -1;

  acquire(&scorelock);

  for(int i = 0; i < MAX_TEAMS; i++){
    if(scores[i] >= target){
      winner = i;
      break;
    }
  }

  release(&scorelock);
  return winner;
}