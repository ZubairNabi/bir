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

interface_t set_interface(struct sr_vns_if* vns_intf, int intf_num) {
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
   intf.hw_port = get_hw_port(intf_num);
#endif
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
   // get instance of router 
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   int i;
   interface_t* intf;
   for( i = 0; i<router->num_interfaces; i++) {
         intf = &router->interface[i];
         cli_send_str("\n-----------------------------------------------------------------------------\n");
         cli_send_str("Interface: \n");
         asprintf(&str, "%-4s|%-17s|%-15s|%-15s|%-14s|Status\n", "Name", "MAC", "IP", "Mask", "Hello Interval");
   cli_send_str(str);
   free(str);
         asprintf(&str, "%-4s|%-16s|%-15s|", intf->name,  quick_mac_to_string(intf->mac.octet), quick_ip_to_string(intf->ip));
         cli_send_str(str);
         free(str);
         asprintf(&str, "%-15s|%-14u|", quick_ip_to_string(intf->subnet_mask), intf->helloint);
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
      asprintf(&str, "%-15s|%-15s|%-17s|%-5s|%-15s|LSU\n", "ID", "IP", "Timestamp", "Hello", "Mask");
      cli_send_str(str);
      free(str);
      asprintf(&str, "%-15s|", quick_ip_to_string(entry->id));
      cli_send_str(str);
      free(str);
      asprintf(&str, "%-15s|", quick_ip_to_string(entry->ip));
      cli_send_str(str);
      free(str);
      asprintf(&str, "%ld.%-6ld|%-5u|%-15s|", entry->timestamp->tv_sec, entry->timestamp->tv_usec, ntohs(entry->helloint), quick_ip_to_string(entry->mask));
      cli_send_str(str);
      free(str);
      if(entry->last_adverts == NULL) {
         cli_send_str("No\n");
      } else {
         cli_send_str("Yes\n");
      }
      head = head->next;
   }
   cli_send_str("\n-----------------------------------------------------------------------------\n");
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
   // write IPs to gateway table
    writeReg(&subsystem->hw_device, ROUTER_OP_LUT_GATEWAY_TABLE_ENTRY_NEXT_HOP, ntohl(subsystem->interface[subsystem->num_interfaces].ip));
   writeReg(&subsystem->hw_device, ROUTER_OP_LUT_GATEWAY_TABLE_WR_ADDR, subsystem->num_interfaces);
   if(subsystem->num_interfaces == 0) {
       //IMP: MAC addresses are already in host order
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_MAC_0_HI, mac_hi);
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_MAC_0_LO, mac_lo);
   } else if(subsystem->num_interfaces == 1) {
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_MAC_1_HI, mac_hi);
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_MAC_1_LO, mac_lo);
   } else if(subsystem->num_interfaces == 2) {
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_MAC_2_HI, mac_hi);
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_MAC_2_LO, mac_lo);
   } else if(subsystem->num_interfaces == 3) {
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_MAC_3_HI, mac_hi);
       writeReg(&subsystem->hw_device, ROUTER_OP_LUT_MAC_3_LO, mac_lo);
       // last interface
       //add allspfrouters ip
       int i = subsystem->num_interfaces + 1;
       int j = subsystem->num_interfaces + 1;
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
       // add zeroes to gateway table as well
       while(j < ROUTER_OP_LUT_GATEWAY_TABLE_DEPTH) {
          writeReg(&subsystem->hw_device, ROUTER_OP_LUT_GATEWAY_TABLE_ENTRY_NEXT_HOP, 0);
          writeReg(&subsystem->hw_device, ROUTER_OP_LUT_GATEWAY_TABLE_WR_ADDR, j);
          j++;
       }
   }
   pthread_mutex_unlock(&subsystem->interface[subsystem->num_interfaces].hw_lock);
#endif
}

uint32_t get_hw_port(int intf_num) {
   if(intf_num == 0)
      return 1;
   else if(intf_num == 1)
      return 4;
   else if(intf_num == 2)
      return 16;
   return 64;
}

interface_t get_hw_intf(int hw_port) {
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   if(hw_port == 1)
      return router->interface[0];
   else if(hw_port == 4)
      return router->interface[1];
   else if(hw_port == 16)
      return router->interface[2];
   return router->interface[3];
}

