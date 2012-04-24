#include "sr_reroute_multipath.h"
#include "sr_dijkstra.h"

void reroute_multipath(sr_router* router) {
   printf(" ** reroute_multipath(..) called \n");
   int i = 0;
   //to hold the confirmed list for each interface
   node* confirmed[4];
   //run dijkstra for each interface
   for(i = 0; i < router->num_interfaces; i++) {
      confirmed[i] = dijkstra(router, &router->interface[i]); 
   }
}
