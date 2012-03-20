/*
 * Filename: sr_interface.h
 * Purpose: Data structure containing information about an interface.
 */

#ifndef SR_INTERFACE_H
#define SR_INTERFACE_H

#include <pthread.h>
#include "sr_common.h"
#include "sr_base_internal.h"
#include "linked_list.h"

#define SR_NAMELEN 32
#define PWOSPF_HELLO_INTERVAL 5;

/* forward declaration */
struct neighbor_t;
struct sr_router;

/** holds info about a router's interface */
typedef struct {
    char name[SR_NAMELEN]; /* name of the interface        */
    addr_mac_t mac;        /* MAC address of the interface */
    addr_ip_t ip;          /* IP address of the interface  */
    bool enabled;          /* whether the interface is on  */
    addr_ip_t subnet_mask; /* subnet mask of the link */
#ifdef _CPUMODE_
#   define INTF0 0x02
#   define INTF1 0x08
#   define INTF2 0x20
#   define INTF3 0x80
    byte hw_id;            /* hardware id of the interface */
    int  hw_fd;            /* socket file descriptor to talk to the hw */
    pthread_mutex_t hw_lock; /* lock to prevent issues w/ multiple writers */
#endif

    pthread_mutex_t neighbor_lock;
    node* neighbor_list; /* neighboring nodes */
    uint16_t helloint;
    uint32_t router_id;
    uint32_t area_id;
    uint16_t lsuint;
} interface_t;

interface_t set_interface(struct sr_vns_if* vns_intf);

interface_t make_interface(char* name, addr_mac_t mac, addr_ip_t ip, bool enabled, addr_ip_t subnet_mask);

interface_t* get_interface_name(struct sr_router* router, const char* name); 

interface_t* get_interface_ip(struct sr_router* router, addr_ip_t ip);

void interface_init(struct sr_router* subsystem);

void display_interface(interface_t* intf);

void display_all_interfaces();

void display_all_interfaces_str();

void toggle_interface(interface_t* intf, int enabled);

bool check_interface(interface_t* intf);

void display_all_interfaces_neighbors_str();

void write_interface_hw(struct sr_router* router);

#endif /* SR_INTERFACE_H */
