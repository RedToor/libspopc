/* this is session.c file, part of the libspopc library sources
 * copyright © 2002- Benoit Rouits <brouits@free.fr>
 * released under the terms of the GNU Lesser General Public Licence.
 *
 * libspopc offers simple API for a pop3 client.
 * See RFC 1725 for pop3 specifications.
 * more information on http://herewe.servebeer.com/libspopc/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*I casted 'port' to true unsigned, now, tell me if warning continues */
/*; #pragma warning(disable: 4761)*/ /* disable "integral size mismatch in argument" -warning */

#ifdef WIN32
#include <winsock.h>
#include <stdio.h>
#else
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif


#include <stdlib.h>
#include <string.h> /* strdup */
#include <stdio.h> /* perror */
#include <sys/types.h>

#include "libspopc.h"
#include "mutex.h"

#define socklen_t int /* actually true on most systems */

/* needs from queries.c */
extern int pop3_recv (pop3sock_t sock, char* buf, int len);

/* mutex locks for this object (must be declared in libspopc.c) */
extern mutex GETHOSTBYNAME;
/* end mutex locks */

#ifdef USE_SSL

#define SSL_USE_NEVER -1	/* never use SSL even on port 995 */
#define SSL_USE_AUTO 0		/* use SSL only on port 995 (default) */
#define SSL_USE_ALWAYS 1	/* force SSL whatever the port is */

static int SSL_UP = 0; /* prevent SSL_library_init() to be called twice */
static char* SSL_CERT = NULL; /* whole application on-disk certfile */

/* set default behaviour as SSL_USE_AUTO for backward compatibility */
static int SSL_USE = SSL_USE_AUTO;


/* let libspopc never use SSL even connecting on port 995 */
DLLIMPORT void pop3_ssl_never(void)
{
	SSL_USE = SSL_USE_NEVER;
}

/* let libspopc use SSL when connecting port 995 */
DLLIMPORT void pop3_ssl_auto(void)
{
	SSL_USE = SSL_USE_AUTO;
}

/* let libspopc use SSL connecting on any port, even 110 */
DLLIMPORT void pop3_ssl_always(void)
{
	SSL_USE = SSL_USE_ALWAYS;
}

/* set the filename of the certificate */
DLLIMPORT void pop3_cert_setup(const char *certfile)
{
	if (SSL_CERT) free(SSL_CERT);
	SSL_CERT=NULL;
	if (certfile) SSL_CERT=strdup(certfile);
}

/* callback for error reporting on ssl verification
 * just return 0 on error, 1 else
 */
DLLIMPORT int ssl_verify_callback(int ok, X509_STORE_CTX *ctx) {
	if (!ok) { /* bad identification */
		int err;
		err = X509_STORE_CTX_get_error(ctx);
		return 0;
	}
	return 1;
}

/* init ssl library and context
 * depending on port and SSL_USE_*
 */ 
DLLIMPORT pop3sock_t ssl_prepare(const int port){
	pop3sock_t sock;

	sock =(pop3sock_t)malloc(sizeof(pop3sock));
	sock->sock=socket(AF_INET,SOCK_STREAM,0);
	if(-1==sock->sock){
		perror("ssl_prepare.socket");
		free(sock);
		return(NULL);
	}
	if (( (995 == port) && (SSL_USE == SSL_USE_AUTO)) 
			|| (SSL_USE == SSL_USE_ALWAYS) ) {
		if (0 == SSL_UP) SSL_UP=SSL_library_init();

		if (1 != SSL_UP) {
#ifdef WIN32
			closesocket(sock->sock);
#else
			close(sock->sock);
#endif
			free(sock);
			perror("ssl_prepare.SSL_library_init");
			return(NULL);
		}
		SSL_load_error_strings();
		sock->ctx = SSL_CTX_new(SSLv23_client_method());
		if (NULL == sock->ctx) {
#ifdef WIN32
			closesocket(sock->sock);
#else
			close(sock->sock);
#endif
			free(sock);
			perror("ssl_prepare.SSLv23_client_method");
			return(NULL);
		}   
		if ( SSL_CERT != NULL ) {
			SSL_CTX_load_verify_locations(sock->ctx, SSL_CERT, 0);
			SSL_CTX_set_verify(sock->ctx, SSL_VERIFY_PEER, &ssl_verify_callback);
		}
		sock->ssl = SSL_new(sock->ctx);
		if (NULL == sock->ssl) {
#ifdef WIN32
			closesocket(sock->sock);
#else
			close(sock->sock);
#endif
			SSL_CTX_free(sock->ctx);
			free(sock);
			perror("ssl_prepare.SSL_new");
			return(NULL);
		}
		SSL_set_fd(sock->ssl, sock->sock);
	} else { /* port != 995 or SSL_USE_NEVER */
		sock->ssl=NULL;
		sock->ctx=NULL;
	}
	return sock;
}

#endif

/* prepares the socket options
 * mainly set read an write timeouts
 * this way may be either ignored
 * or refused on some platform
 */
DLLIMPORT int socket_prepare(int sock){
	struct timeval tv;
	int ret=0;
	tv.tv_sec = SOCKET_TIMEOUT;
	tv.tv_usec = 0 ;

	if (ret += setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, sizeof tv))
		perror("socket_prepare.setsockopt");
	if (ret += setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, sizeof tv))
		perror("socket_prepare.setsockopt");
	return ret;
}

