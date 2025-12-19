# Code Generated with Gemini AI
# Example test script for LWIP Echo Server on FPGA
import socket

# Configuration matching your FPGA's main.c and echo.c
# Note: Ensure your computer's Ethernet adapter is in the 192.168.1.x range!
BOARD_IP = "192.168.1.10"  # From main.c
BOARD_PORT = 7             # From echo.c
BUFFER_SIZE = 1024

def send_message_to_fpga(message):
    print(f"--- Connecting to FPGA at {BOARD_IP}:{BOARD_PORT} ---")
    
    try:
        # 1. Create a TCP/IP socket
        # AF_INET = IPv4, SOCK_STREAM = TCP
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            
            # 2. Connect to the board
            s.settimeout(5) # Don't wait forever if board is off
            s.connect((BOARD_IP, BOARD_PORT))
            print("Connected!")

            # 3. Send the message
            # We must .encode() the string to bytes because TCP sends raw bytes
            print(f"Sending: '{message}'")
            s.sendall(message.encode('utf-8'))

            # 4. Receive the response
            data = s.recv(BUFFER_SIZE)
            
            # 5. Decode and print result
            response_str = data.decode('utf-8')
            print(f"Received: '{response_str.strip()}'")

    except socket.timeout:
        print("Error: Connection timed out. Is the board powered on?")
    except ConnectionRefusedError:
        print("Error: Connection refused. Is the server running on Port 7?")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    # Send a test message to the FPGA echo server
    send_message_to_fpga("Hello World!")