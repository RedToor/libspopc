/* this is mutex.c file, part of the libspopc library sources
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
#ifdef USE_SEM
 #include <semaphore.h>
#else
 #ifdef USE_PMUTEX
  #include <pthread.h>
 #endif
#endif

#include "mutex.h"

/* initializes a thread-wide mutex */
int mutex_init(mutex* id)
{
#ifdef USE_SEM
	return sem_init(id, 0, 1);
#else
 #ifdef USE_PMUTEX
	return pthread_mutex_init(id, NULL);
 #else
	/* dumb and pointelss code */
	(*id) = 0;
	return 0;
 #endif
#endif
}

/* acquire the given mutex to enter in a critical section */
int mutex_in (mutex* id)
{
#ifdef USE_SEM
	return sem_wait(id);
	/* this is a blocking call */
#else
 #ifdef USE_PMUTEX
	return pthread_mutex_lock(id);
 #else
	/* dumb and pointelss code */
	++(*id);
	return 0;
 #endif
#endif
}

/* quit the critical section, by releasing the given mutex */
int mutex_out (mutex* id)
{
#ifdef USE_SEM
	return sem_post(id);
#else
 #ifdef USE_PMUTEX
	return pthread_mutex_unlock(id);
 #else
	/* dumb and pointelss code */
	--(*id);
	return 0;
 #endif
#endif
}

/* clean given mutex, thus re-initable by mutex_init() */
int mutex_clean(mutex* id)
{
#ifdef USE_SEM
	return sem_destroy(id);
#else
 #ifdef USE_PMUTEX
	return pthread_mutex_destroy(id);
 #else
	/* dumb and pointelss code */
	(*id) = -1;
	return 0;
 #endif
#endif
}

