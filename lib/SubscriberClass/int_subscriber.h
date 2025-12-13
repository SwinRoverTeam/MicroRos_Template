#ifndef INT_SUBSCRIBER_H
#define INT_SUBSCRIBER_H

#include <Arduino.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();} }
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){} }

class int_subscriber{
public:
    int_subscriber();
    void init(rcl_node_t * node, const char * topic,rclc_executor_t * executor );
private:
    rcl_subscription_t subscriber;
    const char * topic_name;
    std_msgs__msg__Int32 msg;
    static void SubscriptionCallback(const void * msgin);
};

#endif // INT_SUBSCRIBER_H
