#include "sr_icmp_types.h"
#include "cli/helper.h"
#include "sr_icmp_types_response.h"

#include <string.h>

void icmp_type_echo_reply(sr_router* router, uint8_t code, uint32_t rest, byte* packet, uint8_t packet_len, struct ip* ip_header, interface_t* intf) {
    printf(" ** icmp_type_echo_reply(..) called \n");
   if(code != ICMP_TYPE_CODE_ECHO_REPLY)
      printf(" ** icmp_type_echo_reply(..) code incorrect! \n");
   else {
      icmp_type_echo_reply_response(router, &rest, packet, &packet_len, ip_header, intf);
   }
}

void icmp_type_dst_unreach(uint8_t code, uint32_t rest, byte* packet, uint8_t packet_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_dst_unreach(..) called \n");
   switch(code) {
      case ICMP_TYPE_CODE_DST_UNREACH_NETWORK:
         printf(" ** icmp_type_dst_unreach(..) code: unreachable network \n");
         break;
      case ICMP_TYPE_CODE_DST_UNREACH_HOST:
         printf(" ** icmp_type_dst_unreach(..) code: unreachable host \n");
         break;
      case ICMP_TYPE_CODE_DST_UNREACH_PROTOCOL:
         printf(" ** icmp_type_dst_unreach(..) code: unreachable protocol \n");
         break;
      case ICMP_TYPE_CODE_DST_UNREACH_PORT:
         printf(" ** icmp_type_dst_unreach(..) code: unreachable port \n");
         break;
      case ICMP_TYPE_CODE_DST_UNREACH_FRAG:
         printf(" ** icmp_type_dst_unreach(..) code: fragmentation required \n");
         break;
      case ICMP_TYPE_CODE_DST_UNREACH_SRC_ROUTE:
         printf(" ** icmp_type_dst_unreach(..) code: src route failed \n");
         break;
      case ICMP_TYPE_CODE_DST_UNREACH_NETWORK_UNKNOWN:
         printf(" ** icmp_type_dst_unreach(..) code: network unknown \n");
         break;
      case ICMP_TYPE_CODE_DST_UNREACH_HOST_UNKNOWN:
         printf(" ** icmp_type_dst_unreach(..) code: host unknown \n");
         break;
      default:
         printf(" ** icmp_type_dst_unreach(..) code incorrect! \n");
         break;
   }
}

void icmp_type_echo_request(uint8_t code, uint32_t rest, byte* packet, uint8_t packet_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_echo_request(..) called \n");
   if(code != ICMP_TYPE_CODE_ECHO_REQUEST)
      printf(" ** icmp_type_echo_request(..) code incorrect! \n");
   else {
      // generate id, seq, and data fields
      uint16_t id;
      uint16_t seq;
      byte* data = (byte*) malloc_or_die(packet_len);
      memcpy(&id, &rest, 2);
      memcpy(&seq, &rest + 2, 2);
      memcpy(data, packet, packet_len);
      // display these fields
      printf(" ** icmp_type_echo_request(..) displaying fields \n");
      printf(" ** icmp_type_echo_request id: %u, seq: %u, data: %lu\n", id, seq, data);
      // call the response method
      icmp_type_echo_request_response(&rest, data, &packet_len, ip_header, intf);
   }
}

void icmp_type_ttl(uint8_t code, uint32_t rest, byte* packet, uint8_t packet_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_ttl(..) called \n");
   switch(code) {
      case ICMP_TYPE_CODE_TTL_TRANSIT:
         printf(" ** icmp_type_ttl(..) code: ttl expired in transit \n");
         break;
      case ICMP_TYPE_CODE_TTL_FRAG:
         printf(" ** icmp_type_ttl(..) code: fragment reassembly time exceeded \n");
         break;
      default:
         printf(" ** icmp_type_ttl(..) code incorrect! \n");
         break;
   }
}

void icmp_type_traceroute(uint8_t code, uint32_t rest, byte* packet, uint8_t packet_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_traceroute(..) called \n");
   if(code != ICMP_TYPE_CODE_TRACEROUTE)
      printf(" ** icmp_type_traceroute(..) code incorrect! \n");
   else {
   }
}

void icmp_type_bad_ip(uint8_t code, uint32_t rest, byte* packet, uint8_t packet_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_bad_ip(..) called \n");
   switch(code) {
      case ICMP_TYPE_CODE_BAD_IP_POINTER:
         printf(" ** icmp_type_ttl(..) code: pointer indicates the error \n");
         break;
      case ICMP_TYPE_CODE_BAD_IP_MISSING:
         printf(" ** icmp_type_ttl(..) code: missing a required option \n");
         break;
      case ICMP_TYPE_CODE_BAD_IP_LENGTH:
         printf(" ** icmp_type_ttl(..) code: bad length \n");
         break;
      default:
         printf(" ** icmp_type_bad_ip(..) code incorrect! \n");
         break;
   }
}
