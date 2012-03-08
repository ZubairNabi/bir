#include "sr_neighbor_db.h"
#include "cli/helper.h"

void neighbor_db_init(sr_router* router) {
   router->neighbor_db_list = llist_new();
}

router_entry_t create_router_entry_t_advert(pwospf_ls_advert_t* advert) {
   router_entry_t router_entry;
   router_entry.subnet = advert->subnet;
   router_entry.mask = advert->mask;
   router_entry.router_id = advert->router_id;
   return router_entry;
}

router_entry_t create_router_entry_t(uint32_t subnet, uint32_t mask, uint32_t router_id) {
   router_entry_t router_entry;
   router_entry.subnet = subnet;
   router_entry.mask = mask;
   router_entry.router_id = router_id;
   return router_entry;
}

neighbor_vertex_t create_neighbor_vertex_t(router_entry_t src, router_entry_t dst) {
   neighbor_vertex_t neighbor_vertex;
   neighbor_vertex.src = src;
   neighbor_vertex.dst = dst;
   neighbor_vertex.visited = 0;
   neighbor_vertex.timestamp = (struct timeval*) malloc_or_die(sizeof(struct timeval));
   gettimeofday(neighbor_vertex.timestamp, NULL);
   return neighbor_vertex;
}
