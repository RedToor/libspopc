/* this is mutex.h file, part of the libspopc library sources
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

#ifndef MUTEX_H
#define MUTEX_H

#ifdef USE_SEM /* no posix thread mutex */
 #include <semaphore.h>
 typedef sem_t mutex; /* use unix rt semaphores from librt */
#else
 #ifdef USE_PMUTEX
  #include <pthread.h>
  typedef pthread_mutex_t mutex; /* use posix thread mutexes */
 #else
  typedef int mutex;
 #endif
#endif

/* initializes a thread-wide mutex */
int mutex_init(mutex* id);

/* acquire the given mutex to enter in a critical section */
int mutex_in (mutex* id);

/* quit the critical section, by releasing the given mutex */
int mutex_out (mutex* id);

/* clean the mutex, thus re-initable by mutex_init() */
int mutex_clean(mutex* id);

#endif
