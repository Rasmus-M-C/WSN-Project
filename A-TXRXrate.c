#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdio.h>
#include <string.h>
#include "sys/energest.h"

//#include "PowerConsumption.h"
#include "sys/energest.h"


#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_PORT_A 8765
#define UDP_PORT_C 5678
#define UDP_PORT_B 5679

static struct simple_udp_connection udp_conn;
static struct simple_udp_connection udp_connB;

static clock_time_t timeout = 0;
static int counter = 0;

//static struct PowerConsumptionStates states_power;
#define RX 23.0 // mA
#define TX 21.0 // mA
#define Radiooff 0.0051 // mA Not sure if this is correct
#define CPU_NORMAL 1.8 // mA
#define CPU_SLEEP 0.0051 // mA
#define CPU_DEEP_SLEEP 0.0051 // mA Not sure if this is correct

unsigned int to_seconds(uint32_t time)
{
  return (unsigned int)(time/ ENERGEST_SECOND);
}

void logging(float value) {
    int A;
    A = value;
    float frac = (value-A)*1e4;
    LOG_INFO("Test total = %d.%04umAh |\n", A, frac);
}

float TotalPowerConsumption() {
  float power = 0;
  energest_flush(); // Update all energest times. Should always be called before energest times are read.
  power += ((to_seconds(energest_type_time(ENERGEST_TYPE_CPU))))*CPU_NORMAL+((to_seconds(energest_type_time(ENERGEST_TYPE_LPM))))*CPU_SLEEP+((to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT))))*TX+((to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN))))*RX;
  return power;
}
static int TX_count = 0;
static int RX_count = 0;

// Define the IPv6 address of mote C
uip_ipaddr_t dest_ipaddr_C;
uip_ipaddr_t dest_ipaddr_B;
PROCESS(udp_server_process, "UDP server");
PROCESS(udp_network_process, "UDP network start");
PROCESS(checkTimeout, "TimeoutChecker");
AUTOSTART_PROCESSES(&udp_server_process, &udp_network_process, &checkTimeout);

static void udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  //(uip_ipaddr_cmp(sender_addr, &dest_ipaddr_C))
    //uip_ip6addr(&dest_ipaddr_C, 0xfd00, 0, 0, 0, 0x0212, 0x7403, 0x0003, 0x0303); 

    if (sender_port == UDP_PORT_C) {
    RX_count++;
    timeout = 0;
  }
  // Check the received response from mote C
  if (strncmp((char *)data, "ACK", 3) == 0) {
    // Received an acknowledgment for healthcheck
    //LOG_INFO("Received acknowledgment for healthcheck\n");
  } 
  else if (strncmp((char *)data, "dataExample", 11) == 0) {
    // Received data response
    //LOG_INFO("Received data from IPADDR:");
    log_6addr(sender_addr);
    //LOG_INFO("Received data response: '%.*s'\n", datalen, (char *) data);
    //stop timeout ------------------------------------------------------------
    
  } else {
  //LOG_INFO("Received unknown message\n");
  // Handle unknown messages as needed
 }

  // You can add more checks for different response types as needed.
}

PROCESS_THREAD(udp_network_process, ev, data)
{
  PROCESS_BEGIN();
  // Start the network
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_PORT_A, NULL,
                      UDP_PORT_C, udp_rx_callback);
  simple_udp_register(&udp_connB, UDP_PORT_A, NULL,
                      UDP_PORT_B, udp_rx_callback);  
  //LOG_INFO("Network started\n");
  PROCESS_END();
}

PROCESS_THREAD(udp_server_process, ev, data)
{
  static struct etimer periodic_timer;
  static int8_t ratio = 100;
  PROCESS_BEGIN();
  
  etimer_set(&periodic_timer, CLOCK_SECOND * 10);
  
  // Set the IPv6 address of mote C
  uip_ip6addr(&dest_ipaddr_C, 0xfd00, 0, 0, 0, 0x0212, 0x7403, 0x0003, 0x0303);
  // Set the IPv6 address of mote B
  uip_ip6addr(&dest_ipaddr_B, 0xfd00, 0, 0, 0, 0x0212, 0x7402, 0x0002, 0x0202);
while (1) {
  ratio = (RX_count*1e2)/(TX_count);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  etimer_reset(&periodic_timer);
  //LOG_INFO("Sending message %d\n", counter);
// Every 15th message, send to B
LOG_INFO("Ratio: %d\n", ratio);

if (ratio < 38) {
  LOG_INFO("Sending message B\n");
  static char dataReq_msg[] = "dataReq";
  simple_udp_sendto(&udp_connB, dataReq_msg, strlen(dataReq_msg), &dest_ipaddr_B);
  
  if (counter % 10 == 0) { // Every 10th message, send healthcheck
    //LOG_INFO("Sending healthcheck message\n");
    // Send a healthcheck message
    TX_count++;
    static char health_msg[] = "healthcheck";
    simple_udp_sendto(&udp_conn, health_msg, strlen(health_msg), &dest_ipaddr_C);
  }
}
else {
  //LOG_INFO("Sending dataReq message\n");
  // Send a dataReq message to Mote C
  TX_count++;
  static char dataReq_msg[] = "dataReq";
  simple_udp_sendto(&udp_conn, dataReq_msg, strlen(dataReq_msg), &dest_ipaddr_C);
  timeout = clock_time();
}
counter++;
LOG_INFO("TX: %d, RX: %d\n", TX_count, RX_count);

  // Reset counter when it reaches 10 to start over
  if (counter == 11) {
    counter = 0;
  }
}

  PROCESS_END();
}


PROCESS_THREAD(checkTimeout, ev, data)
{
  static struct etimer timeoutTimer;

  PROCESS_BEGIN();
  etimer_set(&timeoutTimer, CLOCK_SECOND);
  // Power Consumption
  static struct etimer periodic_timer;
  //struct IntDec int_dec;
  float states_power  = 0.0;

  while(1)
  {
   
    
    states_power = TotalPowerConsumption();
    if (clock_time() > timeout + CLOCK_SECOND && timeout != 0)
    {
      static char dataReq_msg[] = "dataReq"; //temp
      //LOG_INFO("Resending package %d\n", counter);
      simple_udp_sendto(&udp_conn, dataReq_msg, strlen(dataReq_msg), &dest_ipaddr_C);
      timeout = clock_time();
      TX_count++;
    }
    logging(states_power);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timeoutTimer));
    etimer_reset(&timeoutTimer);
  }

  PROCESS_END();
}