void read_interface_hw() {
#ifdef _CPUMODE_
   printf(" ** read_interface_hw(..) called \n");
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   char *str;
   unsigned int mac[2];
   uint32_t ip;
   int i;
   // show interface names and mac addresses
   asprintf(&str, "Name, MAC\n"); 
   cli_send_str(str);
   free(str);
   // read hi, lo mac for eth0
   readReg(&router->hw_device, ROUTER_OP_LUT_MAC_0_HI, &mac[0]);
   readReg(&router->hw_device, ROUTER_OP_LUT_MAC_0_LO, &mac[1]);
   mac[0] = htonl(mac[0]);
   mac[1] = htonl(mac[1]);
   asprintf(&str, "%s, %s\n", "eth0",  quick_mac_to_string(((uint8_t*)mac) + 2));
   cli_send_str(str);
   free(str);
    // read hi, lo mac for eth1
   readReg(&router->hw_device, ROUTER_OP_LUT_MAC_1_HI, &mac[0]);
   readReg(&router->hw_device, ROUTER_OP_LUT_MAC_1_LO, &mac[1]);
   mac[0] = htonl(mac[0]);
   mac[1] = htonl(mac[1]);
   asprintf(&str, "%s, %s\n", "eth1",  quick_mac_to_string(((uint8_t*)mac) + 2));
   cli_send_str(str);
   free(str);
    // read hi, lo mac for eth2
   readReg(&router->hw_device, ROUTER_OP_LUT_MAC_2_HI, &mac[0]);
   readReg(&router->hw_device, ROUTER_OP_LUT_MAC_2_LO, &mac[1]);
   mac[0] = htonl(mac[0]);
   mac[1] = htonl(mac[1]);
   asprintf(&str, "%s, %s\n", "eth2",  quick_mac_to_string(((uint8_t*)mac) + 2));
   cli_send_str(str);
   free(str);
    // read hi, lo mac for eth3
   readReg(&router->hw_device, ROUTER_OP_LUT_MAC_3_HI, &mac[0]);
   readReg(&router->hw_device, ROUTER_OP_LUT_MAC_3_LO, &mac[1]);
   mac[0] = htonl(mac[0]);
   mac[1] = htonl(mac[1]);
   asprintf(&str, "%s, %s\n", "eth3",  quick_mac_to_string(((uint8_t*)mac) + 2));
   cli_send_str(str);
   free(str);
   // read ip filter table
   asprintf(&str, "IP\n");    
   cli_send_str(str);
   free(str);
   for (i = 0; i < ROUTER_OP_LUT_DST_IP_FILTER_TABLE_DEPTH; i++) {
        // write index to register
        writeReg(&router->hw_device, ROUTER_OP_LUT_DST_IP_FILTER_TABLE_RD_ADDR, i);
        // read value
        readReg(&router->hw_device, ROUTER_OP_LUT_DST_IP_FILTER_TABLE_ENTRY_IP, &ip);
        ip = htonl(ip);
        //check for blank entries
        if( ip == 0)
           break;
        asprintf(&str, "%s\n",  quick_ip_to_string(ip));
        cli_send_str(str);
        free(str);
    } 
    // read gateway table
    asprintf(&str, "Gateway IP\n");
    cli_send_str(str);
    free(str);
    for (i = 0; i < ROUTER_OP_LUT_GATEWAY_TABLE_DEPTH; i++) {
        // write index to register
        writeReg(&router->hw_device, ROUTER_OP_LUT_GATEWAY_TABLE_RD_ADDR, i);
        // read value
        readReg(&router->hw_device, ROUTER_OP_LUT_GATEWAY_TABLE_ENTRY_NEXT_HOP, &ip);
        ip = htonl(ip);
        //check for blank entries
        if( ip == 0)
           break;
        asprintf(&str, "%s\n",  quick_ip_to_string(ip));
        cli_send_str(str);
        free(str);
    }

#endif
}

void read_info_hw() {
#ifdef _CPUMODE_
   printf(" ** read_interface_hw(..) called \n");
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   char *str;
   uint32_t data;
   // read arp misses
   readReg(&router->hw_device, ROUTER_OP_LUT_ARP_NUM_MISSES, &data);
   asprintf(&str, "arp misses: %lu\n", data);
   cli_send_str(str);
   free(str);
   // read lpm misses
   readReg(&router->hw_device, ROUTER_OP_LUT_LPM_NUM_MISSES, &data);
   asprintf(&str, "lpm misses: %lu\n", data);
   cli_send_str(str);
   free(str);
   // read cpu pkts send
   readReg(&router->hw_device, ROUTER_OP_LUT_NUM_CPU_PKTS_SENT, &data);
   asprintf(&str, "cpu packets sent: %lu\n", data);
   cli_send_str(str);
   free(str);
   readReg(&router->hw_device, ROUTER_OP_LUT_NUM_BAD_OPTS_VER, &data);
   asprintf(&str, "bad opts version: %lu\n", data);
   cli_send_str(str);
   free(str);
   readReg(&router->hw_device, ROUTER_OP_LUT_NUM_BAD_CHKSUMS, &data);
   asprintf(&str, "bad checksums: %lu\n", data);
   cli_send_str(str);
   free(str);
   readReg(&router->hw_device, ROUTER_OP_LUT_NUM_BAD_TTLS, &data);
   asprintf(&str, "bad ttls: %lu\n", data);
   cli_send_str(str);
   free(str);
   readReg(&router->hw_device, ROUTER_OP_LUT_NUM_NON_IP_RCVD, &data);
   asprintf(&str, "non IP received: %lu\n", data);
   cli_send_str(str);
   free(str);
   readReg(&router->hw_device, ROUTER_OP_LUT_NUM_PKTS_FORWARDED, &data);
   asprintf(&str, "packets forwarded: %lu\n", data);
   cli_send_str(str);
   free(str);
   readReg(&router->hw_device, ROUTER_OP_LUT_NUM_WRONG_DEST, &data);
   asprintf(&str, "wrong destination: %lu\n", data);
   cli_send_str(str);
   free(str);
   readReg(&router->hw_device, ROUTER_OP_LUT_NUM_FILTERED_PKTS, &data);
   asprintf(&str, "filtered packets: %lu\n", data);
   cli_send_str(str);
   free(str);
#endif
}
