#include "contiki.h"
#include "../hello-world/hello-world.h"
#include "leds.h"
#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(hello_world1_process, "Hello world1 process");
AUTOSTART_PROCESSES(&hello_world1_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world1_process, ev, data)
{
  PROCESS_BEGIN();
  hello_world_init();
  printf("Hello, world\n");
  leds_on(LEDS_RED);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
~                                                                                                                                             
~                                                                                                                                             
~                                                                                                                                             
~                                                                                                                                             
~                                                            
