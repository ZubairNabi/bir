#pragma once

#include "sr_router.h"
#include "sr_interface.h"

#include <arpa/inet.h>
#include <netinet/ip.h>

#define PWOSPF_VER 2;
#define PWOSPF_TYPE_HELLO 1;
#define PWOSPF_TYPE_LSU 4;
#define PWOSPF_AU_TYPE 0;
#define PWOSPF_AUTHEN 0;
#define ALL_SPF_ROUTERS_IP "224.0.0.5";
#define ALL_SPF_ROUTERS_MAC "01:00:5e:00:00:05";
#define PWOSPF_LSU_INTERVAL 30;
#define PWOSPF_AREA_ID 0;
#define PWOSPF_LSU_TTL 64;


typedef struct pwospf_header_t {
   uint8_t version;
   uint8_t type;
   uint16_t len; // packet + header
   uint32_t router_id;
   uint32_t area_id;
   uint16_t checksum; // checksum of entire packet except authen
   uint16_t au_type;
   uint64_t authentication;
} pwospf_header_t;

typedef struct pwospf_hello_packet_t {
   uint32_t network_mask;
   uint16_t helloint;
   uint16_t padding;
} pwospf_hello_packet_t;

typedef struct pwospf_lsu_packet_t {
   uint16_t seq;
   uint16_t ttl;
   uint32_t no_of_adverts;
} pwospf_lsu_packet_t;

typedef struct pwospf_ls_advert_t {
   uint32_t subnet;
   uint32_t mask;
   uint32_t router_id;
} pwospf_ls_advert_t;

pwospf_header_t* make_pwospf_header(byte* packet);

void pwospf_handle_packet(byte* pwospf_packet,
                          uint16_t packet_len,
                          struct ip* ip_header,
                          interface_t* intf,
                          sr_router* router);

void display_pwospf_header(pwospf_header_t* header);

bool check_pwospf_header(pwospf_header_t* header, byte* pwospf_packet, uint16_t packet_len, sr_router* router);

void pwospf_check_packet_type(struct ip* ip_header, byte* payload, uint16_t payload_len, pwospf_header_t* pwospf_header, interface_t* intf);

void send_hello_packet(void* interface_input);

void pwospf_init( sr_router* router );

void remove_timed_out_neighbors(void* intf_input);

void send_lsu_packet(void* intf_input);
