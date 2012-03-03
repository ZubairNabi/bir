#pragma once

#include "sr_router.h"

#include <time.h>
#include <sys/time.h>

typedef struct neighbor_entry_t {
   struct timeval* timestamp;
} neighbor_entry_t;

void neighbor_db_init(sr_router* router);
