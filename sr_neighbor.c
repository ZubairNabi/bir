#include "sr_neighbor.h"
#include "cli/helper.h"

#include <time.h>
#include <sys/time.h>

neighbor_t* make_neighbor(uint32_t id, addr_ip_t ip, uint16_t helloint, addr_ip_t mask) {
   neighbor_t* neighbor = (neighbor_t*) malloc_or_die(sizeof(neighbor_t));
   neighbor->id = id;
   neighbor->ip = ip;
   neighbor-> helloint = helloint;
   neighbor-> mask = mask;
   neighbor->timestamp = (struct timeval*) malloc_or_die(sizeof(struct timeval));
   neighbor->last_adverts = NULL;
   gettimeofday(neighbor->timestamp, NULL);
   return neighbor;
}

void add_neighbor(interface_t* intf, uint32_t id, addr_ip_t ip, uint16_t helloint, addr_ip_t mask) {
   neighbor_t* neighbor = make_neighbor(id, ip, helloint, mask);
   pthread_mutex_lock(&intf->neighbor_lock);
   intf->neighbor_list = llist_insert_beginning( intf->neighbor_list, (void*) neighbor);
   pthread_mutex_unlock(&intf->neighbor_lock);
}

neighbor_t* make_neighbor_lsu(uint32_t id, addr_ip_t ip, uint16_t helloint, addr_ip_t mask, pwospf_lsu_packet_t lsu_packet, byte* adverts) {
   neighbor_t* neighbor = (neighbor_t*) malloc_or_die(sizeof(neighbor_t));
   neighbor->id = id;
   neighbor->ip = ip;
   neighbor-> helloint = helloint;
   neighbor-> mask = mask;
   neighbor->timestamp = (struct timeval*) malloc_or_die(sizeof(struct timeval));
   neighbor->last_adverts = adverts;
   neighbor->last_lsu_packet = lsu_packet;
   gettimeofday(neighbor->timestamp, NULL);
   return neighbor;
}

interface_t* get_interface_from_id(sr_router* router, uint32_t id) {
   int i;
   node* ret;
   neighbor_t* neighbor;
   for( i = 0; i < router->num_interfaces ; i++) {
      pthread_mutex_lock(&router->interface[i].neighbor_lock);      
      ret = llist_find(router->interface[i].neighbor_list, predicate_id_neighbor_t, (void*) &id);
      pthread_mutex_unlock(&router->interface[i].neighbor_lock);
      if( ret!= NULL) {
         neighbor = (neighbor_t*) ret->data;   
         if(neighbor->id == id)
            return &router->interface[i];
      }
   } 
   return NULL;
}

interface_t* get_interface_from_ip(sr_router* router, uint32_t ip) {
   int i;
   node* ret;
   neighbor_t* neighbor;
   for( i = 0; i < router->num_interfaces ; i++) {
      pthread_mutex_lock(&router->interface[i].neighbor_lock);
      ret = llist_find(router->interface[i].neighbor_list, predicate_ip_neighbor_t, (void*) &ip);
      pthread_mutex_unlock(&router->interface[i].neighbor_lock);
      if( ret!= NULL) {
         neighbor = (neighbor_t*) ret->data;
         if(neighbor->ip == ip)
            return &router->interface[i];
      }
   }
   return NULL;
}
