#include <Arduino.h>
#include <Ethernet.h>
#include <SPI.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include "Publisher/genPublisher.h"                     // Include this library to use the publisher      
#include "Subscriber/genSubscriber.h"                   // Include this library to use the Subscriber
#include <MicroROS_Transport.h>                         // IMPORTANT: MAKE SURE TO INCLUDE FOR CONNECTION BETWEEN THE ESP AND THE MICRO ROS AGENT TO WORK

// Define W5500 Ethernet Chip Pins
#define W5500_CS    14    // CS (Chip Select) PIN
#define W5500_RST   9     // Reset PIN
#define W5500_INT   10    // Interrupt PIN 
#define W5500_MISO  12    // MISO PIN
#define W5500_MOSI  11    // MOSI PIN
#define W5500_SCK   13    // Serial Clock PIN

// Network Configuration
byte esp_mac[] = { 0xDE, 0xAD, 0xAF, 0x91, 0x3E, 0x69 };    // Mac address of ESP32 (Make sure its unique for each ESP32)
IPAddress esp_ip(192, 168, 0, 12);                          // IP address of ESP32   (Make sure its unique for each ESP32)
IPAddress dns(192, 168, 0, 1);                              // DNS Server           (Modify if necessary)
IPAddress gateway(192, 168, 0, 1);                          // Default Gateway      (Modify if necessary)
IPAddress agent_ip(192, 168, 0, 80);                        // IP address of Micro ROS agent   (Modify if necessary)        
size_t agent_port = 8888;                                   // Micro ROS Agent Port Number     (Modify if necessary)

// Define Functions
void error_loop();                                                                            
void HandleConnectionState();
bool CreateEntities();
void DestroyEntities();
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}     // Checks for Errors in Micro ROS Setup
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}              // Checks for Errors in Micro ROS Setup
#define ARRAY_LEN(arr) { (sizeof(arr) / sizeof(arr[0])) }                                     // Calculate the Array Length (Needed for the Int32 Array Publisher)

// Define shared ROS entities
rcl_allocator_t allocator;
rclc_support_t support;
rclc_executor_t executor;
rcl_node_t node;
rmw_context_t* rmw_context;

// Define Node Name
const char * node_name = "nodeExample";


// Define MicroROS Subscriber and Publisher entities
genPublisher pub_int;                                   // Int32 Publisher
genPublisher pub_double;                                // Double Publisher
genPublisher pub_bool;                                  // Boolean Publisher
genPublisher pub_INTarr;                                // Int32 Array Publisher
genPublisher pub_DBarr;                                 // Float64 (double) Array Publisher
genPublisher pub_str;                                   // String Publisher

genSubscriber sub_int;                                  // Integer Subscriber
genSubscriber sub_double;                               // Double Subscriber
genSubscriber sub_bool;                                 // Boolean Subscriber
genSubscriber sub_INTarr;                               // Int32 Array Subscriber
genSubscriber sub_DBarr;                                // Float64 (double) Array Subscriber


// Define variables for the publisher to send values
int INTarr[] = {1, 2, 3, 4, 5};
double DBarr[] = {1.0, 1.5, 2.0, 2.5, 3.0};


// Define Callback functions for the Subscribers
void BooleanCallback(const void * msgin);
void Int32Callback(const void * msgin);
void DoubleCallback(const void * msgin);
void Int32ArrayCallback(const void * msgin);
void Float64ArrayCallback(const void * msgin);


// Connection status for the HandleConnection()
enum class ConnectionState {
  Initializing,
  WaitingForAgent,
  Connecting,
  Connected,
  Disconnected
};

ConnectionState connection_state = ConnectionState::Initializing;


// Include the code for startinh up the ethernet chip and initialising the Micro ROS transport
void setup() {

  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Ethernet Connection... ");


  SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI, W5500_CS);                                   // Initialize SPI with custom pin configuration    
  Ethernet.init(W5500_CS);                                                                  // Select CS PIN and initialize Ethernet chip

  Serial.println("[INIT] Starting micro-ROS node...");
  set_microros_eth_transports(esp_mac, esp_ip, dns, gateway, agent_ip, agent_port);         // IMPORTANT: Start Micro ROS Transport Connection 

  delay(2000);

  connection_state = ConnectionState::WaitingForAgent;

};

