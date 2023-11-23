#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include <stdio.h>
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

static struct simple_udp_connection udp_conn;

PROCESS(udp_relay_process, "UDP relay");
AUTOSTART_PROCESSES(&udp_relay_process);

static void udp_rx_callback(struct simple_udp_connection *c,
              const uip_ipaddr_t *sender_addr,
              uint16_t sender_port,
              const uip_ipaddr_t *receiver_addr,
              uint16_t receiver_port,
              const uint8_t *data,
              uint16_t datalen)
{
 // Check the received message type
 if (strncmp((char *)data, "healthcheck", 11) == 0) {
  LOG_INFO("Responding with 'ACK'\n");
  char ack[] = "ACK";
  simple_udp_sendto(&udp_conn, ack, strlen(ack), sender_addr);
 } else if (strncmp((char *)data, "dataExample", 11) == 0) {
  LOG_INFO("Sending 'dataExample' to Mote A\n");
  // Set the IPv6 address of Mote A
  uip_ipaddr_t dest_ipaddr_A;
  uip_ip6addr(&dest_ipaddr_A, 0xfd00, 0, 0, 0, 0x0212, 0x7401, 0x0001, 0x0101);
  simple_udp_sendto(&udp_conn, data, datalen, &dest_ipaddr_A);
 } else if (strncmp((char *)data, "dataReq", 7) == 0) {
  LOG_INFO("Sending 'dataReq' to Mote C\n");
  // Set the IPv6 address of Mote C
  uip_ipaddr_t dest_ipaddr_C;
  uip_ip6addr(&dest_ipaddr_C, 0xfd00, 0, 0, 0, 0x0212, 0x7403, 0x0003, 0x0303);
  simple_udp_sendto(&udp_conn, data, datalen, &dest_ipaddr_C);
 } else {
  LOG_INFO("Received unknown message\n");
  // Handle unknown messages as needed
 }
}

PROCESS_THREAD(udp_relay_process, ev, data)
{
 static int counter = 0;
 PROCESS_BEGIN();

 // Initialize UDP connection
 simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
           UDP_SERVER_PORT, udp_rx_callback);

 while (1) {
  PROCESS_WAIT_EVENT();
 }

 PROCESS_END();
}