#include "sr_dijkstra.h"
#include "sr_neighbor_db.h"
#include "sr_neighbor.h"
#include "sr_interface.h"
#include "sr_rtable.h"

void dijkstra(sr_router* router) {
   printf(" ** dijkstra(..) called\n");
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   node* head = router->neighbor_db->neighbor_db_list;
   neighbor_vertex_t* vertex;
   node* temp_head;
   neighbor_vertex_t* temp_vertex;
   int i;
   bool exists = 0;
   while( head!= NULL) {
      vertex = (neighbor_vertex_t*) head->data;
      // check if not self entry
      if(vertex->src.router_id != router->ls_info.router_id) {
         //check if entry already doesn't exist in self
         temp_head = router->neighbor_db->neighbor_db_list; 
         while(temp_head != NULL) {
            temp_vertex = (neighbor_vertex_t*) temp_head->data;
            if(temp_vertex->src.router_id == router->ls_info.router_id) {
               //self entry
               // check if no record in self
               if((temp_vertex->dst.subnet & temp_vertex->dst.mask) == (vertex->dst.subnet & vertex->dst.mask))  {
                  exists = 1;
               }
            }
            temp_head = temp_head->next;
         }
         // no record found, so new entry
         if(exists == 0) {
           display_neighbor_vertex_t((void*) vertex);
              display_neighbor_vertex_t((void*) temp_vertex);
               // does not exist in self: mark as visited and increment cost
                vertex->visited = 1;
                vertex->cost = vertex->cost + 1;
                // put it back in the neighbor db
                head->data = (void*) vertex;
         }
      }
      head = head->next;
   }
   //at this point all neighbors have been traversed and costs calculated
   // add all 'visited' nodes to neighbor db: these are not present in the routing table
   neighbor_t* neighbor;
   interface_t* intf;
   bool flag = 0;
   head = router->neighbor_db->neighbor_db_list;
   while( head!= NULL) {
      vertex = (neighbor_vertex_t*) head->data;
      if(vertex->visited == 1)
         // get neighbor info
         for(i = 0; i<router->num_interfaces ; i++) {
            intf = &router->interface[i];
            temp_head = router->interface[i].neighbor_list;
            while( temp_head != NULL) {
               neighbor = (neighbor_t*) temp_head->data;
               if(vertex->src.router_id == neighbor->id) {
                  flag = 1;
                  break;
               }
               temp_head = temp_head->next;
            }
            if (flag == 1)
               break;
         }
         if(neighbor != NULL && intf != NULL && flag == 1) {
            rrtable_route_add(router->rtable, vertex->dst.subnet, neighbor->ip, vertex->dst.mask, intf, 'd');
         }
      head = head->next;
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}
