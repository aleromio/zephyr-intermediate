#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024


#define PRIORITY_HIGH    3
#define PRIORITY_MED     5
#define PRIORITY_LOW     7
#define PRIORITY_COOP  (-1)



void t_low_fn(void *p1, void *p2, void *p3)
{
    LOG_INF("T_LOW starting . . .");

    while (1) {
        LOG_INF("T_LOW running");
        k_msleep(300);
    }
}

void t_med_fn(void *p1, void *p2, void *p3)
{
    LOG_INF("T_MED starting . . .");

    while (1) {
        LOG_INF("T_MED running");
        k_msleep(200);
    }
}

void t_high_fn(void *p1, void *p2, void *p3)
{
    LOG_INF("T_HIGH starting . . .");

    while (1) {
        LOG_INF("T_HIGH running");
        k_msleep(100);
    }
}


void t_coop_fn(void *p1, void *p2, void *p3)
{
    LOG_INF("T_COOP starting . . .");

    for (int i = 0; i < 5; i++) {
        k_busy_wait(100000);   
        LOG_INF("T_COOP running");
    }
    k_yield();
}

K_THREAD_DEFINE(t_low, STACK_SIZE, t_low_fn, NULL, NULL, NULL, PRIORITY_LOW, 0, 0);
K_THREAD_DEFINE(t_med, STACK_SIZE, t_med_fn, NULL, NULL, NULL, PRIORITY_MED, 0, 0);
K_THREAD_DEFINE(t_high,  STACK_SIZE, t_high_fn,  NULL, NULL, NULL, PRIORITY_HIGH,  0, 0);
K_THREAD_DEFINE(t_coop,  STACK_SIZE, t_coop_fn,  NULL, NULL, NULL, PRIORITY_COOP,  0, 0);

int main(void)
{
    return 0;
}