#include <time.h>
#include <sys/time.h>
#include "sr_arp_cache_entry.h"
#include "sr_common.h"
#include "cli/helper.h"

arp_cache_entry_t* create_entry(char* ip, uint8_t o1, uint8_t o2, uint8_t o3, uint8_t o4, uint8_t o5, uint8_t o6) {
   arp_cache_entry_t *entry = (arp_cache_entry_t*) malloc_or_die (sizeof(arp_cache_entry_t));
   entry->ip = make_ip_addr(ip);
   entry->mac = make_mac_addr(o1,o2,o3,o4,o5,o6);
   entry->timeout = (struct timeval*) malloc_or_die(sizeof(struct timeval));
   gettimeofday(entry->timeout, NULL);
   return entry;
}

arp_queue_entry_t* create_queue_entry(char* ip, int tries) {
   arp_queue_entry_t *entry = (arp_queue_entry_t*) malloc_or_die (sizeof(arp_queue_entry_t));
   entry->ip = make_ip_addr(ip);
   entry->tries = tries;
   return entry;
}
