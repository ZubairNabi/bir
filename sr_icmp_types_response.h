#pragma once

#include "sr_router.h"
#include "sr_interface.h"

#include <netinet/ip.h>

void icmp_type_echo_reply_response(sr_router* router, uint32_t* rest, byte* packet, uint8_t* packet_len, struct ip* ip_header, interface_t* intf);

void icmp_type_dst_unreach_response(uint8_t* code, byte* packet, uint8_t* packet_len, struct ip* ip_header, interface_t* intf);

void icmp_type_echo_request_response(uint32_t *rest, byte* data, uint8_t *data_len, struct ip* ip_header, interface_t* intf);

void icmp_type_ttl_response(uint8_t* code, byte* packet, uint8_t* packet_len, struct ip* ip_header, interface_t* intf);

void icmp_type_traceroute_response(uint32_t* rest, byte* packet, uint8_t* packet_len, struct ip* ip_header, interface_t* intf);

void icmp_type_bad_ip_response(uint8_t* code, uint32_t* rest, byte* packet, uint8_t* packet_len, struct ip* ip_header, interface_t* intf);
