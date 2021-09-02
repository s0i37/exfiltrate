#!/usr/bin/env python3
# coding=utf-8

import argparse
import datetime
import sys
from time import sleep
import threading
import traceback
import socketserver
import struct
import string
import random
import socket
try:
    from dnslib import *
except ImportError:
    print("Missing dependency dnslib: <https://pypi.python.org/pypi/dnslib>. Please install it with `pip`.")
    sys.exit(2)


class DomainName(str):
    def __getattr__(self, item):
        return DomainName(item + '.' + self)


D = DomainName('txt.yourzone.tk.')
IP = '10.0.0.1'
TTL = 60 * 5
datas = {}
socks = {}
new_connection = False
last_socket = None

soa_record = SOA(
    mname=D.ns1,  # primary name server
    rname=D.andrei,  # email of the domain administrator
    times=(
        201307231,  # serial number
        60 * 60 * 1,  # refresh
        60 * 60 * 3,  # retry
        60 * 60 * 24,  # expire
        60 * 60 * 1,  # minimum
    )
)
ns_records = [NS(D.ns1), NS(D.ns2)]
records = {
    D: [A(IP), AAAA((0,) * 16), MX(D.mail), soa_record] + ns_records,
    D.ns1: [A(IP)],  # MX and NS records must never point to a CNAME alias (RFC 2181 section 10.3)
    D.ns2: [A(IP)],
    D.mail: [A(IP)],
    D.andrei: [CNAME(D)],
}

#sPOS.size.hexadata.sock.zone.ga
#rPOS.sock.zone.ga
def dns_response(data):
    global records, datas, new_connection, socks, last_socket
    request = DNSRecord.parse(data)

    reply = DNSRecord(DNSHeader(id=request.header.id, qr=1, aa=1, ra=1), q=request.q)

    qname = request.q.qname
    qn = str(qname)
    if qn.find(".txt.yourzone.tk") != -1:
        if qn.split(".")[0].startswith("s"):
            pos,size,data,sock = qn.split(".")[:4]
            pos = int(pos[1:])
            size = int(size)
            if size > 0:
                data = bytes.fromhex(data)

                if not sock in datas:
                    new_connection = True
                    datas[sock] = {"size": None, "input": {}, "output": {}}

                if not sock in socks and last_socket:
                    socks[last_socket] = sock
                    last_socket = None

                datas[sock]["size"] = size
                datas[sock]["input"][pos] = data
            else:
                datas[sock]["size"] = -1
            records[DomainName(qn)] = [A("127.0.0.1")]
            print(f"[*] {qn}")

        elif qn.split(".")[0].startswith("r"):
            pos,sock = qn.split(".")[:2]
            pos = int(pos[1:])

            if not sock in socks and last_socket:
                socks[last_socket] = sock
                last_socket = None

            if not sock in datas:
                new_connection = True
                datas[sock] = {"size": None, "input": {}, "output": {}}
                
            if datas[sock]["size"] == -1:
                answer = "-"
            elif pos in datas[sock]["output"]:
                answer = datas[sock]["output"][pos].hex()
                del(datas[sock]["output"][pos])
            else:
                answer = ""
            records[DomainName(qn)] = [TXT(answer)]
            print(f"[*] {qn} {answer}")

    qtype = request.q.qtype
    qt = QTYPE[qtype]

    if qn == D or qn.endswith('.' + D):
        for name, rrs in records.items():
            if name == qn:
                for rdata in rrs:
                    rqt = rdata.__class__.__name__
                    if qt in ['*', rqt]:
                        reply.add_answer(RR(rname=qname, rtype=getattr(QTYPE, rqt), rclass=1, ttl=TTL, rdata=rdata))

        for rdata in ns_records:
            reply.add_ar(RR(rname=D, rtype=QTYPE.NS, rclass=1, ttl=TTL, rdata=rdata))

        reply.add_auth(RR(rname=D, rtype=QTYPE.SOA, rclass=1, ttl=TTL, rdata=soa_record))

    return reply.pack()


class BaseRequestHandler(socketserver.BaseRequestHandler):

    def get_data(self):
        raise NotImplementedError

    def send_data(self, data):
        raise NotImplementedError

    def handle(self):
        now = datetime.datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S.%f')
        '''
        print("\n\n%s request %s (%s %s):" % (self.__class__.__name__[:3], now, self.client_address[0],
                                               self.client_address[1]))
        '''
        try:
            data = self.get_data()
            #print(len(data), data)  # repr(data).replace('\\x', '')[1:-1]
            self.send_data(dns_response(data))
        except Exception:
            traceback.print_exc(file=sys.stderr)


