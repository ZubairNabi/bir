#include "sr_neighbor_db.h"
#include "cli/helper.h"
#include "sr_integration.h"
#include "cli/cli.h"

#include <string.h>
#include <stdlib.h>

void neighbor_db_init(sr_router* router) {
   printf(" ** neighbor_db_init(..) called \n");
   router->neighbor_db = (neighbor_db_t*) malloc_or_die(sizeof(neighbor_db_t));
   router->neighbor_db->neighbor_db_list = llist_new();
   pthread_mutex_init(&router->neighbor_db->neighbor_db_lock, NULL);
   // now add interfaces and static entries to db
   neighbor_db_add_interfaces_static(router);
}

void neighbor_db_add_interfaces_static(sr_router* router) {
   printf(" ** neighbor_db_add_interfaces_static(..) called \n");
   int i;
   router_entry_t src = create_router_entry_t(router->interface[0].router_id, router->interface[0].area_id);
   // insert the vertex with just router entry
   add_neighbor_vertex_t(router, src);
   subnet_entry_t *subnet;
   for( i = 0; i < router->num_interfaces; i++) {
      // store subnet: apply mask
      subnet = create_subnet_entry_t((router->interface[i].ip & router->interface[i].subnet_mask), router->interface[i].subnet_mask, 0);
      //also add as dynamic entries to routing table
      rrtable_route_add(router->rtable, router->interface[i].ip, make_ip_addr("0.0.0.0"), router->interface[i].subnet_mask, &router->interface[i], 'd');
      add_subnet_entry_t(router, src, subnet);
   }
   // now static entries from static routing table
     //lock table
   pthread_mutex_lock(&router->rtable->lock_rtable);
   node* first = router->rtable->rtable_list;
   route_t* route;
   while( first!= NULL) {
      route = (route_t*) first->data;
      // check if static
      if(route->type == 's') {
         subnet = create_subnet_entry_t((route->destination & route->subnet_mask), route->subnet_mask, 0);
         add_subnet_entry_t(router, src, subnet);
      }
      first = first->next;
   }
   pthread_mutex_unlock(&router->rtable->lock_rtable);
   // now update vertex
   update_neighbor_vertex_t(router, src);
}

subnet_entry_t* create_subnet_entry_t_advert(pwospf_ls_advert_t* advert) {
   subnet_entry_t* subnet_entry = (subnet_entry_t*) malloc_or_die(sizeof(subnet_entry));
   subnet_entry->subnet = advert->subnet;
   subnet_entry->mask = advert->mask;
   subnet_entry->router_id = advert->router_id;
   return subnet_entry;
}

subnet_entry_t* create_subnet_entry_t(uint32_t subnet, uint32_t mask, uint32_t router_id) {
   subnet_entry_t* subnet_entry = (subnet_entry_t*) malloc_or_die(sizeof(subnet_entry));
   subnet_entry->subnet = subnet;
   subnet_entry->mask = mask;
   subnet_entry->router_id = router_id;
   return subnet_entry;
}

router_entry_t create_router_entry_t(uint32_t router_id, uint32_t area_id) {
   router_entry_t router_entry;
   router_entry.router_id = router_id;
   router_entry.area_id = area_id;
   return router_entry;
}

neighbor_vertex_t* create_neighbor_vertex_t(router_entry_t r_entry) {
   printf(" ** create_neighbor_vertex_t(..) called \n");
   neighbor_vertex_t* neighbor_vertex = (neighbor_vertex_t*) malloc_or_die(sizeof(neighbor_vertex_t));
   neighbor_vertex->router_entry = r_entry;
   neighbor_vertex->subnets = llist_new();
   neighbor_vertex->adjacencies = NULL;
   neighbor_vertex->visited = 0;
   neighbor_vertex->cost = 0;
   neighbor_vertex->timestamp = (struct timeval*) malloc_or_die(sizeof(struct timeval));
   gettimeofday(neighbor_vertex->timestamp, NULL);
   return neighbor_vertex;
}

