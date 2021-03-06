/* Filename: sr_rtable.c */

#include <stdio.h>     /* snprintf() */
#include <stdlib.h>
#include "sr_rtable.h"
#include "cli/helper.h"
#include "sr_router.h"
#include "cli/cli.h"
#include "sr_integration.h"
#include "reg_defines.h"

/**
 * Initialize the routing table
 * 
 * @param rtable  A pointer to the routing table to initialize
 */
void rrtable_init(struct sr_router *router){
   printf(" ** rrtable_init(..) called \n");
   // initialize routing table list
   router->rtable = (rtable_t*) malloc_or_die(sizeof(rtable_t));
   router->rtable->rtable_list = llist_new();
   // initialize lock
   pthread_mutex_init(&router->rtable->lock_rtable, NULL); 
}

/**
 * Free memory associated with all members of the routing table, but not
 * the routing table itself.
 *
 * @param rtable  A pointer to the routing table to destroy
 */
void rrtable_destroy(rtable_t *rtable){
    printf(" ** rrtable_destroy(..) called \n");
    // clean routing table list
    pthread_mutex_lock(&rtable->lock_rtable);
    rtable->rtable_list = llist_delete_no_count(rtable->rtable_list);
    pthread_mutex_unlock(&rtable->lock_rtable);
    // delete lock
    pthread_mutex_destroy(&rtable->lock_rtable);
    // write to hw
}

/**
 * Find the route specified by this routing table from a packet addressed
 * to dest_ip.
 *
 * @param rtable  A pointer to the routing table which specifies the route
 * @param dest_ip  The destination IP address
 * 
 * @return interface_t (see sr_rtable.h) specifying a route to dest_ip
 */
route_info_t rrtable_find_route(rtable_t *rtable, uint32_t dest_ip) {
   printf(" ** rrtable_find_route(..) called \n");
   route_info_t route_info;
   // display current state of routing table
   printf(" ** rrtable_find_route(..) routing table state:\n");
   llist_display_all(rtable->rtable_list, display_route_t);
   // lock table
   pthread_mutex_lock(&rtable->lock_rtable);
   // get route from table
   node* ret = llist_find(rtable->rtable_list, predicate_ip_route_t_prefix_match, (void*) &dest_ip);
   // unblock table
   pthread_mutex_unlock(&rtable->lock_rtable);
   if( ret!= NULL) {
      route_t* route = (route_t*) ret->data;
      route_info.ip = route->next_hop;
      route_info.mask = route->subnet_mask;
      route_info.intf = &route->intf;
   } else {
      route_info.ip = 0;
      route_info.mask = 0;
      route_info.intf = NULL;
   }
   return route_info;
}

/**
 * Add a route to the routing table in a threadsafe manner.  Each entry is
 * marked as either static or dynamic depending on its.
 * 
 * @param rtable  The routing table to add to
 * @param dest  The destination IP address
 * @param mask  The subnet mask
 * @param intf  The interface this subnet is on
 */
void rrtable_route_add( rtable_t *rtable,
                       addr_ip_t dest, addr_ip_t gw, addr_ip_t mask,
                       interface_t* intf, char type ) {
   printf(" ** rrtable_route_add(..) called \n");
   // get subnet
   dest = dest & mask;
   // make route_t object
   route_t* route = make_route_t(type, dest, *intf, gw, mask);
   // lock table
   pthread_mutex_lock(&rtable->lock_rtable);
   // add route to table
   rtable->rtable_list = llist_insert_sorted(rtable->rtable_list, predicate_ip_sort_route_t, (void*) route);
   // unblock table
   pthread_mutex_unlock(&rtable->lock_rtable);
}

/**
 * Remove the specified route from the routing table, if it exists.  Return TRUE
 * on success, FALSE if the route does not exist.  Threadsafe.
 *
 * @param rtable A pointer to the routing table to remove the route from
 * @param dest  The destination IP of the route
 * @param mask  The subnet mask of the route
 * 
 * @return bool indicating success (TRUE) or non-existent route (FALSE)
 */
