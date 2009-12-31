/*!
 *  @file    hash.h
 *  @brief    Header for Hash Library (CDSL)
 */

#ifndef HASH_H
#define HASH_H

#include <stddef.h>    /* size_t */


/*! @name    hash string creating functions:
 */
/*@{*/
const char *hash_string(const char *);
const char *hash_int(long);
const char *hash_new(const char *, size_t);
void hash_vload(const char *, ...);
void hash_aload(const char *[]);
/*@}*/

/*! @name    hash destroying functions:
 */
/*@{*/
void hash_free(const char *);
void hash_reset(void);
/*@}*/

/*! @name    misc. functions:
 */
/*@{*/
size_t hash_length(const char *);
/*@}*/


#endif    /* HASH_H */

/* end of hash.h */
