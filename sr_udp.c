#include "sr_udp.h"
#include "cli/helper.h"
#include "lwtcp/lwip/inet.h"

#include <string.h>

bool check_udp_header(udp_header_t* header, byte* raw_packet, struct ip* ip_header) {
   // generate pseudo ip header
   pseudo_ip_header_t* pseudo_header = make_pseudo_ip_header(ip_header, header->len);
   // generate raw packet for checksum
   uint8_t len_raw_packet = sizeof(pseudo_ip_header_t) + header->len;
   udp_header_t* sum_header = (udp_header_t*) malloc_or_die(sizeof(udp_header_t));
   memcpy(sum_header, header, UDP_HEADER_LEN);
   // make the checksum and convert to network order for checksum
   sum_header->checksum = 0;
   sum_header->src_port = htons(sum_header->src_port);
   sum_header->dst_port = htons(sum_header->dst_port);
   sum_header->len = htons(sum_header->len);
    
   // copy values
   byte* raw_packet_sum = (byte*) malloc_or_die(len_raw_packet);
   memcpy(raw_packet_sum, pseudo_header, sizeof(pseudo_ip_header_t));
   memcpy(raw_packet_sum + sizeof(pseudo_ip_header_t) , sum_header, UDP_HEADER_LEN);
   memcpy(raw_packet_sum + sizeof(pseudo_ip_header_t) + UDP_HEADER_LEN, raw_packet + UDP_HEADER_LEN, header->len - UDP_HEADER_LEN);
   if(ntohs(inet_chksum((void*) raw_packet_sum, len_raw_packet) != header->checksum))
      return FALSE;
   return TRUE;
}

void udp_handle_packet(byte* udp_packet, struct ip* ip_header, 
                        interface_t* intf ) {
   printf(" ** udp_handle_packet(..) called \n");
   // generate udp header
   udp_header_t* header = make_udp_header(udp_packet);
   // check checksum
   if(check_udp_header(header, udp_packet, ip_header) == TRUE) {
      // display udp header
      display_udp_header(header);
      printf(" ** udp_handle_packet(..) checksum correct\n");
   } else {
      printf(" ** udp_handle_packet(..) checksum failed, dropping!\n");
   }
}

udp_header_t* make_udp_header(byte* udp_packet) {
   udp_header_t* header = (udp_header_t*) malloc_or_die(sizeof(udp_header_t));
   memcpy(&header->src_port, udp_packet, 2);
   header->src_port = ntohs(header->src_port);
   memcpy(&header->dst_port, udp_packet + 2, 2);
   header->dst_port = ntohs(header->dst_port);
   memcpy(&header->len, udp_packet + 4, 2);
   header->len = ntohs(header->len);
   memcpy(&header->checksum, udp_packet + 6, 2);
   return header;
}

void display_udp_header(udp_header_t* header) {
   printf(" ** src port: %u\n", (header->src_port));
   printf(" ** dst port: %u\n", (header->dst_port));
   printf(" ** len:  %u\n",  (header->len));
   printf(" ** checksum: %u\n", header->checksum);
}

pseudo_ip_header_t* make_pseudo_ip_header(struct ip* header, uint16_t udp_len) {
   pseudo_ip_header_t* pseudo_header = (pseudo_ip_header_t*) malloc_or_die(sizeof(pseudo_ip_header_t));
   pseudo_header->src_ip = header->ip_src;
   pseudo_header->dst_ip = header->ip_dst;
   pseudo_header->protocol = IP_PROTOCOL_UDP;
   pseudo_header->zeros = 0;
   pseudo_header->udp_len = htons(udp_len);
   return pseudo_header; 
}
