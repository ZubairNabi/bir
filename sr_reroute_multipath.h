#pragma once

#include "sr_router.h"

typedef struct multipath_list_data_t {
   hw_route_t route;
   int primary_cost;
   int backup_cost;
   bool primary_flag;
   /*if 0 then no primary, otherwise 1*/
} multipath_list_data_t;

void reroute_multipath(sr_router* router);
