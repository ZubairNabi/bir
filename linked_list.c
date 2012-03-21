#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "linked_list.h"
#include "cli/helper.h"
#include "sr_arp_cache_entry.h"
#include "packet_dispatcher.h"
#include "sr_rtable.h"
#include "sr_neighbor.h"
#include "sr_pwospf_types.h"
#include "sr_neighbor_db.h"

node *llist_new() {
   node* head = NULL;
   return head;
}

node* llist_insert_beginning(node* Head, void* num) {
   node *temp= (node*) malloc_or_die(sizeof(node));
   temp->data = num;
   temp->next = NULL;
   if (Head == NULL) {
      Head=temp;
      temp->next=NULL;
   }
   else {
      temp->next=Head;
      Head=temp;
   }
   return Head;
}

void llist_display_all(node* Head, int(*func)(void*)) {
   node* cur_ptr = Head;
   if(cur_ptr == NULL) {
      printf("Empty\n");
   }
   else {
       while(cur_ptr != NULL) {
           func(cur_ptr->data);
           cur_ptr = cur_ptr->next;
       }
   }
}

node* llist_remove(node* Head, int(*func)(void*,void*), void* num) {
   node *prev_ptr, *cur_ptr;
   cur_ptr = Head;
   while(cur_ptr != NULL) {
      if(func(cur_ptr->data, num) == 1) {
         if(cur_ptr==Head) {
            Head=cur_ptr->next;
            free(cur_ptr);
            return Head;
         }
         else {
            prev_ptr->next=cur_ptr->next;
            free(cur_ptr);
            return Head;
         }
      }
      else {
         prev_ptr=cur_ptr;
         cur_ptr=cur_ptr->next;
      }
   }
   return Head;
   printf("Not found\n");
}

int display_string(void *data) {
   printf("%s\n", (char*) data);
   return 0;
}

int predicate_string(void* node, void* data) {
   
   char* node_data = (char*) node;
   char* test_data = (char*) data;
   if(strcmp(node_data, test_data) == 0) 
      return 1;
   else
      return 0;
}

node* llist_delete(node* ptr, int *count) {
   node* temp;
   node* head = NULL;
   if(ptr == NULL) {
      return head;
   }
   while (ptr != NULL) {
      *count = *count + 1;
      temp = ptr->next;
      free(ptr);
      ptr = temp;
   }
   return head;
}

node* llist_remove_all_no_count(node* Head, int(*func)(void*,void*), void* num) {
   node *prev_ptr, *cur_ptr;
   bool found = FALSE;
   cur_ptr = Head;
   while(cur_ptr != NULL) {

      if(func(cur_ptr->data, num) == 1) {
         found = TRUE;

         if(cur_ptr==Head) {
           printf("head!\n");
            Head=cur_ptr->next;
            free(cur_ptr);
            if(cur_ptr->next == NULL)
               return NULL;
            cur_ptr = Head;

         } else {
            prev_ptr->next=cur_ptr->next;
            free(cur_ptr);
            cur_ptr = Head;

         }
      }
      else { 

         prev_ptr=cur_ptr;
         cur_ptr=cur_ptr->next;
      }
   }
   if(found == FALSE)
      printf("Not found\n");
   return Head;
}

node *llist_find(node *head, int(*func)(void*,void*), void *data)
{
        node* temp = head;
        while(temp) {
                if(func(temp->data, data) == 1) return temp;
                temp=temp->next;
        }
        return NULL;
}

bool llist_exists(node *head, int(*func)(void*,void*), void *data) {
   node* list = head;
   if(list == NULL)
      return FALSE;
   if(func(list->data, data) ==1)
      return TRUE;
   while(list->next && func(list->next->data, data) ==0) {
           list=list->next;
        }
        if(list->next) {
           return TRUE;
        }
        else
           return FALSE;
}

int predicate_ip(void *listdata, void *ip) {
        arp_cache_entry_t *listdata_entry = (arp_cache_entry_t*) listdata;
        addr_ip_t *ip_test = (addr_ip_t*) ip;
        if( listdata_entry->ip ==  *ip_test)
           return 1;
        else
           return 0;
}

int predicate_ip_packet_t(void *listdata, void *ip)
{
        packet_t *listdata_entry = (packet_t*) listdata;
        addr_ip_t *ip_test = (addr_ip_t*) ip;
        if( listdata_entry->ip ==  *ip_test)
           return 1;
        else
           return 0;
}

int predicate_ip_update_t(void *listdata, void *ip)
{
        update_t *listdata_entry = (update_t*) listdata;
        addr_ip_t *ip_test = (addr_ip_t*) ip;
        if( listdata_entry->ip ==  *ip_test)
           return 1;
        else
           return 0;
}

