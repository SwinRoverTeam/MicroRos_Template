#ifndef DATATYPE_H
#define DATATYPE_H

enum DataType {
    INT,
    DOUBLE,
    BOOL,
    STRING
};

union DataMsg {
    std_msgs__msg__Int32 intmsg;
    std_msgs__msg__Float64 doublemsg;
    std_msgs__msg__Bool boolmsg;

};

struct AnyValue {
    DataType type;
    DataMsg msg;
};

#endif // INT_PUBLISHER_H