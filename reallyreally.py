import os,sys,thread,socket
from collections import OrderedDict
from cStringIO import StringIO
import md5
import csv

import logging
import urllib2
from httplib2 import FileCache
from jaraco.net.http.caching import CacheHandler

#********* CONSTANT VARIABLES *********
BACKLOG = 50            # how many pending connections queue will hold
MAX_DATA_RECV = 999999  # max number of bytes we receive at once
DEBUG = True            # set to True to see the debug msgs
#BLOCKED = []            # just an example. Remove with [""] for no blocking at all.


#**************************************
#********* MAIN PROGRAM ***************
#**************************************
def main():
    global oldword, newword
    global CACHEDICT
    CACHEDICT={}
    oldword=str(raw_input("Enter the word you want to change to another\n"))
    newword=str(raw_input("Enter the word you want to replace with\n"))
    # host and port info.
    host = ''               # blank for localhost
    port =8080
    try:
        csv_file=open("./CACHEDICT.csv", "r")
        csv_file.close()
    except:
        csv_file=open("./CACHEDICT.csv", "w")
        csv_file.close()
    print "Proxy Server Running on ",host,":",port

    try:
        # create a socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # associate the socket to host and port
        s.bind((host, port))

        # listenning
        s.listen(BACKLOG)
    
    except socket.error, (value, message):
        if s:
            s.close()
        print "Could not open socket:", message
        sys.exit(1)

    # get the connection from client
    while 1:
        conn, client_addr = s.accept()

        # create a thread to handle request
        thread.start_new_thread(proxy_thread, (conn, client_addr))
        
    s.close()
    
#************** END MAIN PROGRAM ***************

def printout(type,request,address):
    if "Block" in type or "Blacklist" in type:
        colornum = 91
    elif "Request" in type:
        colornum = 92
    elif "Reset" in type:
        colornum = 93

    print "\033[",colornum,"m",address[0],"\t",type,"\t",request,"\033[0m"

def header_and_data(packet):
    header, _, data=packet.partition('\r\n\r\n')
    
    return header, data
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
#*******************************************
#********* PROXY_THREAD FUNC ***************
# A thread to handle request from browser
#*******************************************
def proxy_thread(conn, client_addr):
    
    # get the request from browser
    request = conn.recv(MAX_DATA_RECV)

    # parse the first line
    first_line = request.split('\n')[0]

    # get url
    url = first_line.split(' ')[1]
    csv_file=open("./CACHEDICT.csv", "rb")
    reader=csv.reader(csv_file)
    for row in reader:
        CACHEDICT[row[0]]=row[1]
    csv_file.close()


    if url in CACHEDICT:
        #CACHE HIT
        print "File present in the cache!\n"
        cachename=CACHEDICT.get(url)
        try:
            file=open(cachename, "r")
            """
            caching=file.readlines()
            for i in range(0, len(caching)):
                conn.send(caching[i])
            """
            outputdata=file.readlines()
            conn.send("HTTP/1.0 200 OK\r\n")
            conn.send("Content-Type:text/html\r\n")
            for i in range(0, len(outputdata)):
            	conn.send(outputdata[i])
            file.close()
            print ("Read from Cache\n")
        except Exception, e:
            print e  
        
    else:
        print ("File does not exist in the cache!\n")
        #CAHCE MISS
        printout("Request : ", first_line, client_addr)
        print ("\n\nURL : ",url)
        # find the webserver and port
        http_pos = url.find("://")          # find pos of ://
        if (http_pos==-1):
            temp = url
        else:
            temp = url[(http_pos+3):]       # get the rest of url
        
        port_pos = temp.find(":")           # find the port pos (if any)

        # find end of web server
        webserver_pos = temp.find("/")
        if webserver_pos == -1:
            webserver_pos = len(temp)

        webserver = ""
        port = -1
        if (port_pos==-1 or webserver_pos < port_pos):
            port = 80
            webserver = temp[:webserver_pos]
        else:
            port = int((temp[(port_pos+1):])[:webserver_pos-port_pos-1])
            webserver = temp[:port_pos]

        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  
            s.connect((webserver, port))
            header, content=header_and_data(request)
            headers=read_headers(header)
            #does not accept encoding such as gzip
            if 'Accept-Encoding' in headers:
                del headers['Accept-Encoding']
            print "Request to the web server\n"
            request=unite(headers, content)
            s.send(request)        # send request to webserver
            
            while 1:
                # receive data from web server
                data = s.recv(MAX_DATA_RECV)
                
                if (len(data) > 0):
                    
                    header, body=header_and_data(data)
                    headers=read_headers(header)
                    
                    modified_body=""
                    #not chunked data
                    if 'Content-Length' in headers:
                        modified_body=body.replace(oldword, newword)
                        if len(oldword)!=len(newword):
                            
                            headers['Content-Length']=str(int(headers['Content-Length'])+(len(newword)-len(oldword)))
                            
                        modified_data=unite(headers,modified_body)
                        conn.send(modified_data)
                        #m=md5.new()
                        #m.update(url)
                        tmpFile=open(url.replace('/','_'),"wb")
                        
                        #fileobj=s.makefile('r',0)
                        #fileobj.write("GET " + "http://" + url+"HTTP/1.1\n\n")
                        #buffer=fileobj.readlines()
                        #for i in range(0, len(buffer)):
                        #    tmpFile.write(buffer[i])
                        tmpFile.write(modified_data)
                        CACHEDICT[url]=url.replace('/','_')
                        print CACHEDICT
                        csv_file=open("./CACHEDICT.csv", "a")
                        writer=csv.writer(csv_file)
                        writer.writerow([url, url.replace('/','')])
                        csv_file.close()
                        tmpFile.close()

                    #chunked data
                    
                    else:
                        print body
                        modified_data=data.replace(oldword, newword)
                        modified_data=modified_data.replace('chunked','')
                        conn.send(modified_data)
                        m=md5.new()
                        m.update(url)
                        tmpFile=open(url.replace('/','_'),"wb")
                        
                        #fileobj=s.makefile('r',0)
                        #fileobj.write("GET " + "http://" + url+"HTTP/1.1\n\n")
                        #buffer=fileobj.readlines()
                        #for i in range(0, len(buffer)):
                        #    tmpFile.write(buffer[i])
                        tmpFile.write(modified_data)
                        CACHEDICT[url]=url.replace('/','_')
                        print CACHEDICT
                        csv_file=open("./CACHEDICT.csv", "a")
                        writer=csv.writer(csv_file)
                        writer.writerow([url, url.replace('/','_')])
                        csv_file.close()
                        tmpFile.close()


                    



                    
                else:
                    break
            s.close()
            conn.close()
        except socket.error, (value, message):
            if s:
                s.close()
            if conn:
                conn.close()
            print message
            sys.exit(1)
    


   
    
    
if __name__ == '__main__':
    main()

