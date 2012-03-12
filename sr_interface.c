/* Filename: sr_interface.c
 */

#include "sr_interface.h"
#include "sr_common.h"
#include "sr_base_internal.h"
#include "sr_integration.h"
#include "cli/helper.h"
#include "sr_router.h"
#include "sr_neighbor.h"
#include "cli/cli.h"

#include <string.h>
#include <stdlib.h>

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

void display_all_interfaces() {
   // get instance of router 
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   int i;
   for( i = 0; i<router->num_interfaces; i++) {
      display_interface(&router->interface[i]);
   }
}

void display_all_interfaces_str() {
   char *str;
   asprintf(&str, "Name, MAC, IP, Mask, Hello Interval, Status\n");
   cli_send_str(str);
   free(str);
   // get instance of router 
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   int i;
   interface_t* intf;
   for( i = 0; i<router->num_interfaces; i++) {
         intf = &router->interface[i];
         asprintf(&str, "%s, %s, %s, ", intf->name,  quick_mac_to_string(intf->mac.octet), quick_ip_to_string(intf->ip));
         cli_send_str(str);
         free(str);
         asprintf(&str, "%s, %u, ", quick_ip_to_string(intf->subnet_mask), intf->helloint);
         cli_send_str(str);
         free(str);
   if(intf->enabled == TRUE) {
      asprintf(&str, "enabled\n");
      cli_send_str(str);
      free(str);
   }
   else {
      asprintf(&str, "disabled\n");
      cli_send_str(str);
      free(str);
   }
   }
}

void display_all_interfaces_neighbors_str() {
   char *str;
   node* head;
   neighbor_t* entry;
   asprintf(&str, "Name, MAC, IP, Mask, Hello Interval, Status\n");
   cli_send_str(str);
   free(str);
   // get instance of router 
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   int i;
   interface_t* intf;
   for( i = 0; i<router->num_interfaces; i++) {
         intf = &router->interface[i];
         cli_send_str("Interface: \n");
         asprintf(&str, "%s, %s, %s, ", intf->name,  quick_mac_to_string(intf->mac.octet), quick_ip_to_string(intf->ip));
         cli_send_str(str);
         free(str);
         asprintf(&str, "%s, %u, ", quick_ip_to_string(intf->subnet_mask), intf->helloint);
         cli_send_str(str);
         free(str);
   if(intf->enabled == TRUE) {
      asprintf(&str, "enabled\n");
      cli_send_str(str);
      free(str);
   }
   else {
      asprintf(&str, "disabled\n");
      cli_send_str(str);
      free(str);
   }
   // now display its neighbors
   pthread_mutex_lock(&intf->neighbor_lock);
   head = intf->neighbor_list;
   while(head != NULL) {
      entry = (neighbor_t*) head->data;
      cli_send_str("Neighbor(s): \n");
      cli_send_str("ID, IP, Timestamp, Hello Int, Mask, LSU received\n");
      asprintf(&str, "%lu, %s, %ld.%06ld, %u, %s", entry->id, quick_ip_to_string(entry->ip), entry->timestamp->tv_sec, entry->timestamp->tv_usec, ntohs(entry->helloint), quick_ip_to_string(entry->mask));
      cli_send_str(str);
      free(str);
      if(entry->last_adverts == NULL) {
         cli_send_str(" N\n");
      } else {
         cli_send_str(" Y\n");
      }
      head = head->next;
   }
   pthread_mutex_unlock(&intf->neighbor_lock);
   }
}

