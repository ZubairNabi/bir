#include "sr_neighbor_db.h"
#include "cli/helper.h"

#include <string.h>

void neighbor_db_init(sr_router* router) {
   printf(" ** neighbor_db_init(..) called \n");
   router->neighbor_db = (neighbor_db_t*) malloc_or_die(sizeof(neighbor_db_t));
   router->neighbor_db->neighbor_db_list = llist_new();
   pthread_mutex_init(&router->neighbor_db->neighbor_db_lock, NULL);
   // now add interfaces to db
   neighbor_db_add_interfaces(router);
   // add static entries from rtable
   neighbor_db_add_static(router);
}

void neighbor_db_add_interfaces(sr_router* router) {
   printf(" ** neighbor_db_add_interfaces(..) called \n");
   int i;
   router_entry_t src = create_router_entry_t(router->interface[0].ip, router->interface[0].subnet_mask, router->interface[0].router_id);
   router_entry_t dst;
   for( i = 0; i < router->num_interfaces; i++) {
      // store subnet: apply mask
      dst = create_router_entry_t((router->interface[i].ip & router->interface[i].subnet_mask), router->interface[i].subnet_mask, 0);
      add_neighbor_vertex_t(router, src, dst);
   }
}

void neighbor_db_add_static(sr_router* router) {
   printf(" ** neighbor_db_add_static(..) called \n");
   //lock table
   pthread_mutex_lock(&router->rtable->lock_rtable);
   node* first = router->rtable->rtable_list;
   route_t* route;
   router_entry_t src = create_router_entry_t(router->interface[0].ip, router->interface[0].subnet_mask, router->interface[0].router_id);
   router_entry_t dst;
   while( first!= NULL) {
      route = (route_t*) first->data;
      dst = create_router_entry_t(route->destination, route->subnet_mask, 0); 
      add_neighbor_vertex_t(router, src, dst);
      first = first->next;
   }
   pthread_mutex_unlock(&router->rtable->lock_rtable);
}

router_entry_t create_router_entry_t_advert(pwospf_ls_advert_t* advert) {
   router_entry_t router_entry;
   router_entry.subnet = advert->subnet;
   router_entry.mask = advert->mask;
   router_entry.router_id = advert->router_id;
   return router_entry;
}

router_entry_t create_router_entry_t(uint32_t subnet, uint32_t mask, uint32_t router_id) {
   router_entry_t router_entry;
   router_entry.subnet = subnet;
   router_entry.mask = mask;
   router_entry.router_id = router_id;
   return router_entry;
}

neighbor_vertex_t* create_neighbor_vertex_t(router_entry_t src, router_entry_t dst) {
   neighbor_vertex_t* neighbor_vertex = (neighbor_vertex_t*) malloc_or_die(sizeof(neighbor_vertex_t));
   neighbor_vertex->src = src;
   neighbor_vertex->dst = dst;
   neighbor_vertex->visited = 0;
   neighbor_vertex->cost = 0;
   neighbor_vertex->timestamp = (struct timeval*) malloc_or_die(sizeof(struct timeval));
   gettimeofday(neighbor_vertex->timestamp, NULL);
   return neighbor_vertex;
}

void add_neighbor_vertex_t(sr_router* router, router_entry_t src, router_entry_t dst) {
   printf(" ** add_neighbor_vertex_t(..) called\n");
   neighbor_vertex_t* neighbor_vertex = create_neighbor_vertex_t(src, dst);
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   router->neighbor_db->neighbor_db_list = llist_insert_sorted(router->neighbor_db->neighbor_db_list, predicate_ip_sort_vertex_t, (void*) neighbor_vertex);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}

bool compare_neighbor_vertex_t(neighbor_vertex_t *v1, neighbor_vertex_t *v2) {
   if(compare_router_entry_t(&v1->src, &v2->src) == 1 && compare_router_entry_t(&v1->dst, &v2->dst) == 1)
      return 1;
   return 0;
}

bool compare_router_entry_t(router_entry_t *r1, router_entry_t *r2) {
   if(r1->subnet == r2->subnet && r1->mask == r2->mask && r1->router_id == r2->router_id)
      return 1;
   return 0;
}

bool check_neighbor_vertex_t(sr_router* router, neighbor_vertex_t* vertex) {
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   bool ret = llist_exists(router->neighbor_db->neighbor_db_list, predicate_vertex_neighbor_vertex_t, (void*) vertex);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   return ret;
}

void update_neighbor_vertex_t(sr_router* router, neighbor_vertex_t* vertex) {
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   router->neighbor_db->neighbor_db_list = llist_update_sorted_delete(router->neighbor_db->neighbor_db_list, predicate_vertex_neighbor_vertex_t, (void*) vertex);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}

