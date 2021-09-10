#!/usr/bin/python3
from os import system
from struct import unpack

TIMEOUT = 1
system("xwininfo | grep id | grep 'xwininfo:' | awk '{print $4}' > /tmp/winid.txt")
winid = open("/tmp/winid.txt").read().split()[0]

output = {}
is_start = False
errors = 0
_num = 0
try:
	while True:
		system(f"gm import -window {winid} /tmp/qr.png")
		system("zbarimg --raw --oneshot -Sbinary /tmp/qr.png > /tmp/qr-decoded.bin 2> /dev/null") # apt install zbar-tools
		data = open("/tmp/qr-decoded.bin", "rb").read()
		if data:
			errors = 0
			is_start = True
			num = unpack("<H", data[:2])[0]
			if num != _num:
				print(f"[!] part {_num} lost")
				_num = num
			if not num in output:
				_num += 1
				output[num] = data[2:]
				print(f"[+] {num} {output[num].hex()}")
		elif is_start:
			errors += 1
		if errors >= 3:
			break
except:
	pass

with open("out.bin","wb") as o:
	o.write( b"".join(output.values()) )
