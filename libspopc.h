/* this is libspopc.h file, part of the libspopc library sources
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

#ifndef LIBSPOPC_H
#define LIBSPOPC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#ifdef WIN32
#include <winsock.h>
#if BUILDING_DLL
# define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
# define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define DLLIMPORT
#endif

/* thread-safe version of libspopc (>0.8) */

/* call this function before to use any routine from libspopc */
DLLIMPORT int libspopc_init(void);

/* call this function when you do not use anymore libspopc routines */
DLLIMPORT int libspopc_clean(void);

#ifdef USE_SSL

#include <openssl/ssl.h>

/******************************************************************************
 * If compiled with SSL support, the low-level functions will act on a 
 * "pop3sock" structure, which contains the socket, SSL instance and context 
 * for the connection. This structure is dynamically allocated and initialized 
 * when you do pop3_prepare() or popbegin(), and is cleaned-up and destroyed in 
 * pop3_disconnect() or popend().
 ******************************************************************************/

typedef struct {
	int sock;
	SSL *ssl;
	SSL_CTX *ctx;
} pop3sock;

typedef pop3sock* pop3sock_t;

#define BAD_SOCK NULL

/******************************************************************************
 * Use pop3_cert_setup() to specify the location of your SSL certificate
 * bundle. If it is not set (or set to NULL), SSL connections can be still be 
 * made, but certificates will not be verified!
 * This function sets a global variable, and should be called before 
 * pop3_prepare() or popbegin() if you want to verify certs.
 * 
 * Hint: If you have a recent version of curl/libcurl installed, you can try 
 * setting this to the output of:  "curl-config --ca"
 ******************************************************************************/
DLLIMPORT void pop3_cert_setup(const char *certfile);

/******************************************************************************
 * pop3_ssl_never() disable the use of SSL on any port, even 995.
 * pop3_ssl_auto() enable the use of SSL only on port 995. (default)
 * pop3_ssl_always() enable the use of SSL on any port, even 110.
 *
 * These functions set a library-wide variable, and should be called before
 * pop3_prepare() or popbegin() if you want to modify libspopc behaviour.
 * By default, not calling any of these, the behaviour of libspopc follows
 * the same as pop3_ssl_auto() for backward compatibility reason.
 ******************************************************************************/
DLLIMPORT void pop3_ssl_never(void);
DLLIMPORT void pop3_ssl_auto(void);
DLLIMPORT void pop3_ssl_always(void);

#else /* Non-SSL */

/******************************************************************************
 *  If the library is compiled without SSL, the "pop3sock_t" type is a simple
 *  integer (i.e. socket) and all the functions should work just as they did
 *  in previous libspopc versions (< 0.8).
 ******************************************************************************/

typedef int pop3sock_t;


#define BAD_SOCK -1

#endif /* SSL/Non-SSL */


/***************************************
 * low-level methods for a pop3 client *
 ***************************************/
#define SOCKET_TIMEOUT 15 /* seconds a socket can block read/write */
#define TCPBUFLEN 512 /* length of generic socket buffer */
#define POPBUF 513 /* length of the pop resp: 512 bytes (RFC1939) + '\0' */
/******************************************************************************
 * Be careful, using the low-level API is uncompliant with using the high     
 * level API. Here, you make your choice. If you don't know the pop3 protocol 
 * or what a socket is, it is then warmly recommended to use the *high-level* 
 * API if which is shown far below on this file.                              
 ******************************************************************************/

/**************
 * connecting *
 **************/

DLLIMPORT pop3sock_t pop3_prepare(const char* servername, const int port, struct sockaddr_in* connection, struct hostent* server);
/* prepares the pop session and returns a socket descriptor, or BAD_SOCK on error */

DLLIMPORT char* pop3_connect(pop3sock_t sock, struct sockaddr_in* connection);
/* connects to the server through the sock and returns server's welcome */

DLLIMPORT void pop3_disconnect(pop3sock_t sock, struct hostent* server);
/* close socket and free server info */


/****************
 * pop3 queries *
 ****************/

DLLIMPORT char* pop3_user(pop3sock_t sock, const char* name);
/* performs "USER" pop query and returns server's <=512 bytes resp */

DLLIMPORT char* pop3_pass(pop3sock_t sock, const char* pw);
/* performs "PASS" pop query and return server's <=512 bytes resp */

DLLIMPORT char* pop3_quit(pop3sock_t sock);
/* performs "QUIT" pop query and returns server's <=512 bytes resp */

DLLIMPORT char* pop3_stat(pop3sock_t sock);
/* performs "STAT" pop query and returns server's <=512 bytes resp */


DLLIMPORT char* pop3_list(pop3sock_t sock, int id);
/* performs a "LIST" pop query and returns server's (long) resp */

DLLIMPORT char* pop3_retr(pop3sock_t sock, int id);
/* performs a "RETR" pop query and returns server's (long) resp */

DLLIMPORT char* pop3_dele(pop3sock_t sock, int id);
/* performs a "DELE" pop query and returns server's <=512 bytes resp */

DLLIMPORT char* pop3_noop(pop3sock_t sock);
/* performs a "NOOP" pop query and returns server's <=512 bytes resp */

DLLIMPORT char* pop3_rset(pop3sock_t sock);
/* performs a "RSET" pop query and returns server's <=512 bytes resp */

DLLIMPORT char* pop3_top(pop3sock_t sock, int id, int lines);
/* performs a "TOP" pop query and returns server's (long) resp */

DLLIMPORT char* pop3_uidl(pop3sock_t sock, int id);
/* performs a "UIDL" pop query and returns server's (long) resp */