void loop() {
  HandleConnectionState();
  delay(1000);

}

// This function handles the connect between Micro ROS inside the ESP and the Micro ROS agent
void HandleConnectionState() {
  switch (connection_state) {
    case ConnectionState::WaitingForAgent:
      // Ping to the Micro ROS agent
      if (RMW_RET_OK == rmw_uros_ping_agent(200, 3)) {
        Serial.println("[ROS] Agent found, establishing connection...");
        connection_state = ConnectionState::Connecting;
      }
      break;

    case ConnectionState::Connecting:
      // Create all micro ROS entities
      if (CreateEntities()) {
        Serial.println("[ROS] Connected and ready!");
        connection_state = ConnectionState::Connected;
      } else {
        Serial.println("[ROS] Connection failed, retrying...");
        connection_state = ConnectionState::WaitingForAgent;
      }
      break;

    case ConnectionState::Connected:
    // If Micro ROS agent gets disconnected...
      if (RMW_RET_OK != rmw_uros_ping_agent(200, 3)) {
        Serial.println("[ROS] Agent disconnected!");
        connection_state = ConnectionState::Disconnected;
      } else {
        Serial.println("heartbeat");                                                      // Use it for testing if the code is working
        
      // ADD HERE FOR PUBLISHING VALUES CONTINUOUSLY
        pub_int.publish(8);
        pub_bool.publish(true);
        pub_double.publish(0.69);
        pub_INTarr.publish(INTarr, ARRAY_LEN(INTarr));                                          // Note: ARRAY LENGTH MUST BE ADD FOR THE INT32 ARRAY PUBLISH FUNCTION TO WORK
        pub_DBarr.publish(DBarr, ARRAY_LEN(DBarr));                                             // Note: ARRAY LENGTH MUST BE ADD FOR THE FLOAT64 ARRAY PUBLISH FUNCTION TO WORK
        pub_str.publish("Hello world");

        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));                                  // Spins the executor (Important for Subscribers)
      }
      break;

    case ConnectionState::Disconnected:
      DestroyEntities();
      Serial.println("[ROS] Waiting for agent...");
      connection_state = ConnectionState::WaitingForAgent;
      break;

    default:
      break;
  }
}

// This function creates/initialises all micro ros entites (Publishers, Subscribers, Executors, Nodes, Support...)
bool CreateEntities() {

  allocator = rcl_get_default_allocator();
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // Create node object
  RCCHECK(rclc_node_init_default(&node, node_name, "", &support));
  RCCHECK(rclc_executor_init(&executor, &support.context, 10, &allocator)); // number of subscribers the executor handles is hard coded atm
  
  // ADD ALL YOUR PUBLISHERS AND SUBSCRIBER INITIALISATION HERE
  pub_int.init(&node, "Int32Example", INT);                                                           // Initialise Int32 Publisher
  pub_bool.init(&node, "BooleanExample", BOOL);                                                       // Initialise Boolean Publisher
  pub_double.init(&node, "DoubleExample", DOUBLE);                                                    // Initialise Double Publisher
  pub_INTarr.init(&node, "Int32ArrayExample", INT32_ARRAY);                                           // Initialise Int32 Array Publisher
  pub_DBarr.init(&node, "Float64ArrayExample", FLOAT64_ARRAY);                                        // Initialise Float64 Array Publisher
  pub_str.init(&node, "StringExample", STRING);                                                       // Initialise String Publisher

  sub_int.init(&node, "Int32Example", &executor, Int32Callback, INT);                                 // Initialise Int32 Subscriber
  sub_bool.init(&node, "BooleanExample", &executor, BooleanCallback, BOOL);                           // Initialise Boolean Subscriber
  sub_double.init(&node, "DoubleExample", &executor, DoubleCallback, DOUBLE);                         // Initialise Double Subscriber
  sub_INTarr.init(&node, "Int32ArrayExample", &executor, Int32ArrayCallback, INT32_ARRAY);            // Initialise Int32 Array Subscriber
  sub_DBarr.init(&node, "Float64ArrayExample", &executor, Float64ArrayCallback, FLOAT64_ARRAY);       // Initialise Float64 Array Subscriber

  return true;
}



