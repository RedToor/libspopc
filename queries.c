/* this is queries.c file, part of the libspopc library sources
 * copyright � 2002- Benoit Rouits <brouits@free.fr>
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

#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "libspopc.h"

#ifdef USE_SSL

DLLIMPORT int pop3_recv (pop3sock_t sock, char* buf, int len){
	return sock->ssl?SSL_read(sock->ssl,buf,len):recv(sock->sock,buf,len,0);
}

DLLIMPORT int pop3_send (pop3sock_t sock, char* buf, int len){
	return sock->ssl?SSL_write(sock->ssl,buf,len):send(sock->sock,buf,len,0);
}

#else

DLLIMPORT int pop3_recv (pop3sock_t sock, char* buf, int len){
	return recv(sock,buf,len,0);
}

DLLIMPORT int pop3_send (pop3sock_t sock, char* buf, int len){
	return send(sock,buf,len,0);
}

#endif

DLLIMPORT char* pop3_query(pop3sock_t sock, const char* query){
/* performs a simple pop query and returns server's <=512 bytes resp */
	int r=0;
	int bytes=0;
	char* buf=NULL;

	r=pop3_send(sock,(char *)query,strlen(query));
	if(r==-1){ /* send timeout reached */
		perror("pop3_query.pop3_send");
		return(NULL);
	}
	buf=(char*)malloc(POPBUF); /* 512B + EOS */
	if(!buf){
		perror("pop3_query.malloc");
		return(NULL);
	}
	do {
		r=pop3_recv(sock, buf+bytes, POPBUF-bytes-1);
		if (r>0)
			bytes+=r;
		else { /* closed (0) or timeout reached (-1) */
			free(buf);
			return NULL;
		}
	} while (bytes < 2 || (buf[bytes-2] != '\r' && buf[bytes-1] != '\n'));
	buf[bytes]='\0';
	return(buf);
}

DLLIMPORT char* pop3_user(pop3sock_t sock, const char* name){
/* performs "USER" pop query and returns server's <512 bytes response */
	char query[POPBUF]; /* total "USER ****your_name****\n" is <= 512  */

	snprintf(query,POPBUF,"USER %s\r\n",name);
	return(pop3_query(sock,query));
}

DLLIMPORT char* pop3_pass(pop3sock_t sock, const char* pw){
/* performs "PASS" pop query and return server's <=512 bytes response */
	char query[POPBUF]; /* total "PASS ****your_pass****\n" is <=512 */

	snprintf(query,POPBUF,"PASS %s\r\n",pw);
	return(pop3_query(sock,query));
}

DLLIMPORT char* pop3_quit(pop3sock_t sock){
/* performs "QUIT" pop query and returns server's <=512 bytes response */
	char query[]="QUIT\r\n";

	return(pop3_query(sock,query));
}

DLLIMPORT char* pop3_stat(pop3sock_t sock){
/* performs "STAT" pop query and returns server's <=512 bytes response */
	char query[]="STAT\r\n";

	return(pop3_query(sock,query));
}

char* recv_rest(pop3sock_t sock, char* buf, int cursize, int bufsize){
/* recv rest of data through sock, given a cs  pre-filled buffer sized of bs.
 * end of data is assumed when data has a "\r\n.\r\n" string
 * recv() is TCPBUFLEN  bytes stepped, Warning: after calling this function, 
 * buf must never be used again -not even for a free(buf)- since it may be
 * reallocated. Use the return value instead:
 * usage example: buf=recv_rest(sock,buf,cs,bs); free(buf); 
 */
	char* ret = NULL;
	char* cur = NULL; /* current position ready to receive */
	int total;    /* total received */
	if(!buf){
		return(NULL);
	}
	total = cursize;
	cur = buf;
	if(cursize == bufsize){
		ret=(char*)realloc(buf,bufsize+1);
		if(!ret){
			perror("recv_rest.realloc");
			free(buf);
			return NULL;
		}
		cur = buf = ret;
	}
	cur[total]='\0';
	while(!dotline(buf)){ /* recv until "\r\n.\r\n" */
		if (total >= (bufsize - TCPBUFLEN)){
			ret = (char*)realloc(buf, (bufsize *=2) +1);
		}
		if(!ret){
			perror("recv_rest.realloc");
			free(buf);
			return NULL;
		}
		buf = ret;
		cur = buf + total;
		/* we use blocking sockets WITH TIMEOUT: no need for select() */
		cursize=pop3_recv(sock, cur, TCPBUFLEN);
		if (cursize <= 0){ /* timeout (-1) or closed (0) */
			perror("recv_rest.pop3_recv");
			free(buf);
			return(NULL);
		} /* else, we got some bytes */
		total+=cursize;
		cur[cursize] = '\0';
		/* SUGGEST: we can strdup to a callback here */
	}
	return(buf);
}

DLLIMPORT char* pop3_list(pop3sock_t sock, int id){
/* performs a "LIST" pop query and returns server's (long) response */
	int r;
	char query[POPBUF]; /* total query "LIST ID\n" string is <=512 */
	char* buf;

	if(id>0){
		snprintf(query,POPBUF,"LIST %d\r\n",id);
	}else{
		snprintf(query,POPBUF,"LIST\r\n");
	}
	r=pop3_send(sock,query,strlen(query));
	if(r==-1){
		perror("pop3_list.pop3_send");
		return(NULL);
	}
	/* now prepare a first short 512 bytes recv() */
	/* it might be now enough for recv() from "LIST X" */
	buf=(char*)malloc(POPBUF); /* 512 chars + '\0' */
	if(!buf){
		perror("pop3_list.malloc");
		return(NULL);
	}
	r=pop3_recv(sock,buf,POPBUF-1);
	if(r <= 0){ /* close (0) or timeout (-1) */
		perror("pop3_list.pop3_recv");
		free(buf);
		return(NULL);
	} /* else, got some bytes */
	buf[r]='\0';
	if(id>0){/* +OK id size */
		return(buf); /* 512 bytes are enough as say RFC 1939 */
	}
	/* else : +OK X messages (YYY octets)\n id size\n... */
	if(pop3_error(buf)){
		return(buf);
	}
	return(recv_rest(sock,buf,r,POPBUF-1));
}

