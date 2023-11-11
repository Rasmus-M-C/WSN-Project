#include "contiki.h"
#include "contiki-net.h"

#include "simple-udp.h"

#include <stdio.h>
#include <string.h>

#define CLIENT_PORT 12346
#define SERVER_IP "fd00::1"  // Replace with the actual IP of the server (Mote C)
#define SERVER_PORT 12345

static struct simple_udp_connection udp_conn;

PROCESS(relay_process, "UDP Relay Process");
AUTOSTART_PROCESSES(&relay_process);

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen);

PROCESS_THREAD(relay_process, ev, data)
{
  PROCESS_BEGIN();

  simple_udp_register(&udp_conn, CLIENT_PORT, NULL,
                      SERVER_PORT, udp_rx_callback);

  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) {
  printf("Relay received data from A, forwarding to C\n");

  // Forward the message to the server (Mote C)
  uip_ip6addr_t server_ipaddr;
  uiplib_ip6addrconv(SERVER_IP, &server_ipaddr);
  simple_udp_sendto(&udp_conn, data, datalen, &server_ipaddr);
}
