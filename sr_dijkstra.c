#include "sr_dijkstra.h"
#include "sr_neighbor_db.h"
#include "sr_neighbor.h"
#include "sr_interface.h"
#include "sr_rtable.h"
#include "cli/helper.h"

node* dijkstra(sr_router* router, interface_t* select_intf) {
   printf(" ** dijkstra(..) called\n");
   // tentative list
   node* tentative = llist_new();
   // confirmed list
   node* confirmed = llist_new();
   node* lsp, *ret_tentative, *ret_confirmed, *ret_lowest;
   subnet_entry_t* subnet_entry;
   dijkstra_list_data_t *self_data/*to hold self info*/, *next/*router under consideration*/, *check_tentative, *check_lowest, *lowest;
   int min = 0;
   // to check whether reroute/multipath is being run for a specific interface
   bool select_intf_flag = FALSE;
   //get self entry from db
   neighbor_vertex_t* self_vertex = find_self(router);
   //display_neighbor_vertex_t((void*) self_vertex);
   // create list entry
   self_data = (dijkstra_list_data_t*) malloc_or_die(sizeof(dijkstra_list_data_t));
   // give self a cost of zero
   self_data->cost = 0;
   self_data->destination = self_vertex;
   // self would have no next hop
   self_data->next_hop = 0;
   //add to confirmed list
   confirmed = llist_insert_sorted(confirmed, predicate_dijkstra_list_data_t_sort_cost, (void*) self_data);
   next = (dijkstra_list_data_t*) confirmed->data;
   next->destination->visited = 1;
   // loop?
   while(1) {
      //display newly entered to confirmed list
      display_dijkstra_list_data_t(next);
      // get lsus of this router
      lsp = next->destination->subnets; 
      //traverse over its subnets
      while(lsp!= NULL) {
         //printf("loop\n");
         subnet_entry = lsp->data; 
         // check that focus is not on a specific interface
         if(select_intf != NULL) {
            printf(" ** dijkstra(..) running for: %s\n", select_intf->name);
            node* select_intf_neighbors = select_intf->neighbor_list;
            pthread_mutex_lock(&select_intf->neighbor_lock); 
            select_intf_flag = llist_exists(select_intf_neighbors, predicate_ip_neighbor_t, (void*) &next->destination->router_entry.router_id);
            pthread_mutex_unlock(&select_intf->neighbor_lock);
         } else { //true by default
            select_intf_flag = TRUE;
         }
         // find neighbor
         if(subnet_entry->router_id != 0 && select_intf_flag == TRUE) {
            printf("router_id: %s\n", quick_ip_to_string(subnet_entry->router_id));
            // for each neighbor create new entry
            dijkstra_list_data_t* new_data = (dijkstra_list_data_t*) malloc_or_die(sizeof(dijkstra_list_data_t));
            new_data->cost = next->cost + 1; 
            // next hop would be current router
            new_data->next_hop = next->destination->router_entry.router_id;
            // destination would be neighbors data
            new_data->destination = find_vertex(router, subnet_entry->router_id); 
            // if neighbors data is found
            if(new_data->destination != NULL) {
               ret_tentative = llist_find(tentative, predicate_dijkstra_list_data_t_vertex_t, (void*) new_data->destination);    
               ret_confirmed = llist_find(confirmed, predicate_dijkstra_list_data_t_vertex_t, (void*) new_data->destination);
               //if entry is neither in tenative nor confirmed, add to tentative
               if(ret_tentative == NULL && ret_confirmed == NULL) {
                  tentative = llist_insert_beginning(tentative, (void*) new_data);
               } else if (ret_tentative != NULL){
               // if present in tentative
               check_tentative = ret_tentative->data;   
               //if cost of new entry is less than current in the list, replace current with new
               if(new_data->cost < check_tentative->cost)
                  tentative = llist_update_beginning_delete(tentative, predicate_dijkstra_list_data_t, (void*) new_data);
            }
         }
      }
      lsp = lsp->next;
   } /*end of lsp while loop*/
    // done with this router
   //if tentative is empty then quit
   //else pick the entry with lowest cost from tenative and add to confirmed
   ret_lowest = tentative;
   if(ret_lowest == NULL) {
      break;
   } else { 
      // choose the first entry as the lowest cost one
      check_lowest = (dijkstra_list_data_t*) ret_lowest->data;
      min = check_lowest->cost;
      lowest = check_lowest;
      // now see if there's any entry lower than it
      while (ret_lowest != NULL) {
         check_lowest = (dijkstra_list_data_t*) ret_lowest->data; 
         if(check_lowest->cost < min) {
            min = check_lowest->cost;
            lowest = check_lowest;
         }
         ret_lowest = ret_lowest->next;
      }
      //remove it from the tentative list
      tentative = llist_remove(tentative, predicate_dijkstra_list_data_t, (void*) check_lowest);
      // add to confirmed list
      confirmed = llist_insert_sorted(confirmed, predicate_dijkstra_list_data_t_sort_cost, (void*) check_lowest);
      // make the newly added router next
      next = check_lowest;
      }
   } /*end of while(1)*/  
    // print confirmed list
    printf(" ** Confirmed list: \n");
    llist_display_all(confirmed, display_dijkstra_list_data_t_list);
    printf(" ** dijkstra(..) finished\n");
    return confirmed;
}

