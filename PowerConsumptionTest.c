/*
 * Copyright (c) 2017, RISE SICS.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "contiki.h"
#include "sys/energest.h"
#include "sys/log.h"
#include <math.h>

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
const float RX = 23.0; // mA
const float TX = 21.0; // mA
const float Radiooff = 0.0051; // mA Not sure if this is correct
const float CPU_NORMAL = 1.8; // mA
const float CPU_SLEEP = 0.0051; // mA
const float CPU_DEEP_SLEEP = 0.0051; // mA Not sure if this is correct

PROCESS(energest_example_process, "energest example process");
AUTOSTART_PROCESSES(&energest_example_process);
/*---------------------------------------------------------------------------*/
static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}
/*---------------------------------------------------------------------------*/
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

struct PowerConsumptionStates {
  float CPU_NORMAL;
  float CPU_SLEEP;
  float CPU_DEEP_SLEEP;
  float TX;
  float RX;
  float Radiooff;
  float Total;
  unsigned long CPU_NORMAL_time;
  unsigned long CPU_SLEEP_time;
  unsigned long CPU_DEEP_SLEEP_time;
  unsigned long TX_time;
  unsigned long RX_time;
  unsigned long Radiooff_time;
  unsigned long Total_time;
};
/*---------------------------------------------------------------------------*/

