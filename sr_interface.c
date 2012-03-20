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
#include "sr_cpu_extension_nf2.h"
#include "reg_defines.h"
#include "sr_pwospf.h"

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
#ifdef _CPUMODE_
   pthread_mutex_init(&intf.hw_lock, NULL);
#endif
   printf(" ** set_interface(..) displaying interface object \n");
   display_interface(&intf);
   return intf;
}

void interface_init(struct sr_router* subsystem) {
   printf(" ** interface_init(..) called \n");
   subsystem->num_interfaces = 0;
#ifdef _CPUMODE_
   sr_cpu_init_interface_socket(subsystem);
#endif
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
#ifdef _CPUMODE_
   pthread_mutex_init(&intf.hw_lock, NULL);
#endif
   return intf;
}

interface_t* get_interface_name(struct sr_router* router, const char* name) {
   printf(" ** get_interface_name(..) called for name: %s\n", name);
   int i = 0;
   for(i = 0; i < router->num_interfaces ; i++) {
      if(strcmp(router->interface[i].name, name) == 0) {
         printf(" ** get_interface_name(..) found name: %s\n", name);
         return &router->interface[i];
      }
   }
   printf(" ** get_interface_name(..) not found name: %s\n", name);
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
   printf(" ** check_interface(..) called \n"); 
   printf("************* %u\n", intf->enabled);
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

void write_interface_hw(struct sr_router* subsystem) {
#ifdef _CPUMODE_
   printf(" ** write_interface_hw(..) called \n");
   unsigned int mac_hi = 0;
   unsigned int mac_lo = 0;
   pthread_mutex_lock(&subsystem->interface[subsystem->num_interfaces].hw_lock);
   /*MAC address needs to be of the form 00:00:AA:BB:CC:DD:EE:FF
    * Hi: 00:00:AA:BB
    * Lo: CC:DD:EE:FF
    */
   mac_hi |= ((unsigned int)subsystem->interface[subsystem->num_interfaces].mac.octet[0]) << 8;
   mac_hi |= ((unsigned int)subsystem->interface[subsystem->num_interfaces].mac.octet[1]);
   mac_lo |= ((unsigned int)subsystem->interface[subsystem->num_interfaces].mac.octet[2]) << 24;
   mac_lo |= ((unsigned int)subsystem->interface[subsystem->num_interfaces].mac.octet[3]) << 16;
   mac_lo |= ((unsigned int)subsystem->interface[subsystem->num_interfaces].mac.octet[4]) << 8;
   mac_lo |= ((unsigned int)subsystem->interface[subsystem->num_interfaces].mac.octet[5]);
   //IMP: IP addresses are in network order
   writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DST_IP_FILTER_TABLE_ENTRY_IP, ntohl(subsystem->interface[subsystem->num_interfaces].ip));
   writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DST_IP_FILTER_TABLE_WR_ADDR, subsystem->num_interfaces);
   if(subsystem->num_interfaces == 0) {
       //IMP: MAC addresses are already in host order
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DEFAULT_MAC_0_HI, mac_hi);
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DEFAULT_MAC_0_LO, mac_lo);
   } else if(subsystem->num_interfaces == 1) {
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DEFAULT_MAC_1_HI, mac_hi);
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DEFAULT_MAC_1_LO, mac_lo);
   } else if(subsystem->num_interfaces == 2) {
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DEFAULT_MAC_2_HI, mac_hi);
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DEFAULT_MAC_2_LO, mac_lo);
   } else if(subsystem->num_interfaces == 3) {
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DEFAULT_MAC_3_HI, mac_hi);
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DEFAULT_MAC_3_LO, mac_lo);
       // last interface
       //add allspfrouters ip
       int i = subsystem->num_interfaces + 1;
       char* all_spf_routers =  ALL_SPF_ROUTERS_IP;
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DST_IP_FILTER_TABLE_ENTRY_IP, ntohl(make_ip_addr(all_spf_routers))); 
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DST_IP_FILTER_TABLE_WR_ADDR, i);
       // now add zeroes to the rest
       i++;
       while(i < ROUTER_OP_LUT_DST_IP_FILTER_TABLE_DEPTH) {
          writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DST_IP_FILTER_TABLE_ENTRY_IP, 0);
          writeReg(&subsystem->hw_device, ROUTER_OP_LUT_DST_IP_FILTER_TABLE_WR_ADDR, i);
          i++;
       }
   }
   pthread_mutex_unlock(&subsystem->interface[subsystem->num_interfaces].hw_lock);
#endif
}
