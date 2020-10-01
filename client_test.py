#!/usr/bin/python3

# client_test.py - example of how to connect to sql_state_listener programmatically
# Benjamin Pritchard, October-1-2020
#
 
import socket    # used for TCP/IP communication 
 
TCP_IP 			= 'localhost'
TCP_PORT 		= 4242
BUFFER_SIZE		= 80
command 		= '?\n' 		# tell the engine to update the database...
 
# Open socket, send command, close socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
s.send(str.encode(command))
data = s.recv(1)
print(data)
s.close()

# (python makes life easy!)
 
