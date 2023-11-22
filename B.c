#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>
#include "sys/log.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT_A 8765 // UDP client port for Node A
#define UDP_SERVER_PORT_B 9876 // UDP server port for Node B
#define UDP_SERVER_PORT_C 5678 // UDP server port for Node C

#define SEND_INTERVAL (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
static struct simple_udp_connection udp_conn_b;
static uint32_t rx_count = 0;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
PROCESS(udp_receiver_process, "UDP receiver for Node B");
AUTOSTART_PROCESSES(&udp_client_process, &udp_receiver_process);
/*---------------------------------------------------------------------------*/
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen)
{
    // This callback is for handling responses from Node C (you can keep it as-is)
    LOG_INFO("Received response '%.*s' from ", datalen, (char *)data);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
    rx_count++;
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
    static struct etimer periodic_timer;
    static char str[32];
    uip_ipaddr_t dest_ipaddr_c;
    static uint32_t tx_count;
    static uint32_t missed_tx_count;

    PROCESS_BEGIN();

    /* Initialize UDP connection for Node A */
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT_A, NULL,
                        UDP_SERVER_PORT_C, udp_rx_callback);

    etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        if (NETSTACK_ROUTING.node_is_reachable() &&
            NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr_c))
        {

            /* Print statistics every 10th TX */
            if (tx_count % 10 == 0)
            {
                LOG_INFO("Tx/Rx/MissedTx: %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
                         tx_count, rx_count, missed_tx_count);
            }

            /* Send to Node C (DAG root) directly */
            LOG_INFO("Sending request %" PRIu32 " to Node C (directly)\n", tx_count);
            snprintf(str, sizeof(str), "hello %" PRIu32 "", tx_count);
            simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr_c);
            tx_count++;
        }
        else
        {
            LOG_INFO("Not reachable yet\n");
            if (tx_count > 0)
            {
                missed_tx_count++;
            }
        }

        /* Add some jitter */
        etimer_set(&periodic_timer, SEND_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
    }

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_receiver_process, ev, data)
{
    static char received_message[32];
    static uip_ipaddr_t dest_ipaddr_c;

    PROCESS_BEGIN();

    /* Initialize UDP connection for Node B (as a receiver) */
    simple_udp_register(&udp_conn_b, UDP_SERVER_PORT_B, NULL,
                        UDP_SERVER_PORT_C, NULL);

    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_MSG);

        // Message received from Node A; forward it to Node C
        
        
            // Extract the message from the received data
            strncpy(received_message, (char *)data, sizeof(received_message));

            // Forward the message to Node C
            simple_udp_sendto(&udp_conn, received_message, strlen(received_message), &dest_ipaddr_c);

            LOG_INFO("Forwarded message to Node C: %s\n", received_message);
        
    
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/