/*Predicate function for array list for matching MAC*/
int predicate_mac(void* item, void* mac) {
   arp_cache_entry_t *entry = (arp_cache_entry_t*) item;
   addr_mac_t *mac_test = (addr_mac_t*) mac;
   if(equal_mac(entry->mac, *mac_test) == 1)
        return 1;
   else
        return 0;
}

/*Predicate function for array list for matching timeval*/
int predicate_timeval2(void* item, void* time) {
   arp_cache_entry_t *entry = (arp_cache_entry_t*) item;
   struct timeval *time_test = (struct timeval*) time;
   if(is_later(time_test, entry->timeout) == TRUE)
        return 1;
   else
        return 0;
}

int display_cache_entry(void *data) {
        arp_cache_entry_t *entry = (arp_cache_entry_t*) data;
        addr_mac_t *mac = &entry->mac;
        printf("IP: %s, MAC: %s, Time: %ld.%06ld\n", quick_ip_to_string(entry->ip), quick_mac_to_string(mac->octet), entry->timeout->tv_sec, entry->timeout->tv_usec);
        return 0;
}

node* llist_delete_no_count(node* ptr) {
   node* temp;
   node* head = NULL;
   if(ptr == NULL) {
      return head;
   }
   while (ptr != NULL) {
      temp = ptr->next;
      free(ptr);
      ptr = temp;
   }
   return head;
}

node* llist_remove_all(node* Head, int(*func)(void*,void*), void* num, int* count) {
   node *prev_ptr, *cur_ptr;
   bool found = FALSE;
   cur_ptr = Head;
   while(cur_ptr != NULL) {
      if(func(cur_ptr->data, num) == 1) {
         found = TRUE;
         if(cur_ptr==Head) {
            *count = *count + 1;
            Head=cur_ptr->next;
            free(cur_ptr);
            if(cur_ptr->next == NULL)
               return NULL;
            cur_ptr = Head; 
         }
         else {
            prev_ptr->next=cur_ptr->next;
            free(cur_ptr);
            cur_ptr = Head;
            *count = *count + 1;
         }
      }
      else {
         prev_ptr=cur_ptr;
         cur_ptr=cur_ptr->next;
      }

   }
   if(found == FALSE)
      printf("Not found\n");
   return Head;
}

node* llist_update_beginning(node *head, int(*func)(void*,void*), void *data) {
   node* first = head;
   if(llist_exists(first, func, data) == TRUE)
      return first;
   else {
      first = llist_insert_beginning(first, data);
      return first;
   }
}

int display_cache_entry_str(void *data, void* str_void, void* str_len_void)
{
        char* str = (char*) str_void;
        int *str_len = (int*) str_len_void;
        arp_cache_entry_t *entry = (arp_cache_entry_t*) data;
        addr_mac_t *mac = &entry->mac;
        // make ip string
        char* ip_str_format = "IP: %s\n";
        uint8_t ip_str_len = strlen(quick_ip_to_string(entry->ip) + strlen(ip_str_format));
        char* ip_str = (char*) malloc_or_die(ip_str_len);
        sprintf(ip_str, ip_str_format, quick_ip_to_string(entry->ip));
        // make mac string
        char* mac_str_format = "MAC: %s\n";
        uint8_t mac_str_len = strlen(quick_mac_to_string(mac->octet) + strlen(mac_str_format));
        char* mac_str = (char*) malloc_or_die(mac_str_len);
        sprintf(mac_str, mac_str_format, quick_mac_to_string(mac->octet));
         // make time string
        char* time_str_format = "Time: %ld.%06ld\n";
        uint8_t time_str_len = 20;
        char* time_str = (char*) malloc_or_die(time_str_len);
        sprintf(time_str, time_str_format, entry->timeout->tv_sec, entry->timeout->tv_usec);
        str = (char*) malloc_or_die(strlen(ip_str) + strlen(mac_str) + strlen(time_str));
        strcpy(str, ip_str);  
        strcat(str, mac_str);
        strcat(str, time_str);
        *str_len = strlen(str);
        free(ip_str);
        free(mac_str);
        free(time_str);
        return 0;
}

void llist_display_all_str(node* Head, int(*func)(void*, void*, void*), char* str)
{
   struct Node *cur_ptr;
   char* temp_str = NULL;
   uint8_t temp_str_len = 0;
   char text[] = "Display list contents:\n";
   uint8_t text_len = strlen(text);
   str = (char*) malloc_or_die(text_len);
   strcpy(str, text);

   cur_ptr=Head;

   if(cur_ptr==NULL) {
      printf("List empty\n");
   }
   else {
       while(cur_ptr!=NULL) {
           func(cur_ptr->data, (void*)temp_str, (void*)&temp_str_len);
           str = (char*)realloc((void*) str, temp_str_len);
           strcat(str, temp_str);
           cur_ptr=cur_ptr->next;
       }
   }
   free(str);
}

