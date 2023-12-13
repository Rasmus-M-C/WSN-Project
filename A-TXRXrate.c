#include "contiki.h"
#include "net/routing/routing.h"
#include <stdio.h>
#include <string.h>
#include "sys/energest.h"

// #include "PowerConsumption.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

#include "sys/log.h"
#define LOG_MODULE "A"
#define LOG_LEVEL LOG_LEVEL_INFO

// get a bit from a variable
#define GETBIT(var, bit) (((var) >> (bit)) & 1)

// set a bit to 1
#define SETBIT(var, bit) var |= (1 << (bit))

// set a bit to 0
#define CLRBIT(var, bit) var &= (~(1 << (bit)))

static u_int16_t list = 0b0101010101010101;
static u_int8_t TX_count = 0;
static u_int8_t RX_count = 0;

static linkaddr_t C_addr = {{0x03, 0x03, 0x03, 0x00, 0x03, 0x74, 0x12, 0x00}};
static linkaddr_t B_addr = {{0x02, 0x02, 0x02, 0x00, 0x02, 0x74, 0x12, 0x00}};

static clock_time_t timeout = 0;
static int counter = 0;

// static struct PowerConsumptionStates states_power;
#define RX 23.0               // mA
#define TX 21.0               // mA
#define Radiooff 0.0051       // mA Not sure if this is correct
#define CPU_NORMAL 1.8        // mA
#define CPU_SLEEP 0.0051      // mA
#define CPU_DEEP_SLEEP 0.0051 // mA Not sure if this is correct

struct IntDec
{
  unsigned long integer;
  unsigned long decimal;
};

struct IntDec Get_Float_Parts(float value)
{
  struct IntDec int_dec;

  // Extract the integer part
  int_dec.integer = (unsigned long)value;

  // Extract the fractional part using a different approach
  float fractional = value - int_dec.integer;

  // Convert the fractional part to an integer with desired precision
  int_dec.decimal = (unsigned long)(fractional * 1e7);

  return int_dec;
}

void logging(float value)
{
  struct IntDec int_dec;
  int_dec = Get_Float_Parts(value);
  LOG_INFO("Total power usage A = %10lu.%07lumAh |\n", int_dec.integer, int_dec.decimal);
  // int A = (uint64_t)value; // Get the integer part of the float value
  // LOG_INFO("Total power usage = %u.%04umAh |\n", A, (unsigned int)((value-A)*1e4)); // Print it
}

float TotalPowerConsumption()
{
  float power = 0;
  energest_flush(); // Update all energest times. Should always be called before energest times are read.
  power += (energest_type_time(ENERGEST_TYPE_CPU)) * CPU_NORMAL;
  power += (energest_type_time(ENERGEST_TYPE_LPM)) * CPU_SLEEP;
  power += (energest_type_time(ENERGEST_TYPE_TRANSMIT)) * TX;
  //power += (energest_type_time(ENERGEST_TYPE_LISTEN)) * RX;
  return (power);
}

// Define the IPv6 address of mote C
PROCESS(udp_server_process, "UDP server");
PROCESS(udp_network_process, "UDP network start");
PROCESS(checkTimeout, "TimeoutChecker");
AUTOSTART_PROCESSES(&udp_server_process, &udp_network_process, &checkTimeout);

void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
  //(uip_ipaddr_cmp(sender_addr, &dest_ipaddr_C))
  // uip_ip6addr(&dest_ipaddr_C, 0xfd00, 0, 0, 0, 0x0212, 0x7403, 0x0003, 0x0303);
  const char *received_message = (const char *)data;

  if (linkaddr_cmp(src, &C_addr))
  {
    // Add 0 to list
    list = list << 1;
    timeout = 0;
  }
  // Check the received response from mote C
  if (strncmp(received_message, "ACK", len) == 0)
  {
    // Received an acknowledgment for healthcheck
    // LOG_INFO("Received acknowledgment for healthcheck\n");
  }
  else if (strncmp(received_message, "dataExample", len) == 0)
  {
    // Received data response
    // LOG_INFO("Received data from IPADDR:");
    // log_6addr(sender_addr);
    // LOG_INFO("Received data response: '%.*s'\n", datalen, (char *) data);
    // stop timeout ------------------------------------------------------------
  }
  else
  {
    // LOG_INFO("Received unknown message\n");
    //  Handle unknown messages as needed
  }

  // You can add more checks for different response types as needed.
}

