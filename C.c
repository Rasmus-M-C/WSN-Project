#include "contiki.h"
#include <stdio.h>
#include <string.h>
#include "net/nullnet/nullnet.h"
#include "net/netstack.h"
#include <stdlib.h>
#include "sys/energest.h"
#include "sys/log.h"
#include "cc2420.h"

#define LOG_MODULE "C"
#define LOG_LEVEL LOG_LEVEL_INFO

#define RX 23.0               // mA
#define TX 21.0               // mA
#define Radiooff 0.0051       // mA Not sure if this is correct
#define CPU_NORMAL 1.8        // mA
#define CPU_SLEEP 0.0051      // mA
#define CPU_DEEP_SLEEP 0.0051 // mA Not sure if this is correct

static char ack[] = "ACK";
static char ackHT[] = "ACKHT";

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
  LOG_INFO("Total power usage C = %10lu.%07lumAh |\n", int_dec.integer, int_dec.decimal);
  // int A = (uint64_t)value; // Get the integer part of the float value
  // LOG_INFO("Total power usage = %u.%04umAh |\n", A, (unsigned int)((value-A)*1e4)); // Print it
}

float TotalPowerConsumption()
{
  float power = 0;
  energest_flush(); // Update all energest times. Should always be called before energest times are read.
  power += (energest_type_time(ENERGEST_TYPE_CPU))*CPU_NORMAL;
  power += (energest_type_time(ENERGEST_TYPE_LPM))*CPU_SLEEP;
  power += (energest_type_time(ENERGEST_TYPE_TRANSMIT)) * TX;
  //power += (energest_type_time(ENERGEST_TYPE_LISTEN)) * RX;
  return (power);
}

static linkaddr_t A_addr = {{0x01, 0x01, 0x01, 0x00, 0x01, 0x74, 0x12, 0x00}};
static linkaddr_t B_addr = {{0x02, 0x02, 0x02, 0x00, 0x02, 0x74, 0x12, 0x00}};

// static clock_time_t t1 = 0;
static int state = 100;

PROCESS(power_log_process, "power log");
PROCESS(null_net_client, "null_net client");
PROCESS(updateState, "state");
AUTOSTART_PROCESSES(&null_net_client, &updateState, &power_log_process);

/*---------------------------------------------------------------------------*/
static int getState(int currentState)
{
  int good = 100;
  int bad = 0;
  int newState = currentState;

  int r = (unsigned)rand() % 100;
  if (r > 65)
  {
    newState = good;
  }
  else
  {
    if (r < 10)
    {
      newState = bad;
    }
  }

  return newState;
}

/*---------------------------------------------------------------------------*/

void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{

  const char *received_message = (const char *)data;
  if (strncmp(received_message, "healthcheck", len) == 0)
  {
    LOG_INFO("state: %d\n", state);
    if (state < 50)
    {
      // lost packet
      LOG_INFO("Lost HT from A\n");
    }
    else
    {
      LOG_INFO("Responding with 'ACK'\n");

      // Set the message in NullNet buffer
      
      nullnet_buf = (uint8_t *)ackHT;
      nullnet_len = strlen(ackHT) + 1;
      //LOG_INFO_LLADDR(src);
      NETSTACK_NETWORK.output(&A_addr);
    }
  }
  else
  {
    if (linkaddr_cmp(src, &A_addr))
    { // Sender address must be A.
      // LOG_INFO("state: %d\n", state);
      LOG_INFO("state: %d\n", state);
      if (state < 50)
      {
        // lost packet
        LOG_INFO("Lost packet from A\n");
      }
      else
      {
        LOG_INFO("Recieved a packet, sending ack\n");
        nullnet_buf = (uint8_t *)ack;
        nullnet_len = strlen(ack) + 1;
        NETSTACK_NETWORK.output(&A_addr);
        LOG_INFO("Sent ACK to A\n");
      }
    }
    else
    {
      // LOG_INFO("Recieved a data request from B, sending data\n");
      nullnet_buf = (uint8_t *)ack;
      nullnet_len = strlen(ack) + 1;
      NETSTACK_NETWORK.output(&B_addr);
      LOG_INFO("Sent ACK to B\n");
    }
  }
}

PROCESS_THREAD(null_net_client, ev, data)
{
  cc2420_set_txpower(3);
  static int counter = 0;

  PROCESS_BEGIN();

  // Initialize nullnet callback
  nullnet_set_input_callback(input_callback);

  while (1)
  {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

PROCESS_THREAD(updateState, ev, data)
{
  static struct etimer stateTimer;
  
  PROCESS_BEGIN();
  etimer_set(&stateTimer, CLOCK_SECOND);
  srand(3);

  while (1)
  {
    if ((clock_seconds()/60) % 2 == 0)
    {
      state = 0;
    }
    else
    {
      state = 100;
    }

    //state = getState(state);

    //state = 0;

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&stateTimer));
    etimer_reset(&stateTimer);
  }

  PROCESS_END();
}

PROCESS_THREAD(power_log_process, ev, data)
{
  static struct etimer timeoutTimer;
  static int counter = 0;
  PROCESS_BEGIN();
  etimer_set(&timeoutTimer, CLOCK_SECOND);

  while (1)
  {

    logging(TotalPowerConsumption());
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timeoutTimer));
    etimer_reset(&timeoutTimer);
  }

  PROCESS_END();
}