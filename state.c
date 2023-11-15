#include "contiki.h"
#include <stdio.h> /* For printf() */
#include <stdlib.h>

#define BAD_SUCCES 95
int counter = 0;

/*---------------------------------------------------------------------------*/
PROCESS(cs_process, "CS process");
AUTOSTART_PROCESSES(&cs_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cs_process, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 2 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 2);

  while(counter < 100) {
    

    if(rand() % 100 < BAD_SUCCES){
        //printf("Received: %d\n", counter);
    } else {
        printf("Lost: %d\n", counter);
    }
    


    counter++;

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
