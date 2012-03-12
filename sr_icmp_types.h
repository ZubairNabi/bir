#pragma once

#include "sr_icmp.h"
#include "sr_router.h"
#include "sr_interface.h"

#include <netinet/ip.h>

void icmp_type_echo_reply(sr_router* router, uint8_t code, uint32_t rest, byte* packet, uint16_t packet_len, struct ip* ip_header, interface_t* intf);

void icmp_type_dst_unreach(uint8_t code, uint32_t rest, byte* packet, uint16_t packet_len, struct ip* ip_header, interface_t* intf);

void icmp_type_echo_request(uint8_t code, uint32_t rest, byte* packet, uint16_t packet_len, struct ip* ip_header, interface_t* intf);

void icmp_type_ttl(uint8_t code, uint32_t rest, byte* packet, uint16_t packet_len, struct ip* ip_header, interface_t* intf);

void icmp_type_traceroute(uint8_t code, uint32_t rest, byte* packet, uint16_t packet_len, struct ip* ip_header, interface_t* intf);

void icmp_type_bad_ip(uint8_t code, uint32_t rest, byte* packet, uint16_t packet_len, struct ip* ip_header, interface_t* intf);
