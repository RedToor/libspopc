/* this is parsing.c file, part of the libspopc library sources
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

#include <string.h>
#include "libspopc.h"

int dotline(char* buf){
/* returns 1 if buf contains a "\r\n.\r\n" substring */
	char* ptr;

	if(!buf){
		return(0);
	}
	ptr = buf;
	while((ptr = strchr(ptr,'\r'))){
		/* RFC1939 specifies that the termination line is "CRLF.CRLF" */
		if((ptr[1] == '\n') && (ptr[2] == '.') && (ptr[3] == '\r') && (ptr[4] == '\n')){
			return(1);
		}
		ptr++;
	}
	return(0);
}

DLLIMPORT int pop3_error(char* string){
/* returns 1 if string is a pop server error reply (i.e : -ERR ...) or NULL, 0 else */
	return(string?(!strncmp(string,"-ERR",(size_t)4)):1);
}
