#include "udp_dma_bridge.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/inet.h"
#include "xil_printf.h"
#include "xil_cache.h"

#include "netif/xpqueue.h"

#include "xinterrupt_wrap.h"

#include "xaxidma.h"
#include "xparameters.h"

#define UDP_LISTEN_PORT 9000
#define MAX_PKT_LEN 1460

#define NUM_TX_BDS 1024
#define NUM_RX_BDS 1024

#define MM2S_INTR_ID       XPAR_FABRIC_AXI_DMA_0_INTR
#define S2MM_INTR_ID       XPAR_FABRIC_AXI_DMA_0_INTR_1
#define INTC_DEVICE_ID   XPAR_XSCUGIC_0_BASEADDR

static XAxiDma AxiDma;
static u8 TxBdSpace[NUM_TX_BDS * sizeof(XAxiDma_Bd)] __attribute__((aligned(XAXIDMA_BD_MINIMUM_ALIGNMENT)));
static u8 RxBdSpace[NUM_RX_BDS * sizeof(XAxiDma_Bd)] __attribute__((aligned(XAXIDMA_BD_MINIMUM_ALIGNMENT)));

static struct udp_pcb *global_udp_pcb = NULL;

static pq_queue_t *dma_rx_queue = NULL;

static int init_axi_dma_mm2s(void) {
    XAxiDma_Config *Config;
    XAxiDma_BdRing *TxRingPtr;
    XAxiDma_Bd BdTemplate;
    int Status;

    Config = XAxiDma_LookupConfig(XPAR_AXI_DMA_0_BASEADDR); 
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

static int init_axi_dma_s2mm(void) {
    XAxiDma_BdRing *RxRingPtr = XAxiDma_GetRxRing(&AxiDma); // Get the RX Ring
    XAxiDma_Bd BdTemplate;
    XAxiDma_Bd *BdSetPtr;
    XAxiDma_Bd *CurBdPtr;
    struct pbuf *p;
    int Status;
    int FreeBds;
    int i;

    XAxiDma_BdRingIntDisable(RxRingPtr, XAXIDMA_IRQ_ALL_MASK);
    
    Status = XAxiDma_BdRingCreate(RxRingPtr, (UINTPTR)RxBdSpace, (UINTPTR)RxBdSpace, 
                                  XAXIDMA_BD_MINIMUM_ALIGNMENT, NUM_RX_BDS);
    if (Status != XST_SUCCESS) return -1;

    XAxiDma_BdClear(&BdTemplate);
    Status = XAxiDma_BdRingClone(RxRingPtr, &BdTemplate);
    if (Status != XST_SUCCESS) return -1;
    
    FreeBds = XAxiDma_BdRingGetFreeCnt(RxRingPtr);

    Status = XAxiDma_BdRingAlloc(RxRingPtr, FreeBds, &BdSetPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to allocate RX BDs.\r\n");
        return -1;
    }

    CurBdPtr = BdSetPtr;

    for (i = 0; i < FreeBds; i++) {
        
        p = pbuf_alloc(PBUF_TRANSPORT, MAX_PKT_LEN, PBUF_POOL);
        if (!p) {
            xil_printf("lwIP out of memory during RX init!\r\n");
            // In a robust system, you'd un-allocate the BDs and fail gracefully here
            return -1;
        }

        Xil_DCacheFlushRange((UINTPTR)p->payload, p->len);
        XAxiDma_BdSetBufAddr(CurBdPtr, (UINTPTR)p->payload);
        XAxiDma_BdSetLength(CurBdPtr, p->len, 0x03FFFFFF);
        XAxiDma_BdSetCtrl(CurBdPtr, 0);
        XAxiDma_BdSetId(CurBdPtr, (UINTPTR)p);

        CurBdPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(RxRingPtr, CurBdPtr);
    }

    Status = XAxiDma_BdRingToHw(RxRingPtr, FreeBds, BdSetPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to hand RX BDs to hardware.\r\n");
        return -1;
    }

    Status = XAxiDma_BdRingStart(RxRingPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to start RX ring.\r\n");
        return -1;
    }

    xil_printf("AXI DMA SG RX Ring Initialized and Armed!\r\n");
    return 0;
}

void mm2s_interrupt_handler(void *CallbackRef) {
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
                
                p = (struct pbuf *)(UINTPTR)XAxiDma_BdGetId(CurBd);
                
                if (p != NULL) {
                    pbuf_free(p);
                }
                
                CurBd = (XAxiDma_Bd *)XAxiDma_BdRingNext(TxRingPtr, CurBd);
            }

            XAxiDma_BdRingFree(TxRingPtr, NumBd, BdSetPtr);
        }
    }

    if (IrqStatus & XAXIDMA_IRQ_ERROR_MASK) {
        // xil_printf("DMA TX Error! Hardware halted.\r\n");
        // In a production system, you would trigger a DMA reset here.
    }
}

