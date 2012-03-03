#include "sr_ip.h"
#include "cli/helper.h"
#include "lwtcp/lwip/inet.h"
#include "sr_common.h"
#include "sr_icmp.h"
#include "sr_lwtcp_glue.h"
#include "sr_udp.h"
#include "sr_icmp_types_response.h"
#include "sr_integration.h"
#include "sr_ethernet.h"
#include "sr_rtable.h"
#include "sr_pwospf.h"
#include "sr_icmp_types_send.h"

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

bool fragment_check(uint16_t off) {
   if(off == IP_OFFSET_DF || off == IP_OFFSET_NF)
      return TRUE;
   return FALSE;
}

bool check_ip_header(struct ip* header, byte* raw_header) {
  // create raw header with checksum zero
  byte* sum_header = (byte*) malloc_or_die(header->ip_hl * 4);
  memcpy(sum_header, raw_header, header->ip_hl * 4);
  uint16_t sum_zero = 0;
  memcpy(sum_header + 10, &sum_zero  ,2);
  uint16_t len_min = IP_PACKET_MIN_LEN; 
  uint16_t len_max = IP_PACKET_MAX_LEN;
  // find out exact failure
  if(header->ip_v != IP_4_VERSION)
     printf("ip version incorrect\n");
  if(header->ip_hl < IP_HEADER_MIN_LEN_WORD)
     printf("ip header min incorrect\n");
  if(header->ip_hl > IP_HEADER_MAX_LEN_WORD)
     printf("ip header max incorrect\n");
  if(header->ip_len < len_min)
     printf("ip header min incorrect\n");
  if(header->ip_len > len_max)
     printf("ip header max incorrect\n");
  if(ntohs(inet_chksum((void*) sum_header, header->ip_hl * 4)) != header->ip_sum)
     printf("ip header check sum failed\n");
  if(header->ip_hl > IP_HEADER_MIN_LEN_WORD)
     printf("ip header options present failure\n");
  if(fragment_check(header->ip_off) == FALSE)
     printf("ip header fragments failure\n");

  // complete sanity checks
  if(header->ip_v!= IP_4_VERSION || header->ip_hl != IP_HEADER_MIN_LEN_WORD || header->ip_len < len_min || header->ip_len > len_max || ntohs(inet_chksum((void*) sum_header, header->ip_hl * 4)) != header->ip_sum || fragment_check(header->ip_off) == FALSE) {
//      free(sum_header);
      return FALSE;
   }
  // free(sum_header);
   return TRUE;
}

void ip_handle_packet( sr_router* router,
                        byte* ip_packet,
                        interface_t* intf ) {
   printf(" ** ip_handle_packet(..) called \n");
   // construct ip header
   struct ip* ip_header =  make_ip_header(ip_packet); 
   byte* ip_header_raw = (byte*) malloc_or_die(ip_header->ip_hl * 4);
   memcpy(ip_header_raw, ip_packet, ip_header->ip_hl * 4);
   
   // check if ip header is correct
   bool ret = check_ip_header(ip_header, ip_header_raw);
   if(ret == TRUE) {
      printf(" ** ip_handle_packet(..) header sanity checks passed\n");
      //    display ip header
      printf(" ** ip_handle_packet(..) packet contents \n");
      display_ip_header(ip_header);
      // get payload
      uint8_t payload_len = ip_header->ip_len - (ip_header->ip_hl * 4);
      printf(" ** ip_handle_packet(..) Payload size %u bytes \n", payload_len);
      byte* payload = (byte*) malloc_or_die(payload_len);
      memcpy(payload, ip_packet + (ip_header->ip_hl * 4), payload_len);
      //free(ip_header_raw);
      // check destination
      printf(" ** ip_handle_packet(..) checking packet destination \n");
      // check if spf broadcast packet
      char* all_spf_routers =  ALL_SPF_ROUTERS_IP;
      if(check_packet_destination(ip_header->ip_dst, intf) == TRUE || ip_header->ip_dst.s_addr == make_ip_addr(all_spf_routers)) {
         printf(" ** ip_handle_packet(..) yes I'm the destination \n");
         // check if ttl expired
         if(ip_header->ip_ttl == 1) {
            printf(" ** ip_handle_packet(..) ttl expired, sending ICMP ttl error message \n");
            uint8_t code = ICMP_TYPE_CODE_TTL_TRANSIT;
            icmp_type_ttl_send(&code, payload, (uint8_t*)&ip_header->ip_len, ip_header);
         } else {
            // check protocol
            printf(" ** ip_handle_packet(..) checking protocol \n");
            check_packet_protocol(router, ip_header, payload, payload_len, intf, ip_packet);
         } 
      } else {
         printf(" ** ip_handle_packet(..) no I'm not the destination, send to ip lookup \n");
         ip_header->ip_len = htons((ip_header->ip_len));
         ip_header->ip_off = htons((ip_header->ip_off));
         ip_header->ip_id = htons((ip_header->ip_id));
         ip_header->ip_sum = htons((ip_header->ip_sum));
         ip_look_up_reply(ip_packet, ip_header, intf);
      }
      
   }
   else
      printf(" ** ip_handle_packet(..) header sanity checks failed\n");
      //free(ip_packet);
      //free(ip_header);
      //free(ip_header_raw);
} 

