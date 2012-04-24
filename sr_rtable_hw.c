/* Filename: sr_rtable_hw.c */

#include <stdio.h>     /* snprintf() */
#include <stdlib.h>
#include "sr_rtable_hw.h"
#include "cli/helper.h"
#include "sr_router.h"
#include "cli/cli.h"
#include "sr_integration.h"
#include "reg_defines.h"

/**
 * Initialize the routing table
 * 
 * @param rtable  A pointer to the routing table to initialize
 */
void hw_rrtable_init(struct sr_router *router){
   printf(" ** hw_rrtable_init(..) called \n");
   // initialize routing table list
   router->hw_rtable = (hw_rtable_t*) malloc_or_die(sizeof(hw_rtable_t));
   router->hw_rtable->hw_rtable_list = llist_new();
   // initialize lock
   pthread_mutex_init(&router->hw_rtable->hw_lock_rtable, NULL); 
}

/**
 * Free memory associated with all members of the routing table, but not
 * the routing table itself.
 *
 * @param rtable  A pointer to the routing table to destroy
 */
void hw_rrtable_destroy(hw_rtable_t *hw_rtable){
    printf(" ** hw_rrtable_destroy(..) called \n");
    // clean routing table list
    pthread_mutex_lock(&hw_rtable->hw_lock_rtable);
    hw_rtable->hw_rtable_list = llist_delete_no_count(hw_rtable->hw_rtable_list);
    pthread_mutex_unlock(&hw_rtable->hw_lock_rtable);
    // delete lock
    pthread_mutex_destroy(&hw_rtable->hw_lock_rtable);
    // write to hw
    hw_rrtable_write_hw();
}

/**
 * Add a route to the routing table in a threadsafe manner.  Each entry is
 * marked as either static or dynamic depending on its.
 * 
 * @param rtable  The routing table to add to
 * @param dest  The destination IP address
 * @param mask  The subnet mask
 * @param intf  The interface this subnet is on
 */
void hw_rrtable_route_add( hw_rtable_t *hw_rtable,
                       addr_ip_t dest, addr_ip_t mask,
                       uint16_t primary, uint16_t backup) {
   printf(" ** hw_rrtable_route_add(..) called \n");
   // get subnet
   dest = dest & mask;
   // make route_t object
   hw_route_t* route = hw_make_route_t(dest, primary, backup, mask);
   // lock table
   pthread_mutex_lock(&hw_rtable->hw_lock_rtable);
   // add route to table
   hw_rtable->hw_rtable_list = llist_insert_sorted(hw_rtable->hw_rtable_list, predicate_ip_sort_hw_route_t, (void*) route);
   // unblock table
   pthread_mutex_unlock(&hw_rtable->hw_lock_rtable);
   // write to hw
   hw_rrtable_write_hw();
}

/**
 * Remove the specified route from the routing table, if it exists.  Return TRUE
 * on success, FALSE if the route does not exist.  Threadsafe.
 *
 * @param rtable A pointer to the routing table to remove the route from
 * @param dest  The destination IP of the route
 * @param mask  The subnet mask of the route
 * 
 * @return bool indicating success (TRUE) or non-existent route (FALSE)
 */
bool hw_rrtable_route_remove( hw_rtable_t *hw_rtable, addr_ip_t dest, addr_ip_t mask ) {
   printf(" ** hw_rrtable_route_remove(..) called \n");
    // lock table
   pthread_mutex_lock(&hw_rtable->hw_lock_rtable);
   // remove route from table
   node* ret = llist_remove(hw_rtable->hw_rtable_list, predicate_ip_sort_hw_route_t, (void*) &dest);
   // unlock table
   pthread_mutex_unlock(&hw_rtable->hw_lock_rtable);
   if(ret == NULL)
      return FALSE;
   else {
      hw_rtable->hw_rtable_list = ret;
      // write to hw
      hw_rrtable_write_hw();
      return TRUE;
   }
}

/**
 * Removes all routes from a routing table (threadsafe).
 * 
 * @param rtable  A pointer to the routing table from which all routes will be
 * removed.
 */
void hw_rrtable_purge_all( hw_rtable_t* hw_rtable ) {
   printf(" ** hw_rrtable_purge_all(..) called \n");
   // lock table
   pthread_mutex_lock(&hw_rtable->hw_lock_rtable);
   // remove route from table
   hw_rtable->hw_rtable_list = llist_delete_no_count(hw_rtable->hw_rtable_list);
   // unlock table
   pthread_mutex_unlock(&hw_rtable->hw_lock_rtable);
   // write to hw
   hw_rrtable_write_hw();
}  

