#pragma once

#include "sr_common.h"
#include "sr_interface.h"
#include "sr_ip.h"

#include <netinet/ip.h>
#include <arpa/inet.h>

#define UDP_HEADER_LEN 8

typedef struct udp_header_t {
   uint16_t src_port; 
   uint16_t dst_port;
   uint16_t len; 
   uint16_t checksum;
} udp_header_t;

typedef struct pseudo_ip_header_t {
   struct in_addr src_ip;
   struct in_addr dst_ip;
   uint8_t zeros;
   uint8_t protocol;
   uint16_t udp_len;
} pseudo_ip_header_t;

bool check_udp_header(udp_header_t* header, byte* raw_header, struct ip* ip_header);

void udp_handle_packet(byte* udp_packet, struct ip* ip_header, 
                        interface_t* intf );

udp_header_t* make_udp_header(byte* udp_packet);

void display_udp_header(udp_header_t* header);

pseudo_ip_header_t* make_pseudo_ip_header(struct ip* header, uint16_t udp_len);
