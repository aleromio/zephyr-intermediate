#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE      1024
#define PRIO            5
#define ITERATIONS      500000

static volatile uint32_t counter;

static K_MUTEX_DEFINE(counter_mutex);

/* Added Unprotected functions after review. 
 * Explanation: Counter is not atomic, so counter++ is implemented in assembly  as load -> add -> store. 
 * IF context switch threads between the load and the store both threads store the
 * same value increment and save the same, so one increment is lost.
 */
void unprotected_first_thread_fn(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < ITERATIONS; i++) {
        counter++;
    }
    LOG_INF("[UNPROTECTED FIRST] done, counter = %u", counter);
}

void unprotected_second_thread_fn(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < ITERATIONS; i++) {
        counter++;
    }
    LOG_INF("[UNPROTECTED SECOND] done, counter = %u", counter);
}

/* Protected functions with a mutex.
 * One thread runs frst and the other waits until the mutex is unlucked so we dont loose increments.
 */
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

/* The unprotected threads runs inmediately*/
K_THREAD_DEFINE(t_unprotected_first, STACK_SIZE, unprotected_first_thread_fn,
                NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(t_unprotected_second, STACK_SIZE, unprotected_second_thread_fn,
                NULL, NULL, NULL, PRIO, 0, 0);

/* Added infinit delay with SYS_FOREVER_MS , this threads will run in main after the other two finishes*/
K_THREAD_DEFINE(t_first, STACK_SIZE, first_thread_fn, NULL, NULL, NULL,
                PRIO, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(t_second, STACK_SIZE, second_thread_fn, NULL, NULL, NULL,
                PRIO, 0, SYS_FOREVER_MS);

int main(void)
{

    LOG_INF("=== L2 Task 1: Counter Corruption ===");

    // wait for the two unprotected threads to finish
    k_thread_join(t_unprotected_first, K_FOREVER);
    k_thread_join(t_unprotected_second, K_FOREVER);
    LOG_INF("[UNPROTECTED THREADS] Expected: %d  |  Actual: %u", 2 * ITERATIONS, counter);

    counter = 0;  // reset counter for second try.
    k_thread_start(t_first);   // start protected threads
    k_thread_start(t_second);
    k_thread_join(t_first, K_FOREVER);
    k_thread_join(t_second, K_FOREVER);  

    LOG_INF("[PROTECTED THREADS] Expected: %d  |  Actual: %u", 2 * ITERATIONS, counter);

    return 0;
}