void add_subnet_entry_t(sr_router* router, router_entry_t r_entry, subnet_entry_t* subnet_entry) {
   printf(" ** add_subnet_entry_t(..) called \n");
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   node* ret = llist_find(router->neighbor_db->neighbor_db_list, predicate_vertex_router_id, (void*) &r_entry.router_id);
   if(ret!=NULL) {
      neighbor_vertex_t *vertex = (neighbor_vertex_t*) ret->data;
      vertex->subnets = llist_insert_beginning(vertex->subnets, (void*) subnet_entry);
      router->neighbor_db->neighbor_db_list = llist_update_beginning_delete(router->neighbor_db->neighbor_db_list, predicate_vertex_t_router_id, (void*) vertex);  
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}

void add_neighbor_vertex_t(sr_router* router, router_entry_t r_entry) {
   printf(" ** add_neighbor_vertex_t(..) called\n");
   neighbor_vertex_t* neighbor_vertex = create_neighbor_vertex_t(r_entry);
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   router->neighbor_db->neighbor_db_list = llist_insert_beginning(router->neighbor_db->neighbor_db_list, (void*) neighbor_vertex);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}

bool compare_router_entry_t(router_entry_t *r1, router_entry_t *r2) {
   if(r1->router_id == r2->router_id && r1->area_id == r2->area_id)
      return 1;
   return 0;
}

bool compare_subnet_entry_t(subnet_entry_t *s1, subnet_entry_t *s2) {
   if(s1->subnet == s2->subnet && s1->mask == s2->mask && s1->router_id == s2->router_id)
      return 1;
   return 0;
}

void update_neighbor_vertex_t_timestamp(sr_router* router, router_entry_t r_entry) {
    printf(" ** update_neighbor_vertex_t_timestamp(..) called\n");
   neighbor_vertex_t* vertex;
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   node* ret = llist_find(router->neighbor_db->neighbor_db_list, predicate_vertex_router_id, (void*) &r_entry.router_id);
   if(ret!=NULL) {
         vertex = (neighbor_vertex_t*) ret->data;
         gettimeofday(vertex->timestamp, NULL); 
         router->neighbor_db->neighbor_db_list = llist_update_beginning_delete(router->neighbor_db->neighbor_db_list, predicate_vertex_t_router_id, (void*) vertex);
   }
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

void update_neighbor_vertex_t_rid(sr_router* router, uint32_t ip, uint32_t rid) {
   printf(" ** update_neighbor_vertex_t_rid(..) called \n");
   // first find the vertex
   // vertex will be self vertex so use router id
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   node* ret = llist_find(router->neighbor_db->neighbor_db_list, predicate_vertex_router_id, (void*) &router->ls_info.router_id);
   if (ret != NULL) {
      neighbor_vertex_t* vertex = (neighbor_vertex_t*) ret->data;
      // now find the subnet in the list
      node* ret_subnet = llist_find(vertex->subnets, predicate_subnet_entry_subnet, (void*) &ip);
      //if found
      if(ret_subnet != NULL) {
         subnet_entry_t* subnet_entry = (subnet_entry_t*) ret_subnet->data;
         // update router id
         subnet_entry->router_id = rid;
         printf(" ** update_neighbor_vertex_t_rid(..): updating router id\n");
         printf("subnet %s\n", quick_ip_to_string(subnet_entry->subnet));
         printf("mask %s\n", quick_ip_to_string(subnet_entry->mask));
         printf("rid %s\n", quick_ip_to_string(subnet_entry->router_id));
         //reinsert back into subnets list
         vertex->subnets = llist_update_beginning_delete(vertex->subnets, predicate_subnet_entry, (void*) subnet_entry);
        // now reinsert vertex back into db
         router->neighbor_db->neighbor_db_list = llist_update_beginning_delete(router->neighbor_db->neighbor_db_list, predicate_vertex_t, (void*) vertex);
      }
      pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   }
}

bool check_neighbor_vertex_t_router_entry_t_id(sr_router* router, uint32_t id) {
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   bool ret = llist_exists(router->neighbor_db->neighbor_db_list, predicate_vertex_router_id, (void*) &id);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   return ret;
}

byte* get_ls_adverts_src(sr_router* router, int* size) {
   printf(" ** get_ls_adverts_src(..) called \n");
   int list_size = size_neighbor_vertex_t_src(router, router->interface[0].router_id);
   *size = list_size;
   byte* ls_adverts = (byte*) malloc_or_die(sizeof(pwospf_ls_advert_t) * list_size);
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   neighbor_vertex_t *neighbor_vertex;
   int i = 0;
   // first find source vertex
   node* ret = llist_find(router->neighbor_db->neighbor_db_list, predicate_vertex_router_id, (void*) &router->interface[0].router_id);
   //if found
   if(ret != NULL) {
      neighbor_vertex = ret->data;
      node* head = neighbor_vertex->subnets;
      subnet_entry_t* subnet_entry;
      // now iterate through subnets
      while(head != NULL) {
         subnet_entry = head->data;
         memcpy(ls_adverts + sizeof(pwospf_ls_advert_t) * i, &subnet_entry->subnet, 4);
         memcpy(ls_adverts + 4 + sizeof(pwospf_ls_advert_t) * i, &subnet_entry->mask, 4);
         memcpy(ls_adverts + 8 + sizeof(pwospf_ls_advert_t) * i, &subnet_entry->router_id, 4);
         head = head->next;
         i++;
      }
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   return ls_adverts;
}

int size_neighbor_vertex_t_src(sr_router* router, uint32_t id) {
   int size = 0;
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   // first find src vertex
   node *ret = llist_find(router->neighbor_db->neighbor_db_list, predicate_vertex_router_id, (void*) &id);
   if(ret != NULL) {
      neighbor_vertex_t* vertex = ret->data;
      size = llist_size(vertex->subnets);
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
   return size;
}

void display_neighbor_vertices_src(sr_router* router) {
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   llist_display_all_predicate(router->neighbor_db->neighbor_db_list, display_neighbor_vertex_t, predicate_vertex_router_id, (void*) &router->interface[0].router_id);
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}

void display_neighbor_vertices_str() {
   char *str;
   node* v_head, *s_head;
   neighbor_vertex_t* vertex;
   subnet_entry_t* subnet_entry;
   // get instance of router 
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   // iterate through all vertices
   v_head = router->neighbor_db->neighbor_db_list;
   while(v_head != NULL) {
      vertex = v_head->data; 
      asprintf(&str, "Router Id: %s, Area Id: %lu\n", quick_ip_to_string(vertex->router_entry.router_id), vertex->router_entry.area_id);
      cli_send_str(str);
      free(str);
      // now interate through all subnets of this vertex
      s_head = vertex->subnets;
      asprintf(&str, "Subnet, Mask, Router Id\n");
      cli_send_str(str);
      free(str);
      while(s_head != NULL) {
         subnet_entry = s_head->data; 
         asprintf(&str, "%s, ", quick_ip_to_string(subnet_entry->subnet));
     	 cli_send_str(str);
      	 free(str);
         asprintf(&str, "%s, ", quick_ip_to_string(subnet_entry->mask));
         cli_send_str(str);
         free(str);
         asprintf(&str, "%s\n", quick_ip_to_string(subnet_entry->router_id));
         cli_send_str(str);
         free(str);
         s_head = s_head->next;
      }
      v_head = v_head->next;
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}

void update_neighbor_vertex_t(sr_router* router, router_entry_t r_entry) {
   printf(" ** update_neighbor_vertex_t(..) called\n");
   neighbor_vertex_t* temp_vertex;
   pthread_mutex_lock(&router->neighbor_db->neighbor_db_lock);
   node* ret = llist_find(router->neighbor_db->neighbor_db_list, predicate_vertex_router_id, (void*) &r_entry.router_id);
   if( ret!= NULL) {
      temp_vertex = ret->data;
      gettimeofday(temp_vertex->timestamp, NULL);
      router->neighbor_db->neighbor_db_list = llist_update_beginning_delete(router->neighbor_db->neighbor_db_list, predicate_vertex_t, (void*) temp_vertex);
   }
   pthread_mutex_unlock(&router->neighbor_db->neighbor_db_lock);
}

void add_interfaces_to_rtable(sr_router* router) {
   printf(" ** add_interfaces_to_rtable(..) called \n");
   int i;
   for( i = 0; i < router->num_interfaces; i++) {
      //add as dynamic entries to routing table
      rrtable_route_add(router->rtable, router->interface[i].ip, make_ip_addr("0.0.0.0"), router->interface[i].subnet_mask, &router->interface[i], 'd');
   }
}
