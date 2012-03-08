#include "sr_pwospf.h"
#include "sr_pwospf_types.h"
#include "cli/helper.h"
#include "lwtcp/lwip/inet.h"
#include "lwtcp/lwip/sys.h"
#include "sr_ip.h"
#include "sr_common.h"
#include "sr_ethernet.h"
#include "sr_integration.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

pwospf_header_t* make_pwospf_header(byte* packet) {
   pwospf_header_t* header = (pwospf_header_t*) malloc_or_die(sizeof(pwospf_header_t));
   memcpy(&header->version, packet, 1);
   memcpy(&header->type, packet + 1 , 1);
   memcpy(&header->len, packet + 2, 2);
   header->len = ntohs(header->len);
   memcpy(&header->router_id, packet + 4, 4);
   memcpy(&header->area_id, packet + 8, 4);
   memcpy(&header->checksum, packet + 12, 2);
   header->checksum = ntohs(header->checksum);
   memcpy(&header->au_type, packet + 14, 2);
   memcpy(&header->authentication, packet + 16, 8);
   return header;
}

void pwospf_handle_packet(byte* pwospf_packet,
                          uint8_t packet_len,
                          struct ip* ip_header, 
                          interface_t* intf,
                          sr_router* router) {
   printf(" ** pwospf_handle_packet(..) called \n");
   // construct pwospf header
   printf(" ** pwospf_handle_packet(..) constructing pwospf header \n");
   pwospf_header_t* pwospf_header = make_pwospf_header(pwospf_packet);
   //checksum
   if(check_pwospf_header(pwospf_header, pwospf_packet, packet_len, router) == TRUE) {
      printf(" ** pwospf_handle_packet(..) header sanity checks passed \n");
      // display pwospf header
      printf(" ** pwospf_handle_packet(..) header contents: \n");
      display_pwospf_header(pwospf_header); 
      // get payload
      uint8_t payload_len = pwospf_header->len - sizeof(pwospf_header_t);
      byte* payload = (byte*) malloc_or_die(payload_len);
      memcpy(payload, pwospf_packet + sizeof(pwospf_header_t), payload_len);
      // check type
      pwospf_check_packet_type(ip_header, payload, payload_len, pwospf_header, intf);
   } else {
      printf(" ** pwospf_handle_packet(..) header sanity checks failed\n");
      //free(pwospf_packet);
      //free(ip_header);
      //free(pwospf_header);
   }
}

void display_pwospf_header(pwospf_header_t* header) {
   printf(" ** version: %u, type: %u,  len: %u, router id: %lu, area id: %lu, checksum: %u,  au type: %u, authentication: %lu\n", header->version, header->type, header->len, header->router_id, header->area_id, header->checksum, header->au_type, header->authentication);
}

bool check_pwospf_header(pwospf_header_t* header, byte* pwospf_packet, uint8_t packet_len, sr_router* router) {
   byte* sum_packet = (byte*) malloc_or_die(packet_len);
   memcpy(sum_packet, pwospf_packet, packet_len);
   uint16_t sum_zero = 0;
   memcpy(sum_packet + 12, &sum_zero, 2);
   uint8_t ver = PWOSPF_VER;
   uint16_t au_type = PWOSPF_AU_TYPE;
   uint64_t authen = PWOSPF_AUTHEN;
   if(header->version != ver)  
      printf(" ** pwospf version incorrect\n");
   if(header->au_type != au_type || header->authentication != authen)
      printf(" ** authentication is not disabled error\n");
   if(header->checksum != ntohs(inet_chksum((void*) sum_packet, packet_len))) 
      printf(" ** checksum incorrect\n");
   if(header->area_id != router->ls_info.area_id)
      printf(" ** area id in correct\n");
   // collective sanity checks
   if(header->version != ver || header->au_type != au_type || header->authentication != authen || header->checksum != ntohs(inet_chksum((void*) sum_packet, packet_len)) || header->area_id != router->ls_info.area_id)
      return FALSE;
   return TRUE;
}

