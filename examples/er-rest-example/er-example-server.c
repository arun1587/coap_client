#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"


/* Define which resources to include to meet memory constraints. */

#define REST_RES_AUTH 1

#include "erbium.h"

/* For CoAP-specific example: not required for normal RESTful Web service. */
#include "er-coap-13.h"
#include "er-coap-13-engine.h"

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define NUMBER_OF_URLS 1
#define SERVER_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xfdfd, 0, 0, 0, 0, 0, 0, 0x0001) /* tap0 inf */
#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)

// sample define the eap msg structure
// try to decode the msg, and print the code in the first PUT message from ./openpaa

struct eap_msg{
        unsigned char code;
        unsigned char id;
        unsigned short length;
        unsigned char method;
}__attribute__((packed));



/* leading and ending slashes only for demo purposes, get cropped automatically when setting the Uri-Path */
char* service_urls[NUMBER_OF_URLS] = {"/auth"};
uint8_t* msk;

#if REST_RES_AUTH
RESOURCE(auth, METHOD_GET|METHOD_POST|METHOD_PUT, "auth", "title=\"EAP over CoAP\"");
void
auth_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
 
  uint16_t payload_len = 0;
  uint8_t* payload = NULL;

  rest_resource_flags_t method = REST.get_method_type(request);
  PRINTF("in AUTH HANDLER\n");
  if (method == METHOD_POST) {
    coap_set_status_code (response, CREATED_2_01);
    // TBD: Initialize the EAP state machine
  } else if (method == METHOD_PUT) {
    coap_set_status_code (response, CHANGED_2_04);
    // TBD: if !eapKeyAvailable, then proceed with EAP msg parsing and exchanges.
        // User ID, PSK_MSG2, PSK_MSG3
       payload_len = coap_get_payload(request, &payload);
       PRINTF("\n\nPAYLOAD LENGTH = %d, CODE: = %d \n", payload_len, ((struct eap_msg*) payload)->code);
       // build the identity as "user" 
    // else
       // EAP completed, derive MSK and COAP_AUTH_KEY
  }
  PRINTF("END AUTH HANDLER\n");
}
#endif
/******************************************************************************/


void
client_chunk_handler(void *response)
{
  const uint8_t *chunk;

  int len = coap_get_payload(response, &chunk);
  printf("|%.*s", len, (char *)chunk);
}


/******************************************************************************/

PROCESS(rest_server_example, "Erbium Example Server");
AUTOSTART_PROCESSES(&rest_server_example);

PROCESS_THREAD(rest_server_example, ev, data)
{
  PROCESS_BEGIN();


  PRINTF("Starting Erbium Example Server\n");

  PRINTF("uIP buffer: %u\n", UIP_BUFSIZE);
  PRINTF("LL header: %u\n", UIP_LLH_LEN);
  PRINTF("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
  PRINTF("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

  /* Initialize the REST engine. */
  rest_init_engine();

  /* Activate the application-specific resources. */

#if REST_RES_AUTH
  rest_activate_resource(&resource_auth);
#endif
  if (-1 != system("sudo ip address add fdfd::1/64 dev tap0")) {
	PRINTF("Successfully configured fdfd::1 on TAP0 interface\n");
  } else {PRINTF("ERROR: configuring IP on TAP0 interface\n");};
  /* Define application-specific events here. */
 
  struct eap_msg eap_recv_data;
  static coap_packet_t grequest[1]; 
  uip_ipaddr_t server_ipaddr;
  SERVER_NODE(&server_ipaddr);
  // send initial GET message to Authenticator ./openpaa
  coap_init_message(grequest, COAP_TYPE_CON, COAP_GET, 0 );
  coap_set_header_uri_path(grequest, service_urls[0]);
  COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, grequest, client_chunk_handler);

  while(1) {
    PROCESS_WAIT_EVENT();
  } /* while (1) */

  PROCESS_END();
}
