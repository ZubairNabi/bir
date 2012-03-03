#pragma once

#include <stdio.h>
#include <assert.h>

#ifdef _SOLARIS_
#include <inttypes.h>
#endif /* _SOLARIS_ */

#define  __USE_BSD 1
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include "lwip/ip_addr.h"
#include "lwip/ip.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/transport_subsys.h"

#include "sr_base_internal.h"

void sr_transport_input(uint8_t* packet /* borrowed */);

uint32_t /*nbo*/ ip_route(struct ip_addr *dest);

err_t sr_lwip_output(struct pbuf *p, struct ip_addr *src, struct ip_addr *dst, uint8_t proto );
