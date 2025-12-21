#include "genPublisher.h"
#include "datatype.h"



genPublisher::genPublisher(){}

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){} else{} }
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){} }

void genPublisher::init(rcl_node_t * node, const char * topic, DataType datatype){
    this->topic_name = topic;
    this->data_type = datatype;

    switch (datatype) {
        case DataType::BOOL:
            
            rclc_publisher_init_default(
                &publisher,
                node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
                topic_name

            );
        break;

        case DataType::INT:
            
            rclc_publisher_init_default(
                &publisher,
                node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
                topic_name
                
            );
            
        break;

        case DataType::DOUBLE:

            rclc_publisher_init_default(
                &publisher,
                node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float64),
                topic_name
               
            );

        break;

        case DataType::STRING:
            rclc_publisher_init_default(
                &publisher,
                node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
                topic_name
               
            );
        break;

        case DataType::INT32_ARRAY:
            rclc_publisher_init_default(
                &publisher,
                node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray),
                topic_name
               
            );
        break;

        default:
            while (1)
            {
                delay(2000);
                Serial.println("ERROR: Code doesn't work.");

            }
        break;
    }
};


void genPublisher::publish(int32_t data) {
    if(data_type == INT){
        std_msgs__msg__Int32 msg_int;
        msg_int.data = data;  // Assign to the message
        RCCHECK(rcl_publish(&publisher, &msg_int, NULL));

    } else {
        //some error handling
    }

}

void genPublisher::publish(bool data) {
    if(data_type == BOOL) {
        std_msgs__msg__Bool msg_bool;
        msg_bool.data = data;  // Assign to the message
        RCCHECK(rcl_publish(&publisher, &msg_bool, NULL));
    } else {
        //some error handling
    }

}

void genPublisher::publish(double data) {
    if(data_type == DOUBLE) {
        std_msgs__msg__Float64 msg_doub;
        msg_doub.data = data;  // Assign to the message
        RCCHECK(rcl_publish(&publisher, &msg_doub, NULL));
    } else {
        //some error handling
    }
}

void genPublisher::publish(const char* text) {
    if(data_type == STRING) {
        std_msgs__msg__String msg_str;

        size_t len = strlen(text);
        

        msg_str.data.capacity = len + 1;
        msg_str.data.size = len;
        msg_str.data.data = (char*) malloc(msg_str.data.capacity * sizeof(char));

        memcpy(msg_str.data.data, text, msg_str.data.capacity); // includes '\0'

        RCCHECK(rcl_publish(&publisher, &msg_str, NULL));

    } else {
        //some error handling
    }
}


void genPublisher::publish(int32_t arr[], int arrLen) {
    if (data_type == INT32_ARRAY) {
        std_msgs__msg__Int32MultiArray msgArr;

        msgArr.data.capacity = arrLen;
        msgArr.data.size = arrLen;
        msgArr.data.data = (int32_t *) malloc(arrLen * sizeof(int32_t));

        msgArr.layout.dim.capacity = 0;
        msgArr.layout.dim.size = 0;
        msgArr.layout.dim.data = NULL;
        msgArr.layout.data_offset = 0;

        memcpy(msgArr.data.data, arr, arrLen * sizeof(int32_t));
        
        RCCHECK(rcl_publish(&publisher, &msgArr, NULL));
        free(msgArr.data.data);

    } else {
        //some error handling
    }
}

void genPublisher::destroy(rcl_node_t * node) {
    RCCHECK(rcl_publisher_fini(&publisher, node));
}