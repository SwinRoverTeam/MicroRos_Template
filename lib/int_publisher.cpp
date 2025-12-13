#include "int_publisher.h"

int_publisher::int_publisher(){}

void int_publisher::init(rcl_node_t * node, const char * topic){
    this->topic_name = topic;
    rclc_publisher_init_default(
        &publisher,
        node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        topic_name
    );
}

void int_publisher::publish(int data) {
    msg.data = data;  // Assign to the message
    rcl_publish(&publisher, &msg, NULL);
}

