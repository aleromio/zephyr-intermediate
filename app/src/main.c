#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE      1024
#define PRIO            5
#define ITERATIONS      500000 

static volatile uint32_t counter;

static K_MUTEX_DEFINE(counter_mutex);

void first_thread_fn(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < ITERATIONS; i++) {
        k_mutex_lock(&counter_mutex, K_FOREVER);
        counter++;  
        k_mutex_unlock(&counter_mutex);
    }
    LOG_INF("[FIRST] done, counter = %u", counter);
}

void second_thread_fn(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < ITERATIONS; i++) {
        k_mutex_lock(&counter_mutex, K_FOREVER);
        counter++;
        k_mutex_unlock(&counter_mutex);
    }
    LOG_INF("[SECOND] done, counter = %u", counter);
}

K_THREAD_DEFINE(t_first, STACK_SIZE, first_thread_fn, NULL, NULL, NULL,
                PRIO, 0, 0);
K_THREAD_DEFINE(t_second, STACK_SIZE, second_thread_fn, NULL, NULL, NULL,
                PRIO, 0, 0);

int main(void)
{
    LOG_INF("=== L2 Task 1: Counter Corruption.===");
    k_msleep(30000);  /* wait to both threads end */
    LOG_INF("Expected: %d  |  Actual: %u", 2 * ITERATIONS, counter);
    return 0;
}