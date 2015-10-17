#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "contiki.h"
#include "contiki-net.h"
#include "EAP/include.h"
#include "EAP/eap-peer.h"
#include "EAP/eap-psk.h"



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
#define SERVER_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xfdfd, 0, 0, 0, 0, 0, 0, 0x0001)	/* tap0 inf */
#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)

// sample define the eap msg structure
// try to decode the msg, and print the code in the first PUT message from ./openpaa

/*
struct eap_msg{
        unsigned char code;
        unsigned char id;
        unsigned short length;
        unsigned char method;
}__attribute__((packed));

*/

/* leading and ending slashes only for demo purposes, get cropped automatically when setting the Uri-Path */
char *service_urls[NUMBER_OF_URLS] = { "/auth" };

//uint8_t* msk_key;


void printf_hex(unsigned char *hex, unsigned int l)
{
        int i;
        if (hex != NULL){
                for (i=0; i < l; i++)
                        printf("%02x",hex[i]);

                printf("\n");
        }
}

#if REST_RES_AUTH
RESOURCE (auth, METHOD_GET | METHOD_POST | METHOD_PUT, "auth",
	  "title=\"EAP over CoAP\"");
void
auth_handler (void *request, void *response, uint8_t * buffer,
	      uint16_t preferred_size, int32_t * offset)
{

  uint16_t payload_len = 0;
  uint8_t *payload = NULL;
  bool isTime = false;
  int len;

  rest_resource_flags_t method = REST.get_method_type (request);
  switch (method)
    {
    case METHOD_POST:
    // NEEDS FIX
    // While sending the ACK, the payload and option fields 
    // need to be retained from the request. 
    // currently, the options and the payload are zero.

      coap_set_status_code (response, CREATED_2_01);
      memset (&msk_key, 0, MSK_LENGTH);
      printf_hex (msk_key, 16);
      eapRestart = TRUE;
      eap_peer_sm_step (NULL);
      break;


    case METHOD_PUT:
    // NEEDS FIX
    // While sending the ACK, option field is zero
    // and the userID is not in the payload, as below. 
    // Expected: 
    // Payload of 9 bytes
    // Value: "\02\ffffffef\00\09\01user"

    // Actual:
    // Payload of 5 bytes
    // Value: "\026\00\09\01"

      coap_set_status_code (response, CHANGED_2_04);
      payload_len = coap_get_payload (request, &payload);
      PRINTF ("\n\nPAYLOAD LENGTH = %d, CODE: = %d \n", payload_len,
	      ((struct eap_msg *) payload)->code);
      if (!eapKeyAvailable) {
	  printf ("---------------\nEAP EXCHANGE IN COURSE \n");
	  eapReq = TRUE;
	  eap_peer_sm_step (payload);
	  if (eapResp) {
	      printf ("Hay EAP response %d\n",
		      ntohs (((struct eap_msg *) eapRespData)->length));
	      printf_hex (eapRespData, len =
			  ntohs (((struct eap_msg *) eapRespData)->length));
	  } else {
	      printf ("NO HAY EAP RESPONSE\n");
	  }
	  // response->setPayload (eapRespData, len);
	  coap_set_payload(response, eapRespData, payload_len);
                                                                
      } else {
	  // EAP EXCHANGE FINISHED
	  printf ("EAP EXCHANGE FINISHED\n");
	  printf ("msk: \n");
	  printf_hex (msk_key, 16);
	  isTime = true;
      } break;
    
    default:
      break;
    }
}
#endif
/******************************************************************************/


void
client_chunk_handler (void *response)
{
  const uint8_t *chunk;

  int len = coap_get_payload (response, &chunk);
  printf ("|%.*s", len, (char *) chunk);
}


/******************************************************************************/

PROCESS (rest_server_example, "Erbium Example Server");
AUTOSTART_PROCESSES (&rest_server_example);

PROCESS_THREAD (rest_server_example, ev, data)
{
  PROCESS_BEGIN ();


  PRINTF ("Starting Erbium Example Server\n");

  PRINTF ("uIP buffer: %u\n", UIP_BUFSIZE);
  PRINTF ("LL header: %u\n", UIP_LLH_LEN);
  PRINTF ("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
  PRINTF ("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

  /* Initialize the REST engine. */
  rest_init_engine ();

  /* Activate the application-specific resources. */

#if REST_RES_AUTH
  rest_activate_resource (&resource_auth);
#endif
  if (-1 != system ("sudo ip address add fdfd::1/64 dev tap0"))
    {
      PRINTF ("Successfully configured fdfd::1 on TAP0 interface\n");
    }
  else
    {
      PRINTF ("ERROR: configuring IP on TAP0 interface\n");
    };
  /* Define application-specific events here. */

  struct eap_msg eap_recv_data;
  static coap_packet_t grequest[1];
  uip_ipaddr_t server_ipaddr;
  SERVER_NODE (&server_ipaddr);
  // send initial GET message to Authenticator ./openpaa
  coap_init_message (grequest, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path (grequest, service_urls[0]);
  COAP_BLOCKING_REQUEST (&server_ipaddr, REMOTE_PORT, grequest,
			 client_chunk_handler);

  while (1)
    {
      PROCESS_WAIT_EVENT ();
    }				/* while (1) */

  PROCESS_END ();
}
