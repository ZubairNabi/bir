/**
 * Filename: cli_stus.h
 * Author: David Underhill
 * Purpose: TEMPORARY file where router hooks are defined temporarily until they
 *          are implemented for real.
 */

#ifndef CLI_STUBS_H
#define CLI_STUBS_H

#include "../sr_arp_cache.h"
/**
 * Add a static entry to the static ARP cache.
 * @return 1 if succeeded (fails if the max # of static entries are already
 *         in the cache).
 */
int arp_cache_static_entry_add( struct sr_instance* sr,
                                uint32_t ip,
                                uint8_t* mac ) {
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    addr_mac_t *mac_addr = (addr_mac_t*) malloc_or_die(sizeof(addr_mac_t));
    mac_addr = make_mac_addr_ptr(mac);
    int ret = cache_static_entry_add(subsystem ,ip, mac_addr);
    return ret;
}
/**
 * Remove a static entry to the static ARP cache.
 * @return 1 if succeeded (false if ip wasn't in the cache as a static entry)
 */
int arp_cache_static_entry_remove( struct sr_instance* sr, uint32_t ip ) {
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    int ret = cache_static_entry_remove(subsystem, ip);
    return ret;

}

/**
 * Remove all static entries from the ARP cache.
 * @return  number of static entries removed
 */
unsigned arp_cache_static_purge( struct sr_instance* sr ) {
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    int ret = cache_static_purge(subsystem);
    return ret;
}

/**
 * Remove all dynamic entries from the ARP cache.
 * @return  number of dynamic entries removed
 */
unsigned arp_cache_dynamic_purge( struct sr_instance* sr ) {
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    int ret = cache_dynamic_purge(subsystem);
    return ret;
}

/**
 * Enables or disables an interface on the router.
 * @return 0 if name was enabled
 *         -1 if it does not not exist
 *         1 if already set to enabled
 */
int router_interface_set_enabled( struct sr_instance* sr, const char* name, int enabled ) {
   struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
   interface_t* intf = get_interface_name(subsystem, name);
   if (intf == NULL)
      return -1;
   bool status = check_interface(intf);
   if(status == TRUE)
      return 1;
   toggle_interface(intf, enabled);
   return 0;
}

/**
 * Returns a pointer to the interface which is assigned the specified IP.
 *
 * @return interface, or NULL if the IP does not belong to any interface
 *         (you'll want to change void* to whatever type you end up using)
 */
interface_t* router_lookup_interface_via_ip( struct sr_instance* sr,
                                      uint32_t ip ) {
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    return get_interface_ip(subsystem, ip);
}

/**
 * Returns a pointer to the interface described by the specified name.
 *
 * @return interface, or NULL if the name does not match any interface
 *         (you'll want to change void* to whatever type you end up using)
 */
interface_t* router_lookup_interface_via_name( struct sr_instance* sr,
                                        const char* name ) {
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    return get_interface_name(subsystem, name);
}

/**
 * Returns 1 if the specified interface is up and 0 otherwise.
 */
int router_is_interface_enabled( struct sr_instance* sr, interface_t* intf ) {
   bool status = check_interface(intf);
   if(status == TRUE)
      return 1;
   return 0;
}

/**
 * Returns whether OSPF is enabled (0 if disabled, otherwise it is enabled).
 */
int router_is_ospf_enabled( struct sr_instance* sr ) {
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    bool ret = check_ospf_status(subsystem);
    if(ret == TRUE)
       return 1;
    return 0;
}

/**
 * Sets whether OSPF is enabled.
 */
void router_set_ospf_enabled( struct sr_instance* sr, int enabled ) {
   struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
   toggle_ospf_status(subsystem, enabled);
}

/** Adds a route to the appropriate routing table. */
void rtable_route_add( struct sr_instance* sr,
                       uint32_t dest, uint32_t gw, uint32_t mask,
                       void* intf,
                       int is_static_route ) {
   struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
   char type = 'd';
   if(is_static_route)
      type = 's';
   rrtable_route_add(subsystem->rtable,
                       dest, gw, mask,
                       intf, type );
}

/** Removes the specified route from the routing table, if present. */
int rtable_route_remove( struct sr_instance* sr,
                         uint32_t dest, uint32_t mask,
                         int is_static ) {
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    return rrtable_route_remove(subsystem->rtable, dest, mask);
}

/** Remove all routes from the router. */
void rtable_purge_all( struct sr_instance* sr ) {
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    rrtable_purge_all(subsystem->rtable);
}

/** Remove all routes of a specific type from the router. */
void rtable_purge( struct sr_instance* sr, int is_static ) {
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    rrtable_purge_all(subsystem->rtable);
}

#endif /* CLI_STUBS_H */
