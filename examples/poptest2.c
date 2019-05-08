/* this is part of the libspopc library sources
 * copyright © 2002 Benoit Rouits <brouits@free.fr>
 * released under the terms of GNU LGPL
 * (GNU Lesser General Public Licence).
 * libspopc offers simple API for a pop3 client (MTA).
 * See RFC 1725 for pop3 specifications.
 * more information on http://brouits.free.fr/libspopc/
 */

/* Using objects-level methods of libspopc is very
 * EASY but doesn't allow you to do everything you
 * want. It just makes the useful tasks
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

	char myservername[64];
	char username[64];
	char password[64];

	popsession* mysession;
	char* err=NULL;
	int i=0,s=0;
	int max=0,min=999999999;

	if(argc<4){
		printf("Usage: %s servername[:port] username password\n",argv[0]);
		exit(0);
	}
	strcpy(myservername,argv[1]);
	strcpy(username,argv[2]);
	strncpy(argv[2], "****************", strlen(argv[2]));
	strcpy(password,argv[3]);
	strncpy(argv[3], "****************", strlen(argv[3]));

	libspopc_init();

	err=popbegin(myservername,username,password,&mysession);
	if(err){
		printf("%s",err);
		free(err);
		exit(1);
	}
	printf("you've got %d messages !\n",mysession->num);
	printf("your pop server holds %d bytes of data.\n",mysession->bytes);
	for(i=1;i<=mysession->last;i++){
		max=(popmsgsize(mysession,i)>max)?popmsgsize(mysession,i):max;
		min=(popmsgsize(mysession,i)<min)?popmsgsize(mysession,i):min;
		s=(popmsgsize(mysession,i)==min)?i:s;
	}
	printf("your biggest message is of %d bytes.\n",max);
	printf("your smallest message is of %d bytes.\n",min);
	printf("the smallest message's signature is: %s\n",popmsguid(mysession,s));
	popsetdel(mysession);
	for(i=1;i<=mysession->last;i++){
		err=popgetmsg(mysession,i);
		printf("message %d:\n%s",i,err);
		free(err);err=NULL;
	}
	popcancel(mysession);
	popend(mysession);

	libspopc_clean();

	exit(0);
}

