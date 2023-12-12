#include "contiki.h"
#include <stdio.h>
#include <string.h>
#include "sys/energest.h"
#include "sys/log.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT_A 8765
#define UDP_PORT_C 5678
#define UDP_PORT_B 5679
//static struct PowerConsumptionStates states_power;
#define RX 23.0 // mA
#define TX 21.0 // mA
#define Radiooff 0.0051 // mA Not sure if this is correct
#define CPU_NORMAL 1.8 // mA
#define CPU_SLEEP 0.0051 // mA
#define CPU_DEEP_SLEEP 0.0051 // mA Not sure if this is correct

struct IntDec {
    unsigned long integer;
    unsigned long decimal;
};

struct IntDec Get_Float_Parts(float value) {
    struct IntDec int_dec;

    // Extract the integer part
    int_dec.integer = (unsigned long)value;

    // Extract the fractional part using a different approach
    float fractional = value - int_dec.integer;

    // Convert the fractional part to an integer with desired precision
    int_dec.decimal = (unsigned long)(fractional * 1e7);

    return int_dec;
}

unsigned long to_seconds(uint64_t time)
{
  return (unsigned long)(time/ ENERGEST_SECOND);
}

void logging(float value) {
    struct IntDec int_dec;
    int_dec = Get_Float_Parts(value);
    LOG_INFO("Total power usage B = %10lu.%07lumAh |\n", int_dec.integer, int_dec.decimal);
    //int A = (uint64_t)value; // Get the integer part of the float value
    //LOG_INFO("Total power usage = %u.%04umAh |\n", A, (unsigned int)((value-A)*1e4)); // Print it
}

float TotalPowerConsumption() {
  float power = 0;
  energest_flush(); // Update all energest times. Should always be called before energest times are read.
  power += (to_seconds(energest_type_time(ENERGEST_TYPE_CPU)))*CPU_NORMAL;
  power += (to_seconds(energest_type_time(ENERGEST_TYPE_LPM)))*CPU_SLEEP;
  power += (to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)))*TX;
  //power += (to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)))*RX;
  return (power);
}


static struct simple_udp_connection udp_conn;
static struct simple_udp_connection udp_connC;
PROCESS(udp_log_process, "UDP log");
PROCESS(udp_relay_process, "UDP relay");
AUTOSTART_PROCESSES(&udp_relay_process, &udp_log_process);

void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
    const char *received_message = (const char *)data;
 // Check the received message type
 
 
 if (strncmp(received_message, "dataReq", len) == 0) {
  LOG_INFO("Sending 'dataReq' to Mote C \n");
  // Set the linklayer address of Mote C
  static linkaddr_t C_addr = {{ 0x03, 0x03, 0x03, 0x00, 0x03, 0x74, 0x12, 0x00 }};

  static char msg[] = "dataReq";
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = strlen(msg);
  LOG_INFO((char *)msg);
  NETSTACK_NETWORK.output(&C_addr);
  LOG_INFO("Efter sendto \n");
 } else {
  LOG_INFO("Sending 'dataExample' to Mote A \n");
  // Set the linklayer address of Mote A
  static linkaddr_t A_addr = {{ 0x01, 0x01, 0x01, 0x00, 0x01, 0x74, 0x12, 0x00 }};

  static char datamsg[] = "dataExample";
  nullnet_buf = (uint8_t *)datamsg;
  nullnet_len = strlen(datamsg);
  NETSTACK_NETWORK.output(&A_addr);
 
 }
}

PROCESS_THREAD(udp_relay_process, ev, data)
{
 static int counter = 0;
 PROCESS_BEGIN();
 float states_power  = 0.0;
 // Initialize nullnet connection
 nullnet_set_input_callback(input_callback);   

 while (1) {

  PROCESS_WAIT_EVENT();

 }

 PROCESS_END();
}

PROCESS_THREAD(udp_log_process, ev, data)
{
    static struct etimer timeoutTimer;
 static int counter = 0;
 PROCESS_BEGIN();
 etimer_set(&timeoutTimer, CLOCK_SECOND);
 float states_power  = 0.0;


 while (1) {

  logging(TotalPowerConsumption());
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timeoutTimer));
  etimer_reset(&timeoutTimer);

 }

 PROCESS_END();
}