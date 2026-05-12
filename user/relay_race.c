#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TEAMS 3
#define RUNNERS_PER_TEAM 5
#define TARGET_SCORE 30

void
run_race(int favoritism)
{
  int lock_id = israeli_create(favoritism);

  if(lock_id < 0){
    printf("Failed to create Israeli lock\n");
    exit(1);
  }

  score_reset();

  printf("\n=== Relay race with favoritism = %d ===\n", favoritism);

 for(int runner = 0; runner < RUNNERS_PER_TEAM; runner++){
  for(int team = 0; team < TEAMS; team++){

    int pid = fork();

    if(pid == 0){
      setgid(team);

      while(score_winner(TARGET_SCORE) < 0){

        israeli_acquire(lock_id);

        if(score_winner(TARGET_SCORE) < 0){

          int new_score = score_inc(team);

          printf("Runner %d (Team %d) acquired the baton\n",
                 getpid(), getgid());

          printf("Team %d score = %d\n",
                 getgid(), new_score);
        }

        israeli_release(lock_id);

        sleep(2);
      }

      exit(0);
    }
  }
}

  for(int i = 0; i < TEAMS * RUNNERS_PER_TEAM; i++)
    wait(0);

  int winner = score_winner(TARGET_SCORE);

  printf("Final scores:\n");
  for(int team = 0; team < TEAMS; team++)
    printf("Team %d: %d\n", team, score_get(team));

  printf("Winner with favoritism %d: Team %d\n", favoritism, winner);

  israeli_destroy(lock_id);
}

int
main(void)
{
  run_race(0);
  run_race(50);
  run_race(100);

  exit(0);
}