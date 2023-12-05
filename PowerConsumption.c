
#include <stdio.h>
#include "contiki.h"
#include "sys/energest.h"
#include "PowerConsumption.h"

#define RX 23.0; // mA
#define TX 21.0; // mA
#define Radiooff 0.0051; // mA Not sure if this is correct
#define CPU_NORMAL 1.8; // mA
#define CPU_SLEEP 0.0051; // mA
#define CPU_DEEP_SLEEP 0.0051; // mA Not sure if this is correct


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
  return (unsigned long)(time / ENERGEST_SECOND);
}


float TotalPowerConsumption() {
  //struct PowerConsumptionStates states;
  float power = 0;
  energest_flush(); // Update all energest times. Should always be called before energest times are read.
  power += ((to_seconds(energest_type_time(ENERGEST_TYPE_CPU)))/3600.000)*CPU_NORMAL;
  power += ((to_seconds(energest_type_time(ENERGEST_TYPE_LPM)))/3600.000)*CPU_SLEEP;
  power += ((to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)))/3600.000)*CPU_DEEP_SLEEP;
  power += ((to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)))/3600.000)*TX;
  power += ((to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)))/3600.000)*RX;
  power += ((to_seconds(ENERGEST_GET_TOTAL_TIME() - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN)))/3600.000)*Radiooff;

  //states.Total_time = to_seconds(ENERGEST_GET_TOTAL_TIME());
  return power;
}