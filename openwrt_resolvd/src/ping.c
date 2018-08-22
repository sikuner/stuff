
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/signal.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static const int DEFDATALEN = 56;
static const int MAXIPLEN = 60;
static const int MAXICMPLEN = 76;
static const int MAXPACKET = 65468;
#define	MAX_DUP_CHK	(8 * 128)
static const int MAXWAIT = 10;
static const int PINGINTERVAL = 1;		/* second */

#define O_QUIET   	(1 << 0)

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~B(bit)))
#define	TST(bit)	(A(bit) & B(bit))

/* common routines */
static int in_cksum(unsigned short *buf, int sz)
{
	int nleft = sz;
	int sum = 0;
	unsigned short *w = buf;
	unsigned short ans = 0;
	
	while (nleft > 1) 
	{
		sum += *w++;
		nleft -= 2;
	}
	
	if (nleft == 1) 
	{
		*(unsigned char *) (&ans) = *(unsigned char *) w;
		sum += ans;
	}
	
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	ans = ~sum;
	
	return (ans);
}

static int create_icmp_socket(void)
{
	struct protoent *proto;
	int sock;
	
	proto = getprotobyname("icmp");
	/* if getprotobyname failed, just silently force
	 * proto->p_proto to have the correct value for "icmp" */
	if ((sock = socket(AF_INET, SOCK_RAW,
			(proto ? proto->p_proto : 1))) < 0)         /* 1 == ICMP */
	{
		if (errno == EPERM)
		{
			fprintf(stderr, "bb_error_msg_and_die(bb_msg_perm_denied_are_you_root) \n");
			exit(1);
		}
		else
		{
			fprintf(stderr, "bb_perror_msg_and_die(bb_msg_can_not_create_raw_socket) \n");
			exit(1);
		}
	}
	
	/* drop root privs if running setuid */
	setuid(getuid());
	
	return sock;
}

static char *icmp_type_name (int id)
{
	switch (id) 
	{
	case ICMP_ECHOREPLY: 		return "Echo Reply";
	case ICMP_DEST_UNREACH: 	return "Destination Unreachable";
	case ICMP_SOURCE_QUENCH: 	return "Source Quench";
	case ICMP_REDIRECT: 		return "Redirect (change route)";
	case ICMP_ECHO: 			return "Echo Request";
	case ICMP_TIME_EXCEEDED: 	return "Time Exceeded";
	case ICMP_PARAMETERPROB: 	return "Parameter Problem";
	case ICMP_TIMESTAMP: 		return "Timestamp Request";
	case ICMP_TIMESTAMPREPLY: 	return "Timestamp Reply";
	case ICMP_INFO_REQUEST: 	return "Information Request";
	case ICMP_INFO_REPLY: 		return "Information Reply";
	case ICMP_ADDRESS: 			return "Address Mask Request";
	case ICMP_ADDRESSREPLY: 	return "Address Mask Reply";
	default: 					return "unknown ICMP type";
	}
}

/* full(er) version */
static struct sockaddr_in pingaddr;
static int pingsock = -1;
static int datalen; /* intentionally uninitialized to work around gcc bug */ // 包大小

static long ntransmitted, nreceived, nrepeats, pingcount = 0;
static int myid, options = 0 /*'-q'*/;
static long tmin = LONG_MAX, tmax, tsum, tavg = 0;
static char rcvd_tbl[MAX_DUP_CHK / 8];

static struct hostent *hostent;

static void pingstats(int junk);
static void sendping(int junk);
static void unpack(char *buf, int sz, struct sockaddr_in *from);

/**************************************************************************/

