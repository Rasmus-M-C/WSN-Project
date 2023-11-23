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

PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen)
{
  // Check the received message type
  //if (strncmp((char *)data, "dataReq", 7) == 0) {
  //  // Respond with "exampleData" for a dataReq message
  //  char response[] = "exampleData";
  //  simple_udp_sendto(&udp_conn, response, strlen(response), sender_addr);
  //  printf("Responding to dataReq with 'exampleData'\n");
  //} else if (strncmp((char *)data, "health", 6) == 0) {
    // Respond with "ACK" for a health message
    if(strncmp((char *)data, "healthcheck", 11) == 0){
      LOG_INFO("Responding with 'ACK'\n");
      char ack[] = "ACK";
      simple_udp_sendto(&udp_conn, ack, strlen(ack), sender_addr);
    }
    else{
      LOG_INFO("Recieved a data request, sending data\n");
      char response[] = "exampleData";
      simple_udp_sendto(&udp_conn, response, strlen(response), sender_addr);
      // Print addr which is the IPv6 address of the sender
      LOG_INFO("Sender IPv6 address: ");
      for (int i = 0; i < 16; i++) {
        LOG_INFO_("%02X", sender_addr->u8[i]);
        if (i < 15) {
            LOG_INFO_(":");
    }
}
LOG_INFO_("\n");

    }
    
    
  //}
}

PROCESS_THREAD(udp_client_process, ev, data)
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