DLLIMPORT char* pop3_apop(pop3sock_t sock, const char* name, const char* digest);
/* performs a "APOP" secure pop query and returns server's <=512 bytes resp */


/*********************
 * parsing utilities *
 *********************/
#define DOTBEGIN(s) ((s)[0]=='\n'&&(s)[1]=='.')

int dotline(char* buf);
/* returns 1 if 'buf' contains a "\n.\n" or "\n.\0" or \r.(etc) substring
 * buf must be terminated by '\0' */

DLLIMPORT int pop3_error(char* string);
/* returns 1 on pop server error (i.e : -ERR ...) or NULL reply */

/************************************
 * reply re-formatting, after query *
 ************************************/
DLLIMPORT char* nextline(char* string);
/* returns a pointer to the next line of given string */

DLLIMPORT char* retr2msg(char* data);
/* returns formatted mail from a pop RETR X query
 * must only be called on data returned by pop3_retr() */

DLLIMPORT void freemsg(char* msg);
/* free the message received by reetr2msg */

DLLIMPORT int* list2array(char* poplist);
/* WARNING: must not be called after a mail deletion
 * returns an int array of sizes of messages from a LIST pop query
 * array[0] holds id of the array's last element
 * must only be called on data received by a pop3_list(sock,0) request */

DLLIMPORT void freelistarray(int* array);
/* free the message sizes array created by list2array */

DLLIMPORT int listi2size(char* resp);
/* grep the given size (in bytes) in resp after a pop3_list(sock,ID) request
 * do not use after a pop3_list(sock,0) ! */

DLLIMPORT int stat2num(char* resp);
/* returns the number of downloadable messages on pop server */

DLLIMPORT int stat2bytes(char* resp);
/* returns the sumsize in bytes of all stored messages on server
 * must only be called just after a pop3_stat() request */

DLLIMPORT char** uidl2array(char* resp);
/* WARNING: mus not be called after a mail deletion
 * returns an array of unique strings for each message id
 * array[0] gives array's last id
 * must only be called just after a pop3_uidl(sock,0) request */

DLLIMPORT void freeuidlarray(char** arrray);
/* free the uidl array created by uidl2array */

DLLIMPORT char* uidli2sig(char* resp);
/* grep the pop signature of *one* message signature reply
 * should only be called on data received by a pop3_uidl(sock,ID) request
 * do not use it after a pop3_uidl(sock,0) ! */


/***************************************************
 * high-level API for a SIMPLE MDA/MUA             *
 ***************************************************/

/******************************************************************************
 * This is the high-level API of libspopc and it is recommended to use it     
 * instead of the low-level one. This high-level API, in spite of its very    
 * 'teasing' name, just provides a *very simple* way to access and query a    
 * pop3 server with your e-mail client. This API handles pop3 in a very       
 * convenient manner for the non 'socket-aware' C developper.                 
 ******************************************************************************/

typedef struct{
	pop3sock_t sock;                 /* socket descriptor */
	struct sockaddr_in* connection;
	struct hostent* server;
	int* list;                       /* pop messages size list */
	char** uidl;                     /* pop messages signature list */
	int bytes;                       /* total stored (in bytes) on pop server */
	int last;                        /* last available message id */
	int num;                         /* number of available messages */
	int del;                         /* 0|1 flag to ask deletion of retrieved messages */
	int sync;                        /* session state: 1 = sync-ed; 0 = need sync */
} popsession;

#define popbytes(s) ((s)->bytes)
/* gives the total stored data size (in bytes) on the pop server
 * arg 's' is type 'popsession*'; 'result' is type 'int' */

#define popsetdel(s) ((s)->del=1)
/* asks the session to delete any retrieved messages on the server
 * arg 's' is type 'popsession*' */

#define popsetundel(s) ((s)->del=0)
/* asks the session to not delete any retrieved message on the server
 * arg 's' is type 'popsession*' */

#define popmsgsize(s,i) ((s)->list[(i)])
/* gives the size of message 'i' for session 's'
 * args are type 'session*'(s) and 'int'(i)
 * 'i' must *not* be 0 */

#define popmsguid(s,i) ((s)->uidl[(i)])
/* points to the 'char*' uid (unique signature) of 'int'(i) message id */

DLLIMPORT int poplast(popsession* session);
/* gives the id of the last message of the current session */

DLLIMPORT int popnum(popsession* session);
/* gives the current number of stored message. it is != to poplast() */

DLLIMPORT char* popbegin(const char* servername,const char* user, const char* pass, popsession** sp);
/* prepares, connect and get lists of messages stored on pop server
 * you must give a valid servername, user and pass
 * returns an error message if a problem occurs, else NULL */

DLLIMPORT char* popgethead(popsession* session, int id);
/* returns the header of a message (id between 1 and poplast()) or NULL on bad id */

DLLIMPORT char* popgetmsg(popsession* session, int id);
/* returns a message (id between 1 and poplast()) or NULL on bad id */

DLLIMPORT int popdelmsg(popsession* session, int id);
/* deletes message 'id' on pop server
 * returns -1 on server error, 0 else */

DLLIMPORT int popchkmsg(popsession* session, int id);
/* tells if a message is still accessible on the server (not deleted)
 * returns 1 of so, or 0 if message has been marked for deletion */

DLLIMPORT int popcancel(popsession* session);
/* cancels all previous deletion on pop server
 * returns -1 on server error, 0 else */

DLLIMPORT int popsync(popsession* session);
/* re-synchronize the session object from the server
 * need to be called if session->sync is 0
 * returns -1 on error, 0 else */

DLLIMPORT void popend(popsession* session);
/* quit and destroys pop session */

#ifdef __cplusplus
}
#endif
#endif
