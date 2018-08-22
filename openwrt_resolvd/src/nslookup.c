
#include "networking.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdint.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include <arpa/inet.h>

#include <arpa/nameser.h>
 
extern struct state _res;

#if 0
struct ipaddrn {
	
	char ipaddr[32];
	struct ipaddrn* ia_next;
};
#endif

static int add_ipaddrn(char *addr, struct ipaddrn  **header)
{
	struct ipaddrn *n = (struct ipaddrn *)malloc(sizeof(struct ipaddrn));
	if(!n)
	{
		fprintf(stderr, "Out of memory! \n");
		return -1;
	}
	memset(n, 0, sizeof(struct ipaddrn));
	
	strcpy(n->ipaddr, addr);
	
	if(NULL == *header)
	{
		*header = n;
	}
	else
	{
		struct ipaddrn *p = *header;
		while(p->ia_next)
		{
			p = p->ia_next;
		}
		
		p->ia_next = n;
	}
	
	return 0;
}

struct ipaddrn* nslookup(char *hostname, char *dns)
{
	static struct ipaddrn *header = NULL;
	
	// 清理结构体链表
	struct ipaddrn *p, *tmp;
	p = header;
	while(p)
	{
		tmp = p->ia_next;
		free(p);
		p = tmp;
	}
	header = NULL;
	
	if(!hostname || !dns)
	{
		fprintf(stderr, "arguments is invalid! \n");
		return NULL;
	}
	
	/* initialize DNS structure _res used in printing the default
	 * name server and in the explicit name server option feature. */
	res_init();
	
	// set_default_dns
	struct in_addr dns_in_addr;
	
	if(inet_aton(dns, &dns_in_addr))	
	{
		_res.nscount = 1;		
		_res.nsaddr_list[0].sin_addr = dns_in_addr;	
	}
	
	// getaddrinfo
	struct addrinfo *result = NULL;
	struct addrinfo hint;
	int rc;
	
	struct addrinfo *cur = NULL;
	int cnt = 0;
	
	memset(&hint, 0 , sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;
	
	rc = getaddrinfo(hostname, NULL /*service*/, &hint, &result);
	if(0 == rc)  // getaddrinfo() returns 0 if it succeeds
	{
		cur = result;
		cnt = 0;
		
		printf("\n");
		printf("%-10s %s \n", "DNS", dns);
		printf("%-10s %s \n", "Name", hostname);
		
		while(cur) 
		{
			char host[128];
			char serv[16];
			int r;
			socklen_t salen;
			
			salen = sizeof(struct sockaddr_in);
			r = getnameinfo(cur->ai_addr, salen,
					host, sizeof(host),
					serv, sizeof(serv),
					NI_NUMERICHOST | NI_NUMERICSERV);
			if(!r) // On success 0 is returned
			{
				printf("Address %u: %s \n", ++cnt, host);
				
				// add a node to list
				add_ipaddrn(host, &header);				
			}
			
			cur = cur->ai_next;
		}
	}
	
	return header;
}

