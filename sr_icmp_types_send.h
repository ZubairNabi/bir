#pragma once

#include "sr_router.h"
#include "sr_interface.h"

#include <netinet/ip.h>


void icmp_type_dst_unreach_send(uint8_t* code, byte* packet, uint8_t* packet_len, struct ip* ip_header);

void icmp_type_echo_request_send(sr_router* router, struct in_addr dst, uint16_t seq, int fd);

void icmp_type_ttl_send(uint8_t* code, byte* packet, uint8_t* packet_len, struct ip* ip_header);

void icmp_type_traceroute_send(uint32_t* rest, byte* packet, uint8_t* packet_len, struct ip* ip_header);

void icmp_type_bad_ip_send(uint8_t* code, uint32_t* rest, byte* packet, uint8_t* packet_len, struct ip* ip_header);
