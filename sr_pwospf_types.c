#include "sr_pwospf_types.h"
#include "cli/helper.h"
#include "sr_neighbor.h"
#include "sr_base_internal.h"
#include "sr_integration.h"
#include "lwtcp/lwip/inet.h"
#include "sr_ip.h"
#include "sr_neighbor_db.h"
#include "sr_dijkstra.h"

#include <string.h>
#include <stdlib.h>

void pwospf_hello_type(struct ip* ip_header, byte* payload, uint16_t payload_len, pwospf_header_t* pwospf_header, interface_t* intf) {
   printf(" ** pwospf_hello_type(..) called \n");
   printf(" ** pwospf_hello_type(..) packet with length: %u \n", payload_len);
   // make hello packet
   pwospf_hello_packet_t* hello_packet = (pwospf_hello_packet_t*) malloc_or_die(sizeof(pwospf_hello_packet_t));
   memcpy(&hello_packet->network_mask, payload, 4);
   memcpy(&hello_packet->helloint, payload + 4, 2);
   memcpy(&hello_packet->padding, payload + 6, 2);
   // display packet
   printf(" ** pwospf_hello_type(..) displayed header contents:\n");
   display_hello_packet(hello_packet);
   // check mask and hello interval
   if(intf->subnet_mask == hello_packet->network_mask && intf->helloint == ntohs(hello_packet->helloint)) {
      printf(" ** pwospf_hello_type(..) network mask and hello interval correct\n");
      // if the interface has no neighbors
      if(intf->neighbor_list == NULL) {
         printf(" ** pwospf_hello_type(..) interface has no neighbors, adding new neighbor\n");
         add_neighbor(intf, pwospf_header->router_id, ip_header->ip_src.s_addr, hello_packet->helloint, hello_packet->network_mask);
      } else { // interface has existing neighbors
         // generate neighbor object
         neighbor_t* neighbor = make_neighbor(pwospf_header->router_id, ip_header->ip_src.s_addr, hello_packet->helloint, hello_packet->network_mask);
         // get last instance so that lsu info can be put back
         pthread_mutex_lock(&intf->neighbor_lock);
         node* ret = llist_find( intf->neighbor_list, predicate_id_neighbor_t,(void*) &pwospf_header->router_id);
         pthread_mutex_unlock(&intf->neighbor_lock);
         if( ret!= NULL) {
            neighbor_t *neighbor_ret = (neighbor_t*) ret->data;
            neighbor->last_adverts = neighbor_ret->last_adverts;
            neighbor->last_lsu_packet = neighbor_ret->last_lsu_packet;
         }
         // lock neighbor list
         pthread_mutex_lock(&intf->neighbor_lock);
         // if exists, then update, otherwise add
         intf->neighbor_list = llist_update_beginning_delete(intf->neighbor_list, predicate_neighbor_t, (void*) neighbor);
         pthread_mutex_unlock(&intf->neighbor_lock);
      }
      // update info in neighbor db
      // get instance of router 
      struct sr_instance* sr_inst = get_sr();
      struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
      update_neighbor_vertex_t_rid(router, ip_header->ip_src.s_addr, pwospf_header->router_id);
   } else {
      printf(" ** pwospf_hello_type(..) network mask or hello interval incorrect\n");
   }
}

