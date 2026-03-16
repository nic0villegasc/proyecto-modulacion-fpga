#include "udp_dma_bridge.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "xil_printf.h"
#include "xil_cache.h"
// #include "xaxidma.h" // Uncomment when adding DMA code
// #include "xscugic.h" // Uncomment when adding Interrupt code

#define UDP_LISTEN_PORT 9000

/* * -----------------------------------------------------------------------------
 * HARDWARE STUBS (We will fill these in next)
 * -----------------------------------------------------------------------------
 */
static int init_axi_dma(void) {
    // TODO: Initialize AXI DMA, setup Scatter-Gather BD Ring
    return 0; 
}

static int setup_dma_interrupts(void) {
    // TODO: Connect DMA TX Complete interrupt to the ARM GIC
    return 0;
}

void dma_tx_interrupt_handler(void *CallbackRef) {
    // TODO: Acknowledge interrupt
    // TODO: Check which BDs finished
    // TODO: Call pbuf_free() on the completed payloads!
}

/* * -----------------------------------------------------------------------------
 * UDP RECEIVE CALLBACK (The Bridge to DMA)
 * -----------------------------------------------------------------------------
 */
static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    xil_printf("Received UDP! %s\n\r", p->payload);
    if (p != NULL) {
        // TODO: Map pbuf->payload to AXI DMA Scatter-Gather BD Ring here.
        // TODO: Call Xil_DCacheFlushRange() on the payload.
        // TODO: Tell DMA to start transferring.
        
        // TEMPORARY: Freeing pbuf to prevent memory leak during network-only testing.
        // Once DMA is implemented, REMOVE THIS! The DMA TX Interrupt will free it.
        pbuf_free(p); 
    }
}

/* * -----------------------------------------------------------------------------
 * BRIDGE SETUP
 * -----------------------------------------------------------------------------
 */
int setup_udp_dma_bridge(void)
{
    struct udp_pcb *pcb;
    err_t err;

    xil_printf("Initializing UDP to DMA Bridge...\r\n");

    // 1. Initialize Hardware (DMA & Interrupts)
    if (init_axi_dma() != 0) return -1;
    if (setup_dma_interrupts() != 0) return -1;

    // 2. Create UDP Protocol Control Block
    pcb = udp_new();
    if (!pcb) {
        xil_printf("Error creating PCB.\r\n");
        return -1;
    }

    // 3. Bind to the port
    err = udp_bind(pcb, IP_ANY_TYPE, UDP_LISTEN_PORT);
    if (err != ERR_OK) {
        xil_printf("Unable to bind to port %d: err = %d\r\n", UDP_LISTEN_PORT, err);
        return -2;
    }

    // 4. Set the receive callback function
    udp_recv(pcb, udp_receive_callback, NULL);
    
    xil_printf("Bridge Initialized! UDP listening on port %d\r\n", UDP_LISTEN_PORT);
    return 0;
}