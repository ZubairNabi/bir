#include <pthread.h>
#include <stdlib.h>
#include "packet_dispatcher.h"
#include "sr_common.h"
#include "cli/helper.h"
#include "sr_integration.h"
#include <string.h>
#include "lwtcp/lwip/sys.h"
#include "sr_ethernet.h"


packet_dispatcher_t* packet_dispatcher_new(void) {
   printf(" ** packet_dispatcher_new(..) called \n");
   packet_dispatcher_t* dispatcher = (packet_dispatcher_t*) malloc_or_die(sizeof(packet_dispatcher_t));
   dispatcher->lock = (pthread_mutex_t*) malloc_or_die(sizeof(pthread_mutex_t));
   pthread_mutex_init(dispatcher->lock, NULL);
   dispatcher->updates = llist_new();
   dispatcher->packets = llist_new();
   return dispatcher;
}

packet_t* create_packet(byte* frame, unsigned len, addr_ip_t ip, interface_t* intf, time_t timeout) {
   packet_t* packet = (packet_t*) malloc_or_die(sizeof(packet_t));
   packet->ethernet_frame = (byte*) malloc_or_die(len);
   packet->intf = (interface_t*) malloc_or_die(sizeof(interface_t));
   packet->ethernet_frame = frame;
   packet->len = len;
   packet->intf = intf;
   packet->timeout = timeout;
   packet->ip = ip;
   return packet;
}

packet_t* create_packet_no_ip(byte* frame, unsigned len, interface_t* intf, time_t timeout) {
   packet_t* packet = (packet_t*) malloc_or_die(sizeof(packet_t));
   packet->ethernet_frame = (byte*) malloc_or_die(len);
   packet->intf = (interface_t*) malloc_or_die(sizeof(interface_t));
   packet->ethernet_frame = frame;
   packet->len = len;
   packet->intf = intf;
   packet->timeout = timeout;
   return packet;
}

update_t* create_update(addr_ip_t ip, addr_mac_t mac) {
   update_t* update = (update_t*) malloc_or_die(sizeof(update_t));
   update->ip = ip;
   update->mac = mac;
   return update;
}

/**
 * Starts the packet dispatcher and garbage collector threads.  If there is an
 * error starting either thread, any already started threads are cancelled, but
 * may do some work before being cancelled.
 *
 * @param pd  The packet dispatcher for which to start the thread.
 *
 * @return 0 on success, non-zero if there was an error starting the threads.
 */
int packet_dispatcher_run(packet_dispatcher_t* pd) {
   printf(" ** packet_dispatcher_run(..) called \n");
   sys_thread_new(remove_timedout_packets, (void*) pd);
   sys_thread_new(update_check, (void*) pd);
   return 0;
}

/**
 * Starts the packet dispatcher and garbage collector threads.  If there is an
 * error starting either thread, any already started threads are cancelled, but
 * may do some work before being cancelled.
 *
 * @param pd  The packet dispatcher for which to start the thread.
 *
 * @return 0 on success, non-zero if there was an error starting the threads. */
void packet_dispatcher_put_update(packet_dispatcher_t* pd, addr_ip_t ip, addr_mac_t mac) {
   printf(" ** packet_dispatcher_put_update(..) called \n");
   update_t *update = create_update(ip, mac);
   pthread_mutex_lock(pd->lock);
   pd->updates = llist_insert_beginning(pd->updates, (void*) update);
   pthread_mutex_unlock(pd->lock);
   llist_display_all(pd->updates, display_update_t);
}

/**
 * Adds a packet to the queue.  The packet will be sent to its destination when
 * an update for the packet's IP address is added to the queue.
 *
 * @param pd  The packet dispatcher to which the packet is added
 * @param ip  The IP address of the first hop (not ultimate destination) the
 * packet is sent to
 * @param ethernet_frame  A buffer containing the ethernet frame to be sent.
 * This must include the source and destination MACs and ethertype, but the
 * destination MAC need not be set. This buffer is free'd when the packet has
 * been sent or times out.
 * @param len  The length of ethernet_frame
 * @param intf  The interface which the packet will be sent on.
 * @param timeout  The number of seconds after which this packet will time out
 */
void packet_dispatcher_put_packet(packet_dispatcher_t* pd,
                                                                  addr_ip_t ip,
                                                                  byte* ethernet_frame,
                                                                  unsigned len,
                                                                  interface_t* intf,
                                                                  unsigned timeout) {
   printf(" ** packet_dispatcher_put_packet(..) called \n");
   packet_t* packet = create_packet(ethernet_frame, len, ip, intf, timeout);
   pthread_mutex_lock(pd->lock);
   pd->packets = llist_insert_beginning(pd->packets, (void*) packet);
   pthread_mutex_unlock(pd->lock);
}

