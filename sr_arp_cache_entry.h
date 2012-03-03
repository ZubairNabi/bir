#pragma once
#include "sr_common.h"

typedef struct arp_cache_entry_t {
        addr_ip_t ip;
        addr_mac_t mac;
        struct timeval *timeout;
} arp_cache_entry_t;
/*Packet queue entry*/
typedef struct arp_queue_entry_t {
        addr_ip_t ip;
        int tries;
} arp_queue_entry_t;

arp_cache_entry_t* create_entry(char*, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);

arp_queue_entry_t* create_queue_entry(char*, int);
