/*
 * Cleaned up UDP-to-DMA Bridge main.c
 * Target: ZedBoard (Zynq-7000)
 */

#include <stdio.h>
#include "xparameters.h"
#include "netif/xadapter.h"
#include "platform.h"
#include "platform_config.h"
#include "lwipopts.h"
#include "xil_printf.h"
#include "sleep.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/init.h"
#include "lwip/inet.h"
#include "xil_cache.h"
#include "lwip/udp.h" // Added for UDP functions

#if LWIP_DHCP==1
#include "lwip/dhcp.h"
extern volatile int dhcp_timoutcntr;
#endif

// TCP timers are still required by lwIP for internal maintenance (like DHCP/ARP)
extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;

#define DEFAULT_IP_ADDRESS	"192.168.1.10"
#define DEFAULT_IP_MASK		"255.255.255.0"
#define DEFAULT_GW_ADDRESS	"192.168.1.1"
#define UDP_LISTEN_PORT     9000

struct netif server_netif;

static void print_ip(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\r\n", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

static void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
	print_ip("Board IP:       ", ip);
	print_ip("Netmask :       ", mask);
	print_ip("Gateway :       ", gw);
}

static void assign_default_ip(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
	int err;
	xil_printf("Configuring default IP %s \r\n", DEFAULT_IP_ADDRESS);

	err = inet_aton(DEFAULT_IP_ADDRESS, ip);
	if (!err) xil_printf("Invalid default IP address: %d\r\n", err);

	err = inet_aton(DEFAULT_IP_MASK, mask);
	if (!err) xil_printf("Invalid default IP MASK: %d\r\n", err);

	err = inet_aton(DEFAULT_GW_ADDRESS, gw);
	if (!err) xil_printf("Invalid default gateway address: %d\r\n", err);
}

/* * -----------------------------------------------------------------------------
 * UDP RECEIVE CALLBACK (The Bridge to DMA)
 * -----------------------------------------------------------------------------
 */
void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    if (p != NULL) {
        // TODO: Map pbuf->payload to AXI DMA Scatter-Gather BD Ring here.
        // TODO: Call Xil_DCacheFlushRange() on the payload to sync DDR RAM for the DMA.
        
        // CRITICAL RULE: DO NOT free the pbuf here in the final design!
        // The hardware DMA needs time to read it. You will free it in the DMA TX Interrupt.
        
        // For right now (testing phase), we must free it to prevent lwIP from running out of memory.
        pbuf_free(p); 
    }
}

/* * -----------------------------------------------------------------------------
 * UDP SERVER SETUP
 * -----------------------------------------------------------------------------
 */
int setup_udp_server()
{
    struct udp_pcb *pcb;
    err_t err;

    // 1. Create a new UDP Protocol Control Block
    pcb = udp_new();
    if (!pcb) {
        xil_printf("Error creating PCB.\r\n");
        return -1;
    }

    // 2. Bind to the port (IP_ANY_TYPE ensures it listens whether using DHCP or static IP)
    err = udp_bind(pcb, IP_ANY_TYPE, UDP_LISTEN_PORT);
    if (err != ERR_OK) {
        xil_printf("Unable to bind to port %d: err = %d\r\n", UDP_LISTEN_PORT, err);
        return -2;
    }

    // 3. Set the receive callback function
    udp_recv(pcb, udp_receive_callback, NULL);
    
    xil_printf("UDP server started! Listening on port %d\r\n", UDP_LISTEN_PORT);
    return 0;
}


int main(void)
{
	struct netif *netif;

	/* the mac address of the board. this should be unique per board */
	unsigned char mac_ethernet_address[] = { 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };

	netif = &server_netif;

	init_platform();

	xil_printf("\r\n\r\n");
	xil_printf("----- Ethernet to Manchester DMA Bridge -----\r\n");

	/* initialize lwIP */
	lwip_init();

	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(netif, NULL, NULL, NULL, mac_ethernet_address, PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\r\n");
		return -1;
	}
	netif_set_default(netif);

	/* specify that the network if is up */
	netif_set_up(netif);

#if (LWIP_DHCP==1)
	/* Create a new DHCP client for this interface. */
	dhcp_start(netif);
	dhcp_timoutcntr = 240;
	while (((netif->ip_addr.addr) == 0) && (dhcp_timoutcntr > 0)) {
		xemacif_input(netif);
    }

	if (dhcp_timoutcntr <= 0) {
		if ((netif->ip_addr.addr) == 0) {
			xil_printf("ERROR: DHCP request timed out. Falling back to static IP.\r\n");
			assign_default_ip(&(netif->ip_addr), &(netif->netmask), &(netif->gw));
		}
	}
#else
	assign_default_ip(&(netif->ip_addr), &(netif->netmask), &(netif->gw));
#endif

	print_ip_settings(&(netif->ip_addr), &(netif->netmask), &(netif->gw));
	xil_printf("\r\n");

	/* Initialize the UDP Server and Callback */
	setup_udp_server();

	/* Main Loop */
	while (1) {
		if (TcpFastTmrFlag) {
			tcp_fasttmr();
			TcpFastTmrFlag = 0;
		}
		if (TcpSlowTmrFlag) {
			tcp_slowtmr();
			TcpSlowTmrFlag = 0;
		}
		xemacif_input(netif); // Process incoming Ethernet MAC frames into lwIP
	}

	cleanup_platform();
	return 0;
}