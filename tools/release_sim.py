# Simulate release message responses

responseList = ['']

import socket
from functools import reduce
import operator
import time

UDP_IP = ''
UDP_PORT = 59647
BUFFER_SIZE = 1024  # Max bytes to receive

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(1.0)

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
    
    split_msg = nmea_str[1:].split('*')

    if (len(split_msg) != 2):
        raise ValueError

    data_str = split_msg[0]
  
    cs_str = split_msg[-1]
    cs_str = cs_str.rstrip()
    
    cs_msg = reduce(operator.xor, map(ord, data_str),0)

    cs_calc = int(cs_str,16)

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
                data, addr = sock.recvfrom(BUFFER_SIZE)
                message = data.decode('utf-8')  
                print(f"Received message: '{message}' from {addr}")

                ss = parseNMEA(message)
                tid = ss[1]
                
                for i in range(1,4):

                    msg = f"RSRLA,{tid},{i}"
                    response = "$" + msg + f"*{(checksum(msg)):02X}"
                    time.sleep(3)
                    socktx.sendto(response.encode('utf-8'), ('127.0.0.1', 2947))
                    print(f"Sent response: '{response}' to {('127.0.0.1', 2947)}")

                msg = f"RSRLA,{tid},0"
                response = "$" + msg + f"*{(checksum(msg)):02X}"

                socktx.sendto(response.encode('utf-8'), ('127.0.0.1', 2947))
                print(f"Sent response: '{response}' to {('127.0.0.1', 2947)}")

            except socket.timeout:
                continue

    except KeyboardInterrupt:
        print("\nServer shutting down.")
    except Exception as e:
        print(f"An error occurred: {e}")