struct ip* make_ip_header(byte* ip_packet) {
   struct ip* ip_header = (struct ip*) malloc_or_die(sizeof(struct ip));
   byte ver_len;
   memcpy(&ver_len, ip_packet, 1);
   ip_header->ip_v = ver_len >> 4;
   ip_header->ip_hl = ver_len & ((1 << 4) - 1);
   memcpy(&ip_header->ip_tos, ip_packet + 1, 1);
   memcpy(&ip_header->ip_len, ip_packet + 2 ,2);
   ip_header->ip_len = ntohs(ip_header->ip_len);
   memcpy(&ip_header->ip_id, ip_packet + 4 ,2);
   ip_header->ip_id = ntohs(ip_header->ip_id);
   memcpy(&ip_header->ip_off, ip_packet + 6 ,2);
   ip_header->ip_off = ntohs(ip_header->ip_off);
   memcpy(&ip_header->ip_ttl, ip_packet + 8 ,1);
   memcpy(&ip_header->ip_p, ip_packet + 9 ,1);
   memcpy(&ip_header->ip_sum, ip_packet + 10 ,2);
   ip_header->ip_sum = ntohs(ip_header->ip_sum);
   uint32_t src, dst;
   memcpy(&src, ip_packet + 12 ,4);
   memcpy(&dst, ip_packet + 16 ,4);
   memcpy(&ip_header->ip_src, &src , 4);
   memcpy(&ip_header->ip_dst, &dst ,4);
   return ip_header;
}

void display_ip_header(struct ip* ip_header) {
   char src[INET_ADDRSTRLEN];
   char dst[INET_ADDRSTRLEN];
   inet_ntop(AF_INET, &(ip_header->ip_src), src, INET_ADDRSTRLEN);
   inet_ntop(AF_INET, &(ip_header->ip_dst), dst, INET_ADDRSTRLEN);
   printf(" ** ver: %u,  header len: %u, tos:  %u, packet len: %u, id:  %u, off:  %u, ttl:  %u, protocol:  %u, checksum:  %u, src:  %s, dst:  %s\n", ip_header->ip_v, ip_header->ip_hl, ip_header->ip_tos, ip_header->ip_len, ip_header->ip_id, ip_header->ip_off, ip_header->ip_ttl, ip_header->ip_p, ip_header->ip_sum, src, dst);
}

bool check_packet_destination(struct in_addr ip_dst, interface_t* intf) {
   char str[INET_ADDRSTRLEN];
   inet_ntop(AF_INET, &(ip_dst), str, INET_ADDRSTRLEN);
   printf(" ** dst:  %s\n", str);
   printf(" ** intf: %s\n", quick_ip_to_string(intf->ip));
   if(ip_dst.s_addr == intf->ip)
      return TRUE;
   return FALSE;
}