bool rrtable_route_remove( rtable_t *rtable, addr_ip_t dest, addr_ip_t mask ) {
   printf(" ** rrtable_route_remove(..) called \n");
    // lock table
   pthread_mutex_lock(&rtable->lock_rtable);
   // remove route from table
   node* ret = llist_remove(rtable->rtable_list, predicate_ip_sort_route_t, (void*) &dest);
   // unlock table
   pthread_mutex_unlock(&rtable->lock_rtable);
   if(ret == NULL)
      return FALSE;
   else {
      rtable->rtable_list = ret;
      return TRUE;
   }
}

/**
 * Removes all routes from a routing table (threadsafe).
 * 
 * @param rtable  A pointer to the routing table from which all routes will be
 * removed.
 */
void rrtable_purge_all( rtable_t* rtable ) {
   printf(" ** rrtable_purge_all(..) called \n");
   // lock table
   pthread_mutex_lock(&rtable->lock_rtable);
   // remove route from table
   rtable->rtable_list = llist_delete_no_count(rtable->rtable_list);
   // unlock table
   pthread_mutex_unlock(&rtable->lock_rtable);
}  

/**
 * Fills a buffer with a string representation of the routing table
 *
 */
void rrtable_to_string() {
   char *str;
   asprintf(&str, "------------------------------------------------------------------------\n");
   cli_send_str(str);
   free(str);
   asprintf(&str, "| Type | %-14s | %-14s | %-14s  | %-9s |\n", "Destination", "Next Hop", "Mask", "Interface");
   cli_send_str(str);
   free(str);
   asprintf(&str, "------------------------------------------------------------------------\n");
   cli_send_str(str);
   free(str);
   node* head;
   route_t* entry;
   // get instance of router 
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   pthread_mutex_lock(&router->rtable->lock_rtable);
   head = router->rtable->rtable_list;
   while(head != NULL) {
      entry = (route_t*) head->data;
      asprintf(&str, "| %-4c | %-14s | ", entry->type, quick_ip_to_string(entry->destination));
      cli_send_str(str);
      free(str);
      asprintf(&str, "%-14s |", quick_ip_to_string(entry->next_hop));
      cli_send_str(str);
      free(str);
      asprintf(&str, " %-14s | %-9s |\n", quick_ip_to_string(entry->subnet_mask), entry->intf.name);
      cli_send_str(str);
      free(str);
      head = head->next;
   }
   asprintf(&str, "------------------------------------------------------------------------\n");
   cli_send_str(str);
   free(str);
   pthread_mutex_unlock(&router->rtable->lock_rtable);
}


/**
 * Returns sizeof(rtable_t) to allow memory allocation from within sr.o.
 *
 * @return  The size in bytes of rtable_t
 */
unsigned int sizeof_rtable(void) {
   printf(" ** sizeof_rtable(..) called \n");
  return sizeof(rtable_t);
}

route_t* make_route_t(char type, addr_ip_t dst, interface_t intf, addr_ip_t next_hop, addr_ip_t subnet_mask) {
   route_t* route = (route_t*) malloc_or_die(sizeof(route_t));
   route->type = type;
   route->destination = dst;
   route->intf = intf;
   route->next_hop = next_hop;
   route->subnet_mask = subnet_mask;
   return route;
} 

void rrtable_purge_all_type( rtable_t* rtable, char type ) {
   printf(" ** rrtable_purge_all_type(..) called \n");
   // lock table
   pthread_mutex_lock(&rtable->lock_rtable);
   // remove route from table
   rtable->rtable_list = llist_remove_all_no_count(rtable->rtable_list, predicate_route_t_type, (void*) &type);
   // unlock table
   pthread_mutex_unlock(&rtable->lock_rtable);
}

