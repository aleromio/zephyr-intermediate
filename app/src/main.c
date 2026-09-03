#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE       1024
#define SENSOR_TIMEOUT   100



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

ZBUS_MSG_SUBSCRIBER_DEFINE(logger_msg_sub);



//---------------------------------------------------------------
//  Channel                                                          
//---------------------------------------------------------------

ZBUS_CHAN_DEFINE(sensor_channel, struct sensor_data,
                 NULL, NULL,
                 ZBUS_OBSERVERS(display_listener, logger_msg_sub),
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

//---------------------------------------------------------------
// Slow Logger Callback
//---------------------------------------------------------------
static void logger_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);


    const struct zbus_channel *chan;

    while (1) {
        struct sensor_data msg;

        int ret = zbus_sub_wait_msg(&logger_msg_sub, &chan, &msg, K_MSEC(500));
        if (ret != 0) {
            LOG_WRN("[LOGGER-MSG] timeout ret=%d", ret);
            break;
        }

        LOG_INF("[LOGGER] sensor_value=%d",msg.sensor_value);

        k_msleep(80);
    }
}


K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_thread_fn,
                NULL, NULL, NULL, 5, 0, 0);

K_THREAD_DEFINE(logger_thread, STACK_SIZE, logger_thread_fn,
                NULL, NULL, NULL, 6, 0, 0);



int main(void)
{
    LOG_INF("=== l4_task1 ===");

    return 0;
}