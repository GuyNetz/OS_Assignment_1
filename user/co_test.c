#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
test_bad_pid(void)
{
  int received = co_yield(99999, 1);
  printf("bad pid: %d\n", received);
  if(received != -1)
    exit(1);
}

static void
test_self_yield(void)
{
  int self = getpid();
  int received = co_yield(self, 1);
  printf("self yield: %d\n", received);
  if(received != -1)
    exit(1);
}

static void
test_killed_target(void)
{
  int child = fork();

  if(child < 0){
    printf("killed target: fork failed\n");
    exit(1);
  }

  if(child == 0){
    for(;;)
      sleep(10);
  }

  kill(child);
  sleep(2);

  int received = co_yield(child, 1);
  printf("killed target: %d\n", received);
  if(received != -1)
    exit(1);

  wait(0);
}

static void
test_negative_pid(void)
{
  int received = co_yield(-1, 1);
  printf("negative pid: %d\n", received);
  if(received != -1)
    exit(1);
}

static void
test_zero_pid(void)
{
  int received = co_yield(0, 1);
  printf("zero pid: %d\n", received);
  if(received != -1)
    exit(1);
}

/* Verifies that a process correctly sleeps and waits when the target process has not yet yielded control back. */
// static void 
// test_target_not_ready() 
// {
//   printf("\n--- Running Test 1: Target Not Ready ---\n");
//   int parent_pid = getpid();
//   int child_pid = fork();

//   if (child_pid < 0) {
//     printf("Fork failed\n");
//     return;
//   }
//   if (child_pid == 0) {
//     sleep(20); 
//     int val = co_yield(parent_pid, 42);
//     if (val == 100) {
//         printf("[Child] Successfully woke up and received: %d\n", val);
//     } else {
//         printf("[Child] Error: Received wrong value: %d\n", val);
//     }
//     exit(0);
//   } else {
//     printf("[Parent] Calling co_yield and going to sleep...\n");
//     int val = co_yield(child_pid, 100);
//     if (val == 42) {
//         printf("[Parent] Successfully woke up and received: %d\n", val);
//     } else {
//         printf("[Parent] Error: Received wrong value: %d\n", val);
//     }
//     wait(0);
//     printf("Test 1 Passed!\n");
//   }
// }

// /* Validates a chain of control transfers between multiple processes (P1 -> P2 -> P3 -> P1) to ensure the handoff mechanism works in sequence. */
// static void
// test_chaining() 
// {
//   printf("\n--- Running Test 2: Chaining P1 -> P2 -> P3 -> P1 ---\n");
  
//   int fd[2];
//   pipe(fd); 

//   int p1_pid = getpid();
//   int p2_pid = fork();

//   if (p2_pid == 0) {
//     int p3_pid;
//     read(fd[0], &p3_pid, sizeof(p3_pid));

//     int val_from_p1 = co_yield(p1_pid, 0); 
//     printf("[P2] received from P1: %d\n", val_from_p1);

//     co_yield(p3_pid, val_from_p1 + 1);
//     exit(0);
//   }

//   int p3_pid = fork();
//   if (p3_pid == 0) {
//     int p2_pid_child;
//     read(fd[0], &p2_pid_child, sizeof(p2_pid_child));

//     int val_from_p2 = co_yield(p2_pid_child, 0);
//     printf("[P3] received from P2: %d\n", val_from_p2);

//     co_yield(p1_pid, val_from_p2 + 1);
//     exit(0);
//   }

//   write(fd[1], &p3_pid, sizeof(p3_pid));
//   write(fd[1], &p2_pid, sizeof(p2_pid));

//   printf("[P1] sending value 10 to P2...\n");
//   co_yield(p2_pid, 10);

//   int final_val = co_yield(p3_pid, 0);
//   printf("[P1] received back from P3: %d\n", final_val);

//   wait(0);
//   wait(0);
//   close(fd[0]);
//   close(fd[1]);
//   printf("Test 2 Passed!\n");
// }


static void 
test_from_assignment(void)
{
  int pid1 = getpid(); // Parent PID
  int pid2 = fork(); // Child PID
  if (pid2 == 0) { // Child
    for (;;) {
      int value = co_yield(pid1, 1);
      printf("Child received: %d\n", value); // Should print 2
    }
  } else { // Parent
    for (;;) {
      int value = co_yield(pid2, 2);
      printf("parent received: %d\n", value); // Should print 1
    }
  }
}

int
main(void)
{
  test_negative_pid();
  test_zero_pid();
  test_bad_pid();
  test_self_yield();
  test_killed_target();
  // test_target_not_ready();
  // test_chaining();
  printf("co_test: all checks passed\n");
  test_from_assignment();
  exit(0);
}