static void pingstats(int junk)
{
	int status;
	
	signal(SIGINT, SIG_IGN);
	
	printf("\n--- %s ping statistics ---\n", hostent->h_name);
	printf("%ld packets transmitted, ", ntransmitted);
	printf("%ld packets received, ", nreceived);
	if (nrepeats)
		printf("%ld duplicates, ", nrepeats);
	if (ntransmitted)
		printf("%ld%% packet loss\n",
			(ntransmitted - nreceived) * 100 / ntransmitted);
	if (nreceived)
	{
		tavg = (tsum / (nreceived + nrepeats));
		printf("round-trip min/avg/max = %lu.%lu/%lu.%lu/%lu.%lu ms\n",
				tmin / 1000, tmin % 1000,
				tavg / 1000, tavg % 1000,
				tmax / 1000, tmax % 1000);
	}
	if (nreceived != 0)
		status = EXIT_SUCCESS;
	else
		status = EXIT_FAILURE;
	
	printf("avg = %lu.%lu ms \n", tavg / 1000, tavg % 1000);
	exit(status);
}

static void shutsock(int junk)
{
	shutdown(pingsock, SHUT_RD);
}

static void sendping(int junk)
{
	struct icmp *pkt;
	int i;
	char packet[datalen + 8];
	
	pkt = (struct icmp *) packet;
	
	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_code = 0;
	pkt->icmp_cksum = 0;
	pkt->icmp_seq = ntransmitted++;
	pkt->icmp_id = myid;
	CLR(pkt->icmp_seq % MAX_DUP_CHK);
	
	gettimeofday((struct timeval *) &packet[8], NULL);
	pkt->icmp_cksum = in_cksum((unsigned short *) pkt, sizeof(packet));
	
	i = sendto(pingsock, packet, sizeof(packet), 0,
			(struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in));
	if (i < 0)
	{
		fprintf(stderr, "%s %s\n", "sendto", strerror(errno));
		exit(1);
	}
	else if ((size_t)i != sizeof(packet))
	{
		fprintf(stderr, "ping wrote %d chars; %d expected"" %s \n", 
			i,
			(int)sizeof(packet), 
			strerror(errno));
		exit(1);
	}
	
	signal(SIGALRM, sendping);
	if (pingcount == 0 || ntransmitted < pingcount)
	{
		alarm(PINGINTERVAL); /* schedule next in 1s */
	}
	else 
	{	/* done, wait for the last ping to come back */
		/* todo, don't necessarily need to wait so long... */
//		signal(SIGALRM, pingstats);
		signal(SIGALRM, shutsock);
		alarm(MAXWAIT);
	}
}

static void unpack(char *buf, int sz, struct sockaddr_in *from)
{
	struct icmp *icmppkt;
	struct iphdr *iphdr;
	struct timeval tv, *tp;
	int hlen, dupflag;
	unsigned long triptime;
	
	gettimeofday(&tv, NULL);
	
	/* check IP header */
	iphdr = (struct iphdr *) buf;
	hlen = iphdr->ihl << 2;
	/* discard if too short */
	if (sz < (datalen + ICMP_MINLEN))
		return;
	
	sz -= hlen;
	icmppkt = (struct icmp *) (buf + hlen);
	
	if (icmppkt->icmp_id != myid)
	    return;				/* not our ping */
	
	if (icmppkt->icmp_type == ICMP_ECHOREPLY) 
	{
	    ++nreceived;
		tp = (struct timeval *) icmppkt->icmp_data;
		
		if ((tv.tv_usec -= tp->tv_usec) < 0) 
		{
			--tv.tv_sec;
			tv.tv_usec += 1000000;
		}
		tv.tv_sec -= tp->tv_sec;
		
		triptime = tv.tv_sec * 100000 + tv.tv_usec;
		tsum += triptime;
		if (triptime < tmin)
			tmin = triptime;
		if (triptime > tmax)
			tmax = triptime;
		
		if (TST(icmppkt->icmp_seq % MAX_DUP_CHK)) 
		{
			++nrepeats;
			--nreceived;
			dupflag = 1;
		} 
		else
		{
			SET(icmppkt->icmp_seq % MAX_DUP_CHK);
			dupflag = 0;
		}
		
		if (options & O_QUIET)
			return;
		
		printf("%d bytes from %s: icmp_seq=%u", sz,
			   inet_ntoa(*(struct in_addr *) &from->sin_addr.s_addr),
			   icmppkt->icmp_seq);
		printf(" ttl=%d", iphdr->ttl);
		printf(" time=%lu.%lu ms", triptime / 1000, triptime % 1000);
		if (dupflag)
			printf(" (DUP!)");
		printf("\n");
	} 
	else if (icmppkt->icmp_type != ICMP_ECHO)
	{
		fprintf(stderr, "Warning: Got ICMP %d (%s)"" %s\n", 
			icmppkt->icmp_type, 
			icmp_type_name (icmppkt->icmp_type), 
			strerror(errno));
	}
}

