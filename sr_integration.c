/*-----------------------------------------------------------------------------
 * file:  sr_integration.c
 * date:  Tue Feb 03 11:29:17 PST 2004
 * Author: Martin Casado <casado@stanford.edu>
 *
 * Description:
 *
 * Methods called by the lowest-level of the network system to talk with
 * the network subsystem.
 *
 * This is the entry point of integration for the network layer.
 *
 *---------------------------------------------------------------------------*/

#include <stdlib.h>
#include <netinet/ip.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>

#include "sr_vns.h"
#include "sr_base_internal.h"
#include "sr_router.h"
#include "sr_common.h"
#include "sr_ethernet.h"
#include "sr_arp_cache.h"
#include "linked_list.h"
#include "sr_arp.h"
#include "cli/helper.h"
#include "sr_ip.h"
#include "sr_rtable.h"
#include "sr_pwospf.h"
#include "sr_rt.h"
#include "sr_neighbor_db.h"

#ifdef _CPUMODE_
#include "sr_cpu_extension_nf2.h"
#include "nf2util.h"
#include "nf2.h"
#endif

struct sr_instance* get_sr(); 
int sr_integ_low_level_output(struct sr_instance*,
                             uint8_t*,
                             unsigned int,
                             const char*);

/*-----------------------------------------------------------------------------
 * Method: sr_integ_init(..)
 * Scope: global
 *
 *
 * First method called during router initialization.  Called before connecting
 * to VNS, reading in hardware information etc.
 *
 *---------------------------------------------------------------------------*/

void sr_integ_init(struct sr_instance* sr)
{
    printf(" ** sr_integ_init(..) called \n");
    sr_router* subsystem = (sr_router*)malloc(sizeof(sr_router));
    assert(subsystem);
    sr_set_subsystem(get_sr(), subsystem);
    // initialize arp and interfaces and routing table
    arp_cache_init(subsystem);
    interface_init(subsystem);
    rrtable_init(subsystem);
    //enable ospf by default
    toggle_ospf_status(subsystem, TRUE);
    //flush hw registers
#ifdef _CPUMODE_
    hw_init(subsystem);
    writeReg(&subsystem->hw_device, CPCI_REG_CTRL, 0x00010100);
    usleep(2000);
#endif
} /* -- sr_integ_init -- */

/*-----------------------------------------------------------------------------
 * Method: sr_integ_hw_setup(..)
 * Scope: global
 *
 * Called after all initial hardware information (interfaces) have been
 * received.  Can be used to start subprocesses (such as dynamic-routing
 * protocol) which require interface information during initialization.
 *
 *---------------------------------------------------------------------------*/

void sr_integ_hw_setup(struct sr_instance* sr)
{
    printf(" ** sr_integ_hw(..) called \n");
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    printf(" ** sr_integ_hw(..) adding entries to static rtable \n");
    // read rtable file
    sr_load_rt(sr, sr->rtable);
    // add static entries to routing table
    printf(" ** sr_integ_hw(..) initial state of routing table\n");
    llist_display_all(subsystem->rtable->rtable_list, display_route_t);
    // set link state info
    uint32_t area_id = PWOSPF_AREA_ID;
    uint16_t lsuint = PWOSPF_LSU_INTERVAL;
    set_ls_info_id(subsystem, area_id, lsuint);
    // initialize pwospf
    pwospf_init(subsystem);
    // initialize neighbor db
    neighbor_db_init(subsystem);
    // display initial state of neighbor db
    printf(" ** sr_integ_hw(..) initial state of neighbor db\n");
    display_neighbor_vertices(subsystem);
} /* -- sr_integ_hw_setup -- */

/*---------------------------------------------------------------------
 * Method: sr_integ_input(struct sr_instance*,
 *                        uint8_t* packet,
 *                        char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_integ_input(struct sr_instance* sr,
        const uint8_t * packet/* borrowed */,
        unsigned int len,
        const char* interface/* borrowed */)
{
    /* -- INTEGRATION PACKET ENTRY POINT!-- */

    printf(" ** sr_integ_input(..) called \n");
    // get global router instance
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    // get interface object for this interface name
    interface_t *intf = get_interface_name(subsystem, interface);
    printf(" ********************* new packet received with length %u on inteface: %s\n", len, intf->name);
    // create a copy of the packet
    uint8_t* packet_copy = (uint8_t*) malloc_or_die(len);
    memcpy(packet_copy, packet, len);
    // send to ethernet frame handler
    ethernet_handle_frame(subsystem, packet_copy, len, intf);

} /* -- sr_integ_input -- */