void pwospf_lsu_type(struct ip* ip_header, byte* payload, uint16_t payload_len, pwospf_header_t* pwospf_header, interface_t* intf) {
   printf(" ** pwospf_lsu_type(..) called \n");
   printf(" ** pwospf_lsu_type(..) packet with length: %u \n", payload_len);
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   // make lsu packet
   pwospf_lsu_packet_t* lsu_packet = (pwospf_lsu_packet_t*) malloc_or_die(sizeof(pwospf_lsu_packet_t));
   memcpy(&lsu_packet->seq, payload, 2);
   memcpy(&lsu_packet->ttl, payload + 2, 2);
   memcpy(&lsu_packet->no_of_adverts, payload + 4, 4);
   // display packet
   printf(" ** pwospf_lsu_type(..) displayed header contents:\n");
   display_lsu_packet(lsu_packet);
   // calculate number of adverts
   int num_adverts = ((payload_len - 8)/sizeof(pwospf_ls_advert_t));
   printf(" ** pwospf_lsu_type(..) number of adverts: %d\n", num_adverts);
   // check if I generated the LSU
   if(pwospf_header->router_id != intf->router_id) {
      // not generated by self
      // check if neighbor ever sent hello packet
       pthread_mutex_lock(&intf->neighbor_lock);
       node* ret = llist_find( intf->neighbor_list, predicate_id_neighbor_t,(void*) &pwospf_header->router_id);
       pthread_mutex_unlock(&intf->neighbor_lock);
       if(ret != NULL) {
       // have received a hello packet from this neighbor in the recent past
          neighbor_t *neighbor = (neighbor_t*) ret->data;
          display_neighbor_t((void*) neighbor);
          // check if lsu received for first time
          if(neighbor->last_adverts == NULL) {
             printf(" ** pwospf_lsu_type(..) first lsu received from this neighbor\n");
             neighbor->last_lsu_packet = *lsu_packet;
             neighbor->last_adverts = (byte*) malloc_or_die(payload_len - 8);
             memcpy(neighbor->last_adverts, payload + 8, payload_len - 8);
             // add to list
             pthread_mutex_lock(&intf->neighbor_lock);  
             intf->neighbor_list = llist_update_beginning_delete(intf->neighbor_list, predicate_id_neighbor_t, (void*) neighbor);
             pthread_mutex_unlock(&intf->neighbor_lock);
             display_neighbor_t((void*) neighbor);
             // create router entry
             router_entry_t router_entry;
             router_entry.router_id = neighbor->id;
             router_entry.area_id = 0;
             //add vertex with router entry info
             add_neighbor_vertex_t(router, router_entry);
             //create subnet entry
             subnet_entry_t* subnet_entry;
             //add adverts to db
             printf(" ** pwospf_lsu_type(..) adding adverts to neighbor db\n");
             int i;
             pwospf_ls_advert_t* ls_advert = (pwospf_ls_advert_t*) malloc_or_die(sizeof(pwospf_ls_advert_t));
             for(i = 0; i < ntohl(lsu_packet->no_of_adverts) ; i++) {
                memcpy(&ls_advert->subnet, payload + sizeof(pwospf_lsu_packet_t) + sizeof(pwospf_ls_advert_t) * i, 4);         
                memcpy(&ls_advert->mask, payload + sizeof(pwospf_lsu_packet_t) + 4 + sizeof(pwospf_ls_advert_t) * i, 4);
                memcpy(&ls_advert->router_id, payload + sizeof(pwospf_lsu_packet_t) + 8 + sizeof(pwospf_ls_advert_t) * i, 4);
                display_ls_advert(ls_advert);
                subnet_entry = create_subnet_entry_t_advert(ls_advert);
                add_subnet_entry_t(router, router_entry, subnet_entry);
             }
             // update vertices
             update_neighbor_vertex_t(router, router_entry);
             //run Djikstra's algo
             dijkstra(router); 
             //free(lsu_packet);
             //free(ret);
             //free(neighbor);
             //free(ls_advert);
          } else {
          // check if the previous seq was same
          if(neighbor->last_lsu_packet.seq != lsu_packet->seq) {
             // nope, new seq
             neighbor->last_lsu_packet = *lsu_packet;
             // flood to all neighbors
             pwospf_flood_lsu(ip_header, pwospf_header, lsu_packet, neighbor, intf);
             // check if content the same
             if(strcmp((char*) neighbor->last_adverts, (char*) (payload + 8)) != 0) {
                // nope new content
                if(ntohl(neighbor->last_lsu_packet.no_of_adverts) != ntohl(lsu_packet->no_of_adverts)) {
                   // free mem, as new adverts might need different memory
                   free(neighbor->last_adverts);
                   // now allocate new mem space
                   neighbor->last_adverts = (byte*) malloc_or_die(payload_len - 8);
                }
                memcpy(neighbor->last_adverts, payload + 8, payload_len - 8);
                neighbor->last_lsu_packet = *lsu_packet;
                // update packet info in neighbor list
                pthread_mutex_lock(&intf->neighbor_lock);
                intf->neighbor_list = llist_update_beginning_delete(intf->neighbor_list, predicate_id_neighbor_t, (void*) neighbor);
                pthread_mutex_unlock(&intf->neighbor_lock);
                // check if entry exists in DB
                // create router entry
                router_entry_t router_entry;
                router_entry.router_id = neighbor->id;
                router_entry.area_id = 0;
                //add vertex with router entry info
                add_neighbor_vertex_t(router, router_entry);
               //create subnet entry
                subnet_entry_t* subnet_entry;

             	int i;
             	pwospf_ls_advert_t* ls_advert = (pwospf_ls_advert_t*) malloc_or_die(sizeof(pwospf_ls_advert_t));
             	if(check_neighbor_vertex_t_router_entry_t_id(router, neighbor->id) == 1) {
                   printf(" ** pwospf_lsu_type(..) neighbor already known, checking if any new adverts\n");
                   //neighbor present in db, check if any new adverts
                   for(i = 0; i < ntohl(lsu_packet->no_of_adverts) ; i++) {
                      memcpy(&ls_advert->subnet, payload + sizeof(pwospf_lsu_packet_t) + sizeof(pwospf_ls_advert_t) * i, 4);
                      memcpy(&ls_advert->mask, payload + sizeof(pwospf_lsu_packet_t) + 4
 + sizeof(pwospf_ls_advert_t) * i, 4);
                      memcpy(&ls_advert->router_id, payload + sizeof(pwospf_lsu_packet_t) + 8 + sizeof(pwospf_ls_advert_t) * i, 4);
                         display_ls_advert(ls_advert);
                         subnet_entry = create_subnet_entry_t_advert(ls_advert);
                	 add_subnet_entry_t(router, router_entry, subnet_entry);
                   }    
                   // update vertices
             	   update_neighbor_vertex_t(router, router_entry);
                } 
                  //run Djikstra's algo
                  dijkstra(router);
                } else {
                     printf(" ** pwospf_lsu_type(..) error, content same as previous from this neighbor, only updating time stamp in DB\n");  
                     // update timestamp
                     // create router entry
	             router_entry_t router_entry;
                     router_entry.router_id = neighbor->id;
                     router_entry.area_id = 0;
                     update_neighbor_vertex_t_timestamp(router, router_entry);

             }
          } else {
              printf(" ** pwospf_lsu_type(..) error, seq same as previous one from this neighbor, dropping\n");
          }
       }
       } else {
          printf(" ** pwospf_lsu_type(..) error, neighbor hasn't said hello recently, dropping\n");
       }
   } else {
      //drop packet
      printf(" ** pwospf_lsu_type(..) error, LSU generated by self. dropping\n");
      //free stuff
     free(lsu_packet);
   } 
}

