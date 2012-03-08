/* Filename: sr_interface.c
 */

#include "sr_interface.h"
#include "sr_common.h"
#include "sr_base_internal.h"
#include "cli/helper.h"
#include "sr_router.h"
#include "sr_neighbor.h"
#include <string.h>

interface_t set_interface(struct sr_vns_if* vns_intf) {
   printf(" ** set_interface(..) called \n");
   interface_t intf;
   strcpy(intf.name, vns_intf->name);
   intf.mac = make_mac_addr(vns_intf->addr[0], vns_intf->addr[1], vns_intf->addr[2], vns_intf->addr[3], vns_intf->addr[4], vns_intf->addr[5]);
   intf.ip = vns_intf->ip;
   intf.subnet_mask = vns_intf->mask;
   intf.enabled = TRUE;
   intf.neighbor_list = llist_new();
   pthread_mutex_init(&intf.neighbor_lock, NULL);
   intf.helloint = PWOSPF_HELLO_INTERVAL;
   printf(" ** set_interface(..) displaying interface object \n");
   display_interface(&intf);
   return intf;
}

void interface_init(struct sr_router* subsystem) {
   printf(" ** interface_init(..) called \n");
   subsystem->num_interfaces = 0;
}

interface_t make_interface(char* name, addr_mac_t mac, addr_ip_t ip, bool enabled, addr_ip_t subnet_mask) {
   printf(" ** make_interface(..) called \n");
   interface_t intf;
   strcpy(intf.name, name);
   intf.mac = mac;
   intf.ip = ip;
   intf.enabled = enabled;
   intf.subnet_mask = subnet_mask;
   intf.neighbor_list = llist_new();
   pthread_mutex_init(&intf.neighbor_lock, NULL);
   intf.helloint = PWOSPF_HELLO_INTERVAL;
   return intf;
}

interface_t* get_interface_name(struct sr_router* router, const char* name) {
   printf(" ** get_interface_name(..) called \n");
   int i = 0;
   for(i = 0; i < router->num_interfaces ; i++) {
      if(strcmp(router->interface[i].name, name) == 0)
         return &router->interface[i];
   }
   return NULL;
}

interface_t* get_interface_ip(struct sr_router* router, addr_ip_t ip) {
   printf(" ** get_interface_ip(..) called \n");
   int i = 0;
   for(i = 0; i < router->num_interfaces ; i++) {
      if(router->interface[i].ip == ip)
         return &router->interface[i];
   }
   return NULL;
}

void display_interface(interface_t* intf) {
   printf(" ** name: %s, mac: %s, ip: %s ", intf->name,  quick_mac_to_string(intf->mac.octet), quick_ip_to_string(intf->ip));
   printf("subnet mask: %s, hello interval: %u, ", quick_ip_to_string(intf->subnet_mask), intf->helloint);
   if(intf->enabled == TRUE)
      printf("status: enabled\n");
   else
      printf("status: disabled\n");
}

void toggle_interface(interface_t* intf, int enabled) {
   if (enabled == 1)
      intf->enabled = TRUE;
   else 
      intf->enabled = FALSE;
}

bool check_interface(interface_t* intf) {
   return intf->enabled;
}
