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

// Include our new bridge module
#include "udp_dma_bridge.h" 

#if 0
#include "lwip/dhcp.h"
extern volatile int dhcp_timoutcntr;
#endif

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;

#define DEFAULT_IP_ADDRESS	"192.168.1.10"
#define DEFAULT_IP_MASK		"255.255.255.0"
#define DEFAULT_GW_ADDRESS	"192.168.1.1"

struct netif server_netif;

static void print_ip(char *msg, ip_addr_t *ip) {
	print(msg);
	xil_printf("%d.%d.%d.%d\r\n", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip));
}

static void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw) {
	print_ip("Board IP:       ", ip);
	print_ip("Netmask :       ", mask);
	print_ip("Gateway :       ", gw);
}

static void assign_default_ip(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw) {
	int err;
	xil_printf("Configuring default IP %s \r\n", DEFAULT_IP_ADDRESS);
	err = inet_aton(DEFAULT_IP_ADDRESS, ip);
	if (!err) xil_printf("Invalid default IP address: %d\r\n", err);
	err = inet_aton(DEFAULT_IP_MASK, mask);
	if (!err) xil_printf("Invalid default IP MASK: %d\r\n", err);
	err = inet_aton(DEFAULT_GW_ADDRESS, gw);
	if (!err) xil_printf("Invalid default gateway address: %d\r\n", err);
}

int main(void)
{
	struct netif *netif;
	unsigned char mac_ethernet_address[] = { 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };
	netif = &server_netif;

	init_platform();

	xil_printf("\r\n\r\n----- Ethernet to Manchester DMA Bridge -----\r\n");

	lwip_init();

	if (!xemac_add(netif, NULL, NULL, NULL, mac_ethernet_address, PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\r\n");
		return -1;
	}
	netif_set_default(netif);
	netif_set_up(netif);

#if (0)
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

	if (setup_udp_dma_bridge() != 0) {
        xil_printf("Failed to initialize UDP/DMA Bridge. Halting.\r\n");
        while(1);
    }

  while (1) {
		if (TcpFastTmrFlag) {
			tcp_fasttmr();
			TcpFastTmrFlag = 0;
      print_pbuf_pool_stats();
		}
		if (TcpSlowTmrFlag) {
			tcp_slowtmr();
			TcpSlowTmrFlag = 0;
		}
		xemacif_input(netif);
		process_dma_s2mm_queue();

    extern volatile int pbuf_starvation_flag;

    if (pbuf_starvation_flag) {
        xil_printf("\r\n[CRITICAL] pbuf_alloc failed in RX Interrupt!\r\n");
        print_pbuf_pool_stats();
        print_bridge_dma_stats();

        pbuf_starvation_flag = 0; // Clear the flag
    }
	}

	cleanup_platform();
	return 0;
}