void pwospf_check_packet_type(struct ip* ip_header, byte* payload, uint8_t payload_len, pwospf_header_t* pwospf_header, interface_t* intf) {
   printf(" ** pwospf_check_packet_type(..) called \n");
   uint8_t type_lsu = PWOSPF_TYPE_LSU;
   uint8_t type_hello = PWOSPF_TYPE_HELLO;
   if(pwospf_header->type == type_hello) {// hello type
      printf(" ** pwospf_check_packet_type(..) type: hello\n");
      pwospf_hello_type(ip_header, payload, payload_len, pwospf_header, intf);
   } else if (pwospf_header->type == type_lsu) { //lsu type {
      printf(" ** pwospf_check_packet_type(..) type: lsu\n");
      pwospf_lsu_type(ip_header, payload, payload_len, pwospf_header, intf);
 
   }
}

void send_hello_packet(void* intf_input) {
   printf(" ** send hello packet thread called\n");
   interface_t* intf = (interface_t*) intf_input;
   struct timespec *timeout =(struct timespec*) malloc_or_die(sizeof(struct timespec));
   struct timespec *timeout_rem =(struct timespec*) malloc_or_die(sizeof(struct timespec));
   timeout->tv_sec = (time_t) intf->helloint;
   timeout->tv_nsec = 0;
   while(1) {
      // sleep for interval
      nanosleep(timeout, timeout_rem);
      printf(" ** send hello packet thread awoken for interface: %s\n", intf->name);
      //broadcast packets
      //make hello packet
      pwospf_hello_packet_t* hello_packet = (pwospf_hello_packet_t*) malloc_or_die(sizeof(pwospf_hello_packet_t));
      hello_packet->network_mask = intf->subnet_mask;
      hello_packet->helloint = htons(intf->helloint);
      hello_packet->padding = 0;
      //make pwospf header
      pwospf_header_t* pwospf_header = (pwospf_header_t*) malloc_or_die(sizeof(pwospf_header_t));
      pwospf_header->version = PWOSPF_VER;
      pwospf_header->type = PWOSPF_TYPE_HELLO;
      pwospf_header->len = htons(sizeof(pwospf_hello_packet_t) + sizeof(pwospf_header_t));
      pwospf_header->router_id = intf->router_id;
      pwospf_header->area_id = intf->area_id;
      pwospf_header->checksum = 0;
      pwospf_header->au_type = PWOSPF_AU_TYPE;
      pwospf_header->authentication = PWOSPF_AUTHEN;
      // generate raw packet 
      byte* pwospf_packet = (byte*) malloc_or_die(sizeof(pwospf_hello_packet_t) + sizeof(pwospf_header_t));
      memcpy(pwospf_packet, pwospf_header, sizeof(pwospf_header_t));
      memcpy(pwospf_packet + sizeof(pwospf_header_t), hello_packet, sizeof(pwospf_hello_packet_t));
      // generate checksum
      pwospf_header->checksum = htons(htons(inet_chksum((void*) pwospf_packet, sizeof(pwospf_hello_packet_t) + sizeof(pwospf_header_t))));
      memcpy(pwospf_packet, pwospf_header, sizeof(pwospf_header_t));
      // generate ip header
      struct ip* ip_header = (struct ip*) malloc_or_die(sizeof(struct ip));
      ip_header->ip_v = IP_4_VERSION;
      ip_header->ip_off = htons(IP_OFFSET_DF);
      ip_header->ip_p = IP_PROTOCOL_OSPF;
      char *dst = ALL_SPF_ROUTERS_IP;
      ip_header->ip_dst.s_addr = make_ip_addr(dst);
      ip_header->ip_src.s_addr = intf->ip;
      ip_header->ip_tos = IP_TOS_DEFAULT;
      ip_header->ip_id = htons(IP_ID);
      ip_header->ip_ttl = IP_TTL_DEFAULT;
      ip_header->ip_hl = IP_HEADER_MIN_LEN_WORD;
      ip_header->ip_len = htons(sizeof(pwospf_hello_packet_t) + sizeof(pwospf_header_t) + IP_HEADER_MIN_LEN);
      ip_header->ip_sum = 0;
      ip_header->ip_sum = htons(htons(inet_chksum((void*)ip_header, IP_HEADER_MIN_LEN)));
      // generate ip packet
      uint8_t packet_len = ntohs(ip_header->ip_len);
      byte* ip_packet = (byte*) malloc_or_die(packet_len);
      memcpy(ip_packet, ip_header, IP_HEADER_MIN_LEN);
      memcpy(ip_packet + IP_HEADER_MIN_LEN, pwospf_packet, sizeof(pwospf_hello_packet_t) + sizeof(pwospf_header_t));
      // display generated packet len
      printf(" ** send hello packet thread generated packet with length %u bytes\n", (ntohs(ip_header->ip_len)));
      // send to ethernet
      addr_mac_t dst_mac = make_mac_addr(255,255,255,255,255,255);
      ethernet_send_frame( &dst_mac,
                          intf,
                          ETHERTYPE_IP,
                          ip_packet,
                          packet_len );
   }
}

