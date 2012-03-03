#include "sr_neighbor_db.h"

void neighbor_db_init(sr_router* router) {
   router->neighbor_db_list = llist_new();
}
