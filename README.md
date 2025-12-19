# ZedBoard Implementation: Ethernet Comm & Modulation
This repository contains the source code and block designs for the Xilinx Zynq-7000 (ZedBoard) implementation used in the modulation/demodulation testbench. The design leverages the Zynq's Processing System (PS) for high-speed Ethernet communication and the Programmable Logic (PL) for signal processing.
## Hardware Platform
* **Board:** Digilent ZedBoard 
* **SoC:** Xilinx Zynq-7000 (XC7Z020)
* **PS:** Dual-core ARM Cortex-A9
* **PL:** Artix-7 FPGA fabric (85k Logic Cells) 

## Software Architecture (PS)

The Processing System runs a bare-metal application using the **lwIP (Lightweight IP)** networking stack to handle data transfer between the FPGA and the host PC without the overhead of a full operating system.

### Network Configuration
* **Stack:** lwIP (Lightweight IP)
* **Protocol:** TCP/IP (Layer 4)
* **Port:** 7
* **Addressing:** Static IP (default)
* **IP Address:** `192.168.1.10`

*Note: DHCP is supported if enabled, but static is preferred for the test setup.* 

### Functionality

The C application running on the Cortex-A9 establishes a reliable, error-checked data link. It acts as a bridge, taking demodulated data from the PL fabric and transmitting it to the PC for analysis.

**Interfaces:**
* **Host Link:** AXI Interconnect (PS <-> PL) for data streaming to the Ethernet controller.

## Getting Started

1. **Hardware Setup:** Connect the ZedBoard via Ethernet to the host PC. Ensure the host network adapter is in the same subnet (e.g., `192.168.1.x`).
2. **Flash Bitstream:** Load the relevant bitstream (TX or RX) via Vivado Hardware Manager.
3. **Launch Software:** Run the lwIP echo/transfer application on the Cortex-A9 (via Vitis/SDK).
4. **Connect:** Open a TCP connection to `192.168.1.10` on Port 7.

## Tools Used

* **Vivado:** For hardware block design and bitstream generation.
* **Vitis/Xilinx SDK:** For lwIP library configuration and C code development.
