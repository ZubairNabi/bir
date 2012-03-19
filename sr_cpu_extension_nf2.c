/*-----------------------------------------------------------------------------
 * file:  sr_cpu_extension_nf2.c
 * date:  Mon Feb 09 16:58:30 PST 2004
 * Author: Martin Casado
 *
 * 2007-Apr-04 04:57:55 AM - Modified to support NetFPGA v2.1 /mc
 *
 * Description:
 *
 *---------------------------------------------------------------------------*/

#include "sr_cpu_extension_nf2.h"

#include "sr_base_internal.h"

#include "sr_vns.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <linux/sockios.h>
#include <netinet/in.h>

#include "sr_router.h"
#include "sr_interface.h"
#include "sr_integration.h"
#include "sr_ethernet.h"
#include "sr_dumper.h"
#include "real_socket_helper.h"

struct sr_ethernet_hdr
{
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
#endif
    uint8_t  ether_dhost[ETHER_ADDR_LEN];    /* destination ethernet address */
    uint8_t  ether_shost[ETHER_ADDR_LEN];    /* source ethernet address */
    uint16_t ether_type;                     /* packet type ID */
} __attribute__ ((packed)) ;

static char*    copy_next_field(FILE* fp, char*  line, char* buf);
static uint32_t asci_to_nboip(const char* ip);
static void     asci_to_ether(const char* addr, uint8_t mac[6]);


/*-----------------------------------------------------------------------------
 * Method: sr_cpu_init_hardware(..)
 * scope: global
 *
 * Read information for each of the router's interfaces from hwfile
 *
 * format of the file is (1 interface per line)
 *
 * <name ip mask hwaddr>
 *
 * e.g.
 *
 * eth0 192.168.123.10 255.255.255.0 ca:fe:de:ad:be:ef
 *
 *---------------------------------------------------------------------------*/


int sr_cpu_init_hardware(struct sr_instance* sr, const char* hwfile)
{
    struct sr_vns_if vns_if;
    FILE* fp = 0;
    char line[1024];
    char buf[SR_NAMELEN];
    char *tmpptr;


    if ( (fp = fopen(hwfile, "r") ) == 0 )
    {
        fprintf(stderr, "Error: could not open cpu hardware info file: %s\n",
                hwfile);
        return -1;
    }

    Debug(" < -- Reading hw info from file %s -- >\n", hwfile);
    while ( fgets( line, 1024, fp) )
    {
        line[1023] = 0; /* -- insurance :) -- */

        /* -- read interface name into buf -- */
        if(! (tmpptr = copy_next_field(fp, line, buf)) )
        {
            fclose(fp);
            fprintf(stderr, "Bad formatting in cpu hardware file\n");
            return 1;
        }
        Debug(" - Name [%s] ", buf);
        strncpy(vns_if.name, buf, SR_NAMELEN);
        /* -- read interface ip into buf -- */
        if(! (tmpptr = copy_next_field(fp, tmpptr, buf)) )
        {
            fclose(fp);
            fprintf(stderr, "Bad formatting in cpu hardware file\n");
            return 1;
        }
        Debug(" IP [%s] ", buf);
        vns_if.ip = asci_to_nboip(buf);

        /* -- read interface mask into buf -- */
        if(! (tmpptr = copy_next_field(fp, tmpptr, buf)) )
        {
            fclose(fp);
            fprintf(stderr, "Bad formatting in cpu hardware file\n");
            return 1;
        }
        Debug(" Mask [%s] ", buf);
        vns_if.mask = asci_to_nboip(buf);

        /* -- read interface hw address into buf -- */
        if(! (tmpptr = copy_next_field(fp, tmpptr, buf)) )
        {
            fclose(fp);
            fprintf(stderr, "Bad formatting in cpu hardware file\n");
            return 1;
        }
        Debug(" MAC [%s]\n", buf);
        asci_to_ether(buf, vns_if.addr);

        sr_integ_add_interface(sr, &vns_if);

    } /* -- while ( fgets ( .. ) ) -- */
    Debug(" < --                         -- >\n");

    fclose(fp);
    return 0;

} /* -- sr_cpu_init_hardware -- */

/*-----------------------------------------------------------------------------
 * Method: sr_cpu_input(..)
 * Scope: Local
 *
 *---------------------------------------------------------------------------*/

