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
//#include <winsock.h>
#include <winsock2.h>
#include <io.h>

#include "../libspopc.h"

int main(int argc,char** argv){

	int mysock;
	char myservername[64];
	char username[64];
	char password[64];

	struct hostent myserver;
	struct sockaddr_in myconnection;

	char* srvdata=NULL;
	char* mymessage=NULL;
	int* mylist;
	char** myuidl;
	int i=0,last;

	if(argc<4){
		printf("Usage: %s servername username password\n",argv[0]);
		exit(0);
	}
	strcpy(myservername,argv[1]);
	strcpy(username,argv[2]);
	strcpy(password,argv[3]);

	mysock=pop3_prepare(myservername,0,&myconnection,&myserver);
	if(-1==mysock){
		printf("check your network...\n");
		exit(1);
	}
	srvdata=pop3_connect(mysock,&myconnection);
	free(srvdata);
	srvdata=pop3_user(mysock,username);
	free(srvdata);
	srvdata=pop3_pass(mysock,password);
	free(srvdata);
	printf("\n---\nTEST STAT\n\n");
	srvdata=pop3_stat(mysock);
	printf("stat: %d mail(s)\n",stat2last(srvdata));
	printf("stat: %d bytes\n",stat2bytes(srvdata));
	free(srvdata);
	printf("\n---\nTEST LIST\n\n");
	srvdata=pop3_list(mysock,0);
	mylist=list2array(srvdata);
	free(srvdata);
	printf("list:\n");
	for(i=1;i<=(mylist?mylist[0]:0);i++){
		printf(" %d: %d bytes\n",i,mylist?mylist[i]:0);
	}
	i--;
	free(mylist);mylist=NULL;
	printf("\n---\nTEST TOP\n\n");
	srvdata=pop3_top(mysock,i,0);
	mymessage=retr2msg(srvdata);
	printf("last email's header is;\n");
	printf("%s",mymessage);
	free(mymessage);
	free(srvdata);
	printf("\n---\nTEST RETR\n\n");
	srvdata=pop3_retr(mysock,i);
	mymessage=retr2msg(srvdata);
	free(srvdata);
	printf("last mail is %d:\n",i);
	printf("%s",mymessage);
	free(mymessage);
	printf("\n---\nTEST UIDL\n\n");
	srvdata=pop3_uidl(mysock,0);
	myuidl=uidl2array(srvdata);
	free(srvdata);
	printf("uidl: %s signatures\n",myuidl?myuidl[0]:"0");
	for(i=1;i<=(myuidl?atoi(myuidl[0]):0);i++){
		printf(" %d: %s\n",i,myuidl?myuidl[i]:0);
	}
	last=atoi(myuidl[0]);
	for(i=0;i<=last;i++){
		free(myuidl[i]);
	}
	free(myuidl);
	pop3_quit(mysock);
	pop3_disconnect(mysock);
	exit(0);
}

