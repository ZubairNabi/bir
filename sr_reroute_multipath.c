#include "sr_reroute_multipath.h"
#include "sr_dijkstra.h"
#include "sr_rtable_hw.h"
#include "sr_interface.h"
#include "cli/helper.h"
#include "sr_neighbor.h"

void reroute_multipath(sr_router* router) {
   printf(" ** reroute_multipath(..) called \n");
   int i = 0;
   //to hold the confirmed list for each interface
   node* confirmed[4];
   // list of multiple paths with costs
   node* multipath_list = llist_new();
   //run dijkstra for each interface
   for(i = 0; i < router->num_interfaces; i++) {
      confirmed[i] = dijkstra(router, &router->interface[i]); 
   }
   // delete all entries
   hw_rrtable_purge_all(router->hw_rtable);
   for(i = 0; i < router->num_interfaces; i++) {
      // now traverse through confirmed list and add entries to routing table
      node* confirmed_first = confirmed[i];
      node* confirmed_subnets_first;
      dijkstra_list_data_t* d_list_entry;
      neighbor_vertex_t* d_vertex;
      subnet_entry_t* confirmed_subnets_entry;
      bool local_ip = FALSE;
      int j = 0;
      // display which interface
      printf(" ** reroute_multipath(..) calculating routing table based on: %s\n", router->interface[i].name);
      while(confirmed_first != NULL) {
         d_list_entry = confirmed_first->data;
         // get its router info
         d_vertex = d_list_entry->destination;
         // now get its subnets
         confirmed_subnets_first = d_vertex->subnets;
         // traverse its subnets
         while(confirmed_subnets_first != NULL) {
            confirmed_subnets_entry = confirmed_subnets_first->data;
            printf("subnet: %s ", quick_ip_to_string(confirmed_subnets_entry->subnet));
            printf("rid: %s ", quick_ip_to_string(d_vertex->router_entry.router_id));
            //check if destination isn't self
            uint32_t next_hop = 0;
            if(d_vertex->router_entry.router_id != router->ls_info.router_id) {
               next_hop = d_vertex->router_entry.router_id;
            }
            printf("next hop: %s ", quick_ip_to_string(next_hop));
            printf("mask: %s ", quick_ip_to_string(confirmed_subnets_entry->mask));
            interface_t* intf = NULL;
            local_ip = FALSE;
            //check if destination is a local interface
            for( j = 0; j < router->num_interfaces; j++) {
               if((router->interface[j].ip & confirmed_subnets_entry->mask)  == confirmed_subnets_entry->subnet) {
                  intf = &router->interface[j];
                  local_ip = TRUE;
                  printf("True: %s \n", quick_ip_to_string(confirmed_subnets_entry->subnet));
                  break;
                }
            }
             // if not local then get specific interface
            if(local_ip == FALSE) {
                 printf("False: %s \n", quick_ip_to_string(confirmed_subnets_entry->subnet));
                 intf = get_interface_ip(router, d_vertex->router_entry.router_id & confirmed_subnets_entry->mask);
                 // try neighbor list
                 if(intf == NULL) {
                    intf = get_interface_from_id(router, d_vertex->router_entry.router_id);
                 }
            }
            printf("intf: %s\n" , intf->name);
            // check whether we already have the entry in our multipath list
            node* multipath_list_node = llist_find(multipath_list, predicate_multipath_list_data_t_ip, (void*) &confirmed_subnets_entry->subnet);
            multipath_list_data_t* multipath_data;
            if(multipath_list_node == NULL) {
               multipath_data = (multipath_list_data_t*) malloc_or_die(sizeof(multipath_list_data_t));
               multipath_data->primary_cost = d_list_entry->cost;
               multipath_data->route.destination = confirmed_subnets_entry->subnet;
               multipath_data->route.primary = get_hw_port_from_name(intf->name);
               multipath_data->route.subnet_mask = confirmed_subnets_entry->mask;
               multipath_data->route.backup = 0; 
               multipath_data->primary_flag = 1;
               multipath_list = llist_insert_beginning(multipath_list, (void*) multipath_data);
            } else {
               multipath_data = multipath_list_node->data;
               //check if primary exists
               if(multipath_data->primary_flag == 1) {
                  //check if current cost is the same as primary's
                  if(d_list_entry->cost == multipath_data->primary_cost) {
                     // add this entry too
                     multipath_data->route.primary = multipath_data->route.primary + get_hw_port_from_name(intf->name);
                  } else if (d_list_entry->cost > multipath_data->primary_cost) {
                    // check if backup exists or cost less than present back
                     if(multipath_data->route.backup == 0 || d_list_entry->cost < multipath_data->backup_cost) {
                        //insert as backup
                        multipath_data->route.backup = get_hw_port_from_name(intf->name);
                        multipath_data->backup_cost = d_list_entry->cost;
                     } else {
                        //check if cost the same as backup
                        if(d_list_entry->cost == multipath_data->backup_cost) {
                           // add to backup
                           multipath_data->route.backup = multipath_data->route.backup + get_hw_port_from_name(intf->name);
                        }
                     }
                  } else if (d_list_entry->cost < multipath_data->primary_cost) {
                     //replace as primary
                      multipath_data->route.primary = get_hw_port_from_name(intf->name);
                      multipath_data->primary_cost = d_list_entry->cost;
                    }

               }
            }
            confirmed_subnets_first = confirmed_subnets_first->next;
         }/*end of while loop subnets*/
         confirmed_first = confirmed_first->next;
      }/*end of while loop*/
   }/*end of for loop*/ 
   // display multipath list
   llist_display_all(multipath_list, display_multipath_list_data_t);
}
