import socket
import time
import os

# Configuration
BOARD_IP = "192.168.1.10"
BOARD_PORT = 7
PACKET_SIZE = 1460       # Standard TCP MSS (Max Segment Size)
TOTAL_DATA = 100 * 1024 * 1024 # 10 MB of data to test

def run_benchmark():
    print(f"--- Starting Speed Test: {TOTAL_DATA/1024/1024:.2f} MB ---")
    print(f"Target: {BOARD_IP}:{BOARD_PORT}")
    
    # Generate random data chunk to send
    # We use os.urandom to ensure the FPGA isn't compressing (though lwIP doesn't compress)
    payload = os.urandom(PACKET_SIZE)
    
    total_sent = 0
    total_received = 0
    
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((BOARD_IP, BOARD_PORT))
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1) # Disable Nagle algo for speed
            
            start_time = time.time()
            
            # Inside your loop in benchmark.py
            # Send 10 packets for every 1 packet we check for reception
            BATCH_SIZE = 10 

            while total_received < TOTAL_DATA:
                
                # Burst send
                for _ in range(BATCH_SIZE):
                    if total_sent < TOTAL_DATA:
                        s.send(payload)
                        total_sent += len(payload)
                        
                # Try to clean up the receive buffer
                try:
                    # Don't block; just grab what's there
                    s.setblocking(False) 
                    while True:
                        data = s.recv(4096)
                        total_received += len(data)
                except BlockingIOError:
                    pass # No more data to read right now, go back to sending
                finally:
                    s.setblocking(True)

            end_time = time.time()
            
            duration = end_time - start_time
            mbps = (total_received * 8) / (duration * 1_000_000) # Bits per second
            
            print(f"\n--- Results ---")
            print(f"Time Taken: {duration:.4f} seconds")
            print(f"Throughput: {mbps:.2f} Mbps")

    except Exception as e:
        print(f"Benchmark Failed: {e}")

if __name__ == "__main__":
    run_benchmark()