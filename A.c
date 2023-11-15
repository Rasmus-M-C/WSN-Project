#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include <stdio.h>
#include "simple-udp.h"
#include "net/ipv6/simple-udp.h"
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

static struct simple_udp_connection udp_conn;

PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) {
    printf("Received response from server: '%.*s'\n", datalen, (char *)data);
}

PROCESS_THREAD(udp_client_process, ev, data) {
    static struct etimer periodic_timer;
    uip_ipaddr_t dest_ipaddr;

    PROCESS_BEGIN();

    // Set the IP address for Mote C (Server)
    uip_ip6addr(&dest_ipaddr, 0xfe80, 0, 0, 0, 0x0212, 0x7403, 0x0003, 0x0303);
    
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
    etimer_set(&periodic_timer, CLOCK_SECOND * 2);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        char message[] = "Hello from A";
        simple_udp_sendto(&udp_conn, message, strlen(message), &dest_ipaddr);
        printf("Sent message to Mote C: %s\n", message);

        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}
