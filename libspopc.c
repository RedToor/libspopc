/* this is libspopc.c file, part of the libspopc library sources
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

#include "libspopc.h"
#include "mutex.h"

static int LIBSPOPC_UP = 0;

/* all mutexes are defined here */
mutex GETHOSTBYNAME;

/* ALL THESE FUNCTIONS MUST BE CALLED ONCE AT A TIME */

/* initialize libspopc, mainly for thread safety */
DLLIMPORT int libspopc_init(void)
{
	if (!LIBSPOPC_UP)
	{
		/* initialize all mutexes */
		mutex_init(&GETHOSTBYNAME);
	}
	LIBSPOPC_UP = 1;
	return 0; /* FIXME */
}

/* clean libspopc, thus re-initable by libspopc_init() */
DLLIMPORT int libspopc_clean(void)
{
	if(LIBSPOPC_UP)
	{
		/* clean all mutexes */
		mutex_clean(&GETHOSTBYNAME);
	}
	LIBSPOPC_UP = 0;
	return 0; /* FIXME */
}

