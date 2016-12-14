import sys
import socket

from thread import *
import struct
import hashlib
import os
import urllib2
from collections import OrderedDict
import string
from cStringIO import StringIO


max_conn=20 #max Connection
buff_size=0xffff #max socket buffer size



def decode_chunked(data):
        unchunked = b''
        pos = 0
        while pos <= len(data):
                chunkNumLen = data.find(b'\r\n', pos)-pos
                print('Chunk length found between:',(pos, pos+chunkNumLen))
                chunkLen=int(data[pos:pos+chunkNumLen], 16)
                print('This is the chunk length:', chunkLen)
                if chunkLen == 0:
                    print('The length was 0, we have reached the end of all chunks')
                    break
                chunk = data[pos+chunkNumLen+len('\r\n'):pos+chunkNumLen+len('\r\n')+chunkLen]
                unchunked += chunk
                pos += chunkNumLen+len('\r\n')+chunkLen+len('\r\n')
        
        return unchunked

"""def decode_chunked(data):
    offset = 0
    encdata = ''
    newdata = ''
    offset = string.index(data, "\r\n\r\n") + 4 # get the offset 
    # of the data payload. you can also parse content-length header as well.
    encdata =data[offset:]
    try:
        while (encdata != ''):
            off = int(encdata[:string.index(encdata,"\r\n")],16)
            if off == 0:
                break
            encdata = encdata[string.index(encdata,"\r\n") + 2:]
            newdata = "%s%s" % (newdata, encdata[:off])
            encdata = encdata[off+2:]
                             
    except:
        line = traceback.format_exc()
        print "Exception! %s" %line # probably indexes are wrong
    return newdata
"""

def header_and_data(packet):
	header, _, data=packet.partition('\r\n\r\n')
	
	return header, data

def header_and_data2(packet):
    header=StringIO()
    content=StringIO()
    while True:
        line=packet.readline()
        if line in '\r\n\r\n':
                header.write(line)
                content.write(packet.read())
                packet.close()
                header.close()
                content.close()
                break
        header.write(line)

    
    print "=================data=========\n\n"
    
    
    return header.getvalue(), content.getvalue()

def read_headers(header):
    #headers={}
    headers=OrderedDict()
    lines=header.split("\r\n")
    for line in lines:
        k,_,v=line.partition(":")
        headers[k]=v
    return headers

def unite(headers, data):
    packet=StringIO()
    for key, value in headers.iteritems():
        packet.write(key)
        packet.write(':')
        packet.write(value)
        packet.write('\r\n')
    packet.write('\r\n')
    packet.write(data)
    return packet.getvalue()

def start():
    
    try:
        s=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(('127.0.0.1', 8080))
        s.listen(max_conn)
        print "=======Server started [8080]======\n"
    except Exception, e:
        print "Unable to initialize socket\n"
        print e
        sys.exit(2)

    while True:
        try:
            conn, addr = s.accept() #accept connection from client
            data=conn.recv(buff_size) #receive client data
            #data=recv_msg(conn)
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
        splitMessage=data.split()
        req_type=splitMessage[0]
        req_path=splitMessage[1]
        #req_path=req_path[1:]
        print "Request is", req_type,"to URL : ", req_path
        file_to_use="/"+req_path
        print file_to_use
        try:
            file=open(file_to_use[1:],"r")
            caching=file.readlines()
            print "File present in Cache\n"
            for i in range(0, len(data)):
                print (caching[i])
                conn.send(caching[i])
        except IOError:
            print "File does not exist in cache\n"
            header, content=header_and_data(data)
            headers=read_headers(header)
            #does not accept encoding such as gzip
            if 'Accept-Encoding' in headers:
                del headers['Accept-Encoding']
            #does not accept chunk
            content=content.replace('HTTP/1.1', 'HTTP/1.0')

            modified_data=unite(headers, content)
            proxy_server(webserver, port, conn, addr, modified_data,req_path)


       
        

                
                
                #modified_data=data.replace('gzip','')

                #modified_data=modified_data.replace('HTTP/1.1', 'HTTP/1.0')
                #HTTP/1.0 NOT TO GET 'CHUNKED ENCODING'
                #proxy_server(webserver, port, conn, addr, modified_data,filename)
    except Exception, e:
        print e
        print "\nBad connection\n"
        pass

def proxy_server(webserver, port, conn, addr, data,req_path):
    try:
        s=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((webserver, port))
        #send_msg(s, data)
        s.send(data)
        #while True:
        fileobj=s.makefile('r',0)
        fileobj.write('GET '+"http://" + req_path+"HTTP/1.0\n\n")
        buffer=fileobj.readlines()
        tmpFile=open(req_path.replace("/","."),"wb")
        for i in range(0, len(buffer)):
            tmpFile.write(buffer[i].replace(oldword,newword))
            conn.send(buffer[i].replace(oldword,newword))

        modified_data=reply.replace(oldword, newword)
            #reply=s.recv(buff_size)
            #reply=recv_msg(s)
            #header=read_headers(reply) 
            #if(len(reply)>0): #if receive packet from original target
             #   print "======reply content=====\n"
                #rfile=StringIO(reply)
              #  print reply
               # header, content=header_and_data(reply)
                #print "reply length : "
                #print len(reply)

                #header, _, content=reply.partition('\r\n\r\n')
                
                #result=header.replace(oldword, newword)
                
                #result=reply.replace(oldword, newword)
                #send_msg(conn, modified_data)
                #conn.send(modified_data) #send packet to the client
                #print "======Reply packet arrived======\n"
            #else:
             #   break
        s.close()
        conn.close()
    except socket.error, (value, message):
        s.close()
        conn.close()
        sys.exit(1)



print "=======Start program===========\n"
oldword=str(raw_input("Enter the word you want to change to another\n"))
newword=str(raw_input("Enter the word you want to replace with\n"))


start()
