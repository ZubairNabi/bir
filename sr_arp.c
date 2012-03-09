/* Filename: sr_arp.c */

#include <stdint.h>
#include <string.h>  /* memset() */
#include <arpa/inet.h>

#include "sr_arp.h"
#include "sr_arp_cache.h"
#include "sr_ethernet.h"
#include "sr_interface.h"
#include "sr_common.h"
#include "cli/helper.h"
#include "sr_integration.h"

/**
 * Handles a received ARP packet in a manner compliant with RFC 826.
 *
 * @param router  The router to send the packet from.  router->arp_cache is a 
 * pointer to the ARP cache.
 * @param ap  The ARP packet which was received.
 * @param intf  The interface the ARP packet was received on.
 */
void arp_handle_packet( sr_router* router,
                        packet_arp_t* ap,
                        interface_t* intf ) {

  // TODO: Handle an incoming ARP request.
  /*
   * To save you having to trawl through RFC 826, the general idea is:
   * 1) Check that the hardware and protocol types and their address lengths
   *    are as expected.
   * 2) If the ARP packet is addressed to us (i.e. the destination IP field
   *    is the IP address of the interface the packet was received on) OR we
   *    have an entry for the source IP address already in our cache, we
   *    add/update the IP -> MAC mapping in the cache.  Note that intf->ip is
   *    the IP of the interface the packet was received on.
   * 3) If the ARP packet is addressed to us AND it is an ARP request, reply
   *    to it.  Note that packet_arp_t can be used for assembling ARP packets
   *    and that there are useful constants #defined in sr_arp.h.
   * You will have to write some functions in sr_arp_cache.c to allow you to
   * add entries to the cache.  These functions could (should?) make use of
   * the packet dispatcher (interface defined in packet_dispatcher.h) which will
   * handle the packet queues & asynchronous packet transmission.
   */
   printf(" ** arp_handle_packet(..) called \n");
   print_packet_arp(ap);
   if(ntohs(ap->hw_type) == (ARP_HW_TYPE_ETH) &&
   ap->proto_type == (ARP_PROTO_TYPE_IP) &&
   ap->hw_len == (ARP_HW_TYPE_ETH_LEN) &&
   ap->proto_len == (ARP_PROTO_TYPE_IP_LEN)) {
   printf(" ** arp_handle_packet(..): packet correct \n");
   if (ap->dst_ip == intf->ip || llist_exists(router->arp_cache->arp_cache_list, predicate_ip, &ap->src_ip) == TRUE) {
      printf(" ** arp_handle_packet(..): packet exists or addressed to us \n");
      cache_dynamic_entry_add(router, ap->src_ip, &ap->src_mac);
   }
   if(ap->dst_ip == intf->ip && ntohs(ap->opcode) == ARP_REQUEST) {
      printf(" ** arp_handle_packet(..): packet addressed to us and is a request packet \n");
     // generate arp reply packet
      packet_arp_t *packet = (packet_arp_t*) malloc_or_die(sizeof(packet_arp_t));
      packet->hw_type = htons(ARP_HW_TYPE_ETH);
      packet->proto_type = ARP_PROTO_TYPE_IP;
      packet->hw_len = ARP_HW_TYPE_ETH_LEN;
      packet->proto_len = ARP_PROTO_TYPE_IP_LEN;
      packet->opcode = htons(ARP_REPLY);
      packet->src_mac = intf->mac;
      packet->src_ip = intf->ip;
      packet->dst_ip = ap->src_ip;
      packet->dst_mac = ap->src_mac;
      
      // create raw arp packet
      byte *eth_packet = (byte*) malloc_or_die(sizeof(packet_arp_t));
      memcpy(eth_packet , packet, sizeof(packet_arp_t));
      // send to arp handler
      arp_cache_handle_partial_frame(router, intf, packet->dst_ip,  eth_packet, sizeof(packet_arp_t), ETHERTYPE_ARP);
      } else if(ap->dst_ip == intf->ip && ntohs(ap->opcode) == ARP_REPLY) {
           printf(" ** arp_handle_packet(..): packet addressed to us and is a reply packet \n");
            // delete entry from arp request queue
            pthread_mutex_lock(&router->arp_cache->lock_queue);
            router->arp_cache->arp_request_queue = llist_remove(router->arp_cache->arp_request_queue, predicate_ip_arp_queue_entry_t, (void*) &ap->src_ip);
            pthread_mutex_unlock(&router->arp_cache->lock_queue);
            // put update
           packet_dispatcher_put_update(router->arp_cache->packet_dispatcher, ap->src_ip, ap->src_mac );
      }
   }
}


/**
 * Sends an ARP request for the given IP on the given interface.
 *
 * @param ip  The IP address which the ARP request will query
 * @param intf  The interface from which the request is sent
 */
void arp_send_request( addr_ip_t ip, interface_t* intf ) {

  // TODO: Send the ARP request.
  /*
   * It will probably be useful to make use of packet_arp_t from sr_arp.h; this
   * is the structure of an ARP packet, and can be used to assemble the packet
   * without too much bit manipulation.  You will also need to convert between
   * host and network byte orders; see the functions in arpa/inet.h.
   */
   printf(" ** arp_send_request(..) called \n");
   printf(" ** arp_send_request(..) sending arp request for ip: %s \n", quick_ip_to_string(ip));
   packet_arp_t *packet = (packet_arp_t*) malloc_or_die(sizeof(packet_arp_t));
   packet->hw_type = htons(ARP_HW_TYPE_ETH);
   packet->proto_type = (ARP_PROTO_TYPE_IP);
   packet->hw_len = ARP_HW_TYPE_ETH_LEN;
   packet->proto_len = ARP_PROTO_TYPE_IP_LEN;
   packet->opcode = htons(ARP_REQUEST);
   packet->src_mac = intf->mac;
   packet->src_ip = intf->ip;
   packet->dst_ip = ip;
   packet->dst_mac = make_mac_addr(255,255,255,255,255,255);

   byte* arp_packet = (byte*) malloc_or_die(sizeof(packet_arp_t));
   memcpy(arp_packet, packet, sizeof(packet_arp_t));
   printf(" ** arp_send_request(..) packet: \n");
   print_packet_arp(packet);
   
   // send via ethernet
   ethernet_send_frame(&packet->dst_mac, intf, ETHERTYPE_ARP, arp_packet, sizeof(packet_arp_t)); 
}

void print_packet_arp(packet_arp_t* arp_packet) {
    printf("ARP packet, HTYPE: %u, PTYPE: %u, HLEN: %u, PLEN: %u, OPCODE: %u, SRC MAC: %s, SRC IP: %s, ", ntohs(arp_packet->hw_type), arp_packet->proto_type, arp_packet->hw_len, arp_packet->proto_len,  ntohs(arp_packet->opcode), quick_mac_to_string(arp_packet->src_mac.octet),  quick_ip_to_string(arp_packet->src_ip));
   printf("DST MAC: %s, DST IP: %s\n", quick_mac_to_string(arp_packet->dst_mac.octet), quick_ip_to_string(arp_packet->dst_ip));
}
