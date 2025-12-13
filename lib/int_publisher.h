#ifndef INT_PUBLISHER_H
#define INT_PUBLISHER_H

#include <Arduino.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();} }
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){} }

class int_publisher {
public:
    int_publisher();
    void init(rcl_node_t * node, const char * topic);
    void publish(int data);

private:
    rcl_publisher_t publisher;
    const char * topic_name;
    std_msgs__msg__Int32 msg;
};

#endif // INT_PUBLISHER_H
