#pragma once

/*
 * Filename: sr_arp_cache.h
 * Purpose:
 *   1) Maintain dynamic list of MAC<->IP pairs
 *   2) Expire old entries from the dynamic list
 *   3) Maintain list of static MAC<->IP pairs
 *   4) Provide functionality for searching for and fetching MAC<->IP pairs
 */

#include <pthread.h>
#include <time.h>

#include "sr_common.h"
#include "packet_dispatcher.h"
#include "sr_arp_cache_entry.h"
#include "linked_list.h"

struct sr_router;

/* ARP cache */
typedef struct arp_cache_t {
  /* 
   * TODO: A suitable structure for your ARP cache
   *
   * You will probably want to make use of pthread_mutex_t from pthread.h and
   * packet_dispatcher_t from packet_dispatcher.h.  The array list type,
   * array_list_t, defined in array_list.h may also be useful.
   *
   * You'll need at least one lock to prevent different threads reading/writing
   * the cache simultaneously and thereby corrupting it.  If you have multiple
   * locks, be careful of the order in which they're locked in order to avoid
   * deadlocks.
   *
   * You'll also need at least two different tables/lists: one for the dynamic
 * cache, which maps IPs to MACs and is continuously updated; and one for ARP
   * requests which have been sent but for which a reply has not yet been
   * received, to prevent excessive ARP requests being sent.
   *
   * You could also have a static cache to allow you to control the cache from
   * the telnet command line interface, but this is optional.
   */
   node *arp_cache_list;
   node *arp_static_cache_list;
   node *arp_request_queue;
   pthread_mutex_t lock_static;
   pthread_mutex_t lock_dynamic;
   pthread_mutex_t lock_queue;
   packet_dispatcher_t *packet_dispatcher;
   pthread_t time_out_thread;
} arp_cache_t;


/**
 * Initialize the ARP response cache and starts a new detached thread which will
 * scrub old entries from the cache.
 */
void arp_cache_init( struct sr_router* router );

/**
 * Destroys an existing ARP cache (frees the memory associated with it and its
 * members).
 */
void cache_destroy( arp_cache_t* cache );

/**
 * Add a static entry to the static ARP cache.  Used by the telnet command
 * line interface.
 * @return true if succeeded (fails if the max # of static entries are already
 *         in the cache).
 */
bool cache_static_entry_add( struct sr_router* router,
                                 addr_ip_t ip,
                                 addr_mac_t* mac );

/**
 * Remove a static entry to the static ARP cache.
 * @return true if succeeded (false if ip wasn't in the cache as a static entry)
 */
bool cache_static_entry_remove( struct sr_router* router, addr_ip_t ip );


/**
 * Remove all static entries from the ARP cache.
 * @return  number of static entries removed
 */
unsigned cache_static_purge( struct sr_router* router );

/**
 * Add a dynamic entry to the dynamic ARP cache.  Used by the telnet command
 * line interface.
 * @return true if succeeded (fails if the max # of dynamic entries are already
 *         in the cache).
 */
bool cache_dynamic_entry_add( struct sr_router* router,
                                 addr_ip_t ip,
                                 addr_mac_t* mac );

/**
 * Remove a dynamic entry to the dynamic ARP cache.
 * @return true if succeeded (false if ip wasn't in the cache as a dynamic entry)
 */
bool cache_dynamic_entry_remove( struct sr_router* router, addr_ip_t ip );


/**
 * Remove all dynamic entries from the ARP cache.
 * @return  number of dynamic entries removed
 */
unsigned cache_dynamic_purge( struct sr_router* router );

/*Runs the cache timeout thread*/
void time_out_run(struct sr_router* router);

/*Removed timedout entires from both caches*/
void remove_timed_out(void* router_input); 

/**
 * Fetches the HW address associated with the specified IP address and sets it
 * as the destination MAC address.  If the MAC address cannot be found and the
 * ethernet frame's payload is an IP packet, then ip_handle_failed_packet() is
 * called to determine whether a host unreachable message needs to be sent.
 *
 * @param dst_ip  the IP whose corresponding MAC needs to be placed in the frame
 * @param ethernet_frame  the ethernet frame which needs its dst MAC (will be
 *                        freed by this method).
 * @param len number of bytes in the ethernet_frame
 *
 * @return 0 if the send call failed (had ARP address)
 *         1 if it is delayed (needs ARP could fail), or
 *         2 if the packet was sent right away, or
 */

int arp_cache_handle_partial_frame( struct sr_router* router,
                                    interface_t* intf,
                                    addr_ip_t dst_ip,
                                    byte* payload /* given */,
                                    unsigned len, 
                                    uint16_t type);

/**
 * Fills buf with a string representation of the arp cache.
 *
 */
void arp_cache_to_string();
