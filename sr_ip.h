#pragma once

#include <netinet/ip.h>
#include "sr_common.h"
#include "sr_router.h"
#include "sr_interface.h"

#define IP_4_VERSION 4
#define IP_HEADER_MIN_LEN 20
#define IP_HEADER_MAX_LEN 60
#define IP_HEADER_MIN_LEN_WORD 5
#define IP_HEADER_MAX_LEN_WORD 15
#define IP_PACKET_MIN_LEN 20
#define IP_PACKET_MAX_LEN 65535
#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_TCP 6
#define IP_PROTOCOL_UDP 17
#define IP_PROTOCOL_OSPF 89
#define IP_TOS_DEFAULT 0
#define IP_ID 0
#define IP_TTL_DEFAULT 64
#define IP_OFFSET_DF 16384
#define IP_OFFSET_NF 0

bool check_ip_header(struct ip* header, byte* raw_header);

void ip_handle_packet( sr_router* router,
                        byte* ip_packet,
                        interface_t* intf );

struct ip* make_ip_header(byte* ip_packet);

void display_ip_header(struct ip*);

bool check_packet_destination(struct in_addr ip_dst, sr_router *router);

void check_packet_protocol(sr_router* router, struct ip* ip_header, byte* payload, uint16_t payload_len, interface_t* intf, byte* ip_packet);

void make_ip_packet_reply(byte* payload, uint16_t payload_len, struct in_addr src, struct in_addr dst, uint8_t protocol, interface_t* intf);

uint16_t generate_checksum_ip_header(struct ip* header, uint16_t len);

void ip_look_up(byte* ip_packet, struct ip* ip_header);

void send_ip_packet(byte* ip_packet, struct ip* ip_header, route_info_t route_info);

bool check_if_local_subnet(addr_ip_t dst, addr_ip_t src, addr_ip_t subnet_mask);

void make_ip_packet(byte* payload, uint16_t payload_len, struct in_addr src, struct in_addr dst, uint8_t protocol);

void ip_look_up_reply(byte* ip_packet, struct ip* ip_header, interface_t* intf);

bool check_packet_source(struct in_addr ip_src, sr_router* router);
