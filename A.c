#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdio.h>
#include <string.h>

//#include "PowerConsumption.h"

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
  
  // Check the received response from mote C
  if (strncmp((char *)data, "ACK", 3) == 0) {
    // Received an acknowledgment for healthcheck
    LOG_INFO("Received acknowledgment for healthcheck\n");
  } 
  else if (strncmp((char *)data, "dataExample", 11) == 0) {
    // Received data response
    LOG_INFO("Received data from IPADDR:");
    log_6addr(sender_addr);
    LOG_INFO("Received data response: '%.*s'\n", datalen, (char *) data);
    //stop timeout ------------------------------------------------------------
    timeout = 0;
  } else {
  LOG_INFO("Received unknown message\n");
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
  LOG_INFO("Network started\n");
  PROCESS_END();
}

PROCESS_THREAD(udp_server_process, ev, data)
{
  static struct etimer periodic_timer;
  PROCESS_BEGIN();
  etimer_set(&periodic_timer, CLOCK_SECOND * 10);
  
  // Set the IPv6 address of mote C
  uip_ip6addr(&dest_ipaddr_C, 0xfd00, 0, 0, 0, 0x0212, 0x7403, 0x0003, 0x0303);
  // Set the IPv6 address of mote B
  uip_ip6addr(&dest_ipaddr_B, 0xfd00, 0, 0, 0, 0x0212, 0x7402, 0x0002, 0x0202);
while (1) {
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  etimer_reset(&periodic_timer);
  LOG_INFO("Sending message %d\n", counter);
// Every 15th message, send to B
if (counter % 15 == 0) {
  LOG_INFO("Sending message to B\n");
  static char dataReq_msg[] = "dataReq";
  simple_udp_sendto(&udp_connB, dataReq_msg, strlen(dataReq_msg), &dest_ipaddr_B);
  LOG_INFO("Sent message to B\n");
}
// Every 10th message, send healthcheck
else if (counter % 10 == 0) {
  LOG_INFO("Sending healthcheck message\n");
  // Send a healthcheck message
  static char health_msg[] = "healthcheck";
  simple_udp_sendto(&udp_conn, health_msg, strlen(health_msg), &dest_ipaddr_C);
} else {
  LOG_INFO("Sending dataReq message\n");
  // Send a dataReq message to Mote C
  static char dataReq_msg[] = "dataReq";
  simple_udp_sendto(&udp_conn, dataReq_msg, strlen(dataReq_msg), &dest_ipaddr_C);
  timeout = clock_time();
}
counter++;

  // Reset counter when it reaches 10 to start over
  if (counter == 15) {
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
  //float states_power;

  while(1)
  {
    //states_power = TotalPowerConsumption();
    if (clock_time() > timeout + CLOCK_SECOND && timeout != 0)
    {
      static char dataReq_msg[] = "dataReq"; //temp
      LOG_INFO("Resending package %d\n", counter);
      simple_udp_sendto(&udp_conn, dataReq_msg, strlen(dataReq_msg), &dest_ipaddr_C);
      timeout = clock_time();
    }
    //int_dec = Get_Float_Parts(states_power);
    //LOG_INFO("Test total = %10lu.%07lumAh |\n", int_dec.integer, int_dec.decimal);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timeoutTimer));
    etimer_reset(&timeoutTimer);
  }

  PROCESS_END();
}
