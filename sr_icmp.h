#pragma once

#include "sr_common.h"
#include "sr_router.h"
#include "sr_interface.h"

#include <netinet/ip.h>

#define ICMP_HEADER_LEN 8
#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_DST_UNREACH 3
#define ICMP_TYPE_ECHO_REQUEST 8
#define ICMP_TYPE_TTL 11
#define ICMP_TYPE_TRACEROUTE 30
#define ICMP_TYPE_BAD_IP 12
#define ICMP_TYPE_CODE_ECHO_REPLY 0
#define ICMP_TYPE_CODE_DST_UNREACH_NETWORK 0
#define ICMP_TYPE_CODE_DST_UNREACH_HOST 1
#define ICMP_TYPE_CODE_DST_UNREACH_PROTOCOL 2
#define ICMP_TYPE_CODE_DST_UNREACH_PORT 3
#define ICMP_TYPE_CODE_DST_UNREACH_FRAG 4
#define ICMP_TYPE_CODE_DST_UNREACH_SRC_ROUTE 5
#define ICMP_TYPE_CODE_DST_UNREACH_NETWORK_UNKNOWN 6
#define ICMP_TYPE_CODE_DST_UNREACH_HOST_UNKNOWN 7
#define ICMP_TYPE_CODE_ECHO_REQUEST 0
#define ICMP_TYPE_CODE_TTL_TRANSIT 0
#define ICMP_TYPE_CODE_TTL_FRAG 1
#define ICMP_TYPE_CODE_BAD_IP_POINTER 0
#define ICMP_TYPE_CODE_BAD_IP_MISSING 1
#define ICMP_TYPE_CODE_BAD_IP_LENGTH 2
#define ICMP_TYPE_CODE_TRACEROUTE 0

typedef struct icmp_header_t {
   uint8_t type;
   uint8_t code;
   uint16_t checksum;
   uint32_t rest;
} icmp_header_t;

icmp_header_t* make_icmp_header(byte* icmp_packet);

void icmp_handle_packet(sr_router* router, byte* icmp_packet,
                        uint8_t packet_len, struct ip* ip_header, interface_t* intf); 

void display_icmp_header(icmp_header_t* header);

bool check_icmp_header(icmp_header_t* header, byte* raw_packet, uint8_t len);

void check_packet_type(sr_router* router, icmp_header_t* header, byte* payload, uint8_t payload_len, struct ip* ip_header, interface_t* intf);

void make_icmp_response_packet(uint8_t* type, uint8_t* code, uint32_t* rest, byte* data, uint8_t* data_len, struct ip* ip_header, interface_t* intf);

uint16_t generate_checksum(byte* packet, uint8_t packet_len);

void make_icmp_send_packet(uint8_t* type, uint8_t* code, uint32_t* rest, byte* data, uint8_t* data_len, struct ip* ip_header);

