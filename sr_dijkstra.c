#include "sr_dijkstra.h"
#include "sr_neighbor_db.h"
#include "sr_neighbor.h"
#include "sr_interface.h"
#include "sr_rtable.h"
#include "cli/helper.h"

void dijkstra(sr_router* router) {
   printf(" ** dijkstra(..) called\n");
   node* tentative = llist_new();
   node* confirmed = llist_new();
   node* lsp, *first, *ret_tentative, *ret_confirmed, *ret_lowest;
   subnet_entry_t* subnet_entry;
   dijkstra_list_data_t* next, *check_tentative, *check_lowest, *lowest;
   int min = 0;
   //get self entry from db
   neighbor_vertex_t* self_vertex = find_self(router);
   // create list entry
   dijkstra_list_data_t* self_data = (dijkstra_list_data_t*) malloc_or_die(sizeof(dijkstra_list_data_t));
   self_data->cost = 0;
   self_data->destination = self_vertex;
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
   first = lsp;
   //traverse over its subnets
   while(first!= NULL) {
      printf("loop\n");
      subnet_entry = first->data; 
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
            ret_tentative = llist_find(tentative, predicate_vertex_t, (void*) new_data->destination);    
            ret_confirmed = llist_find(tentative, predicate_vertex_t, (void*) new_data->destination);
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
      first = first->next;
   } // done with this router
   //if tentative is empty then quit
   //else pick the entry with lowest cost from tenative and add to confirmed
      ret_lowest = tentative;
      if(ret_lowest == NULL) {
         break;
      } else { 
         check_lowest = (dijkstra_list_data_t*) ret_lowest->data;
         min = check_lowest->cost;
         lowest = check_lowest;
         while (ret_lowest != NULL) {
            check_lowest = (dijkstra_list_data_t*) ret_lowest->data; 
            if(check_lowest->cost < min) {
               min = check_lowest->cost;
               lowest = check_lowest;
            }
            ret_lowest = ret_lowest->next;
         }
         confirmed = llist_insert_beginning(confirmed, (void*) check_lowest);
      }
}   
}

void dijkstra2(sr_router* router) {
   printf(" ** dijkstra2(..) called\n");   
   // delete all dynamic entries
   rrtable_purge_all_type(router->rtable, 'd'); 
   int i;
   node *head, *head_subnets;
   neighbor_vertex_t* vertex;
   subnet_entry_t* subnet_entry;
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
            subnet_entry = (subnet_entry_t*) head_subnets->data;
            if(llist_exists(router->rtable->rtable_list, predicate_ip_route_t, (void*) &subnet_entry->subnet) == 0) {
               printf("subnet%s\n", quick_ip_to_string(subnet_entry->subnet));
               printf("next hop%s\n", quick_ip_to_string(vertex->router_entry.router_id));
               printf("mask%s\n", quick_ip_to_string(subnet_entry->mask));
               interface_t* intf = get_interface_from_ip(router, vertex->router_entry.router_id);
               printf("intf %s\n" , intf->name);
               rrtable_route_add(router->rtable, subnet_entry->subnet, vertex->router_entry.router_id, subnet_entry->mask, get_interface_from_ip(router, vertex->router_entry.router_id),'d');
            }
            head_subnets = head_subnets->next;
         }
      }
      head = head->next;
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}
