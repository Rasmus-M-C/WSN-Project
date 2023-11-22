#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>

#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define SEND_INTERVAL (10 * CLOCK_SECOND)
#define MAX_RETRIES 1 // Maximum number of retransmission attempts
static bool badStateEnabled = true; // You can set this to false to disable the "bad state"

// Bad state
#define BAD_SUCCES 95
static int counter = 0;
static clock_time_t t1 = 0;

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

// udp_rx_callback is called when a UDP packet is received.
// Parameters:
// - c: A pointer to the simple_udp_connection structure, which represents the UDP connection.
// - sender_addr: The IP address of the sender.
// - sender_port: The port number used by the sender.
// - receiver_addr: The IP address of the receiver.
// - receiver_port: The port number used by the receiver.
// - data: A pointer to the received data.
// - datalen: The length of the received data.
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen)
{
  int result = strncmp((char *)data, "test", 4);
  LOG_INFO("result: %d\n", result);
  LOG_INFO("Received data: '%.*s', length: %d\n", datalen, (char *)data, datalen);
  
  if (badStateEnabled && clock_time() - t1 > 2*CLOCK_SECOND) {
    LOG_INFO("Bad state\n");
    t1 = clock_time();
  }
  else if ( rand() % 100 < BAD_SUCCES) {
    LOG_INFO("Lost: %s\n", (char *)data);
  }
  else {
    LOG_INFO("Good state\n");
    if (strncmp((char *)data, "test", 4) == 0) {
    // Send an acknowledgment back to A
    char ack[] = "ACK from C";
    
    simple_udp_sendto(&udp_conn, ack, strlen(ack), sender_addr);
    LOG_INFO("Sending ACK to A\n");
    }
  }
  LOG_INFO("Received response '%.*s' from ", datalen, (char *)data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
  rx_count++;
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;
  clock_time_t current_time;

  PROCESS_BEGIN();
  t1 = clock_time();
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);
  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  message_buffer.sequence_number = 0;
  message_buffer.retries = 0;

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
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
