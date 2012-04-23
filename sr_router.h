#pragma once
/*
 * File: sr_router.h
 * Author: Zubair Nabi <zn218@cam.ac.uk>
 * 
 * Contains router state: routing table, ARP cache, interface list etc.
 */

#define ROUTER_MAX_INTERFACES 4
#include "sr_arp_cache.h"
#include "sr_interface.h"
#include "sr_rtable.h"
#include "nf2util.h"
#include "sr_rtable_hw.h"

typedef struct neighbor_db_t {
   node* neighbor_db_list;
   pthread_mutex_t neighbor_db_lock;
} neighbor_db_t;

typedef struct ls_info_t {
   uint32_t router_id;
   uint32_t area_id;
   uint16_t lsuint;
   uint32_t lsu_seq;
} ls_info_t;

typedef struct ping_info_t {
   uint32_t rest;
   uint32_t ip;
   int fd;
} ping_info_t;
             
typedef struct sr_router {
   arp_cache_t* arp_cache;   
   interface_t interface[ROUTER_MAX_INTERFACES];
   unsigned num_interfaces;
   rtable_t* rtable;
   hw_rtable_t* hw_rtable;
   bool ospf_status;
   ls_info_t ls_info;
   //meta data for ping
   ping_info_t ping_info;
   neighbor_db_t* neighbor_db;
   // hw device
   struct nf2device hw_device;
   // fast reroute and multipath
   bool reroute_multipath_status;
} sr_router;

void toggle_ospf_status(sr_router* router, bool status);

ls_info_t make_ls_info(uint32_t id, uint32_t, uint16_t lsuint);

void set_ls_info(sr_router* router, ls_info_t ls_info);

void display_ls_info(ls_info_t ls_info);

ls_info_t make_ls_info_id(sr_router* router, uint32_t area_id, uint16_t lsuint);

void set_ls_info_id(sr_router* router, uint32_t area_id, uint16_t lsuint);

void set_ping_info(sr_router* router, uint32_t rest, uint32_t ip, int fd);

void clear_ping_info(sr_router* router);

bool check_ospf_status(sr_router* router);

void hw_init(sr_router* router);

void toggle_reroute_multipath_status(sr_router* router, bool status);

bool show_reroute_multipath_status(sr_router* router);

void show_reroute_multipath();

void toggle_reroute_multipath();

void write_hw_reroute_multipath(sr_router* router, bool status);
