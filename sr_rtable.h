#pragma once
/*
 * Filename: sr_rtable.h
 * Purpose: define the routing table and operations on it
 */

#include <pthread.h>
#include "sr_common.h"
#include "sr_interface.h"
#include "linked_list.h"

#include <pthread.h>

struct sr_router;

/**
 * holds an IP-interface route pair (network byte order for the IP!)
 */
typedef struct {
    addr_ip_t ip;
    addr_ip_t mask;
    interface_t* intf;
} route_info_t;

/** contains information about a route */
typedef struct route_t {
  char type; // 's' for static, 'd' for dynamic
  addr_ip_t destination;
  interface_t intf;
  addr_ip_t next_hop;
  addr_ip_t subnet_mask;
  
} route_t;

/** routing table data structure */
typedef struct {
   node* rtable_list;
   pthread_mutex_t lock_rtable;
} rtable_t;

// TODO: Update the maximum length of the string representation of an rtable
// depending on what rtable_to_string returns
// SR_NAMELEN is #defined in sr_interface.h and is the maximum length of the
// name of an interface.
#define STR_MAX_ROUTES 100
#define STR_RTABLE_MAX_LEN (85+STR_MAX_ROUTES*(52+SR_NAMELEN))

/** Initialize the routing table. */
void rrtable_init(struct sr_router *router);

/** Destroys the object and routes referenced by rtable. */
void rrtable_destroy(rtable_t *rtable);

/**
 * Finds the route with the longest matching prefix for dest_ip.  Both the
 * static and dynamic routing tables are searched.
 *
 * @param dest_ip  the destination to find the next hop for
 *
 * @return information(interface) about the route to take (intf field will be NULL if a
 *         route does not exist).
 */
route_info_t rrtable_find_route(rtable_t *rtable, uint32_t dest_ip);

/** Adds a route to the routing table. */
void rrtable_route_add( rtable_t *rtable,
                       addr_ip_t dest, addr_ip_t gw, addr_ip_t mask,
                       interface_t* intf, char type );

/** Removes the specified route from the routing table, if present. */
bool rrtable_route_remove( rtable_t *rtable,
                          addr_ip_t dest, addr_ip_t mask );

/** Remove all routes from the router. */
void rrtable_purge_all( rtable_t* rtable );

/** Puts a human-readable string representation of the routing table in buf */
void rrtable_to_string();

/** Returns sizeof(rtable_t), to allow dynamic allocation of rtable_t memory from within sr.o */
unsigned int sizeof_rtable(void);

route_t* make_route_t(char type, addr_ip_t dst, interface_t intf, addr_ip_t next_hop, addr_ip_t subnet_mask);

void rrtable_write_hw();

void rrtable_read_hw();

void rrtable_purge_all_type( rtable_t* rtable, char type );
