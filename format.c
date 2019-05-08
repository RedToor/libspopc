/* this is format.c file, part of the libspopc library sources
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
#include <stdio.h>
#include "libspopc.h"

/************************************************************
 * libspopc functions to format pop data returned by server *
 ************************************************************/

DLLIMPORT char* nextline(char* string){
/* returns a pointer to the next line of given string */
	while(((*string) != '\n')&&((*string) != '\0')){
		string++;
	}
	if(!(*string)){
		return (NULL);
	}
	return(string+1);
}

DLLIMPORT char* retr2msg(char* data){
/* retruns formatted mail from a pop RETR X query */
/* should only be called on data returned by pop3_retr() */
	char* msg=NULL;
	char* cur;

	if((!data)||pop3_error(data)){
		return(NULL);/* any suggestions ? */
	}
	while (( data != NULL ) && (strncmp("+OK", data, 3) != 0)){
		data=nextline(data); /* skip ...\n before +OK */
	}
	if(( data != NULL ) && (strncmp("+OK", data, 3) == 0)) {
		data=nextline(data); /* skip +OK ...\n */
	}
	msg=data?(char*)malloc(strlen(data)):NULL;
	if(!msg){
		return(NULL);
	}
	cur=msg;
	while(*data){
		if(DOTBEGIN(data)){
			(*cur)=(*data); /* keep \n */
			data+=2;        /* skip first dot */
			cur++;
		}else{
			(*cur)=(*data);
			cur++;data++;
		}
	}
	(*(cur-2))='\0'; /* erase \r\n */
	return(msg);
}

DLLIMPORT void freemsg(char* msg){
	free(msg);
}

DLLIMPORT int* list2array(char* poplist){
/* returns an int array of sizes of messages from a LIST pop query */
/* array[0] holds id of the array's element */
/* should only be called on data received by a pop3_list() request */
	int* array=NULL;
	int len,size;
	int id=0;
	char* cur;

	if((!poplist)||pop3_error(poplist)){
		return(NULL); /* any suggestions ? */
	}
	if(!dotline(poplist)){/* if simple element list */
		/* one should't use this function for simple element... */
		/* one should better use listi2size() */
		/* skip '+OK ': look for first mail int id */
		for(cur=poplist;(*cur<'0')||(*cur>'9');cur++);
		/* not dot line here */
		sscanf(cur,"%d %d\n",&id,&size);
		array=(int*)malloc((id+1)*sizeof(int));
		memset(array,0,(id+1)*sizeof(int));
		array[0]=id;
		array[id]=size;
		return(array);
	}
	/* else this is a true list */
	/* skip '+OK\r\n' :*/
	for(cur=poplist;(*cur!='.')&&(*cur!='\n'); cur++);
	cur ++; /* one more time to get behind '\n' */
	len=1; /* array len */
	while((*cur)!='.'){
		sscanf(cur,"%d %d\n",&id,&size);
		while(id > len){ /* pad array while id > len */
			len++;
			array=(int*)realloc(array,len*sizeof(int));
			array[len-1]=0; /* no mail */
		}
		len++;
		array=(int*)realloc(array,len*sizeof(int));
		array[id]=size;
		cur=nextline(cur);
	}
	if(id){
		array[0]=id; /* last id */
	}else{ /* no mail */
		array=(int*)realloc(array, 1*sizeof(int));
		array[0]=0;
	}
	return(array);
}

DLLIMPORT void freelistarray(int* array){
/* free array allocated by list2array() */
	free(array);
}

DLLIMPORT int listi2size(char* resp){
/* grep the given size (in bytes) in resp after a pop3_list(sock,ID) request */
	int i;
	int r;

	if(pop3_error(resp)){
			return(0); /* no message ! */
	}
	r=sscanf(resp+5,"%d\n",&i);/* skip '+OK ' should be safer FIXME */
	if(!r){ /* special case when no messages on server */
		return(0);
	}
	return(i);
}

