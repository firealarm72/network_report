import socket, sys
from thread import *

try:
	print "=======Start program===========\n"
	listening_port=int(raw_input("Enter Listening port number\n"))
except KeyboardInterrupt:
	print "=======Exiting Application======\n"
	sys.exit()

max_conn=10 #max Connection
buff_size=4096 #max socket buffer size

def start():
	try:
		s=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.bind(('127.0.0.1', listening_port))
		s.listen(max_conn)
		print "=======Server started [%d]======\n" %(listening_port)
	except Exception, e:
		print "Unable to initialize socket\n"
		print e
		sys.exit(2)

	while True:
		try:
			conn, addr = s.accept() #accept connection from client
			data=conn.recv(buff_size) #receive client data
			start_new_thread(conn_string, (conn, data, addr))
		except KeyboardInterrupt:
			s.close()
			print "=====Proxy server Closing======\n"
			sys.exit(1)
	s.close()

def conn_string(conn, data, addr):
	try:
		first_line=data.split('\n')[0] #first line of the data
		url=first_line.split(' ')[1] #URL is the string after 'GET '
		http_pos=url.find("://")
		if(http_pos==-1):
			temp=url
		else:
			temp=url[(http_pos+3):] #get rest of url after http://
		port_pos=temp.find(":") #find the position of the port
		webserver_pos=temp.find("/")
		if webserver_pos==-1:
			webserver_pos=len(temp)
		webserver =""
		port_pos==-1
		if(port_pos!=-1 and webserver_pos>=port_pos):
			port=int((temp[(port_pos+1):])[:webserver_pos-port_pos-1])
			webserver=temp[:port_pos]
		else:
			port=80
			webserver=temp[:webserver_pos]
		print url +'\n'
		print webserver + '\n'
		print str(port) + '\n'
		proxy_server(webserver, port, conn, addr, data)
	except Exception, e:
		print e
		pass

def proxy_server(webserver, port, conn, addr, data):
	try:
		s=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.connect((webserver, port))
		s.send(data)
		while True:
			reply=s.recv(buff_size) 
			if(len(reply)>0): #if receive packet from original target
				conn.send(reply) #send packet to the client
				print "======Reply packet arrived======\n"
			else:
				break
		s.close()
		conn.close()
	except socket.error, (value, message):
		s.close()
		conn.close()
		sys.exit(1)

start()