void remove_timedout_packets(void* input) {
   printf(" ** packet_dispatcher: remove_timedout_packets(..) (Q GC thread) called \n");
   packet_dispatcher_t* pd = (packet_dispatcher_t*) input;
   struct timespec *timeout = (struct timespec*) malloc_or_die(sizeof(struct timespec));
   struct timespec *timeout_rem = (struct timespec*) malloc_or_die(sizeof(struct timespec));
   timeout->tv_sec = (time_t) Q_GC_INTERVAL;
   timeout->tv_nsec = 0;
   while(1) {
      nanosleep(timeout, timeout_rem);
       printf(" ** packet_dispatcher: remove_timedout_packets(..) (Thread awoken) called \n");
      pthread_mutex_lock(pd->lock);
      printf(" ** packet_dispatcher: remove_timedout_packets(..) Lock acquired\n");
      int temp = 0;
      pd->packets = llist_remove_all_no_count(pd->packets, predicate_timeout, (void*) &temp);
      //if(ret!=NULL)
        // pd->packets = ret;
      update_timeout_traverse(pd);
      pthread_mutex_unlock(pd->lock);
   }
}

void update_timeout_traverse(packet_dispatcher_t *pd) {
   printf(" ** packet_dispatcher: update_timedout_traverse(..) called \n");
   packet_t* packet;
   node *node = pd->packets;
   while(node) {
                packet = (packet_t*) node->data;
                if(packet != NULL) { 
                   if(packet->timeout - Q_GC_INTERVAL < 0) {
                      packet->timeout = 0;
                  
                }
                else {
                   packet->timeout = packet->timeout - Q_GC_INTERVAL;
                }
                node->data = packet;
                }
                node=node->next;
        }
   printf(" ** packet_dispatcher: update_timedout_traverse(..) ended \n");
}

/*Predicate function for timeout*/
int predicate_timeout(void* item, void* time) {
    printf(" ** packet_dispatcher: predicate_timeout(..) called \n");
   packet_t *packet = (packet_t*) item;
   if(packet == NULL)
      return 0;
   if(packet->timeout == 0) {
        printf(" ** packet_dispatcher: predicate_timeout(..) ended on true\n");
        return 1;
   }
   else {
         printf(" ** packet_dispatcher: predicate_timeout(..) ended on false\n");
        return 0;
   }
}

void update_check(void* input) {
   printf(" ** packet_dispatcher: update_check(..) (Update thread) called \n");
   packet_dispatcher_t *pd = (packet_dispatcher_t*) input;
   struct timespec *timeout = (struct timespec*) malloc_or_die(sizeof(struct timespec));
   struct timespec *timeout_rem = (struct timespec*) malloc_or_die(sizeof(struct timespec));
   timeout->tv_sec = (time_t) UPDATE_INTERVAL;
   timeout->tv_nsec = 0;
   update_t *update;
   packet_t *packet;
   node* packets_head, *updates_head;
   while(1) {
      nanosleep(timeout, timeout_rem);
      printf(" ** packet_dispatcher: update_check(..) (Thread awoken) called \n");
      pthread_mutex_lock(pd->lock);
      packets_head = pd->packets;
      updates_head = pd->updates;
      printf(" ** packet_dispatcher: update_check(..) current waiting updates: \n");
      llist_display_all(updates_head, display_update_t);
      printf(" ** packet_dispatcher: update_check(..) current waiting packets: \n");
      llist_display_all(packets_head, display_packet_t);
      while(updates_head) {
          update = (update_t*) updates_head->data;
          if(update != NULL) {
             node* packets_heads = pd->packets;
             while(packets_head) {
                packet = (packet_t*) packets_head->data;
                if(packet != NULL) {
                   if(packet-> ip == update-> ip) {
                      // send packet
                      ethernet_send_frame(&update->mac, packet->intf, ETHERTYPE_IP, packet->ethernet_frame, packet->len);
	           }
                }
             packets_head = packets_head->next;
             }
             // remove all packets for this update
             pd->packets = llist_remove_all_no_count(packets_heads, predicate_ip_packet_t, (void*)&update->ip);
          }
          updates_head = updates_head->next;
       }
      // remove updates
      pd->updates = llist_delete_no_count(pd->updates);
      pthread_mutex_unlock(pd->lock);
   }
   printf(" ** packet_dispatcher: update_check(..) ended \n");
}

