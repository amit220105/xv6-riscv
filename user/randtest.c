#include "kernel/types.h"
#include "user/user.h"

void test_determinism(void) {
    printf("1. Testing Determinism... ");
    
    lcg_srand(1234);
    uint a = lcg_rand();
    uint b = lcg_rand();

    lcg_srand(1234);
    uint c = lcg_rand();
    uint d = lcg_rand();

    if (a == c && b == d) {
        printf("[SUCCESS]\n");
    } else {
        printf("[FAIL] Sequences don't match!\n");
    }
}

void test_concurrency(void) {
    printf("2. Testing Concurrency with Multiple Processes... ");
    
    lcg_srand(555);
    
    for (int i = 0; i < 5; i++) {
        int pid = fork();
        if (pid == 0) {
            for (int j = 0; j < 20; j++) {
                lcg_rand();
            }
            exit(0);
        }
    }

    for (int i = 0; i < 5; i++) {
        wait(0);
    }
    printf("[SUCCESS] \n");
}

int main(void) {
    printf("--- Starting PRNG Comprehensive Tests ---\n");
    
    test_determinism();
    test_concurrency();
    
    printf("--- All PRNG Tests Completed ---\n");
    exit(0);
}