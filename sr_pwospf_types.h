#pragma once

#include "sr_pwospf.h"

void pwospf_hello_type(struct ip* ip_header, byte* payload, uint8_t payload_len, pwospf_header_t* pwospf_header, interface_t* intf);

void pwospf_lsu_type(struct ip* ip_header, byte* payload, uint8_t payload_len, pwospf_header_t* pwospf_header, interface_t* intf);

void display_hello_packet(pwospf_hello_packet_t* packet);

void display_lsu_packet(pwospf_lsu_packet_t* packet);

void pwospf_send_lsu();
