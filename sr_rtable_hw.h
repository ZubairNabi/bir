#pragma once
/*
 * Filename: sr_rtable_hw.h
 * Purpose: define the hw routing table and operations on it
 */

#include <pthread.h>
#include "sr_common.h"
#include "sr_interface.h"
#include "linked_list.h"

#include <pthread.h>

struct sr_router;

/** contains information about a route */
typedef struct hw_route_t {
  addr_ip_t destination;
  uint32_t ports;
  addr_ip_t next_hop;
  addr_ip_t subnet_mask;
  
} hw_route_t;

/** routing table data structure */
typedef struct {
   node* hw_rtable_list;
   pthread_mutex_t hw_lock_rtable;
} hw_rtable_t;

/** Initialize the routing table. */
void hw_rrtable_init(struct sr_router *router);

void hw_rrtable_destroy(hw_rtable_t *rtable);

/** Adds a route to the routing table. */
void hw_rrtable_route_add( hw_rtable_t *rtable,
                       addr_ip_t dest, addr_ip_t gw, addr_ip_t mask,
                       uint32_t ports);

/** Removes the specified route from the routing table, if present. */
bool hw_rrtable_route_remove( hw_rtable_t *rtable,
                          addr_ip_t dest, addr_ip_t mask );

/** Remove all routes from the router. */
void hw_rrtable_purge_all( hw_rtable_t* rtable );

hw_route_t* hw_make_route_t(addr_ip_t dst, uint32_t ports, addr_ip_t next_hop, addr_ip_t subnet_mask);

void hw_rrtable_to_string();

void hw_rrtable_write_hw();

void hw_rrtable_read_hw();
