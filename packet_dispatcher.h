#pragma once

/*
 * This is a packet dispatcher which periodically checks for updates to the ARP
 * cache and, if there are any, sends packets which have been waiting for that
 * MAC address.
 */

#include <pthread.h>
#include "sr_interface.h"
#include "linked_list.h"

// The time after which packets are removed from the queue
#define Q_PACKET_TIMEOUT 10

// Garbage Collection
#define Q_GC_INTERVAL 5

// Time after which the update queue is traversed
#define UPDATE_INTERVAL 2

typedef struct packet_dispatcher_t {
  pthread_mutex_t* lock;
  node* updates;
  node* packets;
  pthread_t* dispatcher_thread;
  pthread_t* gc_thread;
} packet_dispatcher_t;

typedef struct packet_t {
  byte* ethernet_frame;
  unsigned len;
  addr_ip_t ip;
  interface_t* intf;
  int timeout;
} packet_t;

typedef struct update_t {
  addr_ip_t ip;
  addr_mac_t mac;
} update_t;


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
								  unsigned timeout);


/**
 * Adds an update to the queue.  Any packets waiting for the update will be sent
 * the next time the packet dispatcher thread runs.
 *
 * @param pd  The packet dispatcher to which the update is added
 * @param ip  The IP address which has been updated
 * @param mac  The MAC address corresponding to ip
 */
void packet_dispatcher_put_update(packet_dispatcher_t* pd, addr_ip_t ip, addr_mac_t mac);


/**
 * Starts the packet dispatcher and garbage collector threads.  If there is an
 * error starting either thread, any already started threads are cancelled, but
 * may do some work before being cancelled.
 *
 * @param pd  The packet dispatcher for which to start the thread.
 *
 * @return 0 on success, non-zero if there was an error starting the threads.
 */
int packet_dispatcher_run(packet_dispatcher_t* pd);


/**
 * Stops the packet dispatcher and garbage collector threads.  Calling this
 * if the threads are not running results in undefined behaviour.
 *
 * @param pd  The packet dispatcher to stop the threads for
 */
void packet_dispacther_stop(packet_dispatcher_t* pd);


/**
 * Destroys a packet_dispatcher_t.  Note that the running threads must be
 * stopped first with packet_dispatcher_stop().
 *
 * @param pd  The packet dispatcher to destroy
 */
void packet_dispatcher_delete(packet_dispatcher_t* pd);


/**
 * Creates a new packet dispatcher.  Note that the dispatcher thread does not
 * start running until packet_dispatcher_run() is called.  All memory is
 * allocated by this function; nothing need be done externally.
 *
 * @return A pointer to a packet_dispatcher_t
 */
packet_dispatcher_t* packet_dispatcher_new(void);

packet_t* create_packet(byte* frame, unsigned len, addr_ip_t ip, interface_t* intf, time_t timeout);

update_t* create_update(addr_ip_t ip, addr_mac_t mac);

packet_t* create_packet_no_ip(byte* frame, unsigned len, interface_t* intf, time_t timeout);

int predicate_timeout(void* item, void* time);

void update_timeout_traverse(packet_dispatcher_t* pd);

void update_check(void* input);

void remove_timedout_packets(void* input);

