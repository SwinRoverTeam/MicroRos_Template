#include <Arduino.h>
#include <Ethernet.h>
#include <SPI.h>
#include <MicroROS_Transport.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <int_publisher.h>
#include <int_subscriber.h>


// Define W5500 Ethernet Chip Pins
#define W5500_CS    14    // CS (Chip Select) PIN
#define W5500_RST   9     // Reset PIN
#define W5500_INT   10    // Interrupt PIN 
#define W5500_MISO  12    // MISO PIN
#define W5500_MOSI  11    // MOSI PIN
#define W5500_SCK   13    // Serial Clock PIN


// Define shared ROS entities
EthernetUDP udp;
rcl_allocator_t allocator;
rclc_support_t support;
rclc_executor_t executor;

rcl_node_t node;
const char * node_name = "node1";

//Define specific ROS entities
int_publisher publisher1;
int_publisher publisher2;

int_subscriber subscriber1;
int_subscriber subscriber2;



// Define Functions
void error_loop();
//Creates allocator abd support for namespace. Only one support should be made per namespace
void SetupSupport();
//pass node and name objects
void initNode(rcl_node_t * node, const char * node_name);
//inititates executor      
void initExecutor();

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}     // Checks for Errors in Micro ROS Setup
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}              // 


// Network Configuration
byte esp_mac[] = { 0xDE, 0xAD, 0xAF, 0x91, 0x3E, 0xD7 };    // Mac address of ESP32
IPAddress esp_ip(192, 168, 0, 11);                         // IP address of ESP32
IPAddress dns(192, 168, 0, 1);                              // DNS Server (Modify if necessary)
IPAddress gateway(192, 168, 0, 1);                          // Default Gateway (Modify if necessary)
IPAddress agent_ip(192, 168, 0, 80);                        // IP address of Micro ROS agent        
size_t agent_port = 8888;                                   // Micro ROS Agent Port Number

void setup() {

  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Ethernet Connection... ");


  // Initialize SPI with custom pin configuration
  SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI, W5500_CS);


  // Select CS PIN and initialize Ethernet with static IP settings (Selecting CS PIN required for ESP32 as the ardiuno default is different)
  Ethernet.init(W5500_CS);

  // Start Micro ROS Transport Connection
  set_microros_eth_transports(esp_mac, esp_ip, agent_ip, agent_port);
  delay(2000);

  SetupSupport();
  initNode(&node,node_name);
  initExecutor();
  publisher1.init(&node, "topicA");
  publisher2.init(&node, "topicB");

  subscriber1.init(&node, "topicC", &executor);
  subscriber2.init(&node, "topicD", &executor);
}



void loop() {
  Serial.println("heartbeat");
  for(int a = 0; a<10;a++){
    delay(100);
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  }
  publisher1.publish(60);
  publisher2.publish(120);
}



// Error handle loop
void error_loop() {
  while(1) {
    delay(1000);
    Serial.println("error loop :(");
  }
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
  RCCHECK(rclc_executor_init(&executor, &support.context, 10, &allocator)); //number of subscibers the executor handles is hard coded atm
}