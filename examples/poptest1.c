/* this is part of the libspopc library sources
 * copyright © 2002 Benoit Rouits <brouits@free.fr>
 * released under the terms of GNU LGPL
 * (GNU Lesser General Public Licence).
 * libspopc offers simple API for a pop3 client (MTA).
 * See RFC 1725 for pop3 specifications.
 * more information on http://brouits.free.fr/libspopc/
 */
#include <string.h> /* use of strcpy() */
#include <stdio.h> /* use of printf() */
#include <stdlib.h> /* use of exit() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "../libspopc.h"

int main(int argc,char** argv){

	pop3sock_t mysock;
	char myservername[64];
	char username[64];
	char password[64];

	struct hostent myserver;
	struct sockaddr_in myconnection;

	char* srvdata=NULL;
	char* mymessage=NULL;
	int* mylist;
	char** myuidl;
	char *ptr_port;
	int nport;
	int i=0, last; /* 'i'ndex, 'last' cell of an array */

#ifdef USE_SSL
	if(argc<4){
		printf("Usage: %s servername[:port] username password [ssl-cert]\n",argv[0]);
		exit(0);
	} else if (argc>4) pop3_cert_setup(argv[4]);
#else
	if(argc<4){
		printf("Usage: %s servername[:port] username password\n",argv[0]);
		exit(0);
	}
#endif


	strcpy(myservername,argv[1]);
	strcpy(username,argv[2]);
	strncpy(argv[2], "****************", strlen(argv[2]));
	strcpy(password,argv[3]);
	strncpy(argv[3], "****************", strlen(argv[3]));

	ptr_port = strchr (myservername, ':');
	if (ptr_port) {
		*ptr_port = 0;
		nport = (int) strtoul (++ptr_port, NULL, 10);
		if (!nport) nport = 110;
	} else nport = 110;

	libspopc_init();

	mysock=pop3_prepare(myservername,nport,&myconnection,&myserver);
	if(BAD_SOCK==mysock){
		printf("check your network...\n");
		exit(1);
	}
	srvdata=pop3_connect(mysock,&myconnection);
	if (!srvdata){
		printf("connection problem...\n");
		exit(1);
	}
	free(srvdata);
	srvdata=pop3_user(mysock,username);
	free(srvdata);
	srvdata=pop3_pass(mysock,password);
	free(srvdata);

	printf("\n---\nTEST STAT\n\n");
	srvdata=pop3_stat(mysock);
	printf("stat: %d mail(s)\n",stat2num(srvdata));
	printf("stat: %d bytes\n",stat2bytes(srvdata));
	free(srvdata);

	printf("\n---\nTEST LIST\n\n");
	srvdata=pop3_list(mysock,0);
	mylist=list2array(srvdata);
	free(srvdata);
	printf("list:\n");
	last=mylist?mylist[0]:0;
	for(i=1;i<=last;i++){
		printf(" %d: %d bytes\n",i,mylist?mylist[i]:0);
	}
	i--;/* i is the number of messages */
	free(mylist);mylist=NULL;

	printf("\n---\nTEST TOP\n\n");
	srvdata=pop3_top(mysock,i,0);
	mymessage=retr2msg(srvdata);
	printf("last email's header is;\n");
	printf("%s",mymessage);
	free(mymessage);
	free(srvdata);

	printf("\n---\nTEST RETR\n\n");
	while(i){
		srvdata=pop3_retr(mysock,i);
		mymessage=retr2msg(srvdata);
		free(srvdata);
		printf("mail is %d:\n",i);
		printf("%s",mymessage);
		free(mymessage);mymessage=NULL;
		i--;
	}

	printf("\n---TEST DELE\n\n");
	for(i=1; i<=last; i++){
		srvdata=pop3_dele(mysock, i);
		printf("deleted %d: %s\n",i,srvdata);
		free(srvdata);srvdata=NULL;
	}

	printf("\n---TEST RSET\n\n");
	srvdata=pop3_rset(mysock);
	printf("canceled deletion: %s\n", srvdata);
	free(srvdata);srvdata=NULL;

	printf("\n---\nTEST UIDL\n\n");
	srvdata=pop3_uidl(mysock,0);
	myuidl=uidl2array(srvdata);
	free(srvdata); srvdata=NULL;
	if (myuidl) {
		printf("uidl: %s signatures\n",myuidl?myuidl[0]:"0");
		for(i=1;i<=(myuidl?atoi(myuidl[0]):0);i++)
			printf(" %d: %s\n",i,myuidl?myuidl[i]:0);
		last=atoi(myuidl[0]);
		for(i=0;i<=last;i++){
			free(myuidl[i]); myuidl[i]=NULL;
		}
	}
	free(myuidl); myuidl=NULL;

	srvdata=pop3_quit(mysock);
	free(srvdata); srvdata=NULL;
	pop3_disconnect(mysock, &myserver);

	libspopc_clean();

	exit(0);
}

