#pragma once

#include "sr_pwospf.h"
#include "sr_router.h"

#include <time.h>
#include <sys/time.h>

typedef struct router_entry_t {
   uint32_t subnet;
   uint32_t mask;
   uint32_t router_id;
} router_entry_t;

typedef struct neighbor_vertex_t {
   struct timeval* timestamp;
   router_entry_t src;
   router_entry_t dst;
   int visited;
} neighbor_vertex_t;

void neighbor_db_init(struct sr_router* router);

router_entry_t create_router_entry_t_advert(pwospf_ls_advert_t* advert);

router_entry_t create_router_entry_t(uint32_t subnet, uint32_t mask, uint32_t router_id);

neighbor_vertex_t* create_neighbor_vertex_t(router_entry_t src, router_entry_t dst);

void add_neighbor_vertex_t(sr_router* router, router_entry_t src, router_entry_t dst);

bool compare_neighbor_vertex_t(neighbor_vertex_t *v1, neighbor_vertex_t *v2);

bool compare_router_entry_t(router_entry_t *r1, router_entry_t *r2);

bool check_neighbor_vertex_t(sr_router* router, neighbor_vertex_t* vertex);

void update_neighbor_vertex_t(sr_router* router, neighbor_vertex_t* vertex);

void neighbor_db_add_interfaces(sr_router* router);

void neighbor_db_add_static(sr_router* router);

void display_neighbor_vertices(sr_router* router);

int size_neighbor_vertex_t(sr_router* router);

byte* get_ls_adverts(sr_router* router, int* size);

void update_neighbor_vertex_t_rid(sr_router* router, uint32_t ip, uint32_t rid);

bool check_neighbor_vertex_t_src_ip(sr_router* router, uint32_t ip);

bool check_neighbor_vertex_t_dst_ip(sr_router* router, uint32_t ip);

byte* get_ls_adverts_src(sr_router* router, int* size);

int size_neighbor_vertex_t_src(sr_router* router, uint32_t ip);

void display_neighbor_vertices_src(sr_router* router);

void update_neighbor_vertex_t_timestamp(sr_router* router, router_entry_t src, router_entry_t dst);
