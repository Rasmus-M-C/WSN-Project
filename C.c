#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include <stdio.h>
#include "simple-udp.h"
#include "net/ipv6/simple-udp.h"
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

static struct simple_udp_connection udp_conn;

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) {
    printf("Received message from Mote A: '%.*s'\n", datalen, (char *)data);

    const char *response = "ACK from C";
    simple_udp_sendto(&udp_conn, response, strlen(response), sender_addr);
    printf("Sent ACK to Mote A\n");
}

PROCESS_THREAD(udp_server_process, ev, data) {
    PROCESS_BEGIN();

    simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_rx_callback);

    PROCESS_END();
}
