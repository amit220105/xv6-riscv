#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NPROC 10
#define GROUPS 3

int
main(void)
{
  int lock_id = israeli_create(50);

  if(lock_id < 0){
    printf("failed to create lock\n");
    exit(1);
  }

  printf("created lock %d\n", lock_id);

  lcg_srand(1234);

  for(int i = 0; i < NPROC; i++){
    int pid = fork();

    if(pid == 0){
      int gid = lcg_rand() % GROUPS;

      setgid(gid);

      if(israeli_acquire(lock_id) < 0){
        printf("child %d failed acquire\n", getpid());
        exit(1);
      }

      printf("process %d gid=%d acquired lock\n", getpid(), getgid());

      sleep(10);

      if(israeli_release(lock_id) < 0){
        printf("child %d failed release\n", getpid());
        exit(1);
      }

      exit(0);
    }
  }

  for(int i = 0; i < NPROC; i++){
    wait(0);
  }

  if(israeli_destroy(lock_id) < 0){
    printf("failed destroy\n");
    exit(1);
  }

  printf("destroyed lock\n");
  exit(0);
}