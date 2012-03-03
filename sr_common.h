/*
 * Filename: sr_common.h
 * Purpose: include common code and define common basic types for the router
 */

#ifndef SR_COMMON_H
#define SR_COMMON_H

#include <time.h>
#include <sys/time.h>
#ifdef _LINUX_
#include <stdint.h> /* uintX_t */
#endif

/** number of seconds to give helper threads to shutdown */
#define SHUTDOWN_TIME 5

/** length in bytes of an IPv4 address */
#define IP_ADDR_LEN 4

/** length in bytes of an Ethernet (MAC) address */
#define ETH_ADDR_LEN 6

/* 4 octets of 3 chars each, 3 periods, 1 nul => 16 chars */
#define STRLEN_IP  16

/* 12 hex chars, 5 colons, 1 nul => 18 bytes */
#define STRLEN_MAC 18

/* add '/' and two numbers at most to a typical IP */
#define STRLEN_SUBNET (STRLEN_IP+3)

/** length of a time string */
#define STRLEN_TIME 26
#define STRLEN_COST 11

/** byte is an 8-bit unsigned integer */
typedef uint8_t byte;

/** boolean is stored in an 8-bit unsigned integer */
typedef uint8_t bool;
#define TRUE 1
#define FALSE 0

/** IP address type */
typedef uint32_t addr_ip_t;

/** MAC address type */
typedef struct {
    byte octet[ETH_ADDR_LEN];
} __attribute__ ((packed)) addr_mac_t;

/* define endianness constants */
#ifndef __LITTLE_ENDIAN
#  define __LITTLE_ENDIAN 1
#endif
#ifndef __BIG_ENDIAN
#  define __BIG_ENDIAN 2
#endif

/* determine the appropriate byte order */
#ifndef __BYTE_ORDER
#  ifdef _CYGWIN_
#    define __BYTE_ORDER __LITTLE_ENDIAN
#  elif _LINUX_
#    define __BYTE_ORDER __LITTLE_ENDIAN
#  elif _SOLARIS_
#    define __BYTE_ORDER __BIG_ENDIAN
#  elif _DARWIN_
#    define __BYTE_ORDER __BIG_ENDIAN
#  else
#    error "unknown endian!"
#  endif
#endif


/** Uses the string to make an IP address */
addr_ip_t make_ip_addr( char* str );

/** Uses the specified bytes to create a MAC address */
addr_mac_t make_mac_addr(byte byte5, byte byte4, byte byte3, byte byte2, byte byte1, byte byte0);

/** Uses the specified ptr to create a MAC address */
addr_mac_t* make_mac_addr_ptr(uint8_t* mac);

/** Returns TRUE if t1 is later than t2. */
bool is_later( struct timeval* t1, struct timeval* t2 );

/*Function to compare two MAC addresses for equivalence*/
int equal_mac(addr_mac_t mac1, addr_mac_t mac2);

/*Function to compare ip addresses*/
int compare_ip(addr_ip_t ip1, addr_ip_t ip2); 
#endif /* SR_COMMON_H */
