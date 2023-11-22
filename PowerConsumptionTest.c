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

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
static double RX = 23.0; // mA
static double TX = 21.0; // mA
static double CPU_NORMAL = 1.8; // mA
static double CPU_SLEEP = 0.0051; // mA
static double CPU_DEEP_SLEEP = 0.0051; // mA Not sure if this is correct

PROCESS(energest_example_process, "energest example process");
AUTOSTART_PROCESSES(&energest_example_process);
/*---------------------------------------------------------------------------*/
static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}
void print_float_parts(float value) {
    // Convert the float to an integer
    int int_value = (int)value;

    // Extract the integer and decimal parts
    int integer_part = int_value;
    int decimal_part = (int)((value - integer_part) * 1000);  // Assuming you want up to three decimal places

    // Print the integer and decimal parts separately
    LOG_INFO("%d.%d", integer_part, decimal_part);
}
PROCESS_THREAD(energest_example_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  etimer_set(&periodic_timer, CLOCK_SECOND * 30);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

    /*
     * Update all energest times. Should always be called before energest
     * times are read.
     */
    energest_flush();
    
    LOG_INFO("In CPU Normal Mode for %4lus\n", to_seconds(energest_type_time(ENERGEST_TYPE_CPU)));
    LOG_INFO("Energy consumption for CPU Normal Mode: ");
    print_float_parts(to_seconds(energest_type_time(ENERGEST_TYPE_CPU))*CPU_NORMAL);
    LOG_INFO("mAs\n");
    LOG_INFO("In CPU Sleep Mode for %4lus\n", to_seconds(energest_type_time(ENERGEST_TYPE_LPM)));
    LOG_INFO("Energy consumption for CPU Sleep Mode: ");
    print_float_parts(to_seconds(energest_type_time(ENERGEST_TYPE_LPM))*CPU_SLEEP);
    LOG_INFO("mAs\n");
    LOG_INFO("In CPU Deep Sleep Mode for %4lus\n", to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)));
    LOG_INFO("Energy consumption for CPU Deep Sleep Mode: ");
    print_float_parts(to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM))*CPU_DEEP_SLEEP);
    LOG_INFO("mAs\n");
    LOG_INFO("Total time %4lus\n", to_seconds(ENERGEST_GET_TOTAL_TIME()));

    LOG_INFO("Transmitted for %4lus\n", to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)));
    LOG_INFO("Energy consumption for Transmitted: ");
    print_float_parts(to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT))*TX);
    LOG_INFO("mAs\n");
    LOG_INFO("Listened for %4lus\n", to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)));
    LOG_INFO("Energy consumption for Received: ");
    print_float_parts(to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN))*RX);
    LOG_INFO("mAs\n");
    LOG_INFO("Radio off for %4lus\n", to_seconds(ENERGEST_GET_TOTAL_TIME()
            - energest_type_time(ENERGEST_TYPE_TRANSMIT)
            - energest_type_time(ENERGEST_TYPE_LISTEN)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/