/*-----------------------------------------------------------------------------
 * Method: sr_integ_add_interface(..)
 * Scope: global
 *
 * Called for each interface read in during hardware initialization.
 * struct sr_vns_if is defined in sr_base_internal.h
 *
 *---------------------------------------------------------------------------*/

void sr_integ_add_interface(struct sr_instance* sr,
                            struct sr_vns_if* vns_if/* borrowed */)
{
    printf(" ** sr_integ_add_interface(..) called \n");
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(sr);
    subsystem->interface[subsystem->num_interfaces] = set_interface(vns_if, subsystem->num_interfaces);
#ifdef _CPUMODE_
    write_interface_hw(subsystem);
#endif
    subsystem->num_interfaces = subsystem->num_interfaces + 1;
    printf(" ** sr_integ_add_interface(..) current no. of interfaces: %u\n", subsystem->num_interfaces);
    
} /* -- sr_integ_add_interface -- */

struct sr_instance* get_sr() {
    struct sr_instance* sr;

    sr = sr_get_global_instance( NULL );
    assert( sr );
    return sr;
}

/*-----------------------------------------------------------------------------
 * Method: sr_integ_low_level_output(..)
 * Scope: global
 *
 * Send a packet to VNS to be injected into the topology
 *
 *---------------------------------------------------------------------------*/

int sr_integ_low_level_output(struct sr_instance* sr /* borrowed */,
                             uint8_t* buf /* borrowed */ ,
                             unsigned int len,
                             const char* iface /* borrowed */)
{
#ifdef _CPUMODE_
    return sr_cpu_output(sr, buf /*lent*/, len, iface);
#else
    return sr_vns_send_packet(sr, buf /*lent*/, len, iface);
#endif /* _CPUMODE_ */
} /* -- sr_vns_integ_output -- */

/*-----------------------------------------------------------------------------
 * Method: sr_integ_destroy(..)
 * Scope: global
 *
 * For memory deallocation pruposes on shutdown.
 *
 *---------------------------------------------------------------------------*/

void sr_integ_destroy(struct sr_instance* sr)
{
    printf(" ** sr_integ_destroy(..) called \n");
} /* -- sr_integ_destroy -- */

/*-----------------------------------------------------------------------------
 * Method: sr_integ_findsrcip(..)
 * Scope: global
 *
 * Called by the transport layer for outgoing packets generated by the
 * router.  Expects source address in network byte order.
 *
 *---------------------------------------------------------------------------*/

uint32_t sr_integ_findsrcip(uint32_t dest /* nbo */)
{

    /* --
     * e.g.
     *
     * struct sr_instance* sr = sr_get_global_instance();
     * struct my_router* mr = (struct my_router*)
     *                              sr_get_subsystem(sr);
     * return my_findsrcip(mr, dest);
     * -- */
    struct sr_router* subsystem = (struct sr_router*)sr_get_subsystem(get_sr());
   // look up in routing table
    route_info_t route_info = rrtable_find_route(subsystem->rtable, dest);

    return route_info.intf->ip;
} /* -- ip_findsrcip -- */

/*-----------------------------------------------------------------------------
 * Method: sr_integ_ip_output(..)
 * Scope: global
 *
 * Called by the transport layer for outgoing packets that need IP
 * encapsulation.
 *
 *---------------------------------------------------------------------------*/

uint32_t sr_integ_ip_output(uint8_t* payload /* given */,
                            uint8_t  proto,
                            uint32_t src, /* nbo */
                            uint32_t dest, /* nbo */
                            int len)
{

    /* --
     * e.g.
     *
     * struct sr_instance* sr = sr_get_global_instance();
     * struct my_router* mr = (struct my_router*)
     *                              sr_get_subsystem(sr);
     * return my_ip_output(mr, payload, proto, src, dest, len);
     * -- */
      struct in_addr dst_ip, src_ip;
      memcpy(&dst_ip, &src, 4);
      memcpy(&src_ip, &dest, 4);
      make_ip_packet(payload, len, src_ip, dst_ip, proto);

      return 0;
} /* -- ip_integ_route -- */

/*-----------------------------------------------------------------------------
 * Method: sr_integ_close(..)
 * Scope: global
 *
 *  Called when the router is closing connection to VNS.
 *
 *---------------------------------------------------------------------------*/

void sr_integ_close(struct sr_instance* sr)
{
    printf(" ** sr_integ_close(..) called \n");
}  /* -- sr_integ_close -- */
