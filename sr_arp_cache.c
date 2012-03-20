/* Filename: sr_arp_cache.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#include "sr_common.h"
#include "sr_ethernet.h"
#include "sr_router.h"
#include "sr_arp_cache.h"
#include "cli/helper.h"
#include "lwtcp/lwip/sys.h"
#include "packet_dispatcher.h"
#include "linked_list.h"
#include "sr_arp_cache_entry.h"
#include "sr_integration.h"
#include "sr_arp.h"
#include "sr_icmp.h"
#include "sr_ip.h"
#include "sr_icmp_types_send.h"
#include "cli/cli.h"
#include "reg_defines.h"

// Interval for the ARP cache garbage collector thread in seconds
#define ARP_GC_INTERVAL_S 5

// The time after which entries are removed from the ARP cache
#define ARP_CACHE_TIMEOUT 30

// The time after which packets are removed from the queue
#define PACKET_TIMEOUT 2

/**
 * Initialize the ARP response cache and starts a new detached thread which will
 * scrub old entries from the cached as defined by ARP_GC_INTERVAL_S.
 * If initialisation failed, router->arp_cache is set to NULL.
 *
 * @param router  The router whose ARP cache (router->arp_cache) will be inited
 */
void arp_cache_init( sr_router* router ) {
   printf(" ** arp_cache_init(..) called \n");
   // Initialize cache
   router->arp_cache = (arp_cache_t*) malloc_or_die(sizeof(arp_cache_t));
   // Initialize cache lists
   router->arp_cache->arp_cache_list = llist_new();
   router->arp_cache->arp_static_cache_list = llist_new();
   router->arp_cache->arp_request_queue = llist_new();
   // Initialize mutexes
   pthread_mutex_init(&router->arp_cache->lock_static, NULL);
   pthread_mutex_init(&router->arp_cache->lock_dynamic, NULL);
   pthread_mutex_init(&router->arp_cache->lock_queue, NULL);
   // Initialize packet dispatcher
   router->arp_cache->packet_dispatcher = packet_dispatcher_new();
   packet_dispatcher_run(router->arp_cache->packet_dispatcher);
   // Run cache timeout thread
   time_out_run(router);
}

/**
 * Destroys an existing ARP cache (frees the memory associated with it and its
 * members).
 *
 * @param cache  The ARP cache to destroy
 */
void cache_destroy( arp_cache_t* cache ) {
   printf(" ** arp cache_destroy(..) called\n");
   pthread_mutex_lock(&cache->lock_dynamic);
   cache->arp_cache_list = llist_delete_no_count(cache->arp_cache_list);
   pthread_mutex_unlock(&cache->lock_dynamic);

   pthread_mutex_lock(&cache->lock_static);
   cache->arp_static_cache_list = llist_delete_no_count(cache->arp_static_cache_list);
   pthread_mutex_unlock(&cache->lock_static);

   pthread_mutex_unlock(&cache->lock_queue);
   cache->arp_request_queue = llist_delete_no_count(cache->arp_request_queue);
   pthread_mutex_unlock(&cache->lock_queue);

   pthread_mutex_destroy(&cache->lock_static);
   pthread_mutex_destroy(&cache->lock_dynamic);
   pthread_mutex_destroy(&cache->lock_queue);
   pthread_cancel(cache->time_out_thread);
}

/**
 * Add a static entry to the static ARP cache.  Used by the telnet command
 * line interface.
 * @return true if succeeded (fails if the max # of static entries are already
 *         in the cache).
 */
bool cache_static_entry_add( sr_router* router,
                                 addr_ip_t ip,
                                 addr_mac_t* mac ) {
  printf(" ** arp cache_static_entry_add(..) called\n");
  arp_cache_entry_t *entry = (arp_cache_entry_t*) malloc_or_die(sizeof(arp_cache_entry_t));
  entry->ip = ip;
  entry->mac = *mac;
  entry->timeout = (struct timeval*) malloc_or_die(sizeof(struct timeval));
  gettimeofday(entry->timeout, NULL);
  entry->timeout->tv_sec = entry->timeout->tv_sec + ARP_CACHE_TIMEOUT;
  pthread_mutex_lock(&router->arp_cache->lock_static);
  router->arp_cache->arp_static_cache_list = llist_update_beginning(router->arp_cache->arp_static_cache_list, predicate_ip,(void*) entry);   
  pthread_mutex_unlock(&router->arp_cache->lock_static);
  // update hw
  arp_write_hw();
  return TRUE;
}