struct PowerConsumptionStates TotalPowerConsumption() {
  struct PowerConsumptionStates states;
  float hours = 0;
  energest_flush(); // Update all energest times. Should always be called before energest times are read.
  states.CPU_NORMAL_time = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));
  hours = states.CPU_NORMAL_time/3600.000;
  states.CPU_NORMAL = hours*CPU_NORMAL;
  states.Total += states.CPU_NORMAL;

  states.CPU_SLEEP_time = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));
  hours = states.CPU_SLEEP_time/3600.000;
  states.CPU_SLEEP = hours*CPU_SLEEP;
  states.Total += states.CPU_SLEEP;

  states.CPU_DEEP_SLEEP_time = to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM));
  hours = states.CPU_DEEP_SLEEP_time/3600.000;
  states.CPU_DEEP_SLEEP = hours*CPU_DEEP_SLEEP;
  states.Total += states.CPU_DEEP_SLEEP;

  states.TX_time = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
  hours = states.TX_time/3600.000;
  states.TX = hours*TX;
  states.Total += states.TX;

  states.RX_time = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));
  hours = states.RX_time/3600.000;
  states.RX = hours*RX;
  states.Total += states.RX;

  states.Radiooff_time = to_seconds(ENERGEST_GET_TOTAL_TIME() - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
  hours = states.Radiooff_time/3600.000;
  states.Radiooff = hours*Radiooff;
  states.Total += states.Radiooff;

  states.Total_time = to_seconds(ENERGEST_GET_TOTAL_TIME());

  return states;
}
/*
//This works alone but not with the above function memory overflow
void LOGPowerConsumption() {
  struct IntDec int_dec;
  unsigned long time = 0;
  float hours = 0;
  float power = 0;
  float sum = 0;
  energest_flush(); // Update all energest times. Should always be called before energest times are read.
  LOG_INFO("----------------------------------------------------------------------\n");
  
  LOG_INFO("|%-26s | %-11s | %-25s |\n", "Module Name", "Time", "Power Consumption");
  
  time = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));
  hours = time/3600.000;
  power = hours*CPU_NORMAL;
  sum += power;
  int_dec = Get_Float_Parts(power);
  LOG_INFO("| %-25s | %10lus | %10lu.%07lumAh |\n", "CPU Normal Mode", time, int_dec.integer, int_dec.decimal);
  
  time = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));
  hours = time/3600.000;
  power = hours*CPU_SLEEP;
  sum += power;
  int_dec = Get_Float_Parts(power);
  LOG_INFO("| %-25s | %10lus | %10lu.%07lumAh |\n", "CPU Sleep Mode", time, int_dec.integer, int_dec.decimal);
  
  time = to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM));
  hours = time/3600.000;
  power = hours*CPU_DEEP_SLEEP;
  sum += power;
  int_dec = Get_Float_Parts(power);
  LOG_INFO("| %-25s | %10lus | %10lu.%07lumAh |\n", "CPU Deep Sleep Mode", time, int_dec.integer, int_dec.decimal);

  time = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
  hours = time/3600.000;
  power = hours*TX;
  sum += power;
  int_dec = Get_Float_Parts(power);
  LOG_INFO("| %-25s | %10lus | %10lu.%07lumAh |\n", "Transmit Mode", time, int_dec.integer, int_dec.decimal);
  
  time = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));
  hours = time/3600.000;
  power = hours*RX;
  sum += power;
  int_dec = Get_Float_Parts(power);
  LOG_INFO("| %-25s | %10lus | %10lu.%07lumAh |\n", "Listen Mode", time, int_dec.integer, int_dec.decimal);
  
  time = to_seconds(ENERGEST_GET_TOTAL_TIME() - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN));
  hours = time/3600.000;
  power = hours*Radiooff;
  sum += power;
  int_dec = Get_Float_Parts(power);
  LOG_INFO("| %-25s | %10lus | %10lu.%07lumAh |\n", "Radio Off", time, int_dec.integer, int_dec.decimal);

  time = to_seconds(ENERGEST_GET_TOTAL_TIME());
  int_dec = Get_Float_Parts(sum);
  LOG_INFO("| %-25s | %10lus | %10lu.%07lumAh |\n", "Total", time, int_dec.integer, int_dec.decimal);

  LOG_INFO("----------------------------------------------------------------------\n");
}
//This were an etempt to remove the surplus of LOG functions which were using alot of memory. It did reduce the memory usage but not enough.
void LOGPowerConsumption(struct PowerConsumptionStates states) {
  struct IntDec int_dec_CPU_NORMAL;
  struct IntDec int_dec_CPU_SLEEP;
  struct IntDec int_dec_CPU_DEEP_SLEEP;
  struct IntDec int_dec_TX;
  struct IntDec int_dec_RX;
  struct IntDec int_dec_Radiooff;
  struct IntDec int_dec_Total;

  int_dec_CPU_NORMAL = Get_Float_Parts(states.CPU_NORMAL);
  int_dec_CPU_SLEEP = Get_Float_Parts(states.CPU_SLEEP);
  int_dec_CPU_DEEP_SLEEP = Get_Float_Parts(states.CPU_DEEP_SLEEP);
  int_dec_TX = Get_Float_Parts(states.TX);
  int_dec_RX = Get_Float_Parts(states.RX);
  int_dec_Radiooff = Get_Float_Parts(states.Radiooff);
  int_dec_Total = Get_Float_Parts(states.Total);

  LOG_INFO("----------------------------------------------------------------------\n"\
  "|%-26s | %-11s | %-25s |\n"\
  "| %-25s | %10lus | %10lu.%07lumAh |\n"\
  "| %-25s | %10lus | %10lu.%07lumAh |\n"\
  "| %-25s | %10lus | %10lu.%07lumAh |\n"\
  "| %-25s | %10lus | %10lu.%07lumAh |\n"\
  "| %-25s | %10lus | %10lu.%07lumAh |\n"\
  "| %-25s | %10lus | %10lu.%07lumAh |\n"\
  "| %-25s | %10lus | %10lu.%07lumAh |\n"\
  "----------------------------------------------------------------------\n", 
  "Module Name", "Time", "Power Consumption", 
  "CPU Normal Mode", states.CPU_NORMAL_time, int_dec_CPU_NORMAL.integer, int_dec_CPU_NORMAL.decimal, 
  "CPU Sleep Mode", states.CPU_SLEEP_time, int_dec_CPU_SLEEP.integer, int_dec_CPU_SLEEP.decimal,
  "CPU Deep Sleep Mode", states.CPU_DEEP_SLEEP_time, int_dec_CPU_DEEP_SLEEP.integer, int_dec_CPU_DEEP_SLEEP.decimal,
  "Transmit Mode", states.TX_time, int_dec_TX.integer, int_dec_TX.decimal,
  "Listen Mode", states.RX_time, int_dec_RX.integer, int_dec_RX.decimal,
  "Radio Off", states.Radiooff_time, int_dec_Radiooff.integer, int_dec_Radiooff.decimal,
  "Total", states.Total_time, int_dec_Total.integer, int_dec_Total.decimal);
}*/

PROCESS_THREAD(energest_example_process, ev, data)
{
  static struct etimer periodic_timer;
  float total = 0;
  struct IntDec int_dec;
  struct PowerConsumptionStates states;
  PROCESS_BEGIN();

  etimer_set(&periodic_timer, CLOCK_SECOND * 30);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    states = TotalPowerConsumption();
    //LOGPowerConsumption(states);
    //int_dec = Get_Float_Parts(states.Total);
    //LOG_INFO("Test total = %10lu.%07lumAh |\n", int_dec.integer, int_dec.decimal);
  PROCESS_END();
  }
}
/*---------------------------------------------------------------------------*/