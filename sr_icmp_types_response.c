#include "sr_icmp_types_response.h"
#include "sr_icmp_types_send.h"
#include "sr_icmp.h"
#include "cli/helper.h"
#include "sr_udp.h"
#include "cli/socket_helper.h"

#include <string.h>

void icmp_type_echo_reply_response(sr_router* router, uint32_t* rest, byte* packet, uint16_t* packet_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_echo_reply_response(..) called \n");
   // check if reply from intended host
   if(router->ping_info.ip == ip_header->ip_src.s_addr) {
       // check if id and seq correct
       if(router->ping_info.rest == *rest) { 
          writenf(router->ping_info.fd, "Ping reply received\n");
          printf(" ** icmp_type_echo_reply_response(..) Ping reply received\n");
       }
       else {
          writenf(router->ping_info.fd, "Error! Ping reply received, but id or seq incorrect\n");
          printf(" ** icmp_type_echo_reply_response(..) Error! Ping reply received, but id or seq incorrect\n");

       }
   }
   
   clear_ping_info(router);
}

void icmp_type_dst_unreach_response(uint8_t* code, byte* packet, uint16_t* packet_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_dst_unreach_response(..) called \n");
   uint8_t type = ICMP_TYPE_DST_UNREACH;
   uint32_t rest = 0;
   uint16_t data_len = ip_header->ip_hl * 4 + 8;
   byte *data = (byte*) malloc_or_die(data_len);
   // generate original ip header (htons!)
   struct ip ip_header_orig = *ip_header;
   ip_header_orig.ip_len = htons(ip_header_orig.ip_len);
   ip_header_orig.ip_id = htons(ip_header_orig.ip_id);
   ip_header_orig.ip_off = htons(ip_header_orig.ip_off);
   ip_header_orig.ip_sum = htons(ip_header_orig.ip_sum);
   // put original ip header
   memcpy(data, &ip_header_orig, ip_header->ip_hl * 4);
   // put first 8 bytes of original ip payload
   memcpy(data + ip_header->ip_hl * 4, packet + ip_header->ip_hl * 4, 8);
   make_icmp_response_packet(&type, code, &rest, data, &data_len, ip_header, intf);
}

void icmp_type_echo_request_response(uint32_t* rest, byte* data, uint16_t* data_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_echo_request_response(..) called \n");
   uint8_t type = ICMP_TYPE_ECHO_REPLY;
   uint8_t code = ICMP_TYPE_CODE_ECHO_REPLY;
   uint16_t data_len_ = *data_len;
   make_icmp_response_packet(&type, &code, rest, data, &data_len_, ip_header, intf);
}

void icmp_type_ttl_response(uint8_t* code, byte* packet, uint16_t* packet_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_ttl_response(..) called \n");
   uint8_t type = ICMP_TYPE_TTL;
   uint32_t rest = 0;
   uint16_t data_len = ip_header->ip_hl * 4 + 8;
   // generate original ip header (htons!)
   struct ip ip_header_orig = *ip_header;
   ip_header_orig.ip_len = htons(ip_header_orig.ip_len);
   ip_header_orig.ip_id = htons(ip_header_orig.ip_id);
   ip_header_orig.ip_off = htons(ip_header_orig.ip_off);
   ip_header_orig.ip_sum = htons(ip_header_orig.ip_sum);
   byte *data = (byte*) malloc_or_die(data_len);
   // put original ip header
   memcpy(data, &ip_header_orig, ip_header->ip_hl * 4);
   // put first 8 bytes of original ip payload
   memcpy(data + ip_header->ip_hl * 4, packet + ip_header->ip_hl * 4, 8);
   make_icmp_response_packet(&type, code, &rest, data, &data_len, ip_header, intf); 
}

void icmp_type_traceroute_response(uint32_t* rest, byte* packet, uint16_t* packet_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_traceroute_response(..) called \n");
}

void icmp_type_bad_ip_response(uint8_t* code, uint32_t* rest, byte* packet, uint16_t* packet_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** icmp_type_bad_ip_response(..) called \n");
}