void display_hello_packet(pwospf_hello_packet_t* packet) {
   printf(" ** network mask: %lu, hello interval: %u, padding: %u\n", packet->network_mask, ntohs(packet->helloint), packet->padding);
}

void display_lsu_packet(pwospf_lsu_packet_t* packet) {
   printf(" ** seq: %d, ttl: %u, no. of adverts: %d\n", ntohs(packet->seq), ntohs(packet->ttl), ntohl(packet->no_of_adverts));
}

void pwospf_send_lsu() {
    printf(" ** pwospf_send_lsu(..) called\n");
    // get instance of router 
    struct sr_instance* sr_inst = get_sr();
    struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);    
    int num_adverts = 0;
    byte* ls_adverts = get_ls_adverts_src(router, &num_adverts);
    int i = 0;
    node* first = NULL;
    neighbor_t* neighbor = NULL;
    printf(" ** pwospf_send_lsu(..) number of adverts: %d\n", num_adverts);
    printf(" ** pwospf_send_lsu(..) adverts:\n");
    display_neighbor_vertices_src(router);
    // now create lsu_packet
    pwospf_lsu_packet_t *lsu_packet = (pwospf_lsu_packet_t*) malloc_or_die(sizeof(pwospf_lsu_packet_t));
    //increment lsu sequence
    router->ls_info.lsu_seq = router->ls_info.lsu_seq + 1;
    lsu_packet->seq = htons(router->ls_info.lsu_seq);
    uint16_t ttl = PWOSPF_LSU_TTL;
    lsu_packet->ttl = htons(ttl);
    lsu_packet->no_of_adverts = htonl(num_adverts);
    // calculate length of entire pwospf packet
    uint16_t pwospf_packet_len = sizeof(pwospf_lsu_packet_t) + sizeof(pwospf_header_t) + sizeof(pwospf_ls_advert_t) * num_adverts;
    //make pwospf header
    pwospf_header_t* pwospf_header = (pwospf_header_t*) malloc_or_die(sizeof(pwospf_header_t));
    pwospf_header->version = PWOSPF_VER;
    pwospf_header->type = PWOSPF_TYPE_LSU;
    pwospf_header->len = htons(pwospf_packet_len);
    pwospf_header->router_id = router->ls_info.router_id;
    pwospf_header->area_id = router->ls_info.area_id;
    pwospf_header->checksum = 0;
    pwospf_header->au_type = PWOSPF_AU_TYPE;
    pwospf_header->authentication = PWOSPF_AUTHEN;
    // generate raw packet 
    byte* pwospf_packet = (byte*) malloc_or_die(pwospf_packet_len);
    memcpy(pwospf_packet, pwospf_header, sizeof(pwospf_header_t));
    memcpy(pwospf_packet + sizeof(pwospf_header_t), lsu_packet, sizeof(pwospf_lsu_packet_t));
    memcpy(pwospf_packet + sizeof(pwospf_header_t) + sizeof(pwospf_lsu_packet_t), ls_adverts, sizeof(pwospf_ls_advert_t) * num_adverts);
    // generate checksum
    pwospf_header->checksum = htons(htons(inet_chksum((void*) pwospf_packet, pwospf_packet_len)));
    memcpy(pwospf_packet, pwospf_header, sizeof(pwospf_header_t));
    free(ls_adverts);
    free(lsu_packet);
    free(pwospf_header);
    printf(" ** pwospf_send_lsu(..) pwospf packet with length %u generated\n", pwospf_packet_len);
    // now iterate through each interface
    for( i = 0; i < router->num_interfaces ; i++) {
       // now iterate this interface's neighbors list
       first = router->interface[i].neighbor_list;
       pthread_mutex_lock(&router->interface[i].neighbor_lock);
       while(first != NULL) {
          // get neighbor
          neighbor = (neighbor_t*) first->data;
          if(neighbor != NULL) {
             struct in_addr src, dst;
             src.s_addr = router->interface[i].ip;
             dst.s_addr = neighbor->ip;
             make_ip_packet(pwospf_packet, pwospf_packet_len, dst, src, IP_PROTOCOL_OSPF);
             
          }
          first = first->next;
       }
       pthread_mutex_unlock(&router->interface[i].neighbor_lock);
   }
}

