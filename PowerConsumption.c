
#include <stdio.h>
#include "contiki.h"
#include "sys/energest.h"
#include "sys/log.h"
#include "PowerConsumption.h"

const float RX = 23.0; // mA
const float TX = 21.0; // mA
const float Radiooff = 0.0051; // mA Not sure if this is correct
const float CPU_NORMAL = 1.8; // mA
const float CPU_SLEEP = 0.0051; // mA
const float CPU_DEEP_SLEEP = 0.0051; // mA Not sure if this is correct

unsigned long to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

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

struct PowerConsumptionStates TotalPowerConsumption() {
  struct PowerConsumptionStates states;
  float hours = 0;
  energest_flush(); // Update all energest times. Should always be called before energest times are read.
  states.CPU_NORMAL_time = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));
  hours = states.CPU_NORMAL_time/3600.000;
  states.CPU_NORMAL_usage = hours*CPU_NORMAL;
  states.Total_usage += states.CPU_NORMAL_usage;

  states.CPU_SLEEP_time = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));
  hours = states.CPU_SLEEP_time/3600.000;
  states.CPU_SLEEP_usage = hours*CPU_SLEEP;
  states.Total_usage += states.CPU_SLEEP_usage;

  states.CPU_DEEP_SLEEP_time = to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM));
  hours = states.CPU_DEEP_SLEEP_time/3600.000;
  states.CPU_DEEP_SLEEP_usage = hours*CPU_DEEP_SLEEP;
  states.Total_usage += states.CPU_DEEP_SLEEP_usage;

  states.TX_time = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
  hours = states.TX_time/3600.000;
  states.TX_usage = hours*TX;
  states.Total_usage += states.TX_usage;

  states.RX_time = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));
  hours = states.RX_time/3600.000;
  states.RX_usage = hours*RX;
  states.Total_usage += states.RX_usage;

  states.Radiooff_time = to_seconds(ENERGEST_GET_TOTAL_TIME() - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
  hours = states.Radiooff_time/3600.000;
  states.Radiooff_usage = hours*Radiooff;
  states.Total_usage += states.Radiooff_usage;

  states.Total_time = to_seconds(ENERGEST_GET_TOTAL_TIME());

  return states;
}