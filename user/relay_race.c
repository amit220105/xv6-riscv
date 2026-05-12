#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TEAMS 3
#define RUNNERS_PER_TEAM 5
#define TARGET_SCORE 30

int main(int argc, char *argv[]) {
    // If no argument, default to 50
    int favoritism = 50; 
    if (argc >= 2) {
        favoritism = atoi(argv[1]);
    }
    
    printf("Starting relay race with %d teams, %d runners per team, target score = %d, favoritism = %d\n", TEAMS, RUNNERS_PER_TEAM, TARGET_SCORE, favoritism);

    int lock_id = israeli_create(favoritism);
    if (lock_id < 0) {
        printf("Failed to create Israeli lock.\n");
        exit(1);
    }

    race_init(TEAMS);
    lcg_srand(getpid());

    // Create teams and runners
    for (int t = 0; t < TEAMS; t++) {
        for (int r = 0; r < RUNNERS_PER_TEAM; r++) {
            int pid = fork();
            if (pid < 0) {
                printf("fork failed\n");
                exit(1);
            }
            if (pid == 0) {  // Child process (runner)
                setgid(t);
                
                while (1) {
                    // Check if general target is reached by ANY team
                    int max_score = 0;
                    for (int i=0; i<TEAMS; i++) {
                        if (race_get_score(i) > max_score) {
                            max_score = race_get_score(i);
                        }
                    }
                    if (max_score >= TARGET_SCORE) {
                        exit(0);
                    }

                    // A runner waits to acquire the baton (lock)
                    israeli_acquire(lock_id);

                    // Once acquired, double check if race is already finished
                    if (race_get_score(t) >= TARGET_SCORE) {
                        israeli_release(lock_id);
                        exit(0); 
                    }

                    int current_max = 0;
                    for (int i=0; i<TEAMS; i++) {
                        if (race_get_score(i) > current_max) {
                            current_max = race_get_score(i);
                        }
                    }
                    if (current_max >= TARGET_SCORE) {
                        israeli_release(lock_id);
                        exit(0);
                    }
                    
                    // Increment team score
                    int new_score = race_inc_score(t);
                    printf("Runner %d (Team %d) acquired the baton\nTeam %d score = %d\n", getpid(), t, t, new_score);
                    
                    // Release baton so other teams/runners can pick it up
                    israeli_release(lock_id);

                    // Exit if this runner just reached the target
                    if (new_score >= TARGET_SCORE) {
                        exit(0);
                    }
                    
                    // Sleep briefly so others can run
                    sleep(2);
                }
            }
        }
    }

    // Parent waits for all runners to finish
    for (int i = 0; i < TEAMS * RUNNERS_PER_TEAM; i++) {
        wait(0);
    }

    // Determine the winning team
    int winner = -1;
    for (int i = 0; i < TEAMS; i++) {
        if (race_get_score(i) >= TARGET_SCORE) {
            winner = i;
            break;
        }
    }
    
    if (winner != -1) {
        printf("Team %d wins the race!\n", winner);
    } else {
        printf("Race finished without a distinct winner?!\n");
    }

    israeli_destroy(lock_id);
    exit(0);
}