#ifndef fooasyncnshfoo
#define fooasyncnshfoo

/***
  This file is part of libasyncns.

  Copyright 2005-2008 Lennart Poettering

  libasyncns is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 2.1 of the
  License, or (at your option) any later version.

  libasyncns is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with libasyncns. If not, see
  <http://www.gnu.org/licenses/>.
***/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/** \mainpage
 *
 * \section moo Method of operation
 *
 * To use libasyncns allocate an _g_asyncns_t object with
 * _g_asyncns_new(). This will spawn a number of worker threads (or processes, depending on what is available) which
 * are subsequently used to process the queries the controlling
 * program issues via _g_asyncns_getaddrinfo() and
 * _g_asyncns_getnameinfo(). Use _g_asyncns_free() to shut down the worker
 * threads/processes.
 *
 * Since libasyncns may fork off new processes you have to make sure that
 * your program is not irritated by spurious SIGCHLD signals.
 */

/** \example asyncns-test.c
 * An example program */

#ifdef  __cplusplus
extern "C" {
#endif

/** An opaque libasyncns session structure */
typedef struct asyncns _g_asyncns_t;

/** An opaque libasyncns query structure */
typedef struct _g_asyncns_query _g_asyncns_query_t;

/** Allocate a new libasyncns session with n_proc worker processes/threads */
_g_asyncns_t* _g_asyncns_new(unsigned n_proc);

/** Free a libasyncns session. This destroys all attached
 * _g_asyncns_query_t objects automatically */
void _g_asyncns_free(_g_asyncns_t *asyncns);

/** Return the UNIX file descriptor to select() for readability
 * on. Use this function to integrate libasyncns with your custom main
 * loop. */
int _g_asyncns_fd(_g_asyncns_t *asyncns);

/** Process pending responses. After this function is called you can
 * get the next completed query object(s) using _g_asyncns_getnext(). If
 * block is non-zero wait until at least one response has been
 * processed. If block is zero, process all pending responses and
 * return. */
int _g_asyncns_wait(_g_asyncns_t *asyncns, int block);

/** Issue a name to address query on the specified session. The
 * arguments are compatible with the ones of libc's
 * getaddrinfo(3). The function returns a new query object. When the
 * query is completed you may retrieve the results using
 * _g_asyncns_getaddrinfo_done().*/
_g_asyncns_query_t* _g_asyncns_getaddrinfo(_g_asyncns_t *asyncns, const char *node, const char *service, const struct addrinfo *hints);

/** Retrieve the results of a preceding _g_asyncns_getaddrinfo()
 * call. Returns a addrinfo structure and a return value compatible
 * with libc's getaddrinfo(3). The query object q is destroyed by this
 * call and may not be used any further. Make sure to free the
 * returned addrinfo structure with _g_asyncns_freeaddrinfo() and not
 * libc's freeaddrinfo(3)! If the query is not completed yet EAI_AGAIN
 * is returned.*/
int _g_asyncns_getaddrinfo_done(_g_asyncns_t *asyncns, _g_asyncns_query_t* q, struct addrinfo **ret_res);

/** Issue an address to name query on the specified session. The
 * arguments are compatible with the ones of libc's
 * getnameinfo(3). The function returns a new query object. When the
 * query is completed you may retrieve the results using
 * _g_asyncns_getnameinfo_done(). Set gethost (resp. getserv) to non-zero
 * if you want to query the hostname (resp. the service name). */
_g_asyncns_query_t* _g_asyncns_getnameinfo(_g_asyncns_t *asyncns, const struct sockaddr *sa, socklen_t salen, int flags, int gethost, int getserv);

/** Retrieve the results of a preceding _g_asyncns_getnameinfo()
 * call. Returns the hostname and the service name in ret_host and
 * ret_serv. The query object q is destroyed by this call and may not
 * be used any further. If the query is not completed yet EAI_AGAIN is
 * returned. */
int _g_asyncns_getnameinfo_done(_g_asyncns_t *asyncns, _g_asyncns_query_t* q, char *ret_host, size_t hostlen, char *ret_serv, size_t servlen);

/** Issue a resolver query on the specified session. The arguments are
 * compatible with the ones of libc's res_query(3). The function returns a new
 * query object. When the query is completed you may retrieve the results using
 * _g_asyncns_res_done().  */
_g_asyncns_query_t* _g_asyncns_res_query(_g_asyncns_t *asyncns, const char *dname, int class, int type);

/** Issue an resolver query on the specified session. The arguments are
 * compatible with the ones of libc's res_search(3). The function returns a new
 * query object. When the query is completed you may retrieve the results using
 * _g_asyncns_res_done().  */
_g_asyncns_query_t* _g_asyncns_res_search(_g_asyncns_t *asyncns, const char *dname, int class, int type);

/** Retrieve the results of a preceding _g_asyncns_res_query() or
 * _g_asyncns_res_search call.  The query object q is destroyed by this
 * call and may not be used any further. Returns a pointer to the
 * answer of the res_query call. If the query is not completed yet
 * -EAGAIN is returned, on failure -errno is returned, otherwise the
 * length of answer is returned. Make sure to free the answer is a
 * call to _g_asyncns_freeanswer(). */
int _g_asyncns_res_done(_g_asyncns_t *asyncns, _g_asyncns_query_t* q, unsigned char **answer);

/** Return the next completed query object. If no query has been
 * completed yet, return NULL. Please note that you need to run
 * _g_asyncns_wait() before this function will return sensible data.  */
_g_asyncns_query_t* _g_asyncns_getnext(_g_asyncns_t *asyncns);

/** Return the number of query objects (completed or not) attached to
 * this session */
int _g_asyncns_getnqueries(_g_asyncns_t *asyncns);

/** Cancel a currently running query. q is is destroyed by this call
 * and may not be used any futher. */
void _g_asyncns_cancel(_g_asyncns_t *asyncns, _g_asyncns_query_t* q);

/** Free the addrinfo structure as returned by
 * _g_asyncns_getaddrinfo_done(). Make sure to use this functions instead
 * of the libc's freeaddrinfo()! */
void _g_asyncns_freeaddrinfo(struct addrinfo *ai);

/** Free the answer data as returned by _g_asyncns_res_done().*/
void _g_asyncns_freeanswer(unsigned char *answer);

/** Returns non-zero when the query operation specified by q has been completed */
int _g_asyncns_isdone(_g_asyncns_t *asyncns, _g_asyncns_query_t*q);

/** Assign some opaque userdata with a query object */
void _g_asyncns_setuserdata(_g_asyncns_t *asyncns, _g_asyncns_query_t *q, void *userdata);

/** Return userdata assigned to a query object. Use
 * _g_asyncns_setuserdata() to set this data. If no data has been set
 * prior to this call it returns NULL. */
void* _g_asyncns_getuserdata(_g_asyncns_t *asyncns, _g_asyncns_query_t *q);

#ifdef  __cplusplus
}
#endif

#endif
