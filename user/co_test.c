#include "kernel/types.h"
#include "user/user.h"

int main(){
    printf("%d", co_yield(1,2));

    return 0;
}