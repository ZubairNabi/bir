#include "sr_common.h"
#include "cli/helper.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

/** Uses the string to make an IP address */
addr_ip_t make_ip_addr(char* str ) {
   addr_ip_t ip;
   unsigned char bytes[4];
   sscanf(str, "%hhu.%hhu.%hhu.%hhu", &bytes[0], &bytes[1], &bytes[2], &bytes[3]);
   ip  = ntohl(( bytes[0] << 24 ) | ( bytes[1] << 16 ) | ( bytes[2] << 8 ) | bytes[3]);
   return ip;
}

/** Uses the specified bytes to create a MAC address */
addr_mac_t make_mac_addr(byte byte5, byte byte4, byte byte3, byte byte2, byte byte1, byte byte0) {
   addr_mac_t *mac = (addr_mac_t*) malloc_or_die(sizeof(addr_mac_t));
   mac->octet[5] = byte0;
   mac->octet[4] = byte1;
   mac->octet[3] = byte2;
   mac->octet[2] = byte3;
   mac->octet[1] = byte4;
   mac->octet[0] = byte5;
   return *mac;
}

/** Uses the specified ptr to create a MAC address */
addr_mac_t* make_mac_addr_ptr(uint8_t* mac_ptr) {
   addr_mac_t *mac = (addr_mac_t*) malloc_or_die(sizeof(addr_mac_t));
   mac->octet[0] = *mac_ptr;
   mac->octet[1] = *mac_ptr + 1;
   mac->octet[2] = *mac_ptr + 2;
   mac->octet[3] = *mac_ptr + 3;
   mac->octet[4] = *mac_ptr + 4;
   mac->octet[5] = *mac_ptr + 5;
   return mac;
}

/** Returns TRUE if t1 is later than t2. */
bool is_later( struct timeval* t1, struct timeval* t2 ) {
   if(t1->tv_sec > t2->tv_sec)
      return TRUE;
   else
      return FALSE;
}

/*Function to compare two MAC addresses for equivalence*/
int equal_mac(addr_mac_t mac1, addr_mac_t mac2) {
   int i = 0;
   for(i = 0 ; i < ETH_ADDR_LEN; i++) {
      if(mac1.octet[i] != mac2.octet[i])
          return 0;
   }
   return 1;
}

/*Function to compare ip addresses*/
/* @return  1 if ip1 > ip2
            -1 if ip1 < ip2
            0 if ip1 = ip2*/
int compare_ip(addr_ip_t ip1, addr_ip_t ip2) {
   unsigned str_len_ip1 = strlen(quick_ip_to_string(ip1));
   unsigned str_len_ip2 = strlen(quick_ip_to_string(ip2));
   char* str_ip1 = (char*) malloc_or_die(str_len_ip1);
   char* str_ip2 = (char*) malloc_or_die(str_len_ip2);
   strcpy(str_ip1, quick_ip_to_string(ip1));
   strcpy(str_ip2, quick_ip_to_string(ip2));
   unsigned uint_ip1[4];
   unsigned uint_ip2[4];
   sscanf(str_ip1, "%u.%u.%u.%u", &uint_ip1[0], &uint_ip1[1], &uint_ip1[2], &uint_ip1[3]);
   sscanf(str_ip2, "%u.%u.%u.%u", &uint_ip2[0], &uint_ip2[1], &uint_ip2[2], &uint_ip2[3]);
   if(uint_ip1[0] == uint_ip2[0] && uint_ip1[1] == uint_ip2[1] && uint_ip1[2] == uint_ip2[2] && uint_ip1[3] == uint_ip2[3])
      return 0;
   int i =0;
   for( i = 0 ; i<4 ; i++) {
      if( uint_ip1[i] != uint_ip2[i])
         break;
   }
   if(uint_ip1[i] > uint_ip2[i])
      return 1;
   return -1; 
}
