#pragma once

#include "sr_router.h"
#include "sr_neighbor_db.h"

typedef struct dijkstra_list_data_t {
   neighbor_vertex_t* destination;
   int cost;
   uint32_t next_hop;
} dijkstra_list_data_t;

node* dijkstra(sr_router* router);

void dijkstra2(sr_router* router);

void display_dijkstra_list_data_t(dijkstra_list_data_t*);

void calculate_routing_table(sr_router *router);