void check_packet_protocol(sr_router* router, struct ip* ip_header, byte* payload, uint8_t payload_len, interface_t* intf, byte* ip_packet) {
   if(ip_header->ip_p == IP_PROTOCOL_ICMP) {
      //icmp
      printf(" ** ip_handle_packet(..) protocol: ICMP \n");
      icmp_handle_packet(router, payload, payload_len, ip_header, intf);
   } else if(ip_header->ip_p == IP_PROTOCOL_TCP) {
      //tcp
      printf(" ** ip_handle_packet(..) protocol: TCP \n");
      sr_transport_input(ip_packet);
   } else if(ip_header->ip_p == IP_PROTOCOL_UDP) {
      //udp
      printf(" ** ip_handle_packet(..) protocol: UDP \n");
      udp_handle_packet(payload, ip_header, intf);
   } else if(ip_header->ip_p == IP_PROTOCOL_OSPF) {
      //ospf
      printf(" ** ip_handle_packet(..) protocol: OSPF \n");
      pwospf_handle_packet(payload,
                          payload_len,
                          ip_header,
                          intf,
                          router); 
   }else {
      //otherwise
      printf(" ** ip_handle_packet(..) protocol: Error! Not supported. Dropping and sending ICMP response \n");
      uint8_t code = ICMP_TYPE_CODE_DST_UNREACH_PROTOCOL;
      icmp_type_dst_unreach_send(&code, ip_packet, (uint8_t*)&ip_header->ip_len, ip_header);
   }
}

void make_ip_packet_reply(byte* payload, uint8_t payload_len, struct in_addr src, struct in_addr dst, uint8_t protocol, interface_t* intf) {
   printf(" ** make_ip_packet_reply(..) called \n");
   struct ip* ip_header = (struct ip*) malloc_or_die(sizeof(struct ip));
   ip_header->ip_v = IP_4_VERSION;
   ip_header->ip_off = htons(IP_OFFSET_DF);
   ip_header->ip_p = protocol;
   // switching src and dst 
   ip_header->ip_dst = src;
   ip_header->ip_src = dst;
   ip_header->ip_tos = IP_TOS_DEFAULT;
   ip_header->ip_id = htons(IP_ID);
   ip_header->ip_ttl = IP_TTL_DEFAULT;
   ip_header->ip_hl = IP_HEADER_MIN_LEN_WORD; 
   ip_header->ip_len = htons(payload_len + IP_HEADER_MIN_LEN);
   ip_header->ip_sum = 0;
   ip_header->ip_sum = htons(htons(inet_chksum((void*)ip_header, IP_HEADER_MIN_LEN)));
   // generate ip packet
   uint8_t packet_len = ntohs(ip_header->ip_len);
   byte* ip_packet = (byte*) malloc_or_die(packet_len);
   memcpy(ip_packet, ip_header, IP_HEADER_MIN_LEN);
   memcpy(ip_packet + IP_HEADER_MIN_LEN, payload, payload_len); 
   // display generated packet len
   printf(" ** make_ip_packet_reply(..) generated packet with length %u bytes\n", (ntohs(ip_header->ip_len)));
   // send to ip lookup
   ip_look_up_reply(ip_packet, ip_header, intf);
}

uint16_t generate_checksum_ip_header(struct ip* header, uint8_t len) {  
   byte* raw_header = (byte*) malloc_or_die(len);
   memcpy(raw_header, header, len);
   return htons(inet_chksum((void*) raw_header, len));
}

