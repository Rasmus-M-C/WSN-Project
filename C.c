#include "contiki.h"
#include <stdio.h>
#include <string.h>
#include "net/nullnet/nullnet.h"
#include "net/netstack.h"
#include <stdlib.h>
#include "sys/energest.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT_A 8765
#define UDP_PORT_C 5678
#define UDP_PORT_B 5679

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
    LOG_INFO("Total power usage C = %10lu.%07lumAh |\n", int_dec.integer, int_dec.decimal);
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

static linkaddr_t A_addr = {{ 0x01, 0x01, 0x01, 0x00, 0x01, 0x74, 0x12, 0x00 }};
static linkaddr_t B_addr = {{ 0x02, 0x02, 0x02, 0x00, 0x02, 0x74, 0x12, 0x00 }};

// State
#define BAD_SUCCES 95
static clock_time_t t1 = 0;
static int state = 100;

PROCESS(udp_log_process, "UDP log");
PROCESS(udp_client_process, "UDP client");
PROCESS(updateState, "state");
AUTOSTART_PROCESSES(&udp_client_process, &updateState, &udp_log_process);

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


void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{


    const char *received_message = (const char *)data;
    if(strncmp(received_message, "healthcheck", len) == 0){
      if ((unsigned) rand() % 100 >= state){
          //lost packet
          LOG_INFO("Lost packet from A\n");
        } else {
      LOG_INFO("Responding with 'ACK'\n");
  
      // Set the message in NullNet buffer
      char ack[] = "ACK";
      nullnet_buf = (uint8_t *)ack;
      nullnet_len = strlen(ack);
      LOG_INFO_LLADDR(src);
      NETSTACK_NETWORK.output(src);}
    }
    else {
      LOG_INFO("state: %d\n", state);
      if (linkaddr_cmp(src, &A_addr)){ // Sender address must be A.
      //LOG_INFO("state: %d\n", state);

        if ((unsigned) rand() % 100 >= state){
          //lost packet
          LOG_INFO("Lost packet from A\n");
        } else {
          LOG_INFO("Recieved a data request, sending data\n");
          char response[] = "dataExample";
          nullnet_buf = (uint8_t *)response;
          nullnet_len = strlen(response);
          NETSTACK_NETWORK.output(&A_addr);
          LOG_INFO("Sent data to A\n");
        }
      }
      else {
        //LOG_INFO("Recieved a data request from B, sending data\n");
        
        char response[] = "dataExample";
        nullnet_buf = (uint8_t *)response;
        nullnet_len = strlen(response);
        NETSTACK_NETWORK.output(&B_addr);
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
//LOG_INFO_("\n");

}
    
    

PROCESS_THREAD(udp_client_process, ev, data)
{
  static int counter = 0;
  srand(10);

  
  PROCESS_BEGIN();

  // Initialize nullnet callback
  nullnet_set_input_callback(input_callback);
 

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
