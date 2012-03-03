#include "sr_ethernet.h"
#include "cli/helper.h"
#include "sr_ip.h"
#include "sr_arp.h"
#include "sr_integration.h"

#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

/**
 * This determines what kind of payload is contained within the frame and then
 * calls the appropriate protocol's handler on the payload.  Only ARP and IP
 * payloads are recognized and processed.  Other payloads are dropped.
 *
 * @param frame  the Ethernet header and payload to handle.
 * @param len    length of the frame buffer
 * @param intf   the interface the frame arrived on
 */
void ethernet_handle_frame( sr_router* router,
                            byte* frame,
                            unsigned len,
                            interface_t* intf ) {
   
    printf(" ** ethernet_handle_frame(..) called \n");
    // check frame len
    if(check_ethernet_frame(len) == TRUE) {
       // make ethernet header
       hdr_ethernet_t *header = make_ethernet_header(frame);
       // display header
       display_ethernet_header(header);
       // check if packet is ethernet or arp forward to corresponding module
       if(header->type == ETHERTYPE_IP) {
           printf(" ** ethernet_handle_frame(..) packet type: ip\n");
           // get ip packet length
           uint16_t packet_length;
           memcpy(&packet_length, frame+16 ,2);
           packet_length = htons(packet_length);
           // construct ip packet based on length
           byte* ip_packet = (byte*) malloc_or_die(packet_length);
           memcpy(ip_packet, frame + 14, packet_length);
           // send to ip packet handler
           ip_handle_packet(router, ip_packet, intf);
           printf (" ** ethernet_handle_frame(..) ip packet with length: %u\n", packet_length);
        }
        else if (header->type == ETHERTYPE_ARP) {
           printf(" ** ethernet_handle_frame(..) packet type: arp\n");
           // declare arp packet
           packet_arp_t *arp_packet = (packet_arp_t*) malloc_or_die(sizeof(packet_arp_t));
           // construct arp packet
           memcpy(arp_packet, frame + 14, sizeof(packet_arp_t));
           // send to arp packet handler
           arp_handle_packet(router, arp_packet, intf);
       }
   } else {
       printf(" ** ethernet_handle_frame(..) frame len incorrect, dropping \n");
   }
}

hdr_ethernet_t* make_ethernet_header(byte* frame) {
   // declare ethernet header
    hdr_ethernet_t *header = (hdr_ethernet_t*) malloc_or_die(sizeof(hdr_ethernet_t));
    // construct ethernet header
    uint8_t *type = (uint8_t*) malloc_or_die(sizeof(uint8_t));
    memcpy(type, frame + 12, 2);
    memcpy(header->dst.octet, frame, 6);
    memcpy(header->src.octet, frame + 6, 6);
    header->type = type[0];
    header->type <<= 8;
    header->type |= type[1];
    //free(type);
    return header;
}

void display_ethernet_header(hdr_ethernet_t* header) {
    printf(" ** type: %u, dst_mac: %x:%x:%x:%x:%x:%x, src_mac: %x:%x:%x:%x:%x:%x\n", header->type, header->dst.octet[0],
    header->dst.octet[1],
    header->dst.octet[2],
    header->dst.octet[3],
    header->dst.octet[4],
    header->dst.octet[5],
    header->src.octet[0],
    header->src.octet[1],
    header->src.octet[2],
    header->src.octet[3],
    header->src.octet[4],
    header->src.octet[5]);
}

bool check_ethernet_frame(unsigned len) {
//   if(len < ETH_MIN_LEN || len > ETH_MAX_LEN)
  //    return FALSE;
   return TRUE;
}


/**
 * Encapsulates the payload in an Ethernet frame destined from source address
 * src.  The caller is responsible for freeing the returned buffer.  The
 * dst mac address field is not set.
 *
 * @param src      source MAC address
 * @param intf     the interface to send the frame from
 * @param type     type of payload (network byte order)
 * @param payload  the buffer containing the payload (borrowed)
 * @param payload_len  number of bytes in the payload
 *
 * @return ethernet frame with everything except the dst mac set.  The buffer
 *         will be ETH_HEADER_LEN + payload_len bytes in size.
 */
