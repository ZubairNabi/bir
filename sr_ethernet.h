/*
 * Filename: sr_ethernet.h
 * Purpose: Define the Ethernet data structure and methods to send/process them
 */

#ifndef SR_ETHERNET_H
#define SR_ETHERNET_H

#include "sr_common.h"
#include "sr_router.h"
#include "sr_interface.h"

/** number of bytes in the Ethernet header (dst MAC + src MAC + type) */
#define ETH_HEADER_LEN 14

/** max bytes in the payload (usually 1500B, but jumbo may be larger */
#define ETH_MAX_DATA_LEN 2048

/** max bytes in the Ethernet frame (header + data bytes) */
#define ETH_MAX_LEN (ETH_HEADER_LEN + ETH_MAX_DATA_LEN)

/** min bytes in the Ethernet frame */
#define ETH_MIN_LEN 60

/* types IDs for data which may be in the Ethernet payload */
#define ETHERTYPE_IP  0x0800
#define ETHERTYPE_ARP 0x0806

/** Ethernet frame header */
typedef struct {
    addr_mac_t dst;   /** destination address */
    addr_mac_t src;   /** source address */
    uint16_t type;    /** Ethertype */
} __attribute__ ((packed)) hdr_ethernet_t;

/**
 * This determines what kind of payload is contained within the frame and then
 * calls the appropriate protocol's handler on the payload.  Only ARP and IP
 * payloads are recognized and processed.  Other payloads are dropped.
 *
 * @param frame  the Ethernet header and payload to handle.
 * @param len    length of the frame buffer
 * @param intf   the interface the frame arrived on
 */
void ethernet_handle_frame( sr_router* router,
                            byte* frame,
                            unsigned len,
                            interface_t* intf );

hdr_ethernet_t* make_ethernet_header(byte* frame);

void display_ethernet_header(hdr_ethernet_t* header);

bool check_ethernet_frame(unsigned len);

/**
 * Encapsulates the payload in an Ethernet frame destined from source address
 * src.  The caller is responsible for freeing the returned buffer.  The
 * dst mac address field is not set.
 *
 * @param src      source MAC address
 * @param intf     the interface to send the frame from
 * @param type     type of payload (network byte order)
 * @param payload  the buffer containing the payload (borrowed)
 * @param payload_len  number of bytes in the payload
 *
 * @return ethernet frame with everything except the dst mac set.  The buffer
 *         will be ETH_HEADER_LEN + payload_len bytes in size.
 */
byte* ethernet_make_partial_frame( interface_t* intf,
                                   uint16_t type,
                                   byte* payload,
                                   unsigned payload_len );

/**
 * Encapsulates the payload in an Ethernet frame destined for dst from source
 * address src.  The caller is responsible for freeing the returned buffer.
 *
 * @param dst      destination MAC address
 * @param src      source MAC address
 * @param intf     the interface to send the frame from
 * @param type     type of payload (network byte order)
 * @param payload  the buffer containing the payload (borrowed)
 * @param payload_len  number of bytes in the payload
 *
 * @return complete ethernet frame; the buffer's size will be ETH_HEADER_LEN +
 *         payload_len bytes in size.
 */
byte* ethernet_make_frame( addr_mac_t* dst,
                           interface_t* intf,
                           uint16_t type,
                           byte* payload,
                           unsigned payload_len );

/**
 * Encapsulates the payload in an Ethernet frame destined for dest from source
 * address src.  The frame is transmitted via the specified interface.
 *
 * @param dst      destination MAC address
 * @param src      source MAC address
 * @param intf     the interface to send the frame from
 * @param type     type of payload (network byte order)
 * @param payload  the buffer containing the payload
 * @param payload_len  number of bytes in the payload
 *
 * @return TRUE on success; otherwise FALSE (e.g. sending the packet fails)
 */
bool ethernet_send_frame( addr_mac_t* dst,
                          interface_t* intf,
                          uint16_t type,
                          byte* payload,
                          unsigned payload_len );

#endif /* SR_ETHERNET_H */
