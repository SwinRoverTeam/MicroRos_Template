#include "int_subscriber.h"

int_subscriber::int_subscriber(){}

void int_subscriber::init(rcl_node_t * node, const char * topic,rclc_executor_t * executor ){
    this->topic_name = topic;
    rclc_subscription_init_default(
        &subscriber,
        node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        topic_name
    );
    rclc_executor_add_subscription(executor, &subscriber, &msg, &SubscriptionCallback, ON_NEW_DATA);
}
void int_subscriber::SubscriptionCallback(const void * msgin)
{
  // Cast received message to used type
  const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;
  Serial.println(msg->data);
}


