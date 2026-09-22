#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE       1024
#define SENSOR_TIMEOUT   100
#define LOGGER_QUEUE_SIZE 4

struct sensor_data {
    int32_t sensor_value;
};

static void update_display_data(const struct sensor_data * sensor_data)
{
    // simulated display update
        LOG_INF("[DISPLAY_UPDATE] sensor=%d", sensor_data->sensor_value);
}

static void display_listener_fn(const struct zbus_channel *chan);

//---------------------------------------------------------------
// Fast Display Liestener.
//---------------------------------------------------------------

ZBUS_LISTENER_DEFINE(display_listener, display_listener_fn);

//---------------------------------------------------------------
// Slow datalogger.
//---------------------------------------------------------------

ZBUS_SUBSCRIBER_DEFINE(logger_sub, LOGGER_QUEUE_SIZE);



//---------------------------------------------------------------
//  Channel                                                          
//---------------------------------------------------------------

ZBUS_CHAN_DEFINE(sensor_channel, struct sensor_data,
                 NULL, NULL,
                 ZBUS_OBSERVERS(display_listener, logger_sub),
                 ZBUS_MSG_INIT(.sensor_value = 0));


//---------------------------------------------------------------
//  Fast Listener Callback        
//---------------------------------------------------------------

static void display_listener_fn(const struct zbus_channel *chan)
{
    const struct sensor_data *sensor_msg = (const struct sensor_data *)zbus_chan_const_msg(chan);

    update_display_data(sensor_msg);
    
}

//---------------------------------------------------------------
// Sensor publisher
//---------------------------------------------------------------

static void sensor_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);


    while(1){
        static struct sensor_data data = {.sensor_value = 0};

        int ret = zbus_chan_pub(&sensor_channel, &data, K_MSEC(200));
        if (ret != 0) {
            LOG_WRN("[SENSOR] publish failed ret=%d", ret);
        }
        data.sensor_value++;
        if(data.sensor_value >= 100){
            data.sensor_value = 0;
        }
        LOG_INF("[PUBLISH] sensor_value=%d",data.sensor_value);
        k_msleep(SENSOR_TIMEOUT);
    }

}

void my_callback(int channel_id, void *user_data)
{
    k_tid_t failed_thread = (k_tid_t)user_data;
    const char *name = k_thread_name_get(failed_thread);
    LOG_INF("[TASK_WDT] Callback ejecuted from:%s", name);
}



//---------------------------------------------------------------
// Logger Callback
//---------------------------------------------------------------
static void logger_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);


    const struct zbus_channel *chan;

    int err = task_wdt_init(NULL);
    if (err) {
        LOG_INF("[LOGGER] Error inicializando task watchdog:%d", err);
        return;
    }

    int wdt_chan = task_wdt_add(1000,my_callback,(void *)k_current_get());

    while (1) {
        struct sensor_data msg;

        int ret = zbus_sub_wait(&logger_sub, &chan, K_MSEC(500));
        if (ret != 0) {
            LOG_WRN("[LOGGER-MSG] timeout ret=%d", ret);
            break;
        }

        ret = zbus_chan_read(chan, &msg, K_MSEC(100));  
        if (ret != 0) {
            LOG_WRN("[LOGGER] error leyendo canal ret=%d", ret);
            continue;
        }

        LOG_INF("[LOGGER] sensor_value=%d",msg.sensor_value);

        task_wdt_feed(wdt_chan); /* In the thread: feed regularly */

        k_msleep(150);
    }
}


/* ================================================================== */
/*  Health monitor                                                    */
/* ================================================================== */

static void health_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    while(1){
        k_msleep(120);

        uint32_t used  = k_msgq_num_used_get(logger_sub.queue);
        uint32_t free  = k_msgq_num_free_get(logger_sub.queue);
        uint32_t total = used + free;

        if(used*100/total > 75){
            LOG_WRN("[HEALTH] Queue usage over 75 percent!");
        }
    }

}



K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_thread_fn,
                NULL, NULL, NULL, 5, 0, 0);

K_THREAD_DEFINE(logger_thread, STACK_SIZE, logger_thread_fn,
                NULL, NULL, NULL, 6, 0, 0);


K_THREAD_DEFINE(health_thread, STACK_SIZE, health_fn,
                NULL, NULL, NULL, 7, 0, 0);
                


int main(void)
{
    LOG_INF("=== l4_task1 ===");

    return 0;
}