void calculate_routing_table(sr_router* router) {
    printf(" ** calculate_routing_table(..) called\n");
    node* confirmed = dijkstra(router, NULL);
    // delete all dynamic entries
    rrtable_purge_all_type(router->rtable, 'd');
    // now traverse through confirmed list and add entries to routing table
    node* confirmed_first = confirmed;
    node* confirmed_subnets_first;
    dijkstra_list_data_t* d_list_entry;
    neighbor_vertex_t* d_vertex;
    subnet_entry_t* confirmed_subnets_entry;
    bool local_ip = FALSE;
    int i = 0;
    while(confirmed_first != NULL) {
       d_list_entry = confirmed_first->data; 
       // get its router info
       d_vertex = d_list_entry->destination;
       // now get its subnets
       confirmed_subnets_first = d_vertex->subnets;
       // traverse its subnets
       while(confirmed_subnets_first != NULL) {
          confirmed_subnets_entry = confirmed_subnets_first->data;
          // make sure that this subnet doesn't already exist in the routing table
           if(llist_exists(router->rtable->rtable_list, predicate_ip_route_t, (void*) &confirmed_subnets_entry->subnet) == 0) {
               printf("New interface found! ");
               printf("subnet: %s ", quick_ip_to_string(confirmed_subnets_entry->subnet));
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
               for( i = 0; i < router->num_interfaces; i++) {
                  if(router->interface[i].ip == confirmed_subnets_entry->subnet) {
                     intf = &router->interface[i];
                     local_ip = TRUE;
                     printf("True: %s \n", quick_ip_to_string(confirmed_subnets_entry->subnet));
                     break;
                  }
               }
               // if not local then get specific interface
               if(local_ip == FALSE) {
                  printf("False: %s \n", quick_ip_to_string(confirmed_subnets_entry->subnet));
                  intf = get_interface_ip(router, d_vertex->router_entry.router_id & confirmed_subnets_entry->mask);               
               }
               printf("intf: %s\n" , intf->name);
               // add to routing table
               rrtable_route_add(router->rtable, confirmed_subnets_entry->subnet, next_hop, confirmed_subnets_entry->mask, intf,'d');
           }
          confirmed_subnets_first = confirmed_subnets_first->next;
       }
       confirmed_first = confirmed_first->next;
    }
}

void dijkstra2(sr_router* router) {
   printf(" ** dijkstra2(..) called\n");   
   // delete all dynamic entries
   rrtable_purge_all_type(router->rtable, 'd'); 
   int i;
   node *head, *head_subnets, *head_subnets_shortest, *head_shortest;
   neighbor_vertex_t* vertex, *vertex_shortest;
   subnet_entry_t* subnet_entry, *subnet_entry_shortest;
   bool flag_shortest = FALSE;
   for( i = 0; i < router->num_interfaces; i++) {
      //also add as dynamic entries to routing table
      rrtable_route_add(router->rtable, router->interface[i].ip, make_ip_addr("0.0.0.0"), router->interface[i].subnet_mask, &router->interface[i], 'd');
   }
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   head = router->neighbor_db->neighbor_db_list; 
   while( head!= NULL) {
      vertex = (neighbor_vertex_t*) head->data; 
      // check if not self
      if(vertex->router_entry.router_id != router->ls_info.router_id) {
      // now traverse subnets
         head_subnets = vertex->subnets;
         while (head_subnets!= NULL) {
            flag_shortest = FALSE;
            subnet_entry = (subnet_entry_t*) head_subnets->data;
            if(llist_exists(router->rtable->rtable_list, predicate_ip_route_t, (void*) &subnet_entry->subnet) == 0) {
               printf("subnet%s\n", quick_ip_to_string(subnet_entry->subnet));
               printf("next hop%s\n", quick_ip_to_string(vertex->router_entry.router_id));
               printf("mask%s\n", quick_ip_to_string(subnet_entry->mask));
               interface_t* intf = get_interface_ip(router, vertex->router_entry.router_id & subnet_entry->mask);
               printf("intf %s\n" , intf->name);
               // check if shortest hop 
               // this would be when no other router has that subnet directly
               head_shortest = router->neighbor_db->neighbor_db_list;
               while( head_shortest!= NULL) {
                  vertex_shortest = (neighbor_vertex_t*) head_shortest->data;
                  // make sure that not same router or self router
                  if(vertex_shortest->router_entry.router_id != router->ls_info.router_id && vertex_shortest->router_entry.router_id != vertex->router_entry.router_id) { 
                     head_subnets_shortest = vertex->subnets;
                     while(head_subnets_shortest != NULL) {
                        subnet_entry_shortest = (subnet_entry_t*) head_subnets_shortest->data; 
                        if((subnet_entry_shortest->router_id & subnet_entry->mask) == subnet_entry->subnet) {
                           flag_shortest = TRUE;
                        }
                        head_subnets_shortest = head_subnets_shortest->next;
                     }
                  }
                  head_shortest = head_shortest->next;
               }
               if(flag_shortest == FALSE) {
                  rrtable_route_add(router->rtable, subnet_entry->subnet, vertex->router_entry.router_id, subnet_entry->mask, get_interface_ip(router, vertex->router_entry.router_id & subnet_entry->mask),'d');
               }
            }
            head_subnets = head_subnets->next;
         }
      }
      head = head->next;
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}

void display_dijkstra_list_data_t(dijkstra_list_data_t* data) {
   display_neighbor_vertex_t(data->destination);   
   printf(" cost: %d, next_hop: %s\n", data->cost, quick_ip_to_string(data->next_hop));
}
