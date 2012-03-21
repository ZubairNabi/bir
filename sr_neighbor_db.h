#pragma once

#include "sr_pwospf.h"
#include "sr_router.h"

#include <time.h>
#include <sys/time.h>

typedef struct router_entry_t {
   uint32_t router_id;
   uint32_t area_id;
} router_entry_t;

typedef struct subnet_entry_t {
   uint32_t subnet;
   uint32_t mask;
   uint32_t router_id;
} subnet_entry_t;

typedef struct neighbor_vertex_t {
   struct timeval* timestamp;
   router_entry_t router_entry;
   node* subnets;
   node** adjacencies;
   bool visited;
   int cost;
} neighbor_vertex_t;

void neighbor_db_init(struct sr_router* router);

subnet_entry_t* create_subnet_entry_t_advert(pwospf_ls_advert_t* advert);

subnet_entry_t* create_subnet_entry_t(uint32_t subnet, uint32_t mask, uint32_t router_id);

router_entry_t create_router_entry_t(uint32_t router_id, uint32_t area_id);

neighbor_vertex_t* create_neighbor_vertex_t(router_entry_t r_entry);

void add_subnet_entry_t(sr_router* router, router_entry_t r_entry, subnet_entry_t* subnet_entry);

void add_neighbor_vertex_t(sr_router* router, router_entry_t r_entry);

bool compare_subnet_entry_t(subnet_entry_t *s1, subnet_entry_t *s2);

bool compare_router_entry_t(router_entry_t *r1, router_entry_t *r2);

void update_neighbor_vertex_t_timestamp(sr_router* router, router_entry_t r_entry);

void neighbor_db_add_interfaces_static(sr_router* router);

void display_neighbor_vertices(sr_router* router);

int size_neighbor_vertex_t(sr_router* router);

void update_neighbor_vertex_t_rid(sr_router* router, uint32_t ip, uint32_t rid);

bool check_neighbor_vertex_t_router_entry_t_id(sr_router* router, uint32_t id);

byte* get_ls_adverts_src(sr_router* router, int* size);

int size_neighbor_vertex_t_src(sr_router* router, uint32_t ip);

void display_neighbor_vertices_src(sr_router* router);

void display_neighbor_vertices_str();

void update_neighbor_vertex_t(sr_router* router, router_entry_t r_entry);
