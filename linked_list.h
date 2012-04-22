#pragma once

#include "sr_common.h"

typedef struct Node  
{  
  void* data;  
  struct Node *next;  
}node;

node *llist_new();
node* llist_insert_beginning(node*, void*);
void llist_display_all(node* Head, int(*func)(void*));
node* llist_remove(node* Head, int(*func)(void*,void*), void* num);
int display_string(void *data);
int predicate_string(void* node, void* data);
node* llist_delete(node* Head, int* count);
node* llist_remove_all_no_count(node* Head, int(*func)(void*,void*), void* num);
node* llist_find(node *, int(*func)(void*,void*), void*);
bool llist_exists(node *, int(*func)(void*,void*), void *);
int predicate_ip(void*, void*);
int predicate_mac(void*, void*);
int predicate_timeval2(void*, void*);
int display_cache_entry(void *);
int predicate_ip_update_t(void *listdata, void *ip);
int predicate_ip_packet_t(void *listdata, void *ip);
node* llist_delete_no_count(node* ptr);
node* llist_remove_all(node* Head, int(*func)(void*,void*), void* num, int* count);
node* llist_update_beginning(node *head, int(*func)(void*,void*), void *data);
int display_cache_entry_str(void *data, void* str, void* str_len);
void llist_display_all_str(node* Head, int(*func)(void*, void*, void*), char* str);
node* llist_insert_sorted(node*, int(*func)(void*,void*), void*);
int predicate_ip_sort_route_t(void*, void*);
int display_route_t(void *data);
int predicate_ip_route_t(void*, void*);
int predicate_ip_route_t_prefix_match(void *listdata, void *ip);
int display_update_t(void *);
int display_packet_t(void *);
int predicate_ip_neighbor_t(void *listdata, void *ip);
int display_neighbor_t(void *data);
node* llist_update_beginning_delete(node *head, int(*func)(void*,void*), void *data);
int predicate_ip_arp_queue_entry_t(void *listdata, void *ip);
int predicate_timeval_neighbor_t(void* item, void* time);
int display_neighbor_t(void* data);
int predicate_id_neighbor_t(void *listdata, void *id);
int llist_size(node *head);
int display_neighbor_vertex_t(void* data);
node* llist_update_sorted_delete(node *head, int(*func)(void*,void*), void *data);
int llist_size_predicate(node *head, int(*func)(void*,void*), void *data);
void llist_display_all_predicate(node* Head, int(*func)(void*), int(*func2)(void*,void*), void* data);
int predicate_vertex_router_id(void *listdata, void *id);
int predicate_subnet_entry_subnet(void *listdata, void *subnet);
int predicate_vertex_t_router_id(void *listdata, void *data);
int display_subnet_entry_t(void* data);
int predicate_neighbor_t(void *listdata, void *neighbor);
int predicate_subnet_entry(void *listdata, void *subnet_entry);
int predicate_vertex_t(void *listdata, void *vertex);
int predicate_route_t_type(void *listdata, void *type);
int predicate_timeval_vertex_t(void* item, void* time, void* router_id);
node* llist_remove_all_no_count_3(node* Head, int(*func)(void*,void*, void*), void* num, void* arg);
int predicate_dijkstra_list_data_t(void *listdata, void *data);
int predicate_id_neighbor_t_neighbor_t(void *listdata, void *neighbor);
int predicate_dijkstra_list_data_t_vertex_t(void *listdata, void *vertex);
int predicate_dijkstra_list_data_t_sort_cost(void* listdata1, void* listdata2);
int display_dijkstra_list_data_t_list(void* data);