/**
 * Remove a static entry to the static ARP cache.
 * @return true if succeeded (false if ip wasn't in the cache as a static entry)
 */
bool cache_static_entry_remove( sr_router* router, addr_ip_t ip ) {
  printf(" ** arp cache_static_entry_remove(..) called\n");
  pthread_mutex_lock(&router->arp_cache->lock_static);
  node *ret = (node*) llist_remove(router->arp_cache->arp_static_cache_list, predicate_ip, (void*)&ip);
  pthread_mutex_unlock(&router->arp_cache->lock_static);
  if(ret != NULL) {
     router->arp_cache->arp_static_cache_list = ret;
     // update hw
     arp_write_hw();
     return TRUE;
  }
  return FALSE;
}

/**
 * Remove all static entries from the ARP cache.
 * @return  number of static entries removed
 */
unsigned cache_static_purge( struct sr_router* router ) {
  printf(" ** arp cache_static_purge(..) called\n");
  int ret = 0;
  pthread_mutex_lock(&router->arp_cache->lock_static);
  router->arp_cache->arp_static_cache_list = llist_delete(router->arp_cache->arp_static_cache_list, &ret);
  pthread_mutex_unlock(&router->arp_cache->lock_static);
  // update hw
  arp_write_hw();
  return ret;
}

/**
 * Add a dynamic entry to the dynamic ARP cache.  Used by the telnet command
 * line interface.
 * @return true if succeeded (fails if the max # of dynamic entries are already
 *         in the cache).
 */
bool cache_dynamic_entry_add( struct sr_router* router,
                                 addr_ip_t ip,
                                 addr_mac_t* mac ) {
  printf(" ** arp cache_dynamic_entry_add(..) called\n");
  arp_cache_entry_t *entry = (arp_cache_entry_t*) malloc_or_die(sizeof(arp_cache_entry_t));
  entry->ip = ip;
  entry->mac = *mac;
  entry->timeout = (struct timeval*) malloc_or_die(sizeof(struct timeval));
  gettimeofday(entry->timeout, NULL);
  entry->timeout->tv_sec = entry->timeout->tv_sec + ARP_CACHE_TIMEOUT;
  pthread_mutex_lock(&router->arp_cache->lock_dynamic);
  router->arp_cache->arp_cache_list = llist_update_beginning(router->arp_cache->arp_cache_list, predicate_ip, (void*) entry);
  pthread_mutex_unlock(&router->arp_cache->lock_dynamic);
  // update hw
  arp_write_hw();
  return TRUE;
}

/**
 * Remove a dynamic  entry from the dynamic ARP cache.
 * @return true if succeeded (false if ip wasn't in the cache as a dynamic entry)
 */
bool cache_dynamic_entry_remove( sr_router* router, addr_ip_t ip ) {
  printf(" ** arp dynamic_entry_remove(..) called\n");
  pthread_mutex_lock(&router->arp_cache->lock_dynamic);
  node *ret = (node*) llist_remove(router->arp_cache->arp_cache_list, predicate_ip, (void*)&ip);
  pthread_mutex_unlock(&router->arp_cache->lock_dynamic);
  if(ret != NULL) {
     router->arp_cache->arp_cache_list = ret;
      // update hw
      arp_write_hw();
     return TRUE;
  }
  return FALSE;

}

/**
 * Remove all dynamic entries from the ARP cache.
 * @return  number of dynamic entries removed
 */
unsigned cache_dynamic_purge( struct sr_router* router ) {
  printf(" ** arp cache_dynamic_purge(..) called\n");
  int ret = 0;
  pthread_mutex_lock(&router->arp_cache->lock_dynamic);
  router->arp_cache->arp_cache_list = llist_delete(router->arp_cache->arp_cache_list, &ret);
  pthread_mutex_unlock(&router->arp_cache->lock_dynamic);
  // update hw
  arp_write_hw();
  return ret;
}

/*Function to remove timed-out entries from both caches*/