void pwospf_flood_lsu(struct ip* ip_header, pwospf_header_t* pwospf_header, pwospf_lsu_packet_t* lsu_packet, neighbor_t* sending_neighbor, interface_t* intf) {
   printf(" ** pwospf_flood_lsu(..) called\n");
   // decrement ttl
   uint16_t ttl = 0;
   ttl = htons(lsu_packet->ttl);
   ttl = ttl - 1;
   ttl = ntohs(ttl);
   lsu_packet->ttl = ttl;
   // check if ttl expired
   if(lsu_packet->ttl < 1) {
      printf(" ** pwospf_flood_lsu(..) ttl expired, not flooding\n");
   } else {
      // get instance of router 
      struct sr_instance* sr_inst = get_sr();
      struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
      node* first = NULL;
      neighbor_t* neighbor = NULL;
      int i;
      // now iterate through each interface
      for( i = 0; i < router->num_interfaces ; i++) {
         // now iterate this interface's neighbors list
         first = router->interface[i].neighbor_list;
         pthread_mutex_lock(&router->interface[i].neighbor_lock);
         while(first != NULL) {
            // get neighbor
            neighbor = (neighbor_t*) first->data;
            if(neighbor != NULL) {
               // ensure that the sending neighbour is excluded
               if(neighbor->id != sending_neighbor->id) {
                  byte* ip_packet = (byte*) malloc_or_die(ip_header->ip_len);
                  ip_header->ip_dst.s_addr = neighbor->ip;
                  ip_header->ip_len = htons((ip_header->ip_len));
                  ip_header->ip_off = htons((ip_header->ip_off));
                  ip_header->ip_id = htons((ip_header->ip_id));
                  ip_header->ip_sum = htons((ip_header->ip_sum));
                  memcpy(ip_packet, ip_header, ip_header->ip_hl * 4);
                  memcpy(ip_packet + ip_header->ip_hl * 4, pwospf_header, sizeof(pwospf_header_t));
                  memcpy(ip_packet + ip_header->ip_hl * 4 + sizeof(pwospf_header_t), lsu_packet, sizeof(pwospf_lsu_packet_t));
                  ip_look_up_reply(ip_packet, ip_header, intf);
               }

            }  
          first = first->next;
         }
          pthread_mutex_unlock(&router->interface[i].neighbor_lock);
      }
   }
}

void display_ls_advert(pwospf_ls_advert_t* packet) {
   printf(" ** subnet: %s, ", quick_ip_to_string(packet->subnet));
   printf("mask: %s, ", quick_ip_to_string(packet->mask));
   printf("router_id: %s\n", quick_ip_to_string(packet->router_id));
}
