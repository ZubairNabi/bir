#include "sr_icmp.h"
#include "lwtcp/lwip/inet.h"
#include "sr_common.h"
#include "cli/helper.h"
#include "sr_icmp_types.h"
#include "sr_ip.h"

#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <stdlib.h>

icmp_header_t* make_icmp_header(byte* icmp_packet) {
   icmp_header_t* header = (icmp_header_t*) malloc_or_die(ICMP_HEADER_LEN);
   memcpy(&header->type, icmp_packet, 1);
   memcpy(&header->code, icmp_packet + 1, 1);
   memcpy(&header->checksum, icmp_packet + 2, 2);
   header->checksum = ntohs(header->checksum);
   memcpy(&header->rest, icmp_packet + 4, 4);
   return header;
}

void icmp_handle_packet(sr_router* router, byte* icmp_packet,
                        uint8_t packet_len, struct ip* ip_header, interface_t* intf) {
    printf(" ** icmp_handle_packet(..) called \n");
    // construct icmp header
    printf(" ** icmp_handle_packet(..) constructing icmp header \n");
    icmp_header_t *icmp_header = make_icmp_header(icmp_packet);
    // get raw icmp packet
    byte* raw_icmp_packet = (byte*) malloc_or_die(packet_len);
    memcpy(raw_icmp_packet, icmp_packet, packet_len);
    // checksum
    printf(" ** icmp_handle_packet(..) checking header checksum \n");
    if(check_icmp_header(icmp_header, raw_icmp_packet, packet_len) == TRUE) {
       printf(" ** icmp_handle_packet(..) checksum correct \n");
       // display icmp header
       printf(" ** icmp_handle_packet(..) displaying icmp header \n");
       display_icmp_header(icmp_header);
       // get payload
       uint8_t payload_len = packet_len - ICMP_HEADER_LEN;
       printf(" ** icmp_handle_packet(..) Payload size %u bytes \n", payload_len);
       byte* payload = (byte*) malloc_or_die(payload_len);
       memcpy(payload, icmp_packet + ICMP_HEADER_LEN, payload_len);
       // check icmp type
       printf(" ** icmp_handle_packet(..) checking type \n");
       check_packet_type(router, icmp_header, payload, payload_len, ip_header, intf);
    } else
    printf(" ** icmp_handle_packet(..) checksum failed, dropping!\n");
    //free(icmp_packet);
    //free(ip_header);
}

void display_icmp_header(icmp_header_t* header) {
    if(header != NULL) {
       printf(" ** type: %u, code: %u, checksum: %u, rest of header: %lu\n", header->type, header->code, header->checksum, header->rest);
    }
}

bool check_icmp_header(icmp_header_t* header, byte* raw_packet, uint8_t len) {
    // create raw packet with checksum zero
    byte* sum_packet = (byte*) malloc_or_die(len);
    memcpy(sum_packet, raw_packet, len);
    uint16_t sum_zero = 0;
    memcpy(sum_packet + 2, &sum_zero  ,2);
    if(ntohs(inet_chksum((void*) sum_packet, len)) != header->checksum)
       return FALSE;
    return TRUE;
}

void check_packet_type(sr_router* router, icmp_header_t* header, byte* payload, uint8_t payload_len, struct ip* ip_header, interface_t* intf) {
   if(header->type == ICMP_TYPE_ECHO_REPLY) {
      //echo reply
      printf(" ** icmp_handle_packet(..) type: echo reply \n");
      icmp_type_echo_reply(router, header->code, header->rest, payload, payload_len, ip_header, intf);
   } else if(header->type == ICMP_TYPE_DST_UNREACH) {
      //destination unreachable
      printf(" ** icmp_handle_packet(..) type: destination unreachable \n");
      icmp_type_dst_unreach(header->code, header->rest, payload, payload_len, ip_header, intf);
   } else if(header->type == ICMP_TYPE_ECHO_REQUEST) {
      //echo request
      printf(" ** icmp_handle_packet(..) type: echo request \n");
      icmp_type_echo_request(header->code, header->rest, payload, payload_len, ip_header, intf);
   } else if(header->type == ICMP_TYPE_TTL) {
      //ttl exceeded
      printf(" ** icmp_handle_packet(..) type: ttl exceeded \n");
      icmp_type_ttl(header->code, header->rest, payload, payload_len, ip_header, intf);
   } else if(header->type == ICMP_TYPE_TRACEROUTE) {
      //traceroute
      printf(" ** icmp_handle_packet(..) type: traceroute \n");
      icmp_type_traceroute(header->code, header->rest, payload, payload_len, ip_header, intf);
   } else if(header->type == ICMP_TYPE_BAD_IP) {
      //bad ip header
      printf(" ** icmp_handle_packet(..) type: bad ip header \n");
      icmp_type_bad_ip(header->code, header->rest, payload, payload_len, ip_header, intf);
   }
}

void make_icmp_response_packet(uint8_t* type, uint8_t* code, uint32_t* rest, byte* data, uint8_t* data_len, struct ip* ip_header, interface_t* intf) {
   printf(" ** make_icmp_response_packet(..) called \n");
   // make raw icmp packet
   byte* icmp_packet = (byte*) malloc_or_die(ICMP_HEADER_LEN + *data_len);
   memcpy(icmp_packet, type, 1);
   memcpy(icmp_packet + 1, code, 1);
   uint16_t zero_checksum = 0;
   memcpy(icmp_packet + 2, &zero_checksum, 2);
   memcpy(icmp_packet + 4, rest, 4);
   memcpy(icmp_packet + ICMP_HEADER_LEN, data, *data_len);
   // generate checksum
   uint16_t checksum = htons(generate_checksum(icmp_packet, ICMP_HEADER_LEN + *data_len));
   // add checksum to raw packet
   memcpy(icmp_packet + 2, &checksum, 2);
   printf(" ** make_icmp_response_packet(..) packet with length %u generated \n", *data_len + ICMP_HEADER_LEN);
   make_ip_packet_reply(icmp_packet, ICMP_HEADER_LEN + *data_len, ip_header->ip_src, ip_header->ip_dst, IP_PROTOCOL_ICMP, intf);
}

uint16_t generate_checksum(byte* packet, uint8_t packet_len) {
   return htons(inet_chksum((void*) packet, packet_len));   
}

void make_icmp_send_packet(uint8_t* type, uint8_t* code, uint32_t* rest, byte* data, uint8_t* data_len, struct ip* ip_header) {
   printf(" ** make_icmp_send_packet(..) called \n");
   // make raw icmp packet
   byte* icmp_packet = (byte*) malloc_or_die(ICMP_HEADER_LEN + *data_len);
   memcpy(icmp_packet, type, 1);
   memcpy(icmp_packet + 1, code, 1);
   uint16_t zero_checksum = 0;
   memcpy(icmp_packet + 2, &zero_checksum, 2);
   memcpy(icmp_packet + 4, rest, 4);
   memcpy(icmp_packet + ICMP_HEADER_LEN, data, *data_len);
   // generate checksum
   uint16_t checksum = htons(generate_checksum(icmp_packet, ICMP_HEADER_LEN + *data_len));
   // add checksum to raw packet
   memcpy(icmp_packet + 2, &checksum, 2);
   printf(" ** make_icmp_send_packet(..) packet with length %u generated \n", *data_len + ICMP_HEADER_LEN);
   make_ip_packet(icmp_packet, ICMP_HEADER_LEN + *data_len, ip_header->ip_src, ip_header->ip_dst, IP_PROTOCOL_ICMP);
   //free ip header
   //free(ip_header);
}

