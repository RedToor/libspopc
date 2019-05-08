/* this is objects.c, part of the libspopc library sources
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "libspopc.h"

/***************************************
 * high-level methods for a simple mua *
 ***************************************/

DLLIMPORT char* popbegin(const char* servername, const char* user, const char* pass, popsession** sp){
/* prepares, connect and get lists of messages stored on pop server */
/* you must give a valid servername, user and pass */
/* returns an error message if a problem occurs, else NULL */
	char* resp=NULL;
	char* err=NULL;
	char *hostname, *ptr_port;
	int nport;
	popsession* s = NULL;

	if(!(servername && user && pass)){
		err=strdup("popbegin: some NULL args !");
		goto error;
	}
	s=(popsession*)malloc(sizeof(popsession));
	if(!s){
		err=strdup("popbegin.malloc: failed\n");
		goto error;
	}
	/* basic default construction */
	s->sock=BAD_SOCK;
	s->connection=NULL;
	s->server=NULL;
	s->list=NULL;
	s->uidl=NULL;
	s->bytes=-1;
	s->last=-1;
	s->num=-1;
	s->del=0; /* no deletion (by default) at this time */
	s->sync=1; /* this is sync'ed at this time (no cnx yet) */

	s->server=(struct hostent*)malloc(sizeof(struct hostent));
	if(!(s->server)){
		err=strdup("popbegin.malloc: failed\n");
		goto error;
	}
	s->connection=(struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	if(!(s->connection)){
		err=strdup("popbegin.malloc: failed\n");
		goto error;
	}
	hostname = strdup (servername);
	if (!hostname) {
		err=strdup("popbegin.strdup: failed\n");  /* ahem... */
		goto error;
	}
	ptr_port = strchr (hostname, ':');
	if (!ptr_port)
		nport = 110;
	else {
		*ptr_port = 0;
		nport = (int) strtoul (++ptr_port, NULL, 10);
		if (!nport)
			nport = 110;
	}
	s->sock=pop3_prepare(hostname,nport,s->connection,s->server);
	free (hostname);
	if(s->sock==BAD_SOCK){
		err=strdup("popbegin.pop3_prepare: failed\n");
		goto error;
	}
	resp=pop3_connect(s->sock,s->connection);
	if(!resp){
		err=strdup("popbegin.pop3_connect: failed\n");
		goto error;
	}
	free(resp);
	resp=pop3_user(s->sock,user);
	if((!resp) || pop3_error(resp)){
		err=resp?resp:strdup("popbegin.pop3_user: failed\n");
		goto error;
	}
	free(resp);
	resp=pop3_pass(s->sock,pass);
	if((!resp) || pop3_error(resp)){
		err=resp?resp:strdup("popbegin.pop3_pass: failed\n");
		goto error;
	}
	free(resp);
	resp=pop3_stat(s->sock);
	if((!resp) || pop3_error(resp)){
		err=resp?resp:strdup("popbegin.pop3_stat: failed\n");
		goto error;
	}
	s->bytes=stat2bytes(resp);
	s->num=stat2num(resp);
	s->last=stat2num(resp); /* safe here: we did not delete anything */
	free(resp);
	resp=pop3_list(s->sock,0);
	if((!resp) || pop3_error(resp)){
		err=resp?resp:strdup("popbegin.pop3_list: failed\n");
		goto error;
	}
	s->list=list2array(resp);
	free(resp);
	resp=pop3_uidl(s->sock,0);
	if((!resp) || pop3_error(resp)){
		err=resp?resp:strdup("popbegin.pop3_uidl: failed\n");
		goto error;
	}
	s->uidl=uidl2array(resp);
	s->del=0;
	(*sp)=s;
	free(resp);
	return (NULL);

error:
	if (s) {
		if (s->sock != BAD_SOCK) 
		{
			pop3_disconnect (s->sock, s->server);
			free(s->server);
		}
		free (s->connection);
		free(s);
	}
	return (err);
}

DLLIMPORT char* popgethead(popsession* session, int id){
/* returns the header of a message id between 1 and last or NULL if bad id or error */
	char* resp;
	char* msg;
	if(!session){
		return(NULL);
	}
	if((id > session->last) || (id < 1)){
		return(NULL);
	}
	resp=pop3_top(session->sock,id,0); /* 0 means only header */
	if((!resp) || pop3_error(resp)){
		if(resp){
			free(resp);
		}
		return(NULL);
	}
	msg=retr2msg(resp);
	if(!msg){
		msg=resp;
	}else{
		free(resp);
	}
	return(msg);
}

DLLIMPORT char* popgetmsg(popsession* session, int id){
/* returns a message id between 1 to last or NULL if bad id or error */
	char* resp=NULL;
	char* msg=NULL;

	if(!session){
		return(NULL);
	}
	if((id > session->last) || (id < 1)){
		return(NULL);
	}
	resp=pop3_retr(session->sock,id);
	if((!resp) || pop3_error(resp)){
		free(resp);
		return(NULL);
	}
	msg=retr2msg(resp);
	if(!msg){
		msg=resp;
	}else{
		free(resp);
	}
	if(session->del){
		popdelmsg(session, id);
	}
	return(msg);	
}

DLLIMPORT int popdelmsg(popsession* session, int id){
/* deletes a message 'id' on pop server */
/* returns -1 if no deletion (server error), 0 else */
/* sets session->sync to 0 if last id unsync-ed , 1 if OK */
	char* resp;
	int ret;
	if(!session){
		return -1;
	}
	if((id > session->last) || (id < 1)){
		return -1;
	}
	/* actualy delete the email */
	resp=pop3_dele(session->sock,id);
	if((!resp) || pop3_error(resp)){
		free(resp);
		return -1;
	}
	free(resp);
	resp=pop3_stat(session->sock);
	if((!resp) || pop3_error(resp)){
		session->sync=0;
		return -1;
	}
	ret = stat2bytes(resp);
	if (ret < 0)
		session->sync=0;
	
	else
		session->bytes=ret;
	ret = stat2num(resp);
	if (ret < 0)
		session->sync=0;
	else
		session->num=ret;
	free(resp);
	ret=poplast(session); /* check actual last id */
	if (ret < 0){
		session->sync=0;
		return -1;
	}
	session->last=ret;

	/* no more message of this id*/
	session->list[id]=0;
	free(session->uidl[id]);
	session->uidl[id]=NULL;
	session->sync=1;
	return 0;
}

DLLIMPORT int popcancel(popsession* session){
/* cancel all previous deletions on pop server */
/* returns -1 if server error, 0 else */
	char* resp;
	int ret;

	if(!session){
		return(-1);
	}
	resp=pop3_rset(session->sock);
	if((!resp) || pop3_error(resp)){
		free(resp);
		return(-1);
	}
	free(resp);
	resp=pop3_stat(session->sock);
	if((!resp) || pop3_error(resp)){
		session->sync=0;
		return(-1);
	}
	/* sync number of bytes */
	ret = stat2bytes(resp);
	if (ret < 0)
		session->sync=0;
	else
		session->bytes=ret;
	/* sync number of messages */
	ret = stat2num(resp);
	if (ret < 0)
		session->sync=0;
	else
		session->num=ret;
	/* sync last mail id */
	/* safe to use stat2num here since we cancel */
	ret = stat2num(resp);
	if (ret < 0)
		session->sync=0;
	else
		session->last=ret;
	free(resp);
	resp=pop3_list(session->sock,0);
	if((!resp) || pop3_error(resp)){
		session->sync=0;
		return -1;
	}
	freelistarray(session->list);
	session->list=list2array(resp);
	free(resp);
	resp=pop3_uidl(session->sock,0);
	if((!resp) || pop3_error(resp)){
		session->sync=0;
		return -1;
	}
	freeuidlarray(session->uidl);
	session->uidl=uidl2array(resp);
	free(resp);
	session->sync=1;
	return 0;
}

DLLIMPORT void popend(popsession* session){
/* quit and destroys pop session */
	int i;
	char* resp;

	if(!session)
		return; 
	resp=pop3_quit(session->sock);
	free(resp);
	pop3_disconnect(session->sock, session->server);
	free(session->server);
	free(session->connection);
	free(session->list);
	for(i=0;i<=session->last;i++){
		free(session->uidl[i]);
	}
	free(session->uidl);
	free(session);
	return;
}

DLLIMPORT int popnum(popsession* session){
/* returns the number of current (non-deleted) messages */
	char* r=NULL;
	int n;

	if(!session)
		return -1; 
	r=pop3_stat(session->sock);
	if(pop3_error(r)) {
		free(r);
		return -1; /* error (timeout, etc..) */
	}
	n=stat2num(r);
	free(r);
	return(n);
}

DLLIMPORT int poplast(popsession* session){
/* return the id of the last downloadable (non-deleted) message */
/* thanks to Francesco Gennai <francesco.gennai@isti.cnr.it> */
	int i=0;
	char* lines=NULL;
	char* p=NULL;
	
	if(!session)
		return -1; 
	lines=pop3_list(session->sock, 0); /* 0 means 'all' */
	if(pop3_error(lines)) {
		free(lines);
		return -1; /* error (timeout, etc..) */
	}
	p=lines;
	p=nextline(p); /* skip +OK */
	while (p[0]!='.'){ /* dot means list terminated */
		i = atoi(p); /* first number is the id */
		p=nextline(p);
	}
	/* i is now the greatest id */
	free(lines);
	return(i);
}

DLLIMPORT int popchkmsg(popsession* session, int id) {
/* check if the message 'id' is accessible in the current session */
/* thanks to Francesco Gennai <francesco.gennai@isti.cnr.it> */
	if (popmsguid(session, id)) return 1; /* anything but 0 */
	return 0;
}

/* re-synchronize the session object from the server */
DLLIMPORT int popsync(popsession* session) {
	char* resp;
	int ret;

	if(!session){
		return(-1);
	}
	resp=pop3_stat(session->sock);
	if((!resp) || pop3_error(resp)){
		session->sync=0;
		return(-1);
	}
	session->bytes=stat2bytes(resp);
	session->num=stat2num(resp);
	ret=poplast(session); /* check actual last id */
	if (ret < 0){
		session->sync=0;
		return(-1);
	}
	session->last=ret;
	free(resp);
	resp=pop3_list(session->sock,0);
	if((!resp) || pop3_error(resp)){
		session->sync=0;
		return(-1);
	}
	freelistarray(session->list);
	session->list=list2array(resp);
	free(resp);
	resp=pop3_uidl(session->sock,0);
	if((!resp) || pop3_error(resp)){
		session->sync=0;
		return(-1);
	}
	freeuidlarray(session->uidl);
	session->uidl=uidl2array(resp);
	free(resp);
	session->sync=1;
	return(0);
}

