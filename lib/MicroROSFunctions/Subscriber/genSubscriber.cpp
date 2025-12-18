#include "genSubscriber.h"
#include "datatype.h"

genSubscriber::genSubscriber(){}

void genSubscriber::init(rcl_node_t * node, const char * topic, rclc_executor_t * executor, rclc_subscription_callback_t callback, DataType datatype){
    this->topic_name = topic;
    this->datatype = datatype;

    switch (datatype) {
        case INT:
            rclc_subscription_init_default(
                &subscriber,
                node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
                topic_name
            );

            rclc_executor_add_subscription(executor, &subscriber, &msg.intmsg, callback, ON_NEW_DATA);

        break;

        case DOUBLE:
            rclc_subscription_init_default(
                &subscriber,
                node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64),
                topic_name
            );

            rclc_executor_add_subscription(executor, &subscriber, &msg.doublemsg, callback, ON_NEW_DATA);

        break;

        case BOOL:
            rclc_subscription_init_default(
                &subscriber,
                node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
                topic_name
            );

            rclc_executor_add_subscription(executor, &subscriber, &msg.boolmsg, callback, ON_NEW_DATA);

        break;


        default:
            Serial.println("ERROR: Subscriber initialisation has failed!");
        break;
    }

    
}

