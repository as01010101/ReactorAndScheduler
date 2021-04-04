#include "scheduler.h"
#include <stdio.h>
void test_fiber() {
    static int s_count = 5;
    printf("test in fiber s_count=%d\n", s_count);

    sleep(1);
    if (--s_count >= 0) {
      Scheduler::GetThis()->schedule(&test_fiber, GetCurrssThreadId());
    }
}

int main(int argc, char** argv) {
    printf("main\n");
    Scheduler sc(3, false, "test");
    sc.start();
    sleep(2);
    printf("schedule\n");
    sc.schedule(&test_fiber);
    sc.stop();
    printf("over");
    return 0;
}
