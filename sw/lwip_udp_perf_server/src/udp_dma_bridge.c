#include "udp_dma_bridge.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "xil_printf.h"
#include "xil_cache.h"

#include "xaxidma.h"
#include "xparameters.h"

#define UDP_LISTEN_PORT 9000

#define NUM_TX_BDS 64

static XAxiDma AxiDma;
static u8 TxBdSpace[NUM_TX_BDS * sizeof(XAxiDma_Bd)] __attribute__((aligned(XAXIDMA_BD_MINIMUM_ALIGNMENT)));

static int init_axi_dma(void) {
    XAxiDma_Config *Config;
    XAxiDma_BdRing *TxRingPtr;
    XAxiDma_Bd BdTemplate;
    int Status;

    Config = XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID); 
    if (!Config) {
        xil_printf("No DMA configuration found.\r\n");
        return -1;
    }

    Status = XAxiDma_CfgInitialize(&AxiDma, Config);
    if (Status != XST_SUCCESS) {
        xil_printf("DMA Initialization failed.\r\n");
        return -1;
    }

    if (!XAxiDma_HasSg(&AxiDma)) {
        xil_printf("Error: Device configured as Simple mode. SG is required!\r\n");
        return -1;
    }

    TxRingPtr = XAxiDma_GetTxRing(&AxiDma);

    Status = XAxiDma_BdRingCreate(TxRingPtr, (UINTPTR)TxBdSpace, (UINTPTR)TxBdSpace, XAXIDMA_BD_MINIMUM_ALIGNMENT, NUM_TX_BDS);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to create TX BD ring.\r\n");
        return -1;
    }

    XAxiDma_BdClear(&BdTemplate);
    Status = XAxiDma_BdRingClone(TxRingPtr, &BdTemplate);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to clone BD template.\r\n");
        return -1;
    }

    Status = XAxiDma_BdRingStart(TxRingPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to start TX ring.\r\n");
        return -1;
    }

    xil_printf("AXI DMA SG TX Ring Initialized!\r\n");
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

static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    if (p != NULL) {
        // TODO: Map pbuf->payload to AXI DMA Scatter-Gather BD Ring here.
        // TODO: Call Xil_DCacheFlushRange() on the payload.
        // TODO: Tell DMA to start transferring.
        
        // TEMPORARY: Freeing pbuf to prevent memory leak during network-only testing.
        // Once DMA is implemented, REMOVE THIS! The DMA TX Interrupt will free it.
        pbuf_free(p); 
    }
}

int setup_udp_dma_bridge(void)
{
    struct udp_pcb *pcb;
    err_t err;

    xil_printf("Initializing UDP to DMA Bridge...\r\n");

    if (init_axi_dma() != 0) return -1;
    if (setup_dma_interrupts() != 0) return -1;

    pcb = udp_new();
    if (!pcb) {
        xil_printf("Error creating PCB.\r\n");
        return -1;
    }

    err = udp_bind(pcb, IP_ANY_TYPE, UDP_LISTEN_PORT);
    if (err != ERR_OK) {
        xil_printf("Unable to bind to port %d: err = %d\r\n", UDP_LISTEN_PORT, err);
        return -2;
    }

    udp_recv(pcb, udp_receive_callback, NULL);
    
    xil_printf("Bridge Initialized! UDP listening on port %d\r\n", UDP_LISTEN_PORT);
    return 0;
}