void s2mm_interrupt_handler(void *CallbackRef) {
    XAxiDma_BdRing *RxRingPtr = (XAxiDma_BdRing *)CallbackRef;
    XAxiDma_Bd *BdSetPtr;
    XAxiDma_Bd *CurBdPtr;
    struct pbuf *p;
    struct pbuf *p_new;
    u32 IrqStatus;
    int Status;
    int NumBd;
    int i;
    u32 rx_len;

    IrqStatus = XAxiDma_BdRingGetIrq(RxRingPtr);
    XAxiDma_BdRingAckIrq(RxRingPtr, IrqStatus);

    if (IrqStatus & XAXIDMA_IRQ_IOC_MASK) {
        
        NumBd = XAxiDma_BdRingFromHw(RxRingPtr, XAXIDMA_ALL_BDS, &BdSetPtr);
        
        if (NumBd > 0) {
            CurBdPtr = BdSetPtr;
            
            for (i = 0; i < NumBd; i++) {
                
                p = (struct pbuf *)(UINTPTR)XAxiDma_BdGetId(CurBdPtr);
                
                if (p != NULL) {
                    p_new = pbuf_alloc(PBUF_TRANSPORT, MAX_PKT_LEN, PBUF_POOL);
                    
                    if (p_new != NULL) {
                        rx_len = XAxiDma_BdGetActualLength(CurBdPtr, 0x03FFFFFF);
                        Xil_DCacheInvalidateRange((UINTPTR)p->payload, rx_len);

                        p->len = rx_len;
                        p->tot_len = rx_len;
                        
                        if (pq_enqueue(dma_rx_queue, (void*)p) < 0) {
                            // Queue is full, drop the packet
                            pbuf_free(p);
                        }

                        Xil_DCacheFlushRange((UINTPTR)p_new->payload, p_new->len);
                        XAxiDma_BdSetBufAddr(CurBdPtr, (UINTPTR)p_new->payload);
                        XAxiDma_BdSetLength(CurBdPtr, p_new->len, 0x03FFFFFF);
                        XAxiDma_BdSetCtrl(CurBdPtr, 0); 
                        XAxiDma_BdSetId(CurBdPtr, (UINTPTR)p_new); 

                    } else {
                        Xil_DCacheFlushRange((UINTPTR)p->payload, MAX_PKT_LEN);
                        XAxiDma_BdSetBufAddr(CurBdPtr, (UINTPTR)p->payload);
                        XAxiDma_BdSetLength(CurBdPtr, MAX_PKT_LEN, 0x03FFFFFF); 
                        XAxiDma_BdSetCtrl(CurBdPtr, 0);
                    }
                }

                CurBdPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(RxRingPtr, CurBdPtr);
            }

            Status = XAxiDma_BdRingFree(RxRingPtr, NumBd, BdSetPtr);

            Status = XAxiDma_BdRingAlloc(RxRingPtr, NumBd, &BdSetPtr);
            Status = XAxiDma_BdRingToHw(RxRingPtr, NumBd, BdSetPtr);
        }
    }

    if (IrqStatus & XAXIDMA_IRQ_ERROR_MASK) {
        xil_printf("DMA RX Error!\r\n");
    }
}