byte* ethernet_make_partial_frame( interface_t* intf,
                                   uint16_t type,
                                   byte* payload,
                                   unsigned payload_len ) {
   printf(" ** ethernet_make_partial_frame(..) called \n");
   byte* frame = (byte*) malloc_or_die(payload_len + sizeof(hdr_ethernet_t));
   hdr_ethernet_t* header = (hdr_ethernet_t*) malloc_or_die(sizeof(hdr_ethernet_t));
   header->src = intf->mac;
   header->dst.octet[0] = 255;
   header->dst.octet[1] = 255;
   header->dst.octet[2] = 255;
   header->dst.octet[3] = 255;
   header->dst.octet[4] = 255;
   header->dst.octet[5] = 255;
   header->type = htons(type);
   printf(" ** ethernet_make_partial_frame(..) ethernet header contents \n");
   display_ethernet_header(header);
   memcpy(frame, header, sizeof(hdr_ethernet_t));
   memcpy(frame + sizeof(hdr_ethernet_t), payload, payload_len);
   //free(header);
   return frame;
}

/**
 * Encapsulates the payload in an Ethernet frame destined for dst from source
 * address src.  The caller is responsible for freeing the returned buffer.
 *
 * @param dst      destination MAC address
 * @param src      source MAC address
 * @param intf     the interface to send the frame from
 * @param type     type of payload (network byte order)
 * @param payload  the buffer containing the payload (borrowed)
 * @param payload_len  number of bytes in the payload
 *
 * @return complete ethernet frame; the buffer's size will be ETH_HEADER_LEN +
 *         payload_len bytes in size.
 */
byte* ethernet_make_frame( addr_mac_t* dst,
                           interface_t* intf,
                           uint16_t type,
                           byte* payload,
                           unsigned payload_len ) {
   printf(" ** ethernet_make_frame(..) called \n");
   byte* frame = (byte*) malloc_or_die(payload_len + sizeof(hdr_ethernet_t));
   hdr_ethernet_t* header = (hdr_ethernet_t*) malloc_or_die(sizeof(hdr_ethernet_t));
   header->src = intf->mac;
   header->dst = *dst;
   header->type = htons(type);
   printf(" ** ethernet_make_frame(..) ethernet header contents \n");
   display_ethernet_header(header);
   memcpy(frame, header, sizeof(hdr_ethernet_t));
   memcpy(frame + sizeof(hdr_ethernet_t), payload, payload_len);
   //free(header);
   return frame;
}

/**
 * Encapsulates the payload in an Ethernet frame destined for dest from source
 * address src.  The frame is transmitted via the specified interface.
 *
 * @param dst      destination MAC address
 * @param src      source MAC address
 * @param intf     the interface to send the frame from
 * @param type     type of payload (network byte order)
 * @param payload  the buffer containing the payload
 * @param payload_len  number of bytes in the payload
 *
 * @return TRUE on success; otherwise FALSE (e.g. sending the packet fails)
 */
bool ethernet_send_frame( addr_mac_t* dst,
                          interface_t* intf,
                          uint16_t type,
                          byte* payload,
                          unsigned payload_len ) {
   printf(" ** ethernet_send_frame(..) called \n");
   byte* ethernet_frame = NULL;
   // check if dst mac is zero (arp request)
   // make frame with ethernet header
   if(dst->octet[0] == 255  && dst->octet[1] == 255 && dst->octet[2] == 255 && dst->octet[3] == 255 && dst->octet[4] == 255 && dst-> octet[5] == 255) {
       ethernet_frame = ethernet_make_partial_frame(intf, type, payload, payload_len); 
   } else {
       ethernet_frame = ethernet_make_frame(dst, intf, type, payload, payload_len);
   }
   if(ethernet_frame == NULL)
      return FALSE;
   // send via output
   if(sr_integ_low_level_output(get_sr(), ethernet_frame, payload_len + sizeof(hdr_ethernet_t), intf->name) == 0)
      return TRUE;
   return FALSE;
}
