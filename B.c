#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include <stdio.h>
#include <string.h>
#include "sys/energest.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT_A 8765
#define UDP_PORT_C 5678
#define UDP_PORT_B 5679
//static struct PowerConsumptionStates states_power;
#define RX 23.0 // mA
#define TX 21.0 // mA
#define Radiooff 0.0051 // mA Not sure if this is correct
#define CPU_NORMAL 1.8 // mA
#define CPU_SLEEP 0.0051 // mA
#define CPU_DEEP_SLEEP 0.0051 // mA Not sure if this is correct

uint32_t to_seconds(uint32_t time)
{
  return (uint32_t)(time/ ENERGEST_SECOND);
}

void logging(float value) {
    int A = (uint32_t)value; // Get the integer part of the float value
    LOG_INFO("Total power usage B = %u.%04umAh |\n", A, (unsigned int)((value-A)*1e4)); // Print it
}

float TotalPowerConsumption() {
  float power = 0;
  energest_flush(); // Update all energest times. Should always be called before energest times are read.
  power += (to_seconds(energest_type_time(ENERGEST_TYPE_CPU)))*CPU_NORMAL;
  power += (to_seconds(energest_type_time(ENERGEST_TYPE_LPM)))*CPU_SLEEP;
  power += (to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)))*TX;
  power += (to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)))*RX;
  return (power);
}


static struct simple_udp_connection udp_conn;
static struct simple_udp_connection udp_connC;
PROCESS(udp_log_process, "UDP log");
PROCESS(udp_relay_process, "UDP relay");
AUTOSTART_PROCESSES(&udp_relay_process, &udp_log_process);

static void udp_rx_callback(struct simple_udp_connection *c,
              const uip_ipaddr_t *sender_addr,
              uint16_t sender_port,
              const uip_ipaddr_t *receiver_addr,
              uint16_t receiver_port,
              const uint8_t *data,
              uint16_t datalen)
{
 // Check the received message type
 
 
 if (strncmp((char *)data, "dataReq", 7) == 0) {
  LOG_INFO("Sending 'dataReq' to Mote C \n");
  // Set the IPv6 address of Mote C
  uip_ipaddr_t dest_ipaddr_C;
  uip_ip6addr(&dest_ipaddr_C, 0xfd00, 0, 0, 0, 0x0212, 0x7403, 0x0003, 0x0303);

  static char datat[] = "dataReq";
  LOG_INFO((char *)data);
  simple_udp_sendto(&udp_connC, datat, datalen, &dest_ipaddr_C);
  LOG_INFO("Efter sendto \n");
 } else {
  LOG_INFO("Sending 'dataExample' to Mote A \n");
  // Set the IPv6 address of Mote A
  uip_ipaddr_t dest_ipaddr_A;
  uip_ip6addr(&dest_ipaddr_A, 0xfd00, 0, 0, 0, 0x0212, 0x7401, 0x0001, 0x0101);

  static char data[] = "dataExample";
  simple_udp_sendto(&udp_conn, data, datalen, &dest_ipaddr_A);
 
 }
}

PROCESS_THREAD(udp_relay_process, ev, data)
{
 static int counter = 0;
 PROCESS_BEGIN();
 float states_power  = 0.0;
 // Initialize UDP connection
 simple_udp_register(&udp_conn, UDP_PORT_B, NULL,
           UDP_PORT_A, udp_rx_callback);
 simple_udp_register(&udp_connC, UDP_PORT_B, NULL,
           UDP_PORT_C, udp_rx_callback);     

 while (1) {

  PROCESS_WAIT_EVENT();

 }

 PROCESS_END();
}

PROCESS_THREAD(udp_log_process, ev, data)
{
    static struct etimer timeoutTimer;
 static int counter = 0;
 PROCESS_BEGIN();
 etimer_set(&timeoutTimer, CLOCK_SECOND);
 float states_power  = 0.0;


 while (1) {

  logging(TotalPowerConsumption());
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timeoutTimer));
  etimer_reset(&timeoutTimer);

 }

 PROCESS_END();
}