void ip_look_up(byte* ip_packet, struct ip* ip_header) {
   printf(" ** ip_lookup(..) called \n");
   // get ptr to global router instance
   struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(get_sr());
   // get dst ip object
   addr_ip_t dst_ip;
   memcpy(&dst_ip, &ip_header->ip_dst, 4);
   // look up in routing table
   route_info_t route_info = rrtable_find_route(subsystem->rtable, dst_ip);
   // display output interface
   printf(" ** ip_lookup(..) output interface for ip: %s\n", quick_ip_to_string(dst_ip));
   display_interface(route_info.intf);
   // set src ip as the ip of the output interface
   memcpy(&ip_header->ip_src, &route_info.intf->ip, 4);
   memcpy(ip_packet, ip_header, ip_header->ip_hl);
   // check if ttl expired
   if(ip_header->ip_ttl == 1) {
      printf(" ** ip_lookup(..) ttl expired, sending ICMP ttl error message \n");
      uint8_t code = ICMP_TYPE_CODE_TTL_TRANSIT;
      uint8_t len = ntohs(ip_header->ip_len);
      icmp_type_ttl_send(&code, ip_packet, &len, ip_header);
   } else {
      printf(" ** ip_lookup(..) decrementing ttl \n");
      // create new ip header
      struct ip* ip_header_ttl = (struct ip*) malloc_or_die(ip_header->ip_hl * 4);
      memcpy(ip_header_ttl, ip_header, ip_header->ip_hl * 4);
      // decrement ttl
      ip_header_ttl->ip_ttl = ip_header_ttl->ip_ttl - 1;
      // generate checksum
      ip_header_ttl->ip_sum = 0;
      ip_header_ttl->ip_sum = htons(htons(inet_chksum((void*)ip_header_ttl, ip_header_ttl->ip_hl * 4)));
      // get payload
      uint8_t payload_len = ntohs(ip_header_ttl->ip_len) - (ip_header_ttl->ip_hl * 4);
      printf(" ** ip_lookup(..) Payload size %u bytes \n", payload_len);
      memcpy(ip_packet, ip_header_ttl, ip_header_ttl->ip_hl * 4);
      // make new ip packet
      send_ip_packet(ip_packet, ip_header_ttl, route_info);

   }
}

void send_ip_packet(byte* ip_packet, struct ip* ip_header, route_info_t route_info) {
   printf(" ** send_ip_packet(..) called \n");
   // display generated packet len
   addr_ip_t dst_ip, src_ip;
   memcpy(&dst_ip, &ip_header->ip_dst, 4);   
   memcpy(&src_ip, &ip_header->ip_src, 4);
   if(check_if_local_subnet(dst_ip, src_ip, route_info.intf->subnet_mask) == FALSE) { // non local subnet
      printf(" ** send_ip_packet(..) destination on non-local subnet\n");
      dst_ip = route_info.ip;
   } else
      printf(" ** send_ip_packet(..) destination on local subnet\n");
   struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(get_sr());
   arp_cache_handle_partial_frame(subsystem, route_info.intf, dst_ip, ip_packet, ntohs(ip_header->ip_len), ETHERTYPE_IP);
}

bool check_if_local_subnet(addr_ip_t dst, addr_ip_t src, addr_ip_t subnet_mask) {
   addr_ip_t dst_subnet = dst & subnet_mask;
   addr_ip_t src_subnet = src & subnet_mask;
   if(dst_subnet == src_subnet){ // is local
      return TRUE;
   }
   return FALSE;
}