int sr_cpu_input(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    /*
     * TODO: Read packet from the hardware and pass to sr_integ_input(..)
     *       e.g.
     *
     *  sr_integ_input(sr,
     *          packet,   * lent *
     *          len,
     *          "eth2" ); * lent *
     */

    /*
     * Note: To log incoming packets, use sr_log_packet from sr_dumper.[c,h]
     */

    /* RETURN 1 on success, 0 on failure.
     * Note: With a 0 result, the router will shut-down
     */
    printf(" ** sr_cpu_input(..) called\n");
#ifdef _CPUMODE_
    byte buf[ETH_MAX_LEN];
    unsigned int len;
    sr_router* router;
    interface_t* intf;
    fd_set rdset, errset;
    int ret, max_fd, i;
    struct timeval timeout;

    do {
        FD_ZERO( &rdset );
        FD_ZERO( &errset );

        max_fd = -1;
        router = sr->interface_subsystem;
        for( i = 0; i<router->num_interfaces; i++ ) {
            if( router->interface[i].enabled ) {
                FD_SET( router->interface[i].hw_fd, &rdset );
                FD_SET( router->interface[i].hw_fd, &errset );

                if( router->interface[i].hw_fd > max_fd )
                    max_fd = router->interface[i].hw_fd;
            }
        }

        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;
        ret = select( max_fd + 1, &rdset, NULL, &errset, &timeout );

        for( i = 0; i<router->num_interfaces; i++ ) {
            intf = &router->interface[i];
            if( intf->enabled ) {
                if( FD_ISSET( intf->hw_fd, &rdset ) ) {
                    len = real_read_once( intf->hw_fd, buf, ETH_MAX_LEN );
                    if( len <= 0 )
                        printf( " ** sr_cpu_input(..) length error on interface %s",
                                       intf->name );
                    else {

                        sr_integ_input( sr, buf, len, intf->name );
                        sr_log_packet( sr, buf, len );

                        return 1;
                    }
                }

                if( FD_ISSET( intf->hw_fd, &errset ) ) {
                    printf(" ** sr_cpu_input(..) error %s\n",
                                   intf->name );
                    return 0;
                }
            }
        }
    }
    while( 1 );
#endif
    return 1;
} /* -- sr_cpu_input -- */

/*-----------------------------------------------------------------------------
 * Method: sr_cpu_output(..)
 * Scope: Global
 *
 *---------------------------------------------------------------------------*/

int sr_cpu_output(struct sr_instance* sr /* borrowed */,
                       uint8_t* buf /* borrowed */ ,
                       unsigned int len,
                       const char* iface /* borrowed */)
{
    printf(" ** sr_cpu_output(..) called\n");
    /* REQUIRES */
    assert(sr);
    assert(buf);
    assert(iface);
#ifdef _CPUMODE_
    struct sr_instance* sr_inst = get_sr();
    struct sr_router* router = (struct sr_router*)sr_get_subsystem(sr_inst);
    // get interface instance
    interface_t* intf = get_interface_name(router, iface);
    pthread_mutex_lock(&intf->hw_lock);
    int ret = writen(intf->hw_fd, buf, len);
    pthread_mutex_unlock(&intf->hw_lock); 
    /* Return the length of the packet on success, -1 on failure */
    return ret;
#endif
    return 0;
} /* -- sr_cpu_output -- */


/*-----------------------------------------------------------------------------
 * Method: copy_next_field(..)
 * Scope: Local
 *
 *---------------------------------------------------------------------------*/

static
char* copy_next_field(FILE* fp, char* line, char* buf)
{
    char* tmpptr = buf;
    while ( *line  && isspace((int)*line)) /* -- XXX: potential overrun here */
    { line++; }
    if(! *line )
    { return 0; }
    while ( *line && ! isspace((int)*line) && ((tmpptr - buf) < SR_NAMELEN))
    { *tmpptr++ = *line++; }
    *tmpptr = 0;
    return line;
} /* -- copy_next_field -- */

/*-----------------------------------------------------------------------------
 * Method: asci_to_nboip(..)
 * Scope: Local
 *
 *---------------------------------------------------------------------------*/

static uint32_t asci_to_nboip(const char* ip)
{
    struct in_addr addr;

    if ( inet_pton(AF_INET, ip, &addr) <= 0 )
    { return 0; } /* -- 0.0.0.0 unsupported so its ok .. yeah .. really -- */

    return addr.s_addr;
} /* -- asci_to_nboip -- */

/*-----------------------------------------------------------------------------
 * Method: asci_to_ether(..)
 * Scope: Local
 *
 * Look away .. please ... just look away
 *
 *---------------------------------------------------------------------------*/

static void asci_to_ether(const char* addr, uint8_t mac[6])
{
    uint32_t tmpint;
    const char* buf = addr;
    int i = 0;
    for( i = 0; i < 6; ++i )
    {
        if (i)
        {
            while (*buf && *buf != ':')
            { buf++; }
            buf++;
        }
        sscanf(buf, "%x", &tmpint);
        mac[i] = tmpint & 0x000000ff;
    }
} /* -- asci_to_ether -- */

void sr_cpu_init_interface_socket(sr_router* router) {
   printf(" ** sr_cpu_init_interface(..) called\n");
   char iface_name[32] = "nf2c";
   int i;
   for (i = 0; i < ROUTER_MAX_INTERFACES; ++i) {
	sprintf(&(iface_name[4]), "%i", i);
	int s = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	struct ifreq ifr;
	bzero(&ifr, sizeof(struct ifreq));
	strncpy(ifr.ifr_ifrn.ifrn_name, iface_name, IFNAMSIZ);
	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
	        perror("ioctl SIOCGIFINDEX");
	        exit(1);
	}

	struct sockaddr_ll saddr;
	bzero(&saddr, sizeof(struct sockaddr_ll));
	saddr.sll_family = AF_PACKET;
	saddr.sll_protocol = htons(ETH_P_ALL);
	saddr.sll_ifindex = ifr.ifr_ifru.ifru_ivalue;

	if (bind(s, (struct sockaddr*)(&saddr), sizeof(saddr)) < 0) {
        	perror("bind error");
	        exit(1);
	}

        //add fd to interface hw_fd
#ifdef _CPUMODE_
        router->interface[i].hw_fd = s;
#endif
   }
}