void remove_timed_out(void* router_input) {
   printf(" ** arp time out thread called\n");
   sr_router* router = (sr_router*) router_input;
   struct timespec *timeout =(struct timespec*) malloc_or_die(sizeof(struct timespec));
   struct timespec *timeout_rem =(struct timespec*) malloc_or_die(sizeof(struct timespec));
   timeout->tv_sec = (time_t) ARP_GC_INTERVAL_S;
   timeout->tv_nsec = 0;
   while(1) {
      // sleep for interval
      nanosleep(timeout, timeout_rem);
      printf(" ** arp time out thread awoken\n");
      // get current time
      struct timeval *current_time = (struct timeval*)malloc_or_die(sizeof(struct timeval));
      gettimeofday(current_time, NULL);
      // remove timed out entries from dynamic list
      // lock list
      pthread_mutex_lock(&router->arp_cache->lock_dynamic);
      // display current list
      llist_display_all(router->arp_cache->arp_cache_list, display_cache_entry);
      // remove all timed out 
      if(current_time == NULL)
         printf("Current time is null\n");
      router->arp_cache->arp_cache_list = llist_remove_all_no_count(router->arp_cache->arp_cache_list, predicate_timeval2, (void*)current_time);
      // unlock list
      pthread_mutex_unlock(&router->arp_cache->lock_dynamic);
      // update hw
      arp_write_hw();
   }
}


void time_out_run(struct sr_router* router) {
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
   sys_thread_new(remove_timed_out, (void*) router);
}

/**
 * Fetches the HW address associated with the specified IP address and sets it
 * as the destination MAC address, then sends the packet.  If the IP cannot be
 * resolved to a MAC, sends an ICMP host unreachable back to the sender (except
 * where a host unreachable would be inappropriate).
 *
 * @param dst_ip  the IP whose corresponding MAC needs to be placed in the frame
 * @param ethernet_frame  the ethernet frame which needs its dst MAC (will be
 *                        freed by this method).
 * @param len number of bytes in the ethernet_frame
 *
 * @return 0 if the send call failed (had ARP address), or
 *         1 if it is delayed (needs ARP, could fail), or
 *         2 if the packet was sent right away
 */