PROCESS_THREAD(udp_network_process, ev, data)
{
  PROCESS_BEGIN();
  // Start the network
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize Nullnet connection */
  nullnet_set_input_callback(input_callback);
  // LOG_INFO("Network started\n");
  PROCESS_END();
}

PROCESS_THREAD(udp_server_process, ev, data)
{
  static struct etimer periodic_timer;
  static u_int32_t ratio = 1024;
  PROCESS_BEGIN();

  etimer_set(&periodic_timer, CLOCK_SECOND * 1);

  while (1)
  {
    // Read list
    u_int8_t tx_counter = 0;
    u_int8_t rx_counter = 0;
    for (int i = 15; i >= 0; i--)
    {
      tx_counter += GETBIT(list, i) == 1;
    }
    rx_counter = 16 - tx_counter;
    LOG_INFO("TX: %d, RX: %d\n", tx_counter, rx_counter);

    if (tx_counter != 0)
    {
      ratio = (rx_counter << 10) / tx_counter;
    }

    // ratio = (RX_count*1e2)/(TX_count);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    // LOG_INFO("Sending message %d\n", counter);
    // Every 15th message, send to B
    LOG_INFO("Ratio: %d\n", ratio);

    if (ratio < (1024 * 0.5))
    {
      // LOG_INFO("Sending message B\n");
      static char msg[128] = "dataReq";
      nullnet_buf = (uint8_t *)msg;
      nullnet_len = strlen(msg);
      // LOG_INFO((char *)msg);
      LOG_INFO("%c\n", msg);
      LOG_INFO("Sending B\n");
      NETSTACK_NETWORK.output(&B_addr);
      timeout = 0;

      if (counter % 10 == 0)
      { // Every 10th message, send healthcheck
        // LOG_INFO("Sending healthcheck message\n");
        //  Send a healthcheck message
        // Add 1 to list
        list = list << 1;
        SETBIT(list, 0);
        static char msg[] = "healthcheck";
        nullnet_buf = (uint8_t *)msg;
        nullnet_len = strlen(msg);
        LOG_INFO((char *)msg);
        LOG_INFO("Sending C\n");
        NETSTACK_NETWORK.output(&C_addr);
        counter++;
      }
    }
    else
    {
      // LOG_INFO("Sending dataReq message\n");
      //  Send a dataReq message to Mote C
      // Add 1 to list
      list = list << 1;
      SETBIT(list, 0);
      static char msg[] = "dataReq";
      nullnet_buf = (uint8_t *)msg;
      nullnet_len = strlen(msg);
      LOG_INFO("%c\n", msg);
      LOG_INFO("Sending C\n");
      NETSTACK_NETWORK.output(&C_addr);
      timeout = clock_time();
    }
    counter++;
    // LOG_INFO("TX: %d, RX: %d\n", TX_count, RX_count);
    LOG_INFO("%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d\n", GETBIT(list, 15), GETBIT(list, 14), GETBIT(list, 13), GETBIT(list, 12), GETBIT(list, 11), GETBIT(list, 10), GETBIT(list, 9), GETBIT(list, 8), GETBIT(list, 7), GETBIT(list, 6), GETBIT(list, 5), GETBIT(list, 4), GETBIT(list, 3), GETBIT(list, 2), GETBIT(list, 1), GETBIT(list, 0));
  }

  PROCESS_END();
}

PROCESS_THREAD(checkTimeout, ev, data)
{
  static struct etimer timeoutTimer;

  PROCESS_BEGIN();
  etimer_set(&timeoutTimer, CLOCK_SECOND);
  // Power Consumption
  static struct etimer periodic_timer;
  // struct IntDec int_dec;
  float states_power = 0.0;

  while (1)
  {

    if (clock_time() > timeout + CLOCK_SECOND && timeout != 0)
    {
      // LOG_INFO("Resending package %d\n", counter);
      static char msg[] = "dataReq";
      nullnet_buf = (uint8_t *)msg;
      nullnet_len = strlen(msg);
      LOG_INFO((char *)msg);
      LOG_INFO("Sending C\n");
      NETSTACK_NETWORK.output(&C_addr);
      timeout = clock_time();
    }

    logging(TotalPowerConsumption());
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timeoutTimer));
    etimer_reset(&timeoutTimer);
  }

  PROCESS_END();
}