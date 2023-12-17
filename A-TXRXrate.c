#include "contiki.h"
#include "net/routing/routing.h"
#include <stdio.h>
#include <string.h>
#include "sys/energest.h"
#include "cc2420.h"

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

static u_int16_t list = 0b1010101010101010;
static u_int8_t TX_count = 0;
static u_int8_t RX_count = 0;

static linkaddr_t C_addr = {{0x03, 0x03, 0x03, 0x00, 0x03, 0x74, 0x12, 0x00}};
static linkaddr_t B_addr = {{0x02, 0x02, 0x02, 0x00, 0x02, 0x74, 0x12, 0x00}};

static int timeout = -1;
static int counter = 0;
static int packageSent = 0;
static int ackReceived = 0;

#define THRESHOLD(x) (x * 0.125 + 0.0625)

// static struct PowerConsumptionStates states_power;
#define RX 23.0               // mA
#define TX 21.0               // mA
#define Radiooff 2.4      // mA Not sure if this is correct
#define CPU_NORMAL 1.8        // mA
#define CPU_SLEEP 0.0051      // mA

static char msg[128] = "packet";

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
  // power += (energest_type_time(ENERGEST_TYPE_LISTEN)) * RX;
  return (power);
}

// Define the IPv6 address of mote C
PROCESS(null_net_server, "null_net server");
PROCESS(null_net_network, "null_net network start");
PROCESS(checkTimeout, "TimeoutChecker");
AUTOSTART_PROCESSES(&null_net_server, &null_net_network, &checkTimeout);

void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
  const char *received_message = (const char *)data;
  if (linkaddr_cmp(src, &C_addr) || linkaddr_cmp(src, &B_addr))
  {
    if (strncmp(received_message, "ACK", len) == 0)
    {
      LOG_INFO("got ACK\n");
      ackReceived++;
    }
    // if (strncmp(received_message, "ACKHT", len) == 0)
    // {
    //   ackReceived--;
    // }
    if (linkaddr_cmp(src, &C_addr))
    {
      // Add 0 to list
      SETBIT(list, 0);
      timeout = -1;
    }
  }
  // Check the received response from mote C
}

PROCESS_THREAD(null_net_network, ev, data)
{
  PROCESS_BEGIN();

  cc2420_set_txpower(3);
  // Start the network
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize Nullnet connection */
  nullnet_set_input_callback(input_callback);
  // LOG_INFO("Network started\n");
  PROCESS_END();
}

PROCESS_THREAD(null_net_server, ev, data)
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
    for (int i = 15; i >= 0; i -= 2)
    {
      tx_counter += GETBIT(list, i) == 1;
      rx_counter += GETBIT(list, i - 1) == 1;
    }
    LOG_INFO("TX: %d, RX: %d\n", tx_counter, rx_counter);
    LOG_INFO("Sent: %d, Received: %d\n", packageSent, ackReceived);

    if (tx_counter != 0)
    {
      ratio = (rx_counter << 10) / tx_counter;
    }
    LOG_INFO("Ratio: %d\n", ratio);

    // LOG_INFO("Sending message %d\n", counter);
    // Every 15th message, send to B

    if (ratio < (1024 * THRESHOLD(7)))
    {
      // LOG_INFO("Sending message B\n");
      packageSent++;
      nullnet_buf = (uint8_t *)msg;
      nullnet_len = strlen(msg);
      // LOG_INFO((char *)msg);
      LOG_INFO("%c\n", msg);
      LOG_INFO("Sending B\n");
      NETSTACK_NETWORK.output(&B_addr);
      timeout = -1;

      if (counter % 10 == 0)
      { // Every 10th message, send healthcheck
        // LOG_INFO("Sending healthcheck message\n");
        //  Send a healthcheck message
        // packageSent++;
        // Add 1 to list
        list = list << 2;
        SETBIT(list, 1);
        CLRBIT(list, 0);
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
      packageSent++;
      // Add 1 to list
      list = list << 2;
      SETBIT(list, 1);
      CLRBIT(list, 0);
      nullnet_buf = (uint8_t *)msg;
      nullnet_len = strlen(msg);
      LOG_INFO("%c\n", msg);
      LOG_INFO("Sending C\n");
      NETSTACK_NETWORK.output(&C_addr);
      timeout = 5;
    }
    counter++;
    // LOG_INFO("TX: %d, RX: %d\n", TX_count, RX_count);
    logging(TotalPowerConsumption());
    //LOG_INFO("%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d\n", GETBIT(list, 15), GETBIT(list, 14), GETBIT(list, 13), GETBIT(list, 12), GETBIT(list, 11), GETBIT(list, 10), GETBIT(list, 9), GETBIT(list, 8), GETBIT(list, 7), GETBIT(list, 6), GETBIT(list, 5), GETBIT(list, 4), GETBIT(list, 3), GETBIT(list, 2), GETBIT(list, 1), GETBIT(list, 0));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}

PROCESS_THREAD(checkTimeout, ev, data)
{
  static struct etimer timeoutTimer;

  PROCESS_BEGIN();
  etimer_set(&timeoutTimer, CLOCK_SECOND * 0.05);

  while (1)
  {

    
    if (timeout != -1){
      timeout--;
    }
    //LOG_INFO("timeout: %d \n", timeout);
    if (timeout == 0)
    {
      packageSent++;
      // Add 1 to list
      list = list << 2;
      SETBIT(list, 1);
      CLRBIT(list, 0);
      nullnet_buf = (uint8_t *)msg;
      nullnet_len = strlen(msg);
      LOG_INFO((char *)msg);
      LOG_INFO("Sending C\n");
      LOG_INFO("Resend C\n");
      NETSTACK_NETWORK.output(&C_addr);
      timeout = 5;
    }

    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timeoutTimer));
    etimer_reset(&timeoutTimer);
  }

  PROCESS_END();
}