int arp_cache_handle_partial_frame( struct sr_router* router,
                                    interface_t* intf,
                                    addr_ip_t dst_ip,
                                    byte* payload /* given */,
                                    unsigned len,
                                    uint16_t type) {
  /*
   * This functions handles all outgoing packets.  There are three conditions
   * that could occur for an outgoing packet:
   * a) The IP address is already in the ARP cache.  In this case the
   *    corresponding MAC address just needs to be copied into the destination
   *    MAC address in the ethernet header of the packet and the packet sent.
   *    Note that the MAC address can just be copied from a addr_mac_t with
   *    memcpy, and the packet can be sent with sr_integ_low_level_output() from
   *    sr_integration.h.
   * b) The IP address is not in the ARP cache but we have recently sent an ARP
   *    request for this IP.  In this case, the packet should be queued for when
   *    the ARP reply arrives.
   * c) The IP address is not in the ARP cache and we haven't recently sent an
   *    ARP request for that IP.  In this case, we need to send an ARP request
   *    and queue the packet.
   * For queued packets, you could (should?) make use of the functions and
   * structures in packet_dispatcher.h.  You can queue any packets with the
   * packet dispatcher using packet_dispatcher_put_packet() and tell it about
   * any updates to the cache using packet_dispatcher_put_update().  It will
   * send out any packets, ICMP host unreachables, etc. that are appropriate
   */

   printf(" ** arp_cache_handle_partial_frame(..) called \n");
   printf(" ** arp_cache_handle_partial_frame(..): packet of len: %u \n", len);
   node* ret, *q_ret;
   pthread_mutex_lock(&router->arp_cache->lock_dynamic);
   ret = llist_find(router->arp_cache->arp_cache_list, predicate_ip, (void*) &dst_ip);
   pthread_mutex_unlock(&router->arp_cache->lock_dynamic);

   if(ret != NULL) {
      printf(" ** arp_cache_handle_partial_frame(..): mac found in cache\n");
      arp_cache_entry_t* entry_dynamic = (arp_cache_entry_t*) ret->data;
      ethernet_send_frame(&entry_dynamic->mac, intf, type, payload, len);
      return 2;
   } else {
      pthread_mutex_lock(&router->arp_cache->lock_static);
      ret = llist_find(router->arp_cache->arp_static_cache_list, predicate_ip, (void*) &dst_ip);
      pthread_mutex_unlock(&router->arp_cache->lock_static);
      if(ret != NULL) {
          printf(" ** arp_cache_handle_partial_frame(..): mac found in static cache\n");
          arp_cache_entry_t* entry_static = (arp_cache_entry_t*) ret->data;
          ethernet_send_frame(&entry_static->mac, intf, type, payload, len);
          return 2;
      }
      else {
         pthread_mutex_lock(&router->arp_cache->lock_queue);
         q_ret = llist_find(router->arp_cache->arp_request_queue, predicate_ip_arp_queue_entry_t, (void*) &dst_ip);
         pthread_mutex_unlock(&router->arp_cache->lock_queue);
         if(q_ret == NULL) {
            printf(" ** arp_cache_handle_partial_frame(..): Sending ARP request\n");
            arp_queue_entry_t *item = (arp_queue_entry_t*) malloc_or_die(sizeof(arp_queue_entry_t));
            item->ip = dst_ip;
            item->tries = 1;
            pthread_mutex_lock(&router->arp_cache->lock_queue);
            router->arp_cache->arp_request_queue = llist_insert_beginning(router->arp_cache->arp_request_queue, (void*) item);
            pthread_mutex_unlock(&router->arp_cache->lock_queue);
            // Send ARP Request
            arp_send_request( dst_ip, intf );
            packet_dispatcher_put_packet(router->arp_cache->packet_dispatcher, dst_ip, payload, len, intf, Q_PACKET_TIMEOUT );
            return 1;
          } else {
            printf(" ** arp_cache_handle_partial_frame(..): ARP request recently sent\n");
            //check if request sent 5 times
            arp_queue_entry_t *item = (arp_queue_entry_t*) q_ret->data;
            if(item->tries >=5) {
               printf(" ** arp_cache_handle_partial_frame(..): Host hasn't replied to 5 requests, sending ICMP host unreachable\n");
            // send icmp destination host unreachable
            uint8_t code = ICMP_TYPE_CODE_DST_UNREACH_HOST;
            // get original ip header
            struct ip* ip_header = make_ip_header(payload);
            uint16_t packet_len = ip_header->ip_len;
            icmp_type_dst_unreach_send(&code, payload, &packet_len, ip_header);

            } else {
               // increment tries
               item->tries = item->tries + 1;
                printf(" ** arp_cache_handle_partial_frame(..): Sending ARP request tries: %d\n", item->tries);
               // put item back on queue
               pthread_mutex_lock(&router->arp_cache->lock_queue);
               router->arp_cache->arp_request_queue = llist_update_beginning_delete(router->arp_cache->arp_request_queue, predicate_ip_arp_queue_entry_t, (void*) item);
               pthread_mutex_unlock(&router->arp_cache->lock_queue);
               // Send ARP Request
               arp_send_request( dst_ip, intf );
               packet_dispatcher_put_packet(router->arp_cache->packet_dispatcher, dst_ip, payload, len, intf, Q_PACKET_TIMEOUT );
            }
            packet_dispatcher_put_packet(router->arp_cache->packet_dispatcher, dst_ip, payload, len, intf, Q_PACKET_TIMEOUT );
            return 0;
          }
      }
   }
}


/**
 * Fills buf with a string representation of the arp cache.
 *
 */
void arp_cache_to_string() {
   char *str;
   cli_send_str("Static entries: \n");
   asprintf(&str, "IP, MAC, Timestamp\n");
   cli_send_str(str);
   free(str);
   arp_cache_entry_t *item;
   node* head;
   // get instance of router 
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   // put static cache
   pthread_mutex_lock(&router->arp_cache->lock_static);
   head = router->arp_cache->arp_static_cache_list;
   while(head != NULL) {
      item = (arp_cache_entry_t*) head->data;
      asprintf(&str, "%s, %s, %ld.%06ld\n", quick_ip_to_string(item->ip), quick_mac_to_string(item->mac.octet), item->timeout->tv_sec, item->timeout->tv_usec);
      cli_send_str(str);
      free(str);
      head = head->next;
   }
   pthread_mutex_unlock(&router->arp_cache->lock_static);
   // put dynamic cache
   cli_send_str("Dynamic entries: \n");
   asprintf(&str, "IP, MAC, Timestamp\n");
   cli_send_str(str);
   free(str);
   pthread_mutex_lock(&router->arp_cache->lock_dynamic);
   head = router->arp_cache->arp_cache_list;
   while(head != NULL) {
      item = (arp_cache_entry_t*) head->data;
      asprintf(&str, "%s, %s, %ld.%06ld\n", quick_ip_to_string(item->ip), quick_mac_to_string(item->mac.octet), item->timeout->tv_sec, item->timeout->tv_usec);
      cli_send_str(str);
      free(str);
      head = head->next;
   }
   pthread_mutex_unlock(&router->arp_cache->lock_dynamic);
}

