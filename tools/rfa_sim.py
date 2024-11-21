import socket

UDP_IP = '127.0.0.1'
UDP_PORT = 2947

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

msg = "$RSRFA,1688649984.75,41.5698,-71.4031,355.53,RSI,3,3,360.3,86.48,41.5700,-71.3988,8.2,15.5,100,*0C"
s.sendto(bytes(msg,'utf-8'), (UDP_IP, UDP_PORT))