/**************************************************************************/

int get_ping_delay(char *host, int *delay)
{
	if(!host || !delay)
	{
		fprintf(stderr, "arguments is invalid! \n");
		return -1;
	}
	
	// 初始化全局参数
	{
		memset(&pingaddr, 0, sizeof(struct sockaddr_in));
		pingsock = -1;
		datalen = DEFDATALEN;
		ntransmitted = 0, nreceived = 0, nrepeats = 0, pingcount = 0;
		myid = 0, options = 0;
		tmin = LONG_MAX, tmax = 0, tsum = 0, tavg = 0;
		
		memset(&rcvd_tbl, 0, sizeof(rcvd_tbl));
		hostent = NULL;
		
		pingcount = 5;
		myid = getpid() & 0xFFFF;		
	}
	
	// ping
	{
		char packet[datalen + MAXIPLEN + MAXICMPLEN];
		int sockopt;
		
		pingsock = create_icmp_socket();
		
		memset(&pingaddr, 0, sizeof(struct sockaddr_in));
		
		pingaddr.sin_family = AF_INET;
		hostent = gethostbyname(host);
		if (!hostent)
		{
			fprintf(stderr, "name: %s \n", host);
			return -1;
		}
		if (hostent->h_addrtype != AF_INET)
		{
			fprintf(stderr, "bb_error_msg_and_die(\"unknown address type; only AF_INET is currently supported.\") \n");
			return -1;
		}
		memcpy(&pingaddr.sin_addr, hostent->h_addr, sizeof(pingaddr.sin_addr));
		
		/* enable broadcast pings */
		sockopt = 1;
		setsockopt(pingsock, SOL_SOCKET, SO_BROADCAST, (char *)&sockopt, sizeof(sockopt));
		/* set recv buf for broadcast pings */
		sockopt = 48 * 1024;
		setsockopt(pingsock, SOL_SOCKET, SO_RCVBUF, (char *)&sockopt, sizeof(sockopt));
		/* 设置socket接受超时 */
		struct timeval tv = { 10, 0 };
		setsockopt(pingsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
		
		printf("PING %s (%s): %d data bytes\n",
			hostent->h_name,
			inet_ntoa(*(struct in_addr *) &pingaddr.sin_addr.s_addr),
			datalen);
		
		signal(SIGINT, pingstats); // 中断,停止并统计ping信息
		
		/* start the ping's going ... */
		sendping(0);
		
		/* listen for replies */
		while (1) 
		{
			struct sockaddr_in from;
			socklen_t fromlen = (socklen_t) sizeof(from);
			int c;
			
			if ((c = recvfrom(pingsock, packet, sizeof(packet), 0,
				  (struct sockaddr *) &from, &fromlen)) < 0) 
			{
				if (errno == EINTR)
				{
					continue;
				}
				
				fprintf(stderr, "%s %s\n", "recvfrom", strerror(errno));
				continue;
			}
			
			unpack(packet, c, &from);
			
			// The return value will be 0 when the peer has performed an orderly shutdown.
			if ((pingcount>0 && nreceived>=pingcount) || 0==c)
				break;
		}
	}
	
	// tavg统计
	{
		*delay = 0x0FFFFFFF;
		
		if (nreceived)
		{
			tavg = (tsum / (nreceived + nrepeats));
			*delay = tavg;
		}
	}
	
	printf("ping %s \n", host);
	printf("round-trip min/avg/max = %lu.%lu/%lu.%lu/%lu.%lu ms \n",
			tmin / 1000, tmin % 1000,
			tavg / 1000, tavg % 1000,
			tmax / 1000, tmax % 1000);
	
	return 0;
}