DLLIMPORT int stat2num(char* resp){
/* returns the num of retrievable (non-deleted) messages */
/* should only be called just after a pop3_stat() request */
	int n,s,r;

	if((!resp)||pop3_error(resp)){
		return(-1);
	}
	r=sscanf(resp+4,"%d %d\n",&n,&s); /* with skip '+OK ' */
	if (r == 2)
		return n;
	else
		return -1;
}

DLLIMPORT int stat2bytes(char* resp){
/* returns the sumsize in bytes of all stored messages on server */
/* should only be called just after a pop3_stat() request */
	int n,s,r;

	if((!resp)||pop3_error(resp)){
		return(-1);
	}
	r=sscanf(resp+4,"%d %d\n",&n,&s); /* skip '+OK ' */
	if (r == 2)
		return(s);
	else
		return -1;
}

DLLIMPORT char** uidl2array(char* resp){
/* returns an array of unique strings for each message id */
/* array[0] gives array's last id */
/* should only be called just after a pop3_uidl() request */
	char** array=NULL;
	int l,i=0; /* l is array lenth, i is id of msg */
	char s[POPBUF]; /* temp signature string : sig theorically <=512B  */
	char* cur;

	if((!resp)||pop3_error(resp)){
		return(NULL); /* any suggestions ? */
	}
	if(!dotline(resp)){ /* simple element uidl */
		/* one should not use this function for simple element */
		/* one would better use uidli2sig() */
		/* skip '+OK ': look for first mail int id */
		for(cur=resp;(*cur<'0')||(*cur>'9');cur++);
		/* no dot line here */
		sscanf(cur,"%d %s\n",&i,s);
		array=(char**)malloc((i+1)*sizeof(char*));
		memset(array,0,(i+1)*sizeof(char*));
		array[0]=(char*)malloc(POPBUF); /* up to 512B */
		snprintf(array[0],POPBUF,"%d",i);
		array[i]=strdup(s);
		return(array);
	}
	/* else this is a true uid list */
	/* skip '+OK\r\n : look for first mail integer id */
	for(cur=resp;(*cur!='.')&&(*cur!='\n'); cur++);
	cur ++; /* one more time to get behind '\n' */
	l=1; /* array len */
	while((*cur)!='.'){
		sscanf(cur,"%d %s\n",&i,s);
		while(i > l){ /* pad array while id > len */
			l++;
			array=(char**)realloc(array,l*sizeof(char*));
#if 0
			array[l-1]=(char*)malloc(sizeof(char));
			array[l-1]='\0';
#else
			array[l-1]=NULL; /* for consitency with popchkmsg() */
#endif

		}
		l++;
		array=(char**)realloc(array,l*sizeof(char*));
		array[i]=(char*)malloc(POPBUF); /* up to 512B */
		array[i]=strncpy(array[i],s,POPBUF); 
		cur=nextline(cur);
	}
	if(i){ /* i is now the last message id met in this session */
		array[0]=(char*)malloc(9); /* up to 99999999 msg uids FIXME */
		snprintf(array[0],9,"%d",i);
		/* contains the id of the last msg (char*) (also tab length) */
	}else{ /* zero message */
		array=(char**)malloc(1*sizeof(char*));
		array[0]=(char*)malloc(2*sizeof(char)); /* 2 because of '\0' */
		snprintf(array[0],2,"%d",0);
	}
	return(array);
}

DLLIMPORT void freeuidlarray(char** array){
/* free the array allocated by uidl2array() */
	int i,last;

	last= atoi(array[0]);
	for (i=1;i<=last;i++){
		free(array[i]);
	}
	free(array[0]);
	free(array);
}

DLLIMPORT char* uidli2sig(char* resp){
/* greps signature from server resp */
/* should only be called after a pop3_uidl(sock,ID) */
	char* sig=NULL;

	if(pop3_error(resp)){
		return(NULL);/* no message ! */
	}
	sig=strdup(resp+5); /* skip '+OK ID' should be safer, FIXME */
	if(sig[1]=='.'){/* special case when no messages on the server */
		free(sig);
		return(NULL);
	}
	return(sig);
}

