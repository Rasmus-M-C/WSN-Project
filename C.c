#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define SEND_INTERVAL (10 * CLOCK_SECOND)
#define MAX_RETRIES 1 // Maximum number of retransmission attempts

static struct simple_udp_connection udp_conn;
static uint32_t rx_count = 0;
static uint32_t tx_count = 0;
static uint32_t missed_tx_count = 0;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/

static struct {
  uint32_t sequence_number;
  char message[32];
  uint8_t retries;
  clock_time_t last_sent_time;
} message_buffer;

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen)
{
  
  LOG_INFO("Received data: '%.*s', length: %d\n", datalen, (char *)data, datalen);
  if (strncmp((char *)data, "HealthCheck", 10) == 0) {
    // Send an acknowledgment back to A
    char ack[] = "ACK from C";
    
    simple_udp_sendto(&udp_conn, ack, strlen(ack), sender_addr);
    LOG_INFO("Sending ACK to A\n");
  }
  LOG_INFO("Received response '%.*s' from ", datalen, (char *)data);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
  
  char dataExample[] = "dataExample";
  simple_udp_sendto(&udp_conn, dataExample, strlen(dataExample), sender_addr);
  LOG_INFO_("Sent data to requester: ");
  LOG_INFO_6ADDR(sender_addr);
  
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;
  clock_time_t current_time;
  uip_ipaddr_t ipaddr;
  PROCESS_BEGIN();
  uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0x0212, 0x7403, 0x0003, 0x0303);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  message_buffer.sequence_number = 0;
  message_buffer.retries = 0;

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    LOG_INFO(NETSTACK_ROUTING.node_is_reachable() ? "Routing node" : "Not routing node");
    LOG_INFO(NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr) ? "Root reachable" : "Root not reachable");
    if (NETSTACK_ROUTING.node_is_reachable() &&
        NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr))
    {
      /* Print statistics every 10th TX */
      if (tx_count % 10 == 0)
      {
        LOG_INFO("Tx/Rx/MissedTx: %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
                 tx_count, rx_count, missed_tx_count);
      }

      /* Send to DAG root */
      LOG_INFO("Sending request %" PRIu32 " to ", tx_count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");

      // Increment the sequence number and prepare the message
      message_buffer.sequence_number++;
      snprintf(message_buffer.message, sizeof(message_buffer.message), "hello %" PRIu32 "", message_buffer.sequence_number);

      // Send the message
      simple_udp_sendto(&udp_conn, message_buffer.message, strlen(message_buffer.message), &dest_ipaddr);
      tx_count++;

      // Set the last sent time for the message
      message_buffer.last_sent_time = clock_time();

      // Reset retries for a new message
      message_buffer.retries = 0;
    }
    else
    {
      LOG_INFO("Not reachable yet\n");

      if (message_buffer.sequence_number > 0)
      {
        // Check if the message has reached the maximum number of retries
        if (message_buffer.retries < MAX_RETRIES)
        {
          // Check if it's time to retransmit the message
          current_time = clock_time();

          if (current_time - message_buffer.last_sent_time >= SEND_INTERVAL)
          {
            // Retransmit the message
            LOG_INFO("Retransmitting request %" PRIu32 " to ", message_buffer.sequence_number);
            LOG_INFO_6ADDR(&dest_ipaddr);
            LOG_INFO_("\n");

            // Send the message
            simple_udp_sendto(&udp_conn, message_buffer.message, strlen(message_buffer.message), &dest_ipaddr);
            tx_count++;

            // Set the last sent time for the message
            message_buffer.last_sent_time = current_time;

            // Increment the retry count
            message_buffer.retries++;
          }
        }
        else
        {
          // Message has reached the maximum number of retries
          LOG_INFO("Message %" PRIu32 " has reached the maximum number of retries\n", message_buffer.sequence_number);
          missed_tx_count++;
          message_buffer.sequence_number = 0; // Reset sequence number for a new message
        }
      }
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