/**
 * Fills a buffer with a string representation of the routing table
 *
 */

void hw_rrtable_to_string() {
   char *str;
   asprintf(&str, "---------------------------------------------------------\n");
   cli_send_str(str);
   free(str);
   asprintf(&str, "|%-15s|%-15s|%-11s|%-11s|\n", "Destination", "Mask", "Primary", "Backup");
   cli_send_str(str);
   free(str);
   asprintf(&str, "---------------------------------------------------------\n");
   cli_send_str(str);
   free(str);
   node* head;
   hw_route_t* entry;
   // get instance of router 
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   pthread_mutex_lock(&router->hw_rtable->hw_lock_rtable);
   head = router->hw_rtable->hw_rtable_list;
   while(head != NULL) {
      entry = (hw_route_t*) head->data;
      asprintf(&str, "|%-15s|", quick_ip_to_string(entry->destination));
      cli_send_str(str);
      free(str);
      asprintf(&str, "%-15s|%-11d|%-11d|\n", quick_ip_to_string(entry->subnet_mask), entry->primary, entry->backup);
      cli_send_str(str);
      free(str);
      head = head->next;
   }
   asprintf(&str, "---------------------------------------------------------\n");
   cli_send_str(str);
   free(str);
   pthread_mutex_unlock(&router->hw_rtable->hw_lock_rtable);
}

hw_route_t* hw_make_route_t(addr_ip_t dst, uint16_t primary, uint16_t backup, addr_ip_t subnet_mask) {
   hw_route_t* route = (hw_route_t*) malloc_or_die(sizeof(hw_route_t));
   route->destination = dst;
   route->primary = primary;
   route->backup = backup;
   route->subnet_mask = subnet_mask;
   return route;
} 

void hw_rrtable_write_hw() {
#ifdef _CPUMODE_
   printf(" ** hw_rrtable_write_hw(..) called \n");
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   pthread_mutex_lock(&router->hw_rtable->hw_lock_rtable);
   hw_route_t* route = NULL;
   node *head = router->hw_rtable->hw_rtable_list;
   int i = 0;
   while(head != NULL && i != ROUTER_OP_LUT_ROUTE_TABLE_DEPTH) {
      route = (hw_route_t*) head->data;
      // Destination IP
      writeReg(&router->hw_device, ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_IP_REG, ntohl(route->destination));
      // Mask 
      writeReg(&router->hw_device, ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_MASK_REG, ntohl(route->subnet_mask));
      //Output ports
      writeReg(&router->hw_device, ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_PRIMARY_OUTPUT_PORT_REG, route->primary + route->backup);
      writeReg(&router->hw_device, ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_BACKUP_OUTPUT_PORT_REG, route->backup);
      // row index
      writeReg(&router->hw_device, ROUTER_OP_LUT_ROUTE_TABLE_WR_ADDR_REG, i);
      head = head->next;
      i++;
   }
   pthread_mutex_unlock(&router->hw_rtable->hw_lock_rtable);
#endif
}

void hw_rrtable_read_hw() {
#ifdef _CPUMODE_
   printf(" ** hw_rrtable_read_hw(..) called \n");
   struct sr_instance* sr_inst = get_sr();
   struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
   char *str;
   int i;
   uint32_t destination, mask, primary = 0, backup = 0;
   asprintf(&str, "Dst IP, Subnet Mask, Primary, Backup\n");
   cli_send_str(str);
   free(str);
   for(i = 0; i < ROUTER_OP_LUT_ROUTE_TABLE_DEPTH; i++) {
        // write index to read register
        writeReg(&router->hw_device, ROUTER_OP_LUT_ROUTE_TABLE_RD_ADDR_REG, i);
        // read values from registers
        readReg(&router->hw_device, ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_IP_REG, &destination);
        readReg(&router->hw_device, ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_MASK_REG, &mask);
        readReg(&router->hw_device, ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_PRIMARY_OUTPUT_PORT_REG, &primary);
        readReg(&router->hw_device, ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_BACKUP_OUTPUT_PORT_REG, &backup);
        //check for blank entries
        if(destination == 0)
           break;
        // get interface for hw port
        asprintf(&str, "%s, ", quick_ip_to_string(htonl(destination)));
        cli_send_str(str);
        free(str);
        asprintf(&str, " %s, %d, %d\n", quick_ip_to_string(htonl(mask)), primary, backup);
        cli_send_str(str);
        free(str);
   }
#endif   

}
