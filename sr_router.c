#include "sr_router.h"
#include <stdlib.h>

#define DEFAULT_IFACE   "nf2c0"

void toggle_ospf_status(sr_router* router, bool status) {
   router->ospf_status = status;
}
   
ls_info_t make_ls_info(uint32_t id, uint32_t area_id, uint16_t lsuint) {
   ls_info_t ls_info;
   ls_info.router_id = id;
   ls_info.area_id = area_id;
   ls_info.lsuint = lsuint;
   ls_info.lsu_seq = 0;
   return ls_info;
}

ls_info_t make_ls_info_id(sr_router* router, uint32_t area_id, uint16_t lsuint) {
   ls_info_t ls_info;
   interface_t* intf = get_interface_name(router, "eth0");
   ls_info.router_id = intf->ip;
   ls_info.area_id = area_id;
   ls_info.lsuint = lsuint;
   ls_info.lsu_seq = 0;
   return ls_info;
}

void set_ls_info(sr_router* router, ls_info_t ls_info) {
   router->ls_info = ls_info;
   // copy ls info to each interface
   int i = 0;
   for( i = 0; i < router->num_interfaces ; i++) {
      router->interface[i].router_id = ls_info.router_id;
      router->interface[i].area_id = ls_info.area_id;
      router->interface[i].lsuint = ls_info.lsuint;
   }
}

void set_ls_info_id(sr_router* router, uint32_t area_id, uint16_t lsuint) {
   printf(" ** set_ls_info_id(..) called \n");
   ls_info_t ls_info = make_ls_info_id(router, area_id, lsuint);
   set_ls_info(router, ls_info);
}

void display_ls_info(ls_info_t ls_info) {
   printf(" ** id: %u, area id: %u, lsu int: %u, lsu seq: %lu\n", ls_info.router_id, ls_info.area_id, ls_info.lsuint, ls_info.lsu_seq);
}

void set_ping_info(sr_router* router, uint32_t rest, uint32_t ip, int fd) {
   router->ping_info.rest = rest;
   router->ping_info.ip = ip;
   router->ping_info.fd = fd;
}

void clear_ping_info(sr_router* router) {
   router->ping_info.rest = 0;
   router->ping_info.ip = 0;
   router->ping_info.fd = 0;
}

bool check_ospf_status(sr_router* router) {
   return router->ospf_status;
}

void hw_init(sr_router* router) {
   printf(" ** hw_init(..) called\n");
   router->hw_device.device_name = DEFAULT_IFACE;
    if(check_iface(&router->hw_device) == 0) {
       int ret = openDescriptor(&router->hw_device);
       if(ret < 0) {
          printf(" ** hw_init(..): error opening device\n");
          exit(1);
       }
    }
}

void toggle_reroute_multipath_status(sr_router* router, bool status) {
   router->reroute_multipath_status = status;
}


bool show_reroute_multipath_status(sr_router* router) {
   return router->reroute_multipath_status;
}

void toggle_reroute_multipath() {
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   char *str;
   bool status = show_reroute_multipath_status(router);
   if (status == TRUE) {
      toggle_reroute_multipath_status(router, FALSE);
      asprintf(&str, "Reroute and Multipath disabled\n");
      cli_send_str(str);
      free(str);
   } else {
      toggle_reroute_multipath_status(router, TRUE);
      asprintf(&str, "Reroute and Multipath enabled\n");
      cli_send_str(str);
      free(str);
   }
}

void show_reroute_multipath() {
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   char *str;
   bool status = show_reroute_multipath_status(router);
   if (status == TRUE) {
      asprintf(&str, "Reroute and Multipath is enabled\n");
      cli_send_str(str);
      free(str);
   } else {
      asprintf(&str, "Reroute and Multipath is disabled\n");
      cli_send_str(str);
      free(str);
   }
}
