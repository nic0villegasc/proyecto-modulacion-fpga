#include "udp_dma_bridge.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "xil_printf.h"
#include "xil_cache.h"

#include "xinterrupt_wrap.h"

#include "xaxidma.h"
#include "xparameters.h"

#define UDP_LISTEN_PORT 9000

#define NUM_TX_BDS 64

// TODO: Update these based on your actual hardware configuration (xparameters.h)
#define TX_INTR_ID       XPAR_FABRIC_AXIDMA_0_MM2S_INTROUT_INTR
#define INTC_DEVICE_ID   XPAR_SCUGIC_0_DEVICE_ID

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
    int Status;
    
    XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AxiDma);

    Status = XSetupInterruptSystem(
        (void *)TxRingPtr,           // arg: Passed directly to dma_tx_interrupt_handler
        dma_tx_interrupt_handler,    // IntrHandler: Callback function
        TX_INTR_ID,                  // IntrId: The hardware IRQ number
        INTC_DEVICE_ID,              // IntrParent: The Interrupt Controller ID/Base
        0xA0                         // Priority: 0xA0 is a standard middle priority
    );

    if (Status != XST_SUCCESS) {
        xil_printf("Failed to route DMA TX Interrupt to GIC.\r\n");
        return -1;
    }

    XAxiDma_BdRingIntEnable(TxRingPtr, XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK); 

    xil_printf("DMA TX Interrupts routed and enabled!\r\n");
    return 0;
}

void dma_tx_interrupt_handler(void *CallbackRef) {
    XAxiDma_BdRing *TxRingPtr = (XAxiDma_BdRing *)CallbackRef;
    XAxiDma_Bd *BdSetPtr;
    XAxiDma_Bd *CurBd;
    struct pbuf *p;
    u32 IrqStatus;
    int NumBd;
    int i;

    IrqStatus = XAxiDma_BdRingGetIrq(TxRingPtr);
    XAxiDma_BdRingAckIrq(TxRingPtr, IrqStatus);

    if (IrqStatus & XAXIDMA_IRQ_IOC_MASK) {
        
        NumBd = XAxiDma_BdRingFromHw(TxRingPtr, XAXIDMA_ALL_BDS, &BdSetPtr);
        
        if (NumBd > 0) {
            CurBd = BdSetPtr;
            
            for (i = 0; i < NumBd; i++) {
                
                p = (struct pbuf *)XAxiDma_BdGetId(CurBd);
                
                if (p != NULL) {
                    pbuf_free(p);
                }

                CurBd = XAxiDma_BdRingNext(TxRingPtr, CurBd);
            }

            XAxiDma_BdRingFree(TxRingPtr, NumBd, BdSetPtr);
        }
    }

    if (IrqStatus & XAXIDMA_IRQ_ERROR_MASK) {
        xil_printf("DMA TX Error! Hardware halted.\r\n");
        // In a production system, you would trigger a DMA reset here.
    }
}

static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AxiDma); // Get our TX ring
    XAxiDma_Bd *BdSetPtr;
    XAxiDma_Bd *CurBdPtr;
    struct pbuf *q;
    int NumBd = 0;
    int Status;
    u32 CrBits;

    if (p == NULL) return;

    for (q = p; q != NULL; q = q->next) {
        NumBd++;
    }

    Status = XAxiDma_BdRingAlloc(TxRingPtr, NumBd, &BdSetPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to allocate %d BDs. Dropping packet.\r\n", NumBd);
        pbuf_free(p);
        return;
    }

    CurBdPtr = BdSetPtr;

    for (q = p; q != NULL; q = q->next) {

        Xil_DCacheFlushRange((UINTPTR)q->payload, q->len);

        XAxiDma_BdSetBufAddr(CurBdPtr, (UINTPTR)q->payload);
        XAxiDma_BdSetLength(CurBdPtr, q->len, TxRingPtr->MaxTransferLen); 

        // Set Control Bits (SOF / EOF)
        CrBits = 0;
        if (q == p) {
            CrBits |= XAXIDMA_BD_CTRL_TXSOF_MASK;
        }
        if (q->next == NULL) {
            CrBits |= XAXIDMA_BD_CTRL_TXEOF_MASK;
        }
        XAxiDma_BdSetCtrl(CurBdPtr, CrBits);

        if (q == p) {
            XAxiDma_BdSetId(CurBdPtr, (UINTPTR)p);
        } else {
            XAxiDma_BdSetId(CurBdPtr, (UINTPTR)NULL);
        }

        CurBdPtr = XAxiDma_BdRingNext(TxRingPtr, CurBdPtr);
    }

    Status = XAxiDma_BdRingToHw(TxRingPtr, NumBd, BdSetPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to commit BDs to HW. Dropping packet.\r\n");
        XAxiDma_BdRingUnAlloc(TxRingPtr, NumBd, BdSetPtr); 
        pbuf_free(p);
        return;
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