node* llist_insert_sorted(node* head, int(*func)(void*,void*), void* item) {
    
    node* new_node = (node*) malloc_or_die(sizeof(node));
    new_node->data = item;
    new_node->next = NULL;
    assert (item);

    node* prev_node = NULL;
    node* search_node = head; 
    while ((search_node!= NULL) && 
        (func((void*)search_node->data, (void*)new_node->data) == 1)) {
        prev_node = search_node;
        search_node = search_node->next;
    }   

    if (prev_node == NULL) {
        head = new_node;
    }

    else {
        prev_node->next = new_node;
    }
    new_node->next = search_node;        
    return head;
}

int predicate_ip_sort_route_t(void* listdata1, void* listdata2) {
  route_t *listdata1_route = (route_t*) listdata1;
  route_t *listdata2_route = (route_t*) listdata2;
    if( compare_ip(listdata1_route->destination, listdata2_route->destination) == 1 ||
  compare_ip(listdata1_route->destination, listdata2_route->destination) == 0)
       return 1;
    else
       return 0;
} 

int display_route_t(void *data) {
        route_t *entry = (route_t*) data;
        printf("Type: %c, Destination IP: %s, ", entry->type, quick_ip_to_string(entry->destination));
        printf("Next HOP IP: %s ", quick_ip_to_string(entry->next_hop));
        printf("Subnet Mask: %s Interface:\n", quick_ip_to_string(entry->subnet_mask));
        display_interface(&entry->intf);
        return 0;
}

int predicate_ip_route_t(void *listdata, void *ip) {
        route_t *listdata_entry = (route_t*) listdata;
        addr_ip_t *ip_test = (addr_ip_t*) ip;
        if( listdata_entry->destination ==  *ip_test)
           return 1;
        else
           return 0;
}

int predicate_ip_route_t_prefix_match(void *listdata, void *ip) {
        route_t *listdata_entry = (route_t*) listdata;
        addr_ip_t *ip_test = (addr_ip_t*) ip;
        addr_ip_t subnet = *ip_test & listdata_entry->subnet_mask;
        if( listdata_entry->destination ==  subnet)
           return 1;
        else
           return 0;
}

int display_update_t(void* data) {
   update_t *entry = (update_t*) data;
   printf("IP: %s, MAC: %s\n", quick_ip_to_string(entry->ip), quick_mac_to_string(entry->mac.octet));
   return 0;
}

int display_packet_t(void* data) {
   packet_t *entry = (packet_t*) data;
   printf("IP: %s\n", quick_ip_to_string(entry->ip));
   return 0;
}

int predicate_ip_neighbor_t(void *listdata, void *ip) {
        neighbor_t *listdata_entry = (neighbor_t*) listdata;
        addr_ip_t *ip_test = (addr_ip_t*) ip;
        if( listdata_entry->ip ==  *ip_test)
           return 1;
        else
           return 0;
}

node* llist_update_beginning_delete(node *head, int(*func)(void*,void*), void *data) {
   node* first = head;
   if(llist_exists(first, func, data) == TRUE) {
      first = llist_remove(first, func, data);
      first = llist_insert_beginning(first, data);
      return first;
   }
   else {
      first = llist_insert_beginning(first, data);
      return first;
   }
}

int predicate_ip_arp_queue_entry_t(void *listdata, void *ip) {
        arp_queue_entry_t *listdata_entry = (arp_queue_entry_t*) listdata;
        addr_ip_t *ip_test = (addr_ip_t*) ip;
        if( listdata_entry->ip ==  *ip_test)
           return 1;
        else
           return 0;
}

/*Predicate function for array list for matching timeval*/
int predicate_timeval_neighbor_t(void* item, void* time) {
   neighbor_t *entry = (neighbor_t*) item;
   struct timeval *time_test = (struct timeval*) time;
   time_test->tv_sec = time_test->tv_sec - ntohs(entry->helloint) * 3;
   if(is_later(time_test, entry->timestamp) == TRUE)
        return 1;
   else
        return 0;
}

int display_neighbor_t(void* data) {
   neighbor_t *entry = (neighbor_t*) data;
   printf("ID: %lu, IP: %s, Timestamp: %ld.%06ld, Helloint: %u, Mask: %s", entry->id, quick_ip_to_string(entry->ip), entry->timestamp->tv_sec, entry->timestamp->tv_usec, ntohs(entry->helloint), quick_ip_to_string(entry->mask));
   if(entry->last_adverts == NULL) {
      printf(", no lsus from this neighbor yet\n");
   } else {
      display_lsu_packet(&entry->last_lsu_packet);                                 }
   return 0;
}