static int setup_dma_interrupts(void) {
    int Status;
    
    XAxiDma_BdRing *TxRingPtr = XAxiDma_GetTxRing(&AxiDma);
    XAxiDma_BdRing *RxRingPtr = XAxiDma_GetRxRing(&AxiDma); // Get the RX ring

    // 1. Setup MM2S PBUF Freeing Interrupt
    Status = XSetupInterruptSystem(
        (void *)TxRingPtr,           
        mm2s_interrupt_handler,    
        MM2S_INTR_ID,                  
        INTC_DEVICE_ID,              
        0x90
    );
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to route DMA MM2S Interrupt to GIC.\r\n");
        return -1;
    }

    // 2. Setup S2MM Packet Reception and Sent Interrupt
    Status = XSetupInterruptSystem(
        (void *)RxRingPtr,           // Pass the RX ring pointer as the callback ref
        s2mm_interrupt_handler,    // The specific RX callback function
        S2MM_INTR_ID,                  // The hardware IRQ number for RX
        INTC_DEVICE_ID,
        0xA0
    );
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to route DMA S2MM Interrupt to GIC.\r\n");
        return -1;
    }

    // 3. Enable Interrupts on both DMA rings
    XAxiDma_BdRingIntEnable(TxRingPtr, XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK); 
    XAxiDma_BdRingIntEnable(RxRingPtr, XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK); 

    xil_printf("DMA MM2S and S2MM Interrupts routed and enabled!\r\n");
    return 0;
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
        XAxiDma_BdSetLength(CurBdPtr, q->len, 0x03FFFFFF);

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

        CurBdPtr = (XAxiDma_Bd *)XAxiDma_BdRingNext(TxRingPtr, CurBdPtr);
    }

    Status = XAxiDma_BdRingToHw(TxRingPtr, NumBd, BdSetPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to commit BDs to HW. Dropping packet.\r\n");
        XAxiDma_BdRingUnAlloc(TxRingPtr, NumBd, BdSetPtr); 
        pbuf_free(p);
        return;
    }
}

void process_dma_s2mm_queue(void) {
    struct pbuf *p;
    ip_addr_t dest_ip;

    if (dma_rx_queue == NULL) return;

    while (pq_qlength(dma_rx_queue) > 0) {
        p = (struct pbuf *)pq_dequeue(dma_rx_queue);

        if (p != NULL) {
            if (global_udp_pcb != NULL) {
                inet_aton("192.168.1.125", &dest_ip); 
                udp_sendto(global_udp_pcb, p, &dest_ip, 9001); 
            }

            pbuf_free(p);
        }
    }
}

int setup_udp_dma_bridge(void)
{
    err_t err;

    xil_printf("Initializing UDP to DMA Bridge...\r\n");

    dma_rx_queue = pq_create_queue();
    if (!dma_rx_queue) {
        xil_printf("Error creating DMA RX queue.\r\n");
        return -1;
    }

    if (init_axi_dma_mm2s() != 0) return -1;
    if (init_axi_dma_s2mm() != 0) return -1;
    if (setup_dma_interrupts() != 0) return -1;

    global_udp_pcb = udp_new();
    if (!global_udp_pcb) {
        xil_printf("Error creating PCB.\r\n");
        return -1;
    }

    err = udp_bind(global_udp_pcb, IP_ANY_TYPE, UDP_LISTEN_PORT);
    if (err != ERR_OK) {
        xil_printf("Unable to bind to port %d: err = %d\r\n", UDP_LISTEN_PORT, err);
        return -2;
    }

    udp_recv(global_udp_pcb, udp_receive_callback, NULL);
    
    xil_printf("Bridge Initialized! UDP listening on port %d\r\n", UDP_LISTEN_PORT);
    return 0;
}