void pwospf_init( sr_router* router ) {
   printf(" ** pwospf_init(..) called \n");
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
   int i = 0;
   for( i = 0; i < router->num_interfaces ; i++) { 
      sys_thread_new(send_hello_packet, (void*) &router->interface[i]);
      sys_thread_new(remove_timed_out_neighbors, (void*) &router->interface[i]);
   }
   sys_thread_new(send_lsu_packet, (void*) &router);
}

void remove_timed_out_neighbors(void* intf_input) {
   printf(" ** remove timed out neighbors thread called\n");
   interface_t* intf = (interface_t*) intf_input;
   struct timespec *timeout =(struct timespec*) malloc_or_die(sizeof(struct timespec));
   struct timespec *timeout_rem =(struct timespec*) malloc_or_die(sizeof(struct timespec));
   int count = 0;
   timeout->tv_sec = (time_t) intf->helloint;
   timeout->tv_nsec = 0;
   while(1) {
      // sleep for interval
      nanosleep(timeout, timeout_rem);
      printf(" ** remove neighbors timed out thread awoken for interface: %s\n", intf->name);
      //refresh count
      count = 0;
      // get current time
      struct timeval *current_time = (struct timeval*)malloc_or_die(sizeof(struct timeval));
      gettimeofday(current_time, NULL);
      // remove timed out entries from neighbors list
      // lock list
      pthread_mutex_lock(&intf->neighbor_lock);
      // display current list
      printf(" ** remove neighbors timed out thread current neighbors of interface: %s\n", intf->name);
      llist_display_all(intf->neighbor_list, display_neighbor_t);
      // remove all timed out 
      if(current_time == NULL)
         printf("Current time is null\n");
      intf->neighbor_list = llist_remove_all(intf->neighbor_list, predicate_timeval_neighbor_t, (void*)current_time, &count);
      // unlock list
      pthread_mutex_unlock(&intf->neighbor_lock);
      if (count > 0) {
      // initiate LSU flood
          printf(" ** remove neighbors timed out thread, LS changed. Flooding LSU, Interface: %s\n", intf->name);
          pwospf_send_lsu();
      } else
         printf(" ** remove neighbors timed out thread, All neighbors HELLOing for interface: %s\n",intf->name);
   }
}

void send_lsu_packet(void* intf_input) {
   printf(" ** send lsu packet thread called\n");
   struct timespec *timeout =(struct timespec*) malloc_or_die(sizeof(struct timespec));
   struct timespec *timeout_rem =(struct timespec*) malloc_or_die(sizeof(struct timespec));
   timeout->tv_sec = (time_t) PWOSPF_LSU_INTERVAL;
   timeout->tv_nsec = 0;
   while(1) {
      // sleep for interval
      nanosleep(timeout, timeout_rem);
      printf(" ** send lsu packet thread awoken\n");
      pwospf_send_lsu();
   }
}
