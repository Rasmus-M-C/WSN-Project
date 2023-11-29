#include "contiki.h"
#include "contiki-net.h"
#include "simple-udp.h"

#include <stdio.h>
#include <string.h>

#define SERVER_IP "fd00::1"  // Replace with the actual IP of the server (Mote C)
#define SERVER_PORT 12345
#define CLIENT_PORT 12346
#define MESSAGE_INTERVAL (CLOCK_SECOND * 5)
#define ACK_WAIT_INTERVAL (CLOCK_SECOND * 2)

static struct simple_udp_connection udp_conn;
enum ConnectionMode { DIRECT, RELAY };
static enum ConnectionMode current_mode = DIRECT;

static int message_count = 0;
static int ack_received = 0;  // Global variable to track ACK reception

PROCESS(client_process, "UDP Client Process");
AUTOSTART_PROCESSES(&client_process);

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) {
    printf("Client (A) Received from server: %s\n", (char *)data);
    if (strncmp((char *)data, "ACK", 3) == 0) {
        ack_received = 1;  // Set flag when ACK is received
        if (current_mode == RELAY) {
            current_mode = DIRECT; // Switch back to direct mode on receiving an ACK
        }
    }
}

PROCESS_THREAD(client_process, ev, data) {
    static struct etimer message_timer, ack_timer;
    uip_ip6addr_t server_ipaddr;

    PROCESS_BEGIN();

    uiplib_ip6addrconv(SERVER_IP, &server_ipaddr);
    simple_udp_register(&udp_conn, CLIENT_PORT, NULL,
                        SERVER_PORT, udp_rx_callback);

    etimer_set(&message_timer, MESSAGE_INTERVAL);

    while(1) {
        PROCESS_WAIT_EVENT();

        if (ev == PROCESS_EVENT_TIMER && data == &message_timer) {
            ack_received = 0;  // Reset ACK received flag before sending a new message

            const char *msg = current_mode == DIRECT ? "Message from A" : "Probe from A";
            printf("Client (A) Sending: %s\n", msg);
            simple_udp_sendto(&udp_conn, msg, strlen(msg) + 1, &server_ipaddr);

            etimer_set(&ack_timer, ACK_WAIT_INTERVAL);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ack_timer));

            if (!ack_received) {
                printf("Client (A) No ACK Received, Switching to Relay Mode\n");
                current_mode = RELAY;
            }
            message_count++;
            etimer_reset(&message_timer);
        }
    }

    PROCESS_END();
}
