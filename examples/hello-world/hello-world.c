#include "contiki.h"
#include "hello-world.h"
#include "leds.h"
#include <stdio.h> /* For printf() */



void hello_world_init()
{
  process_start(&hello_world_process, NULL);
}



/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
//AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Hello, world\n");
  leds_on(LEDS_GREEN);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
~    