// This function destroys all micro ros entites (Publishers, Subscribers, Executors, Nodes, Support...)
void DestroyEntities() {
    rmw_context = rcl_context_get_rmw_context(&support.context);
    (void)rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);
    
    // ADD FUNCTION THAT DESTROYS PUBLISHER AND SUBSCRIBER HERE
    pub_int.destroy(&node);                                     // Destroy Int32 Publisher
    pub_bool.destroy(&node);                                    // Destroy Boolean Publisher
    pub_double.destroy(&node);                                  // Destroy Double Publisher
    pub_INTarr.destroy(&node);                                  // Destroy Int32 Array Publisher
    pub_DBarr.destroy(&node);                                   // Destroy Float64 Array Publisher
    pub_str.destroy(&node);                                     // Destroy String Publisher

    sub_int.destroy(&node);                                     // Destroy Int32 Subscriber
    sub_bool.destroy(&node);                                    // Destroy Boolean Subscriber
    sub_double.destroy(&node);                                  // Destroy Double Subscriber
    sub_INTarr.destroy(&node);                                  // Destroy Int32 Array Subscriber
    sub_DBarr.destroy(&node);                                   // Destroy Float64 Array Subscriber

    rclc_executor_fini(&executor);                              // Destroy Executors
    RCCHECK(rcl_node_fini(&node));                              // Destroy Node
    rclc_support_fini(&support);                                // Destroy Support
}

// Error handle loop
void error_loop() {
  Serial.println("An error has occured. Restarting...");
  delay(2000);
  ESP.restart();

};

// ========================================= CALLBACK FUNCTIONS ========================================= //
// Create your callback functions here.


// Example Callback funtion for Integer values
void Int32Callback(const void * msgin) {
  const std_msgs__msg__Int32 * msg_int = (const std_msgs__msg__Int32 *)msgin;               // IMPORTANT: DO NOT FORGET TO ADD THIS !!!

  // Enter code for when the subscriber receives a message.
  Serial.print("Integer value: ");
  Serial.println(msg_int->data);
}

// Example Callback funtion for Boolean values
void BooleanCallback(const void * msgin) {

  const std_msgs__msg__Bool * msg_bool = (const std_msgs__msg__Bool *)msgin;                  // IMPORTANT: DO NOT FORGET TO ADD THIS !!!

  // Enter code here for when the subscriber receives a message.
  Serial.print("Boolean value: ");
  Serial.println(msg_bool->data);
}


// Example Callback funtion for Double (Float64) values
void DoubleCallback(const void * msgin) {

  const std_msgs__msg__Float64 * msg_double = (const std_msgs__msg__Float64 *)msgin;          // IMPORTANT: DO NOT FORGET TO ADD THIS !!!

  // Enter code here for when the subscriber receives a message.
  Serial.print("Double value: ");
  Serial.println(msg_double->data);
}

// Example Callback funtion for Int32 Array values
void Int32ArrayCallback(const void * msgin) {

    const std_msgs__msg__Int32MultiArray * IntArrmsg = (const std_msgs__msg__Int32MultiArray *)msgin;              // IMPORTANT: DO NOT FORGET TO ADD THIS !!!

    // Access the data array
    size_t size = IntArrmsg->data.size;
    
    const int32_t * array_data = IntArrmsg->data.data;

    Serial.print("Array size: ");
    Serial.println(size);
    for(size_t i = 0; i < size; i++)
    {
        Serial.print("Element ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(array_data[i]);
    }

}

// Example Callback funtion for Int32 Array values
void Float64ArrayCallback(const void * msgin) {

    const std_msgs__msg__Float64MultiArray * DoubleArrmsg = (const std_msgs__msg__Float64MultiArray *)msgin;              // IMPORTANT: DO NOT FORGET TO ADD THIS !!!

    // Access the data array
    size_t size = DoubleArrmsg->data.size;
    
    const double * array_data = DoubleArrmsg->data.data;

    Serial.print("Array size: ");
    Serial.println(size);
    for(size_t i = 0; i < size; i++)
    {
        Serial.print("Element ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(array_data[i]);
    }

}