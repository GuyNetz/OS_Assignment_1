#include "kernel/types.h"
#include "user/user.h"

static void
test_basic_handoff(void)
{
  int parent = getpid();
  int child = fork();

  if(child < 0){
    printf("basic: fork failed\n");
    exit(1);
  }

  if(child == 0){
    int received = co_yield(parent, 1);
    printf("basic child received: %d\n", received);
    exit(received == 2 ? 0 : 1);
  }

  int received = co_yield(child, 2);
  printf("basic parent received: %d\n", received);

  int status = -1;
  wait(&status);
  if(received == 1 && status == 0){
    printf("basic: ok\n");
  } else {
    printf("basic: failed\n");
    exit(1);
  }
}

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

int
main(void)
{
  test_basic_handoff();
  test_bad_pid();
  test_self_yield();
  test_killed_target();
  printf("co_test: all checks passed\n");
  exit(0);
}
