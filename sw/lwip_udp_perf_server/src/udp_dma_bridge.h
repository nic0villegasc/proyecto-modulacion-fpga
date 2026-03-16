#ifndef UDP_DMA_BRIDGE_H
#define UDP_DMA_BRIDGE_H

#include "lwip/err.h"

// Initialize the UDP Server and the AXI DMA Engine
int setup_udp_dma_bridge(void);

#endif // UDP_DMA_BRIDGE_H