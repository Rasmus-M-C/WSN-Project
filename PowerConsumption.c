
#include <stdio.h>
#include "contiki.h"
#include "sys/energest.h"
#include "sys/log.h"
#include "PowerConsumption.h"

// const float RX = 23.0; // mA
// const float TX = 21.0; // mA
// const float Radiooff = 0.0051; // mA Not sure if this is correct
// const float CPU_NORMAL = 1.8; // mA
// const float CPU_SLEEP = 0.0051; // mA
// const float CPU_DEEP_SLEEP = 0.0051; // mA Not sure if this is correct

#define RX = 23.0 // mA
#define TX = 21.0 // mA
#define Radiooff = 0.0051 // mA Not sure if this is correct
#define CPU_NORMAL = 1.8 // mA
#define CPU_SLEEP = 0.0051 // mA
#define CPU_DEEP_SLEEP = 0.0051 // mA Not sure if this is correct

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

float TotalPowerConsumption() {
  //struct PowerConsumptionStates states;
  float hours = 0;
  unsigned long time;
  float power = 0;
  energest_flush(); // Update all energest times. Should always be called before energest times are read.
  time = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));
  hours = time/3600.000;
  //states.CPU_NORMAL_time = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));
  //hours = states.CPU_NORMAL_time/3600.000;
  //states.CPU_NORMAL_usage = hours*CPU_NORMAL;
  power += hours*CPU_NORMAL;
  //states.Total_usage += states.CPU_NORMAL_usage;

  time = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));
  hours = time/3600.000;
  //states.CPU_SLEEP_time = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));
  //hours = states.CPU_SLEEP_time/3600.000;
  //states.CPU_SLEEP_usage = hours*CPU_SLEEP;
  power += hours*CPU_SLEEP;

  time = to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM));
  hours = time/3600.000;
  //states.CPU_DEEP_SLEEP_time = to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM));
  //hours = states.CPU_DEEP_SLEEP_time/3600.000;
  //states.CPU_DEEP_SLEEP_usage = hours*CPU_DEEP_SLEEP;
  power += hours*CPU_DEEP_SLEEP;

  time = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
  hours = time/3600.000;
  //states.TX_time = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
  //hours = states.TX_time/3600.000;
  //states.TX_usage = hours*TX;
  power += hours*TX;

  time = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));
  hours = time/3600.000;
  //states.RX_time = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));
  //hours = states.RX_time/3600.000;
  //states.RX_usage = hours*RX;
  power += hours*RX;

  time = to_seconds(ENERGEST_GET_TOTAL_TIME() - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
  hours = time/3600.000;
  //states.Radiooff_time = to_seconds(ENERGEST_GET_TOTAL_TIME() - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
  //hours = states.Radiooff_time/3600.000;
  //states.Radiooff_usage = hours*Radiooff;
  power += hours*Radiooff;

  //states.Total_time = to_seconds(ENERGEST_GET_TOTAL_TIME());



  return power;
}