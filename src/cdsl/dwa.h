/*
 *  double-word arithmetic (cdsl)
 */

#ifndef DWA_H
#define DWA_H

typedef unsigned long dwa_base_t;    /* single-word base type */

/* represents double-word integers */
typedef struct dwa_t {
    union {
        dwa_base_t w[2];                            /* single-word alias */
        unsigned char v[sizeof(dwa_base_t) * 2];    /* base-256 representation; little endian */
    } u;
} dwa_t;


/* min/max values for dwa_t */
const dwa_t dwa_umax, dwa_max, dwa_min;


/* conversion from and to native integers */
dwa_t dwa_fromuint(unsigned long);
dwa_t dwa_fromint(long);
unsigned long dwa_touint(dwa_t);
long dwa_toint(dwa_t);

/* arithmetic */
dwa_t dwa_neg(dwa_t);
dwa_t dwa_addu(dwa_t, dwa_t);
dwa_t dwa_add(dwa_t, dwa_t);
dwa_t dwa_subu(dwa_t, dwa_t);
dwa_t dwa_sub(dwa_t, dwa_t);
dwa_t dwa_mulu(dwa_t, dwa_t);
dwa_t dwa_mul(dwa_t, dwa_t);
dwa_t dwa_divu(dwa_t, dwa_t, int);
dwa_t dwa_div(dwa_t, dwa_t, int);

/* bit-wise */
dwa_t dwa_bcom(dwa_t);
dwa_t dwa_lsh(dwa_t, int);
dwa_t dwa_rshl(dwa_t, int);
dwa_t dwa_rsha(dwa_t, int);
dwa_t dwa_bit(dwa_t, dwa_t, int);

/* comparison */
int dwa_cmpu(dwa_t, dwa_t);
int dwa_cmp(dwa_t, dwa_t);

/* conversion from and to string */
char *dwa_tostru(char *, dwa_t, int);
char *dwa_tostr(char *, dwa_t, int);
dwa_t dwa_fromstr(const char *, int, char **);

/* conversion from and to floating-point */
dwa_t dwa_fromfp(long double);
long double dwa_tofpu(dwa_t);
long double dwa_tofp(dwa_t);


#endif    /* DWA_H */

/* end of dwa.h */
