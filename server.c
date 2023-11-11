#include "contiki.h"
#include "contiki-net.h"
#include "simple-udp.h"

#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 12345
#define LOSS_PROBABILITY 10 // Probability (in percentage) to simulate message loss

static struct simple_udp_connection udp_conn;

PROCESS(server_process, "UDP Server Process");
AUTOSTART_PROCESSES(&server_process);

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen);

PROCESS_THREAD(server_process, ev, data)
{
  uip_ipaddr_t ipaddr;
  PROCESS_BEGIN();
  uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 1); // fd00::1
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
  simple_udp_register(&udp_conn, SERVER_PORT, NULL,
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
  printf("Server (C) Received: %s from [%u]\n", (char *)data, sender_port);

  // Simulate fading or packet loss for messages from A
  if (strncmp((char *)data, "Message from A", strlen("Message from A")) == 0 ||
      strncmp((char *)data, "Probe from A", strlen("Probe from A")) == 0) {
    if (rand() % 100 < LOSS_PROBABILITY) {
      printf("Server (C) Simulating message loss from A\n");
      return; // Do not respond to simulate loss
    }
  }

  // Respond to messages from B and messages from A that passed the loss simulation
  const char *response = "ACK";
  printf("Server (C) Sending ACK to [%u]\n", sender_port);
  simple_udp_sendto(&udp_conn, response, strlen(response) + 1, sender_addr);
}
