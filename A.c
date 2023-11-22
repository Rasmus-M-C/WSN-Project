#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

static struct simple_udp_connection udp_conn;
static bool direct_connection_to_C = false;
static void testCconnection()
{
  uip_ipaddr_t dest_ipaddr_C;
  uip_ip6addr(&dest_ipaddr_C, 0xfd00, 0, 0, 0, 0x0212, 0x7403, 0x0003, 0x0303);
  static char str[32];
  
  printf("dest_ipaddr: ");
  for (int i = 0; i < 16; i++)
  {
    printf("%d ", dest_ipaddr_C.u8[i]);
  }
  snprintf(str, sizeof(str), "test");
  simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr_C);
  LOG_INFO("Test connection\n");
}

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  LOG_INFO("Received message tester '%.*s'\n", datalen, (char *) data);
  
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
if (strcmp((char *)data, "ACK from C") == 0) {
    direct_connection_to_C = true;
    LOG_INFO("Direct connection to C established\n");
  }
char data2[] = "Response";
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  static struct etimer periodic_timer;
  
  PROCESS_BEGIN();
  etimer_set(&periodic_timer, CLOCK_SECOND * 10);

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    testCconnection();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/