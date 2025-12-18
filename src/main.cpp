#include <Arduino.h>
#include <Ethernet.h>
#include <SPI.h>
#include <MicroROS_Transport.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include "Publisher/genPublisher.h"
#include "Subscriber/genSubscriber.h"

// Define W5500 Ethernet Chip Pins
#define W5500_CS    14    // CS (Chip Select) PIN
#define W5500_RST   9     // Reset PIN
#define W5500_INT   10    // Interrupt PIN 
#define W5500_MISO  12    // MISO PIN
#define W5500_MOSI  11    // MOSI PIN
#define W5500_SCK   13    // Serial Clock PIN

// Network Configuration
byte esp_mac[] = { 0xDE, 0xAD, 0xAF, 0x91, 0x3E, 0x69 };    // Mac address of ESP32
IPAddress esp_ip(192, 168, 0, 14);                         // IP address of ESP32
IPAddress dns(192, 168, 0, 1);                              // DNS Server (Modify if necessary)
IPAddress gateway(192, 168, 0, 1);                          // Default Gateway (Modify if necessary)
IPAddress agent_ip(192, 168, 0, 80);                        // IP address of Micro ROS agent        
size_t agent_port = 8888;                                   // Micro ROS Agent Port Number

// Define Functions
void error_loop();
void SetupSupport();                                                                          // Creates allocator and support for namespace. Only one support should be made per namespace
void initNode(rcl_node_t * node, const char * node_name);                                     // pass node and name objects     
void initExecutor();                                                                          // inititates executor
void HandleConnectionState();
bool CreateEntities();
void DestroyEntities();
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}     // Checks for Errors in Micro ROS Setup
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

// Define shared ROS entities
EthernetUDP udp;
rcl_allocator_t allocator;
rclc_support_t support;
rclc_executor_t executor;
rcl_node_t node;
rmw_context_t* rmw_context;

// MICRO ROS MODIFICATION 
const char * node_name = "node1";

// Define MicroROS Subscriber and Publisher entities
genPublisher pub_str1;
genPublisher pub_str2;

// Define Callback functions for the Subscribers
void BooleanCallback(const void * msgin);
void Int32Callback(const void * msgin);
void DoubleCallback(const void * msgin);

// Connection management
enum class ConnectionState {
  kInitializing,
  kWaitingForAgent,
  kConnecting,
  kConnected,
  kDisconnected
};

ConnectionState connection_state = ConnectionState::kInitializing;



void setup() {

  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Ethernet Connection... ");


  // Initialize SPI with custom pin configuration
  SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI, W5500_CS);


  // Select CS PIN and initialize Ethernet with static IP settings (Selecting CS PIN required for ESP32 as the ardiuno default is different)
  Ethernet.init(W5500_CS);

  Serial.println("[INIT] Starting micro-ROS node...");

  // Start Micro ROS Transport Connection
  set_microros_eth_transports(esp_mac, esp_ip, dns, gateway, agent_ip, agent_port);

  delay(2000);
  connection_state = ConnectionState::kWaitingForAgent;

};

void loop() {
  HandleConnectionState();
  delay(1000);

}

void HandleConnectionState() {
  switch (connection_state) {
    case ConnectionState::kWaitingForAgent:
      if (RMW_RET_OK == rmw_uros_ping_agent(200, 3)) {
        Serial.println("[ROS] Agent found, establishing connection...");
        connection_state = ConnectionState::kConnecting;
      }
      break;

    case ConnectionState::kConnecting:
      if (CreateEntities()) {
        Serial.println("[ROS] Connected and ready!");
        connection_state = ConnectionState::kConnected;
      } else {
        Serial.println("[ROS] Connection failed, retrying...");
        connection_state = ConnectionState::kWaitingForAgent;
      }
      break;

    case ConnectionState::kConnected:
      if (RMW_RET_OK != rmw_uros_ping_agent(200, 3)) {
        Serial.println("[ROS] Agent disconnected!");
        connection_state = ConnectionState::kDisconnected;
      } else {
        Serial.println("heartbeat");
        pub_str1.publish("Hello World");
        pub_str2.publish("ROS2");
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
      }
      break;

    case ConnectionState::kDisconnected:
      DestroyEntities();
      Serial.println("[ROS] Waiting for agent...");
      connection_state = ConnectionState::kWaitingForAgent;
      break;

    default:
      break;
  }
}

bool CreateEntities() {

  SetupSupport();
  initNode(&node, node_name);
  initExecutor();
  
  // ADD ALL YOUR PUBLISHERS AND SUBSCRIBER INITIALISATION HERE
  pub_str1.init(&node, "StringPubA", STRING);
  pub_str2.init(&node, "StringPubB", STRING);
  
  return true;
}

void DestroyEntities() {
    rmw_context = rcl_context_get_rmw_context(&support.context);
    (void)rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);
    
    // ADD FUNCTION THAT DESTROYS PUBLISHER AND SUBSCRIBER HERE
    pub_str1.destroy(&node);
    pub_str2.destroy(&node);


    rclc_executor_fini(&executor);
    RCCHECK(rcl_node_fini(&node));
    rclc_support_fini(&support);
}

void SetupSupport(){
  allocator = rcl_get_default_allocator();
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
}


void initNode(rcl_node_t * node, const char * node_name){
  // Create node object
  RCCHECK(rclc_node_init_default(node, node_name, "", &support));
}

void initExecutor(){
  RCCHECK(rclc_executor_init(&executor, &support.context, 10, &allocator)); // number of subscibers the executor handles is hard coded atm
}

// Error handle loop
void error_loop() {

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
