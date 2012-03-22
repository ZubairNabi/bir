/*
 * Filename: sr_neighbor.h
 * Purpose: Data structure containing information about neighbours of an interfa * ce
 */

#ifndef SR_NEIGHBOR_H
#define SR_NEIGHBOR_H

#include "sr_common.h"
#include "sr_interface.h"
#include "sr_pwospf.h"

typedef struct neighbor_t {
   uint32_t id;
   addr_ip_t ip;
   struct timeval *timestamp;
   uint16_t helloint;
   addr_ip_t mask;
   pwospf_lsu_packet_t last_lsu_packet;
   byte* last_adverts;
} neighbor_t;

neighbor_t* make_neighbor(uint32_t id, addr_ip_t ip, uint16_t helloint, addr_ip_t mask);

void add_neighbor(interface_t* intf, uint32_t id, addr_ip_t ip, uint16_t helloint, addr_ip_t mask);

neighbor_t* make_neighbor_lsu(uint32_t id, addr_ip_t ip, uint16_t helloint, addr_ip_t mask, pwospf_lsu_packet_t lsu_packet, byte* adverts);

interface_t* get_interface_from_ip(sr_router* router, uint32_t ip);

interface_t* get_interface_from_id(sr_router* router, uint32_t id);


#endif
