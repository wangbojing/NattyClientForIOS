


#include <netdb.h>
#include <sys/socket.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#define BUF_SIZE 500

char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen) {
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }

    return s;
}


int main(int argc, char **argv) {
#if 0
	char *ptr, **pptr;
	struct hostent *hptr;
	char str[32] = {0};

	ptr = argv[1];

	if ((hptr = gethostbyname(ptr)) == NULL) {
		printf(" gethostbyname error for host:%s", ptr);
		return 0;
	}

	printf(" official hostname:%s\n", hptr->h_name);
	for(pptr = hptr->h_aliases;*pptr != NULL; pptr ++) {
		printf(" alias:%s\n", *pptr);
	}

	switch(hptr->h_addrtype) {
		case AF_INET:
		case AF_INET6: {
			pptr = hptr->h_addr_list;
			for (;*pptr != NULL; pptr ++) {
				inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));
				printf("address : %s\n", str);
				
				inet_ntop(hptr->h_addrtype, hptr->h_addr, str, sizeof(str));
				printf("frist address : %s\n", str);
			}
			break;
		}
		default:
			printf("unknown address type\n");
			break;
	}
#else

	struct sockaddr_in *addr;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s, j;
	size_t len;
	ssize_t nread;
	char buf[BUF_SIZE];

	if (argc < 3) {
		fprintf(stderr, "Usage: %s host port msg...\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Obtain address(es) matching host/port */

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;          /* Any protocol */

	s = getaddrinfo(argv[1], argv[2], &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %d %s\n", s,  gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	  Try each address until we successfully connect(2).
	  If socket(2) (or connect(2)) fails, we (close the socket
	  and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
#if 0
	   sfd = socket(rp->ai_family, rp->ai_socktype,
	                rp->ai_protocol);
	   if (sfd == -1)
	       continue;

	   if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
	       break;                  /* Success */

	   close(sfd);
#else
		//ntylog(" %d.%d.%d.%d:%d --> Client Disconnected\n", *(unsigned char*)(&client_addr.sin_addr.s_addr), *((unsigned char*)(&client_addr.sin_addr.s_addr)+1),													
		//		*((unsigned char*)(&client_addr.sin_addr.s_addr)+2), *((unsigned char*)(&client_addr.sin_addr.s_addr)+3),													
		//		client_addr.sin_port);	

		printf(" family: %d, socktype: %d, protocol: %d\n", rp->ai_family, rp->ai_socktype, rp->ai_protocol);
#if 0
		printf(" %d.%d.%d.%d:%d --> Client Disconnected\n", *(unsigned char*)(&rp->ai_addr.sin_addr.s_addr), *((unsigned char*)(&rp->ai_addr.sin_addr.s_addr)+1),													
				*((unsigned char*)(&rp->ai_addr.sin_addr.s_addr)+2), *((unsigned char*)(&rp->ai_addr.sin_addr.s_addr)+3),													
				rp->ai_addr.sin_port)
#else
		
		get_ip_str(rp->ai_addr, buf, 128);
		
		printf("%s\n", buf);
#endif
#endif
	}

#endif
	return 0;

}



