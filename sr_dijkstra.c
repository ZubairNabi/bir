#include "sr_dijkstra.h"
#include "sr_neighbor_db.h"
#include "sr_neighbor.h"
#include "sr_interface.h"
#include "sr_rtable.h"
#include "cli/helper.h"

void dijkstra(sr_router* router) {
   printf(" ** dijkstra(..) called\n");
   // tentative list
   node* tentative = llist_new();
   // confirmed list
   node* confirmed = llist_new();
   node* lsp, *ret_tentative, *ret_confirmed, *ret_lowest;
   subnet_entry_t* subnet_entry;
   dijkstra_list_data_t *self_data/*to hold self info*/, *next/*router under consideration*/, *check_tentative, *check_lowest, *lowest;
   int min = 0;
   //get self entry from db
   neighbor_vertex_t* self_vertex = find_self(router);
   // create list entry
   self_data = (dijkstra_list_data_t*) malloc_or_die(sizeof(dijkstra_list_data_t));
   // give self a cost of zero
   self_data->cost = 0;
   self_data->destination = self_vertex;
   // self would have no next hop
   self_data->next_hop = 0;
   //add to confirmed list
   confirmed = llist_insert_beginning(confirmed, (void*) self_data);
   // loop?
   while(1) {
   printf("while loop\n");
   next = (dijkstra_list_data_t*) confirmed->data;
   next->destination->visited = 1;
   // get lsus of this router
   lsp = next->destination->subnets; 
   //traverse over its subnets
   while(lsp!= NULL) {
      printf("loop\n");
      subnet_entry = lsp->data; 
      // find neighbor
      if(subnet_entry->router_id != 0) {
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
            ret_confirmed = llist_find(tentative, predicate_dijkstra_list_data_t_vertex_t, (void*) new_data->destination);
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
   } // done with this router
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
         //TODO: remove it from the tentative list
         // add to confirmed list
         confirmed = llist_insert_beginning(confirmed, (void*) check_lowest);
      }
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
               interface_t* intf = get_interface_from_ip(router, vertex->router_entry.router_id);
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
                  rrtable_route_add(router->rtable, subnet_entry->subnet, vertex->router_entry.router_id, subnet_entry->mask, get_interface_from_ip(router, vertex->router_entry.router_id),'d');
               }
            }
            head_subnets = head_subnets->next;
         }
      }
      head = head->next;
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}
