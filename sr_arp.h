#pragma once
/*
 * Filename: sr_arp.h
 * Purpose:
 *   1) Define the ARP data structure
 *   2) Process incoming ARP messages
 *   3) Send ARP requests
 */

#include "sr_common.h"
#include "sr_interface.h"
#include "sr_router.h"

/* length of an ARP request or reply packet for Ethernet and IPv4 */
#define ARP_LEN sizeof(packet_arp_t)

/* HW types IDs */
#define ARP_HW_TYPE_ETH 1

/* Protocol types IDs */
#define ARP_PROTO_TYPE_IP 8

/* HW types IDs */
#define ARP_HW_TYPE_ETH_LEN 6

/* Protocol types IDs */
#define ARP_PROTO_TYPE_IP_LEN 4


/* ARP opcodes */
#define ARP_REQUEST 1
#define ARP_REPLY   2

/**
 * The structure of an ARP packet for IP on ethernet as specified in RFC 826.
 */
typedef struct {
    uint16_t   hw_type;
    uint16_t   proto_type;
    byte       hw_len;
    byte       proto_len;
    uint16_t   opcode;
    addr_mac_t src_mac;
    addr_ip_t  src_ip;
    addr_mac_t dst_mac;
    addr_ip_t  dst_ip;
} __attribute__ ((packed)) packet_arp_t;

/**
 * Handles an ARP packet as specified by RFC 826.  In particular, if the source
 * hardware or protocol types are not Ethernet and IP, respectively, then the
 * packet is ignored.  Otherwise, the source hardware and protocol address are
 * updated in the cache if the hardware address is already present.  If it is a
 * request packet and the target protocol address is one of sr's addresses, then
 * an appropriate ARP reply is sent and then source hardware and protocol
 * address are added to the ARP cache if not already present.
 *
 * Note: ARP requests received for an interface other than one specified are not
 *       replied to (e.g. if eth0 receives a request for another interface on
 *       the router, eth0 will not reply).
 */
void arp_handle_packet( sr_router* router,
                        packet_arp_t* ap,
                        interface_t* intf );

void print_packet_arp(packet_arp_t* arp_packet);

void arp_send_request( addr_ip_t ip, interface_t* intf );
