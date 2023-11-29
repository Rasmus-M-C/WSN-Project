
#include <stdio.h>
#include "contiki.h"
#include "sys/energest.h"
#include "sys/log.h"
#include "PowerConsumption.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(energest_example_process, "energest example process");
AUTOSTART_PROCESSES(&energest_example_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(energest_example_process, ev, data)
{
  static struct etimer periodic_timer;
  struct IntDec int_dec;
  struct PowerConsumptionStates states;
  PROCESS_BEGIN();

  etimer_set(&periodic_timer, CLOCK_SECOND * 30);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    states = TotalPowerConsumption();
    //LOGPowerConsumption(states);
    int_dec = Get_Float_Parts(states.Total_usage);
    LOG_INFO("Test total = %10lu.%07lumAh |\n", int_dec.integer, int_dec.decimal);
  PROCESS_END();
  }
}
/*---------------------------------------------------------------------------*/