class TCPRequestHandler(BaseRequestHandler):

    def get_data(self):
        data = self.request.recv(8192).strip()
        sz = struct.unpack('>H', data[:2])[0]
        if sz < len(data) - 2:
            raise Exception("Wrong size of TCP packet")
        elif sz > len(data) - 2:
            raise Exception("Too big TCP packet")
        return data[2:]

    def send_data(self, data):
        sz = struct.pack('>H', len(data))
        return self.request.sendall(sz + data)


class UDPRequestHandler(BaseRequestHandler):

    def get_data(self):
        return self.request[0]#.strip()

    def send_data(self, data):
        return self.request[1].sendto(data, self.client_address)

def tcp_send(sock): # dns_recv
    global datas, socks
    while True:
        if sock in socks:
            if not socks[sock] in datas:
                print("[*] connection closed dns")
                break
            partial = b"".join(datas[socks[sock]]["input"].values())
            #print(len(partial), datas[socks[sock]]["size"])
            if len(partial) == datas[socks[sock]]["size"]:
                buf = b""
                parts = list(datas[socks[sock]]["input"].keys()); parts.sort()
                for part in parts:
                    buf += datas[socks[sock]]["input"][part]
                sock.send(buf)
                datas[socks[sock]]["input"] = {}
                print(f"[+] -> {buf.hex()}")
        sleep(0.1)
    sock.close()
    datas[socks[sock]]["size"] = -1

def tcp_recv(sock): # dns_send
    global datas, socks
    buf = ""
    FRAG = int(250/2)
    while True:
        try:
            if not buf:
                buf = sock.recv(1024)
            if sock in socks:
                if buf:
                    if datas[socks[sock]]["size"] == -1:
                        print("[*] connection closed dns")
                        break
                    if not datas[socks[sock]]["output"].keys():
                        print(f"[+] <- {buf.hex()}")
                        for p in range(0, len(buf), FRAG):
                            datas[socks[sock]]["output"][p] = buf[p:p+FRAG]
                        buf = ""
                else:
                    print("[*] connection closed, no data")
                    break
            sleep(0.1)
        except Exception as e:
            print("[*] connection closed " + str(e))
            break
    sock.close()
    datas[socks[sock]]["size"] = -1

def _listen(port):
    global last_socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("0.0.0.0", port))
    s.listen(10)
    while True:
        c,info = s.accept()
        last_socket = c
        print("[*] new tcp connection")
        threading.Thread(target=tcp_recv, args=(c,)).start()
        threading.Thread(target=tcp_send, args=(c,)).start()

def _connect(ip, port):
    global new_connection, last_socket
    while True:
        if new_connection:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            last_socket = s
            s.connect((ip, port))
            print("[*] tcp connected")
            threading.Thread(target=tcp_send, args=(s,)).start()
            threading.Thread(target=tcp_recv, args=(s,)).start()
            new_connection = False
        sleep(1)

def main():
    parser = argparse.ArgumentParser(description='Start a DNS implemented in Python. Usually DNSs use UDP on port 53.')
    parser.add_argument('--port', default=53, type=int, help='The port to listen on.')
    parser.add_argument('--tcp', action='store_true', help='Listen to TCP connections.')
    parser.add_argument('--udp', action='store_true', help='Listen to UDP datagrams.')
    parser.add_argument('-l', type=int, help='Listen port')
    parser.add_argument('-c', type=str, help='Connect to target:port')

    
    args = parser.parse_args()
    if not (args.udp or args.tcp): parser.error("Please select at least one of --udp or --tcp.")

    print("Starting nameserver...")

    servers = []
    if args.udp: servers.append(socketserver.ThreadingUDPServer(('', args.port), UDPRequestHandler))
    if args.tcp: servers.append(socketserver.ThreadingTCPServer(('', args.port), TCPRequestHandler))

    for s in servers:
        thread = threading.Thread(target=s.serve_forever)  # that thread will start one more thread for each request
        thread.daemon = True  # exit the server thread when the main thread terminates
        thread.start()
        print("%s server loop running in thread: %s" % (s.RequestHandlerClass.__name__[:3], thread.name))

    if args.l:
        port = args.l
        _listen(port)
    elif args.c:
        ip,port = args.c.split(":")
        _connect(ip, int(port))

    try:
        input()
    except KeyboardInterrupt:
        pass
    finally:
        for s in servers:
            s.shutdown()

if __name__ == '__main__':
    main()