void arp_write_hw() {
#ifdef _CPUMODE_
   printf(" ** arp_write_hw(..) called \n");
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   node* static_head;
   node* dynamic_head;
   unsigned int mac_hi = 0;
   unsigned int mac_lo = 0;
   int i = 0;
   arp_cache_entry_t* cache_entry;
   pthread_mutex_lock(&router->arp_cache->lock_static);
      static_head = router->arp_cache->arp_static_cache_list;
      while(static_head != NULL && i < ROUTER_OP_LUT_ARP_TABLE_DEPTH) {
         cache_entry = (arp_cache_entry_t*) static_head->data;
         mac_hi |= ((unsigned int)cache_entry->mac.octet[0]) << 8;
         mac_hi |= ((unsigned int)cache_entry->mac.octet[1]);
         mac_lo |= ((unsigned int)cache_entry->mac.octet[2]) << 24;
         mac_lo |= ((unsigned int)cache_entry->mac.octet[3]) << 16;
         mac_lo |= ((unsigned int)cache_entry->mac.octet[4]) << 8;
	 mac_lo |= ((unsigned int)cache_entry->mac.octet[5]);
         // write mac
         writeReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_HI, mac_hi);
         writeReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_LO, mac_lo);
         // write ip
         writeReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_ENTRY_NEXT_HOP_IP, ntohl(cache_entry->ip));
         // write table index
         writeReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_WR_ADDR, i);
         static_head = static_head->next;
         i++;
      }
   pthread_mutex_unlock(&router->arp_cache->lock_static);
   // now dynamic cache
    pthread_mutex_lock(&router->arp_cache->lock_dynamic);
      dynamic_head = router->arp_cache->arp_cache_list;
      while(dynamic_head != NULL && i < ROUTER_OP_LUT_ARP_TABLE_DEPTH) {
         cache_entry = (arp_cache_entry_t*) dynamic_head->data;
         mac_hi |= ((unsigned int)cache_entry->mac.octet[0]) << 8;
         mac_hi |= ((unsigned int)cache_entry->mac.octet[1]);
         mac_lo |= ((unsigned int)cache_entry->mac.octet[2]) << 24;
         mac_lo |= ((unsigned int)cache_entry->mac.octet[3]) << 16;
         mac_lo |= ((unsigned int)cache_entry->mac.octet[4]) << 8;
         mac_lo |= ((unsigned int)cache_entry->mac.octet[5]);
         // write mac
         writeReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_HI, mac_hi);
         writeReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_LO, mac_lo);
         // write ip
         writeReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_ENTRY_NEXT_HOP_IP, ntohl(cache_entry->ip));
         // write table index
         writeReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_WR_ADDR, i);
         dynamic_head = dynamic_head->next;
         i++;
      }
   pthread_mutex_unlock(&router->arp_cache->lock_dynamic); 
#endif
}

void arp_read_hw() {
#ifdef _CPUMODE_
   printf(" ** arp_read__hw(..) called \n");
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   char *str;
   int i;
   unsigned int mac_hi = 0, mac_lo = 0;
   uint32_t ip;
   addr_mac_t mac;
   asprintf(&str, " MAC, IP\n");
   cli_send_str(str);
   free(str);
   for (i = 0; i < ROUTER_OP_LUT_ROUTE_TABLE_DEPTH; i++) {
       // write table index to read register   
       writeReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_RD_ADDR, i);
       // read mac hi, lo
       readReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_LO, &mac_lo);
       readReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_HI, &mac_hi);
       //convert to mac 
       mac = get_mac_from_hi_lo(&mac_hi, &mac_lo);
       // read ip
       readReg(&router->hw_device, ROUTER_OP_LUT_ARP_TABLE_ENTRY_NEXT_HOP_IP, &ip);
       ip = htonl(ip);
       asprintf(&str, "%s, %s\n", quick_mac_to_string(mac.octet), quick_ip_to_string(ip));
       cli_send_str(str);
       free(str);
   }
#endif
}
