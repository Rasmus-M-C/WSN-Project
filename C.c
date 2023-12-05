#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT_A 8765
#define UDP_PORT_C 5678
#define UDP_PORT_B 5679
static struct simple_udp_connection udp_conn;
static struct simple_udp_connection udp_connB;

// State
#define BAD_SUCCES 95
static clock_time_t t1 = 0;
static int state = 100;

PROCESS(udp_client_process, "UDP client");
PROCESS(updateState, "state");
AUTOSTART_PROCESSES(&udp_client_process, &updateState);

/*---------------------------------------------------------------------------*/
static int getState(int currentState){
  int good = 100;
  int mix = 50;
  int bad = 0;
  int newState = currentState;

  if (currentState != good)
  {
    if((unsigned) rand() % 100 > 90) //if(clock_time() - t1 > 2 * CLOCK_SECOND)
    {
      
      //LOG_INFO("%d\n", newState);
      newState = good;
    }
  } else 
  {
    int r = (unsigned) rand() % 100;
    if (r < 10) 
    {
      newState = bad;
      
      //LOG_INFO("%d\n", newState);
      t1 = clock_time();
    } else if (r < 20) 
    {
      newState = mix;
      //LOG_INFO("%d\n", newState);
      t1 = clock_time();
    } 
  }

  return newState;

}

/*---------------------------------------------------------------------------*/


static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen)
{

  uip_ipaddr_t dest_ipaddr_A;
  uip_ip6addr(&dest_ipaddr_A, 0xfd00, 0, 0, 0, 0x0212, 0x7401, 0x0001, 0x0101);
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
    else {
      LOG_INFO("state: %d\n", state);
      if (uip_ipaddr_cmp(sender_addr, &dest_ipaddr_A)){ // Sender address must be A.
      //LOG_INFO("state: %d\n", state);

        if ((unsigned) rand() % 100 >= state){
          //lost packet
          LOG_INFO("Lost packet from A\n");
        } else {
          LOG_INFO("Recieved a data request, sending data\n");
          char response[] = "dataExample";
          simple_udp_sendto(&udp_conn, response, strlen(response), sender_addr);
          LOG_INFO("Sent data to A\n");
        }
      }
      else {
        LOG_INFO("Recieved a data request from B, sending data\n");
        
        char response[] = "dataExample";
        simple_udp_sendto(&udp_connB, response, strlen(response), sender_addr);
        LOG_INFO("Sent data to B\n");
      }
      // Print addr which is the IPv6 address of the sender
      //LOG_INFO("A IPv6 address: ");
    //   for (int i = 0; i < 16; i++) {
    //     LOG_INFO_("%02X", dest_ipaddr_A->u8[i]);
    //     if (i < 15) {
    //         LOG_INFO_(":");
    // }
}
LOG_INFO_("\n");

}
    
    

PROCESS_THREAD(udp_client_process, ev, data)
{
  static int counter = 0;
  srand(10);

  
  PROCESS_BEGIN();

  // Initialize UDP connection
  simple_udp_register(&udp_conn, UDP_PORT_C, NULL,
                      UDP_PORT_A, udp_rx_callback);
  simple_udp_register(&udp_connB, UDP_PORT_C, NULL,
                      UDP_PORT_B, udp_rx_callback);

  while (1) {
    PROCESS_WAIT_EVENT();
    
  }

  PROCESS_END();
}

PROCESS_THREAD(updateState, ev, data)
{
  static struct etimer stateTimer;

  PROCESS_BEGIN();
  etimer_set(&stateTimer, CLOCK_SECOND);

  while(1)
  {
    state = getState(state);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&stateTimer));
    etimer_reset(&stateTimer);
  }

  PROCESS_END();
}
