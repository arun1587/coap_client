#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"


/* Define which resources to include to meet memory constraints. */
#define REST_RES_HELLO 1
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


/* leading and ending slashes only for demo purposes, get cropped automatically when setting the Uri-Path */
char* service_urls[NUMBER_OF_URLS] = {"/auth"};


#if REST_RES_AUTH
RESOURCE(auth, METHOD_GET|METHOD_POST|METHOD_PUT, "auth", "title=\"EAP over CoAP\"");
void
auth_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

  rest_resource_flags_t method = REST.get_method_type(request);
  PRINTF("in AUTH HANDLER\n");
  if (method == METHOD_POST) {
    coap_set_status_code (response, CREATED_2_01);
  } else if (method == METHOD_PUT) {
    coap_set_status_code (response, CHANGED_2_04);
  }
  PRINTF("END AUTH HANDLER\n");
}
#endif
/******************************************************************************/
#if REST_RES_HELLO
/*
 * Resources are defined by the RESOURCE macro.
 * Signature: resource name, the RESTful methods it handles, and its URI path (omitting the leading slash).
 */

RESOURCE(helloworld, METHOD_GET|METHOD_POST|METHOD_PUT, "hello", "title=\"Hello world: ?len=0..\";rt=\"Text\"");

/*
 * A handler function named [resource name]_handler must be implemented for each RESOURCE.
 * A buffer for the response payload is provided through the buffer pointer. Simple resources can ignore
 * preferred_size and offset, but must respect the REST_MAX_CHUNK_SIZE limit for the buffer.
 * If a smaller block size is requested for CoAP, the REST framework automatically splits the data.
 */
void
helloworld_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *len = NULL;
  /* Some data that has the length up to REST_MAX_CHUNK_SIZE. For more, see the chunk resource. */
  char const * const message = "Hello VWorld! ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxy";
  int length = 12; /*           |<-------->| */

  /* The query string can be retrieved by rest_get_query() or parsed for its key-value pairs. */
  if (REST.get_query_variable(request, "len", &len)) {
    length = atoi(len);
    if (length<0) length = 0;
    if (length>REST_MAX_CHUNK_SIZE) length = REST_MAX_CHUNK_SIZE;
    memcpy(buffer, message, length);
  } else {
    memcpy(buffer, message, length);
  }

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN); /* text/plain is the default, hence this option could be omitted. */
  REST.set_header_etag(response, (uint8_t *) &length, 1);
  REST.set_response_payload(response, buffer, length);
}

#endif

void sendGET () {

}

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
#if REST_RES_HELLO
  rest_activate_resource(&resource_helloworld);
#endif
#if REST_RES_AUTH
  rest_activate_resource(&resource_auth);
#endif
  system("sudo ip address add fdfd::1/64 dev tap0");
  /* Define application-specific events here. */
  sendGET();

  static coap_packet_t grequest[1]; 
  uip_ipaddr_t server_ipaddr;
  SERVER_NODE(&server_ipaddr);
  // send initial GET message
  coap_init_message(grequest, COAP_TYPE_CON, COAP_GET, 0 );
  coap_set_header_uri_path(grequest, service_urls[0]);
  COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, grequest, client_chunk_handler);

  while(1) {
    PROCESS_WAIT_EVENT();
  } /* while (1) */

  PROCESS_END();
}
