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
#include <ctype.h> /* use of toupper() */
#include <stdio.h> /* use of printf() */
#include <stdlib.h> /* use of exit() */
/* #include <unistd.h> */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "../libspopc.h"
/* 
 * Butcher's hook for a look
 */

char* strstrup(const char* string, const char* word){
	char* upstring;
	char* upword;
	unsigned int i;
	char* match;

	/* preliminaries */
	upstring = strdup(string);
	upword = strdup(word);
	for (i=0; upstring[i]!='\0'; i++){
		upstring[i]=toupper(upstring[i]);
	}
	for (i=0; upword[i]!='\0'; i++){
		upword[i]=toupper(upword[i]);
	}

	/* match upword in upstring */
	match=strstr(upstring, upword);
	if (match!=NULL){
		free(upstring);
		free(upword);
		/* return pointer to 'match' minus upstring offset */
		return (string - upstring + match);
	}
	/* else */
	free(upstring);
	free(upword);
	return(NULL);
}

char* getlinewhere(const char* header, const char* word){
	char *begin, *end, *line;

	begin=strstrup(header, word);
	if(begin==NULL){
		return NULL;
	}
	end=strstr(begin, "\n");
	if (end==NULL) {
		free(begin);
		return NULL;
	}
	line=(char*)malloc(((end-begin)*sizeof(char))+1);
	strncpy(line,begin,(end-begin));
	line[end-begin]='\0';
	return line;
}


void freeline(char* line){
	free(line);
}

int main(int argc,char** argv){

	char myservername[64];
	char username[64];
	char password[64];


	popsession* mysession;
	char* err=NULL;
	char* subject=NULL;
	char* from=NULL;
	int i=0, ret=0, answer=9999999;

	if(argc<4){
		printf("Usage: %s servername[:port] username password\n",argv[0]);
		exit(0);
	}
	strcpy(myservername,argv[1]);
	strcpy(username,argv[2]);
	strncpy(argv[2],"****************",strlen(argv[2]));
	strcpy(password,argv[3]);
	strncpy(argv[3],"****************",strlen(argv[3]));

	libspopc_init();

	err=popbegin(myservername,username,password,&mysession);
	if(err){
		printf("%s",err);
		free(err);
		exit(1);
	}
	printf("Now having %d messages. The last id is %d\n",mysession->num,mysession->last);
	if(mysession->last > 0){
		int ok;

		printf("Holds %d bytes of data.\n",popbytes(mysession));
		printf("Press [Enter] to list the messages headers\n");
		getchar();
		/* listing */
		for(i=1;i<=mysession->last;i++){
			err=popgethead(mysession,i);
			if (pop3_error(err)) {
				printf("\nError on message %d: %s: timeout reached?\n\n", i, err);
				free(err);
				continue;
			}
			from=getlinewhere(err, "From:");
			subject=getlinewhere(err, "Subject:");
			printf("Message %d:\n\t%s\n\t%s\n\t%d chars\n",i,from,subject, popmsgsize(mysession,i));
			freeline(from);
			freeline(subject);
			free(err);
		}
		printf("Now having %d messages. The last id is %d\n",mysession->num,mysession->last);
		/* deletion */
		printf("WARNING: POSSIBILITY TO LOOSE EMAILS FROM NOW: RESPOND 0 TO AVOID THAT\n");
		while(answer != 0){
			printf("Which mail do you want to delete ? (0 means none)\n");
			ok = scanf("%d", &answer);
			if (!ok) { /* defaults to 0 */
				char c;
				ok = scanf("%c", &c);
				answer = 0;
			}
			if(answer)
				ret = popdelmsg(mysession, answer);
		}
		if(ret) printf("Got some errors (timeout too short?)\n");
		printf("Now having %d messages. The last id is %d\n",mysession->num,mysession->last);
		printf("Enter '0' to cancel all deletion: ");
		ok = scanf("%d", &answer);
		if (!ok) { /* defaults to 0 */
			char c;
			ok = scanf("%c", &c);
			answer = 0;
		}
		if (!answer)
			ret = popcancel(mysession);
		if(ret) printf("Got some errors (timeout too short?)\n");
		printf("Now having %d messages. The last id is %d\n",mysession->num,mysession->last);
	}
	popend(mysession);

	libspopc_clean();

	exit(0);
}