int predicate_id_neighbor_t(void *listdata, void *id) {
        neighbor_t *listdata_entry = (neighbor_t*) listdata;
        uint32_t *id_test = (uint32_t*) id;
        if( listdata_entry->id ==  *id_test)
           return 1;
        else
           return 0;
}

int llist_size(node *head) {
   int count = 0;
   node* temp = head;
   while(temp) {
      count++;
      temp = temp->next;
   }
   return count;
}

int display_neighbor_vertex_t(void* data) {
   neighbor_vertex_t *entry = (neighbor_vertex_t*) data;
   printf("Timestamp: %ld.%06ld, ", entry->timestamp->tv_sec, entry->timestamp->tv_usec);
   printf("router_entry: id: %s ", quick_ip_to_string(entry->router_entry.router_id));
   printf("area id: %u\n", entry->router_entry.area_id);
   llist_display_all(entry->subnets, display_subnet_entry_t);
   return 0;
}

node* llist_update_sorted_delete(node *head, int(*func)(void*,void*), void *data) {
   node* first = head;
   if(llist_exists(first, func, data) == TRUE) {
      first = llist_remove(first, func, data);
      first = llist_insert_sorted(first, func, data);
      return first;
   }
   else {
      first = llist_insert_sorted(first, func, data);
      return first;
   }
}

int predicate_vertex_t_router_id(void *listdata, void *data) {
        neighbor_vertex_t *listdata_entry = (neighbor_vertex_t*) listdata;
        neighbor_vertex_t *data_entry = (neighbor_vertex_t*) data;
        if( listdata_entry->router_entry.router_id == data_entry->router_entry.router_id)
           return 1;
        else
           return 0;
}

int llist_size_predicate(node *head, int(*func)(void*,void*), void *data) {
   int count = 0;
   node* temp = head;
   while(temp) {
      if(func(temp->data, data) == 1)
         count++;
      temp = temp->next;
   }
   return count;
}

void llist_display_all_predicate(node* Head, int(*func)(void*), int(*func2)(void*,void*), void* data) {
   node* cur_ptr = Head;
   if(cur_ptr == NULL) {
      printf("Empty\n");
   }
   else {
       while(cur_ptr != NULL) {
           if(func2(cur_ptr->data, data) == 1)
              func(cur_ptr->data);
           cur_ptr = cur_ptr->next;
       }
   }
}

int predicate_vertex_router_id(void *listdata, void *id) {
        neighbor_vertex_t *listdata_entry = (neighbor_vertex_t*) listdata;
        uint32_t *id_test = (uint32_t*) id;
        if( listdata_entry->router_entry.router_id == *id_test)
           return 1;
        else
           return 0;
}

int predicate_subnet_entry_subnet(void *listdata, void *subnet) {
        subnet_entry_t *listdata_entry = (subnet_entry_t*) listdata;
        uint32_t *subnet_test = (uint32_t*) subnet;
        uint32_t subnet_test_var = *subnet_test;
        subnet_test_var = subnet_test_var & listdata_entry->mask;
        if( listdata_entry->subnet == subnet_test_var && listdata_entry->mask != 0)
           return 1;
        else
           return 0;
}

int display_subnet_entry_t(void* data) {
   subnet_entry_t *entry = (subnet_entry_t*) data;
   printf("subnet: %s ", quick_ip_to_string(entry->subnet));
   printf("mask: %s ", quick_ip_to_string(entry->mask));
   printf("router id: %s\n", quick_ip_to_string(entry->router_id));
   return 0;
}

int predicate_neighbor_t(void *listdata, void *neighbor) {
        neighbor_t *listdata_entry = (neighbor_t*) listdata;
        neighbor_t *n_test = (neighbor_t*) neighbor;
        if( listdata_entry->ip ==  n_test->ip)
           return 1;
        else
           return 0;
}

int predicate_subnet_entry(void *listdata, void *subnet_entry) {
        subnet_entry_t *listdata_entry = (subnet_entry_t*) listdata;
        subnet_entry_t *subnet_test = (subnet_entry_t*) subnet_entry;
        if( listdata_entry->subnet == subnet_test->subnet)
           return 1;
        else
           return 0;
}

int predicate_vertex_t(void *listdata, void *vertex) {
        neighbor_vertex_t *listdata_entry = (neighbor_vertex_t*) listdata;
        neighbor_vertex_t *vertex_test = (neighbor_vertex_t*) vertex;
        if( listdata_entry->router_entry.router_id == vertex_test->router_entry.router_id)
           return 1;
        else
           return 0;
}

int predicate_route_t_type(void *listdata, void *type) {
        route_t *listdata_entry = (route_t*) listdata;
        char *type_test = (char*) type;
        if( listdata_entry->type == *type_test)
           return 1;
        else
           return 0;
}