void display_neighbor_vertices(sr_router* router) {
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   llist_display_all(router->neighbor_db->neighbor_db_list, display_neighbor_vertex_t);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}

int size_neighbor_vertex_t(sr_router* router) {
   int size = 0;
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   size = llist_size(router->neighbor_db->neighbor_db_list);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   return size;
}

byte* get_ls_adverts(sr_router* router, int* size) {
   printf(" ** get_ls_adverts(..) called \n"); 
   int list_size = size_neighbor_vertex_t(router);
   *size = list_size;
   byte* ls_adverts = (byte*) malloc_or_die(sizeof(pwospf_ls_advert_t) * list_size);
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   node* head = router->neighbor_db->neighbor_db_list;
   neighbor_vertex_t *neighbor_vertex;
   int i = 0;
   while(head != NULL) {
      neighbor_vertex = head->data;
      memcpy(ls_adverts + sizeof(pwospf_ls_advert_t) * i, &neighbor_vertex->dst.subnet, 4);
      memcpy(ls_adverts + 4 + sizeof(pwospf_ls_advert_t) * i, &neighbor_vertex->dst.mask, 4);
      memcpy(ls_adverts + 8 + sizeof(pwospf_ls_advert_t) * i, &neighbor_vertex->dst.router_id, 4);
      head = head->next;
      i++;
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   return ls_adverts;
}

void update_neighbor_vertex_t_rid(sr_router* router, uint32_t ip, uint32_t rid) {
   printf(" ** update_neighbor_vertex_t_rid(..) called \n");
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   node* ret = llist_find(router->neighbor_db->neighbor_db_list, predicate_vertex_dst_ip, (void*) &ip);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   if (ret != NULL) {
      neighbor_vertex_t* vertex = (neighbor_vertex_t*) ret->data;
      vertex->dst.router_id = rid;
      pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
      router->neighbor_db->neighbor_db_list = llist_update_sorted_delete(router->neighbor_db->neighbor_db_list, predicate_vertex_neighbor_vertex_t, (void*) vertex);
      pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   }
}

bool check_neighbor_vertex_t_src_ip(sr_router* router, uint32_t ip) {
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   bool ret = llist_exists(router->neighbor_db->neighbor_db_list, predicate_vertex_src_ip, (void*) &ip);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   return ret;
}

bool check_neighbor_vertex_t_dst_ip(sr_router* router, uint32_t ip) {
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   bool ret = llist_exists(router->neighbor_db->neighbor_db_list, predicate_vertex_dst_ip, (void*) &ip);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   return ret;
}

byte* get_ls_adverts_src(sr_router* router, int* size) {
   printf(" ** get_ls_adverts_src(..) called \n");
   int list_size = size_neighbor_vertex_t_src(router, router->interface[0].ip);
   *size = list_size;
   byte* ls_adverts = (byte*) malloc_or_die(sizeof(pwospf_ls_advert_t) * list_size);
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   node* head = router->neighbor_db->neighbor_db_list;
   neighbor_vertex_t *neighbor_vertex;
   int i = 0;
   while(head != NULL) {
      neighbor_vertex = head->data;
      if(predicate_vertex_src_ip((void*) neighbor_vertex, (void*) &router->interface[0].ip) == 1) {
         memcpy(ls_adverts + sizeof(pwospf_ls_advert_t) * i, &neighbor_vertex->dst.subnet, 4);
         memcpy(ls_adverts + 4 + sizeof(pwospf_ls_advert_t) * i, &neighbor_vertex->dst.mask, 4);
         memcpy(ls_adverts + 8 + sizeof(pwospf_ls_advert_t) * i, &neighbor_vertex->dst.router_id, 4);
         i++;
      }
      head = head->next;
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   return ls_adverts;
}

int size_neighbor_vertex_t_src(sr_router* router, uint32_t ip) {
   int size = 0;
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   size = llist_size_predicate(router->neighbor_db->neighbor_db_list, predicate_vertex_src_ip, (void*) &ip);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   return size;
}

void display_neighbor_vertices_src(sr_router* router) {
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   llist_display_all_predicate(router->neighbor_db->neighbor_db_list, display_neighbor_vertex_t, predicate_vertex_src_ip, (void*) &router->interface[0].ip);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}

void update_neighbor_vertex_t_timestamp(sr_router* router, router_entry_t src, router_entry_t dst) {
   printf(" ** update_neighbor_vertex_t_timestamp(..) called \n");
   neighbor_vertex_t* vertex = create_neighbor_vertex_t(src, dst);
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   router->neighbor_db->neighbor_db_list = llist_update_sorted_delete(router->neighbor_db->neighbor_db_list, predicate_vertex_neighbor_vertex_t, (void*) vertex);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}