/* prepares the pop session and returns a socket descriptor */
DLLIMPORT pop3sock_t pop3_prepare(const char* servername, const int port, struct sockaddr_in* connection, struct hostent* server){
	pop3sock_t sock;
	struct hostent* hostent_buf;
	int i;
#ifdef WIN32

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
	{
		exit(1);
	}

#endif

	memset((char*)connection,0,sizeof(struct sockaddr_in));

	/* here, enter a critical section to perform atomic name resolution */
	mutex_in(&GETHOSTBYNAME);
	hostent_buf=gethostbyname(servername);
	if(!hostent_buf){
#ifdef WIN32
		perror("pop3_prepare.gethostbyname");
#else
		herror("pop3_prepare.gethostbyname");
#endif
		return BAD_SOCK;
	}
	/* begin hostent deep copy */
	memmove(server,hostent_buf,sizeof(struct hostent));
	server->h_name=strdup(hostent_buf->h_name);
	server->h_aliases=NULL;
	i=0;
	while (hostent_buf->h_aliases[i])
	{
		server->h_aliases=realloc(server->h_aliases,(i+1)*sizeof(char*));
		if(hostent_buf->h_aliases[i])
			server->h_aliases[i]=strdup(hostent_buf->h_aliases[i]);
		else server->h_aliases[i]=NULL;
		i++;
	}
	server->h_addr_list=NULL;
	i=0;
	while (hostent_buf->h_addr_list[i])
	{ /* has at least one adress */
		server->h_addr_list=realloc(server->h_addr_list,(i+1)*sizeof(char*));
		server->h_addr_list[i]=malloc(server->h_length);
		memmove(server->h_addr_list[i],hostent_buf->h_addr_list[i],server->h_length);
		i++;
	} 
	server->h_addr_list=realloc(server->h_addr_list, (i+1)*sizeof(char*));
#if 0
	server->h_addr_list[i]=malloc(1);
	server->h_addr_list[i]='\0';
#else
	server->h_addr_list[i]=NULL;
#endif /* 0 */
	/* end hostent deep copy */
	mutex_out(&GETHOSTBYNAME);
	/* end of critical section (name resolution) */

	memmove((char*)&(connection->sin_addr.s_addr),server->h_addr,server->h_length);
	connection->sin_family=AF_INET;
	connection->sin_port=htons(port?(unsigned short int)port:(unsigned short int)110);
	/* integral size mismatch in argument - htons(port)*/

#ifdef USE_SSL
	sock = ssl_prepare(port);
#else
	sock=socket(AF_INET,SOCK_STREAM,0);
	if(BAD_SOCK == sock){
		perror("pop3_prepare.socket");
	}
#endif
#ifdef USE_SSL
	socket_prepare(sock->sock);
#else
	socket_prepare(sock);
#endif
	return(sock);
}

/* connects to the server through the sock and returns server's welcome */
DLLIMPORT char* pop3_connect(pop3sock_t sock, struct sockaddr_in* connection){
	int r;
	char* buf;


#ifdef USE_SSL
	r=connect(sock->sock,(struct sockaddr*)connection,(socklen_t)sizeof(*connection));
	if ( r!=-1  && sock->ssl ) {
		if (SSL_connect(sock->ssl) == -1) {
#ifdef WIN32
			closesocket(sock->sock);
#else
			close(sock->sock);
#endif
			return(NULL);
		}
		if ( SSL_CERT ) {
			if ( !SSL_get_peer_certificate(sock->ssl) ) {
#ifdef WIN32
				closesocket(sock->sock);
#else
				close(sock->sock);               
#endif
				return(NULL);
			}
		}
	}   
#else
	r=connect(sock,(struct sockaddr*)connection,(socklen_t)sizeof(*connection));
#endif

	if(r==-1){
		perror("pop3_connect.connect");
		return(NULL);
	}


	buf=(char*)malloc(POPBUF);
	if(!buf){
		perror("pop3_connect.malloc");
		return(NULL);
	}
	r=pop3_recv(sock,buf,POPBUF); /* 512 is enough, as say RFC 1939 */
	if(r==0 || r==-1){
	/* -1: transport error (timeout?) */
	/*  0: server error (busy?) */
		perror("pop3_connect.pop3_recv");
		free(buf);
		return(NULL);
	}
	buf[r]='\0';
	return buf;
}

DLLIMPORT void pop3_disconnect(pop3sock_t sock, struct hostent* server){
	int i;
	/* close socket  */
#ifdef USE_SSL
	if (sock->sock>0) 
	{
#ifdef WIN32
		closesocket(sock->sock);
#else
		shutdown(sock->sock,SHUT_RDWR);
		close(sock->sock);
#endif
	}
	if (NULL != sock->ssl) {
		/* a memory leak has been reported with: SSL_shutdown(sock->ssl);*/
		SSL_free(sock->ssl);
	}
	if ( NULL != sock->ctx ) {
		SSL_CTX_free(sock->ctx);
	}

	free(sock);
#else
	if (sock>0) {
#ifdef WIN32
		closesocket(sock);
#else
		shutdown(sock,SHUT_RDWR);
		close(sock);
#endif
	}
#endif
	/* free deep copy of struct hostent */
	i=0;
	while(NULL != server->h_addr_list[i]) {
		/* has at least one adress */
		free(server->h_addr_list[i]);
		i++;
	}
	/*    free(server->h_addr_list[i]); FIXME  */
	free(server->h_addr_list);
	free(server->h_name);
#ifdef WIN32
	WSACleanup();
#endif
}
