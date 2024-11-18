# Simulate release message responses

responseList = ['']

import socket
from functools import reduce
import operator

# Configuration
UDP_IP = ''
UDP_PORT = 59647
BUFFER_SIZE = 1024  # Max bytes to receive

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(1.0)

# Create a UDP socket
socktx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#socktx.bind(('127.0.0.1', 1457))

## Ex: b'$RSRLB,3,333*5D\r\n'

print(f"UDP server listening on {UDP_IP}:{UDP_PORT}")

def checksum(nmea_str):
    return reduce(operator.xor, map(ord, nmea_str),0)
        
def parseNMEA(nmea_str):

    # Check for starting '$'
    if (nmea_str[0] != '$'):
        raise ValueError
    
    # Remove '$' and Split off *
    split_msg = nmea_str[1:].split('*')

    if (len(split_msg) != 2):
        raise ValueError

    data_str = split_msg[0]
  
    cs_str = split_msg[-1]
    cs_str = cs_str.rstrip()
    
    cs_msg = reduce(operator.xor, map(ord, data_str),0)

    cs_calc = int(cs_str,16)

    #print('Check Sum: %02x == %02x' % (cs_msg,cs_calc))
    
    # Did the CS pass?
    if (cs_msg != cs_calc):
        raise ValueError
    else:
        # Split on ,
        splitStr = data_str.split(',')
        return splitStr
        
if __name__ == '__main__':

    try:
        while True:

                try:
                    # Receive message
                    data, addr = sock.recvfrom(BUFFER_SIZE)
                    message = data.decode('utf-8')  # Decode message to string
                    print(f"Received message: '{message}' from {addr}")

                    # Parse the message (for demonstration, assume it's a simple string)
                    #response = f"Echo: {message}"
                    msg = "RSRLA,2,-1"
                    response = "$" + msg + f"*{(checksum(msg)):02X}"

                    # Send a reply
                    socktx.sendto(response.encode('utf-8'), ('127.0.0.1', 2947))
                    print(f"Sent response: '{response}' to {('127.0.0.1', 2947)}")

                except socket.timeout:
                    continue

    except KeyboardInterrupt:
        print("\nServer shutting down.")
    except Exception as e:
        print(f"An error occurred: {e}")
