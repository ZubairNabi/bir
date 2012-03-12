#include "sr_icmp_types_response.h"
#include "sr_icmp.h"
#include "cli/helper.h"
#include "sr_udp.h"
#include "lwtcp/lwip/inet.h"
#include "sr_common.h"

#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void icmp_type_dst_unreach_send(uint8_t* code, byte* packet, uint16_t* packet_len, struct ip* ip_header) {
   printf(" ** icmp_type_dst_unreach_send(..) called \n");
   uint8_t type = ICMP_TYPE_DST_UNREACH;
   uint32_t rest = 0;
   uint16_t data_len = ip_header->ip_hl * 4 + 8;
   byte *data = (byte*) malloc_or_die(data_len);
   // generate original ip header (htons!)
   struct ip ip_header_orig = *ip_header;
   ip_header_orig.ip_len = htons(ip_header_orig.ip_len);
   ip_header_orig.ip_id = htons(ip_header_orig.ip_id);
   ip_header_orig.ip_off = htons(ip_header_orig.ip_off);
   ip_header_orig.ip_sum = htons(ip_header_orig.ip_sum);
   // put original ip header
   memcpy(data, &ip_header_orig, ip_header->ip_hl * 4);
   // put first 8 bytes of original ip payload
   memcpy(data + ip_header->ip_hl * 4, packet + ip_header->ip_hl * 4, 8);
   make_icmp_send_packet(&type, code, &rest, data, &data_len, ip_header);
}

void icmp_type_echo_request_send(sr_router* router, struct in_addr dst, uint16_t seq, int fd) {
   printf(" ** icmp_type_echo_request_send(..) called \n");
   addr_ip_t src = make_ip_addr("128.128.128.128");
   struct ip* ip_header = (struct ip*) malloc_or_die(sizeof(struct ip));
   ip_header->ip_v = IP_4_VERSION;
   ip_header->ip_off = htons(IP_OFFSET_DF);
   ip_header->ip_p = IP_PROTOCOL_ICMP;
   // deliberate switching. will be corrected by make_ip_packet(..)
   ip_header->ip_dst.s_addr = src;
   ip_header->ip_src = dst;
   ip_header->ip_tos = IP_TOS_DEFAULT;
   ip_header->ip_id = htons(IP_ID);
   ip_header->ip_ttl = IP_TTL_DEFAULT;
   ip_header->ip_hl = IP_HEADER_MIN_LEN_WORD;
   ip_header->ip_len = htons(IP_HEADER_MIN_LEN);
   ip_header->ip_sum = 0;
   ip_header->ip_sum = htons(htons(inet_chksum((void*)ip_header, IP_HEADER_MIN_LEN)));
   uint8_t type = ICMP_TYPE_ECHO_REQUEST;
   uint8_t code = ICMP_TYPE_CODE_ECHO_REQUEST;
   uint16_t id = (uint16_t) getpid();
   uint32_t rest = 0;
   memcpy(&rest, &id, 2);
   memcpy(&rest + 2, &seq, 2);
   uint32_t data_ = 0;
   uint16_t data_len_ = sizeof(uint32_t);
   byte* data = (byte*) malloc_or_die(data_len_);
   memcpy(data, &data_, data_len_);
   // set ping info
   printf("$$$$$$$$$$$$$ fd %d", fd);
   set_ping_info(router, rest, dst.s_addr, fd);
   make_icmp_send_packet(&type, &code, &rest, data, &data_len_, ip_header);
}

void icmp_type_ttl_send(uint8_t* code, byte* packet, uint16_t* packet_len, struct ip* ip_header) {
   printf(" ** icmp_type_ttl_send(..) called \n");
   uint8_t type = ICMP_TYPE_TTL;
   uint32_t rest = 0;
   uint16_t data_len = ip_header->ip_hl * 4 + 8;
   // generate original ip header (htons!)
   struct ip ip_header_orig = *ip_header;
   ip_header_orig.ip_len = htons(ip_header_orig.ip_len);
   ip_header_orig.ip_id = htons(ip_header_orig.ip_id);
   ip_header_orig.ip_off = htons(ip_header_orig.ip_off);
   ip_header_orig.ip_sum = htons(ip_header_orig.ip_sum);
   byte *data = (byte*) malloc_or_die(data_len);
   // put original ip header
   memcpy(data, &ip_header_orig, ip_header->ip_hl * 4);
   // put first 8 bytes of original ip payload
   memcpy(data + ip_header->ip_hl * 4, packet, 8);
   make_icmp_send_packet(&type, code, &rest, data, &data_len, ip_header); 
}

void icmp_type_traceroute_send(uint32_t* rest, byte* packet, uint16_t* packet_len, struct ip* ip_header) {
   printf(" ** icmp_type_traceroute_send(..) called \n");
}

void icmp_type_bad_ip_send(uint8_t* code, uint32_t* rest, byte* packet, uint16_t* packet_len, struct ip* ip_header) {
   printf(" ** icmp_type_bad_ip_send(..) called \n");
}