DLLIMPORT char* pop3_retr(pop3sock_t sock, int id){
/* performs a "RETR" pop query and returns server's (long) response */
	int r;
	char query[POPBUF];
	char *buf;

	snprintf(query, POPBUF, "RETR %d\r\n", id);
	r=pop3_send(sock, query, strlen(query));
	if(r==-1){
		perror("pop3_retr.pop3_send");
		return(NULL);
	}
	buf=(char*)malloc(POPBUF);/* 512 chars + '\0' */
	if(!buf) {
		perror("pop3_retr.malloc");
		return(NULL);
	}
	/* using blocking sockets WITH TIMEOUT: no need for select() */
	r=pop3_recv(sock, buf, POPBUF-1); 
	if(r <= 0){ /* timeout (-1) or close (0) */
		perror("pop3_retr.pop3_recv");
		free(buf);
		return(NULL);
	} /* else, got some bytes */
	if(pop3_error(buf)){
		buf[r] = '\0';
		return(buf); /* 512 are enough as say RFC 1939 */
	}
	return(recv_rest(sock, buf, r, POPBUF-1));
}

DLLIMPORT char* pop3_dele(pop3sock_t sock, int id){
/* performs a "DELE" pop query and returns server's <=512 bytes response */
	char query[POPBUF]; /* total "DELE X\n" string <=512 */

	if(id<=0){
		return(NULL);
	}
	snprintf(query,POPBUF,"DELE %d\r\n",id);
	return(pop3_query(sock,query));
}

DLLIMPORT char* pop3_noop(pop3sock_t sock){
/* performs a "NOOP" pop query and returns server's <=512 bytes response */
	char query[]="NOOP\r\n";

	return(pop3_query(sock,query));
}

DLLIMPORT char* pop3_rset(pop3sock_t sock){
/* performs a "RSET" pop query and returns server's <=512 bytes response */
	char query[]="RSET\r\n";

	return(pop3_query(sock,query));
}

DLLIMPORT char* pop3_top(pop3sock_t sock, int id, int lines){
/* performs a "TOP" pop query and returns server's (long) response */
	int r;
	char query[POPBUF]; /* total "TOP X Y\n" is <=512 */
	char* buf;

	snprintf(query,POPBUF,"TOP %d %d\r\n",id,lines);
	r=pop3_send(sock,query,strlen(query));
	if(r==-1){
		perror("pop3_top.pop3_send");
		return(NULL);
	}
	/* prepare first recv() of 512 bytes */
	buf=(char*)malloc(POPBUF); /* 512 chars + '\0' */
	if(!buf){
		perror("pop3_top.malloc");
		return(NULL);
	}
	r=pop3_recv(sock,buf,POPBUF-1);
	if(r <= 0){ /* timeout (-1) or close (0) */
		perror("pop3_top.pop3_recv");
		free(buf);
		return(NULL);
	} /* else, got some bytes */
	buf[r]='\0';
	if(pop3_error(buf)){
		return(buf); /* 512 bytes are enough as say RFC 1939 */
	}
	return(recv_rest(sock,buf,r,POPBUF-1));
}

DLLIMPORT char* pop3_uidl(pop3sock_t sock, int id){
/* performs a "UIDL" pop query and returns server's (long) response */
	int r;
	char query[POPBUF]; /* total "UIDL X\n" is <=512 */
	char* buf;

	if(id>0){
		snprintf(query,POPBUF,"UIDL %d\r\n",id);
	}else{
		snprintf(query,POPBUF,"UIDL\r\n");
	}
	r=pop3_send(sock,query,strlen(query));
	if(r==-1){
		perror("pop3_uidl.pop3_send");
		return(NULL);
	}
	/* prepare first 512 bytes for recv() */
	/* i hope this is also enough for the 'one line' short response */
	buf=(char*)malloc(POPBUF); /* 512 chars + '\0' */
	if(!buf){
		perror("pop3_uidl.malloc");
		return(NULL);
	}
	memset(buf,0,POPBUF);
	r=pop3_recv(sock,buf,POPBUF-1);
	if (r <= 0){ /* timeout (-1) or close (0) */
		perror("pop3_uidl.pop3_recv");
		free(buf);
		return(NULL);
	} /* else, got some bytes */
	buf[r]='\0';
	if(id>0){/* +OK id sig */
		/* return the short buf, error or not */
		return(buf); /* 512 are enough as say RFC 1939 */
	}
	/* else : +OK\n id sig\nid sig\nid sig\n... */
	if(pop3_error(buf)){
		return(buf); /* hope error msg were <=512 bytes */
	}
	return(recv_rest(sock,buf,r,POPBUF-1));
}

DLLIMPORT char* pop3_apop(pop3sock_t sock, const char* name, const char* digest){
/* performs a "APOP" secure query and returns server's <=512 bytes response */
	char query[POPBUF]; /* total "APOP name digest\r\n" is <=512 */

	snprintf(query,POPBUF,"APOP %s %s\r\n",name,digest);
	return(pop3_query(sock,query));
}

