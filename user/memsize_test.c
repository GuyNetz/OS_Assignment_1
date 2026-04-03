#include "kernel/types.h"
#include "user/user.h"

int main(){
    printf("before: the current process memory in bytes: %d\n", memsize());
    void *ptr = malloc(20000);
    printf("after: the current process memory in bytes: %d\n", memsize());
    free(ptr);
    printf("after free: the current process memory in bytes: %d\n", memsize());
    exit(0);
}