void make_ip_packet(byte* payload, uint8_t payload_len, struct in_addr src, struct in_addr dst, uint8_t protocol) {
   printf(" ** make_ip_packet(..) called \n");
   struct ip* ip_header = (struct ip*) malloc_or_die(sizeof(struct ip));
   ip_header->ip_v = IP_4_VERSION;
   ip_header->ip_off = htons(IP_OFFSET_DF);
   ip_header->ip_p = protocol;
   // switching src and dst 
   ip_header->ip_dst = src;
   ip_header->ip_src = dst;
   ip_header->ip_tos = IP_TOS_DEFAULT;
   ip_header->ip_id = htons(IP_ID);
   ip_header->ip_ttl = IP_TTL_DEFAULT;
   ip_header->ip_hl = IP_HEADER_MIN_LEN_WORD;
   ip_header->ip_len = htons(payload_len + IP_HEADER_MIN_LEN);
   ip_header->ip_sum = 0;
   ip_header->ip_sum = htons(htons(inet_chksum((void*)ip_header, IP_HEADER_MIN_LEN)));
   // generate ip packet
   uint8_t packet_len = ntohs(ip_header->ip_len);
   byte* ip_packet = (byte*) malloc_or_die(packet_len);
   memcpy(ip_packet, ip_header, IP_HEADER_MIN_LEN);
   memcpy(ip_packet + IP_HEADER_MIN_LEN, payload, payload_len);
   // display generated packet len
   printf(" ** make_ip_packet(..) generated packet with length %u bytes\n", (ntohs(ip_header->ip_len)));
   // send to ip lookup
   ip_look_up(ip_packet, ip_header);
   // free payload mem
   //free(payload);
}

void ip_look_up_reply(byte* ip_packet, struct ip* ip_header, interface_t* intf) {
   printf(" ** ip_lookup_reply(..) called \n");
   // get ptr to global router instance
   struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(get_sr());
   // get dst ip object
   addr_ip_t dst_ip;
   memcpy(&dst_ip, &ip_header->ip_dst, 4);
   // look up in routing table
   route_info_t route_info = rrtable_find_route(subsystem->rtable, dst_ip);
   // display output interface
   printf(" ** ip_lookup_reply(..) output interface for ip: %s\n", quick_ip_to_string(dst_ip));
   display_interface(route_info.intf);
      // check if destination same as ip of the source interface
   if(strcmp(route_info.intf->name, intf->name) == 0 && route_info.intf->ip != ip_header->ip_src.s_addr) {
      printf(" ** make_ip_packet_reply(..) error! destination interface same as source \n");
     // send icmp destination host unreachable
     uint8_t code = ICMP_TYPE_CODE_DST_UNREACH_HOST;
     // reverse order
     ip_header->ip_len = ntohs((ip_header->ip_len));
     ip_header->ip_off = ntohs((ip_header->ip_off));
     ip_header->ip_id = ntohs((ip_header->ip_id));
     ip_header->ip_sum = ntohs((ip_header->ip_sum));
     uint8_t packet_len = ip_header->ip_len;
     icmp_type_dst_unreach_send(&code, ip_packet, &packet_len, ip_header);
   } else {
      // check if ttl expired
      if(ip_header->ip_ttl == 1) {
         printf(" ** ip_lookup_reply(..) ttl expired, sending ICMP ttl error message \n");
         uint8_t code = ICMP_TYPE_CODE_TTL_TRANSIT;
         uint8_t len = ntohs(ip_header->ip_len);
         icmp_type_ttl_send(&code, ip_packet, &len, ip_header);
      } else {
         printf(" ** ip_lookup_reply(..) decrementing ttl \n");
         // create new ip header
         struct ip* ip_header_ttl = (struct ip*) malloc_or_die(ip_header->ip_hl * 4);
         memcpy(ip_header_ttl, ip_header, ip_header->ip_hl * 4);
         // decrement ttl
         ip_header_ttl->ip_ttl = ip_header_ttl->ip_ttl - 1;
         // generate checksum
         ip_header_ttl->ip_sum = 0;
         ip_header_ttl->ip_sum = htons(htons(inet_chksum((void*)ip_header_ttl, ip_header_ttl->ip_hl * 4)));
         // get payload
         uint8_t payload_len = ntohs(ip_header_ttl->ip_len) - (ip_header_ttl->ip_hl * 4);
         printf(" ** ip_lookup_reply(..) Payload size %u bytes \n", payload_len);
         memcpy(ip_packet, ip_header_ttl, ip_header_ttl->ip_hl * 4);
         // make new ip packet
         send_ip_packet(ip_packet, ip_header_ttl, route_info);
      }
   }
}
