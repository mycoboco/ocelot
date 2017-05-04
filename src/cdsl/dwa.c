/*
 *  double-word arithmetic (cdsl)
 */

#include <ctype.h>     /* isxdigit, isspace */
#include <limits.h>    /* LONG_MAX, LONG_MIN */
#include <math.h>      /* fmodl */
#include <string.h>    /* strchr, memcpy, memset */

#include "cbl/assert.h"    /* assert with exception support */
#include "dwa.h"

#define BASE (1 << 8)                          /* radix */
#define SIZE (sizeof(((dwa_t *)NULL)->u.v))    /* size of double word in bytes */

#define sign(x) ((x).u.v[SIZE-1] >> 7)    /* true if msb is set */


/* declares fmodl() when no declaration is visible */
#ifndef HUGE_VALL
long double fmodl(long double, long double);
#endif    /* !HUGE_VALL */


static const char *map = "0123456789abcdefghijklmnopqrstuvwxyz";


/* conversion from and to native integers */

/*
 *  generates a double-word integer from an unsigned long
 */
dwa_t (dwa_fromuint)(unsigned long v)
{
    dwa_t t = { 0, };
    int i = 0;

    do {
        t.u.v[i++] = v % BASE;
    } while((v /= BASE) > 0 && i < SIZE);

    return t;
}


/*
 *  generates a double-word integer from a signed long;
 *  ASSUMPTION: 2sC is used for native integer types
 */
dwa_t (dwa_fromint)(long v)
{
    return (v < 0)? dwa_neg(dwa_fromuint((v == LONG_MIN)? LONG_MAX+1ul: -v)):
                    dwa_fromuint(v);
}


/*
 *  generates an unsigned long from a double-word integer
 */
unsigned long (dwa_touint)(dwa_t x)
{
    int i;
    unsigned long v = 0;

    for (i = SIZE / 2; i > 0;)
        v = v*BASE + x.u.v[--i];

    return v;
}


/*
 *  generates a signed long from a double-word integer;
 *  ASSUMPTION: overflows are benign
 */
long (dwa_toint)(dwa_t x)
{
    return (sign(x))? -(long)dwa_touint(dwa_neg(x)): dwa_touint(x);
}


/* arithmetic */

/*
 *  computes a 2sC of a double-word integer for negation
 */
dwa_t (dwa_neg)(dwa_t x)
{
    int i;
    dwa_t t;
    unsigned c = 1;    /* 2sC */

    for (i = 0; i < SIZE; i++) {
        c += (unsigned char)~x.u.v[i];
        t.u.v[i] = c % BASE;
        c /= BASE;
    }

    return t;
}


/*
 *  adds two unsigned double-word integers
 */
dwa_t (dwa_addu)(dwa_t x, dwa_t y)
{
    int i;
    dwa_t t;
    unsigned c = 0;

    for (i = 0; i < SIZE; i++) {
        c += x.u.v[i] + y.u.v[i];
        t.u.v[i] = c % BASE;
        c /= BASE;
    }

    return t;
}


/*
 *  adds two 2sC double-word integers
 */
dwa_t (dwa_add)(dwa_t x, dwa_t y)
{
    return dwa_addu(x, y);
}


/*
 *  subtracts an unsigned double-word integer from another
 */
dwa_t (dwa_subu)(dwa_t x, dwa_t y)
{
    return dwa_addu(x, dwa_neg(y));
}


/*
 *  subtracts a 2sC double-word integer from another
 */
dwa_t (dwa_sub)(dwa_t x, dwa_t y)
{
    return dwa_addu(x, dwa_neg(y));
}


/*
 *  multiplys two unsigned double-word integers
 */
dwa_t (dwa_mulu)(dwa_t x, dwa_t y)
{
    dwa_t t;
    int i, j;
    unsigned char z[2*SIZE] = { 0, };

    for (i = 0; i < SIZE; i++) {
        unsigned c = 0;
        for (j = 0; j < SIZE; j++) {
            c += x.u.v[i]*y.u.v[j] + z[i+j];
            z[i+j] = c % BASE;
            c /= BASE;
        }
        for (; j < 2*SIZE - i; j++) {
            c += z[i+j];
            z[i+j] = c % BASE;
            c /= BASE;
        }
    }

    memcpy(t.u.v, z, SIZE);
    return t;
}


/*
 *  multiplies two signed double-word integers
 */
dwa_t (dwa_mul)(dwa_t x, dwa_t y)
{
    dwa_t t;
    int sx = sign(x), sy = sign(y);

    if (sx)
        x = dwa_neg(x);
    if (sy)
        y = dwa_neg(y);
    t = dwa_mulu(x, y);

    return (sx != sy)? dwa_neg(t): t;
}


/*
 *  counts the length of a double-word integer
 */
static int len(dwa_t x)
{
    int i;

    for (i = SIZE; i > 1 && x.u.v[i-1] == 0; i--)
        continue;

    return i;
}


/*
 *  divides a double-word integer by a single-digit one
 */
static dwa_t quot(dwa_t x, int y, int mod)
{
    int i;
    unsigned c = 0;
    dwa_t t = { 0, };

    for (i = SIZE-1; i >= 0; i--) {
        c = c*BASE + x.u.v[i];
        if (!mod)
            t.u.v[i] = c / y;
        c %= y;
    }
    if (mod)
        t.u.v[0] = c;

    return t;
}


/*
 *  multiplies an n-digit integer by a single-digit one
 */
static unsigned prod(int n, unsigned char *z, unsigned char *x, int y)
{
    int i;
    unsigned c = 0;

    for (i = 0; i < n; i++) {
        c += x[i]*y;
        z[i] = c % BASE;
        c /= BASE;
    }

    return c;
}


/*
 *  subtracts an n-digit integer from another
 */
static void sub(int n, unsigned char *z, unsigned char *x, unsigned char *y)
{
    int i;
    unsigned b = 0;

    for (i = 0; i < n; i++) {
        int d = (x[i] + BASE) - b - y[i];
        z[i] = d % BASE;
        b = 1 - d/BASE;
    }
}


/*
 *  divides a double-word integer by another
 */
dwa_t (dwa_divu)(dwa_t x, dwa_t y, int mod)
{
    static unsigned char tmp[SIZE*2 + 2];

    int n, m;
    dwa_t t = { 0, };

    n = len(x);
    m = len(y);

    if (m == 1) {
        if (y.u.v[0] == 0)
            return t;
        t = quot(x, y.u.v[0], mod);
    } else if (m > n) {
        if (mod)
            memcpy(t.u.v, x.u.v, SIZE);
        else
            memset(t.u.v, 0, SIZE);
    } else {
        int i, k;
        unsigned char *rem = tmp, *dq = tmp+n+1;
        memcpy(rem, x.u.v, n);
        rem[n] = 0;
        for (k = n - m; k >= 0; k--) {
            int qk;
            int km = k + m;
            unsigned long y2 = y.u.v[m-1]*BASE + y.u.v[m-2];
            unsigned long r3 = rem[km]*(BASE*BASE) + rem[km-1]*BASE + rem[km-2];
            qk = r3/y2;
            if (qk >= BASE)
                qk = BASE - 1;
            dq[m] = prod(m, dq, y.u.v, qk);
            for (i = m; i > 0; i--)
                if (rem[i+k] != dq[i])
                    break;
            if (rem[i+k] < dq[i])
                dq[m] = prod(m, dq, y.u.v, --qk);
            t.u.v[k] = qk;
            sub(m+1, &rem[k], &rem[k], dq);
        }
        if (mod) {
            memcpy(t.u.v, rem, m);
            i = m;
        } else
            i = n-m+1;
        while (i < SIZE)
            t.u.v[i++] = 0;
    }

    return t;
}


/*
 *  divides a signed double-word integer by another
 */
dwa_t (dwa_div)(dwa_t x, dwa_t y, int mod)
{
    dwa_t t;
    int sx = sign(x), sy = sign(y);

    if (sx)
        x = dwa_neg(x);
    if (sy)
        y = dwa_neg(y);
    t = dwa_divu(x, y, mod);

    return ((!mod && sx != sy) || (mod && sx))? dwa_neg(t): t;
}


/* bit-wise */

/*
 *  negates each bits of a double-word integer
 */
dwa_t (dwa_bcom)(dwa_t x)
{
    int i;

    for (i = 0; i < SIZE; i++)
        x.u.v[i] = ~x.u.v[i];

    return x;
}


/*
 *  performs left shift on a double-word integer
 */
dwa_t (dwa_lsh)(dwa_t x, int n)
{
    dwa_t t;
    int i, j = SIZE-1;

    i = SIZE - n/8 - 1;
    for (; j >= SIZE + n/8; j--)
        t.u.v[j] = 0;
    for (; i >= 0; i--, j--)
        t.u.v[j] = x.u.v[i];
    for (; j >= 0; j--)
        t.u.v[j] = 0;
    n %= 8;
    if (n > 0)
        prod(SIZE, t.u.v, t.u.v, 1 << n);

    return t;
}


/*
 *  performs logical right shift on a double-word integer
 */
dwa_t (dwa_rshl)(dwa_t x, int n)
{
    dwa_t t;
    int i, j = 0;

    for (i = n / 8; i < SIZE && j < SIZE; i++, j++)
        t.u.v[j] = x.u.v[i];
    for (; j < SIZE; j++)
        t.u.v[j] = 0;
    n %= 8;
    if (n > 0)
        t = quot(t, 1 << n, 0);

    return t;
}


/*
 *  performs arithmetic right shift on a double-word integer
 */
dwa_t (dwa_rsha)(dwa_t x, int n)
{
    dwa_t t;
    int i, j = 0;
    unsigned fill = (sign(x))? 0xff: 0;

    for (i = n / 8; i < SIZE && j < SIZE; i++, j++)
        t.u.v[j] = x.u.v[i];
    for (; j < SIZE; j++)
        t.u.v[j] = fill;
    n %= 8;
    if (n > 0) {
        t = quot(t, 1 << n, 0);
        t.u.v[SIZE-1] |= fill << (8-n);
    }

    return t;
}


/*
 *  performs bit-wise operations on double-word integers
 */
dwa_t (dwa_bit)(dwa_t x, dwa_t y, int op)
{
    int i;
    dwa_t t;

    switch(op) {
        case 0:    /* and */
            for (i = 0; i < SIZE; i++)
                t.u.v[i] = x.u.v[i] & y.u.v[i];
            break;
        case 1:    /* xor */
            for (i = 0; i < SIZE; i++)
                t.u.v[i] = x.u.v[i] ^ y.u.v[i];
            break;
        case 2:    /* or */
            for (i = 0; i < SIZE; i++)
                t.u.v[i] = x.u.v[i] | y.u.v[i];
            break;
        default:
            assert(!"invalid bit-wise operation code");
            break;
    }

    return t;
}


/* comparison */

/*
 *  compares two unsigned double-word integers
 */
int (dwa_cmpu)(dwa_t x, dwa_t y)
{
    int i = SIZE - 1;

    while (i > 0 && x.u.v[i] == y.u.v[i])
        i--;

    return x.u.v[i] - y.u.v[i];
}


/*
 *  compares two signed double-word integers
 */
int (dwa_cmp)(dwa_t x, dwa_t y)
{
    int sx = sign(x), sy = sign(y);

    return (sx != sy)? sy - sx: dwa_cmpu(x, y);
}


/* conversion from and to string */

/*
 *  converts an unsigned double-word integer to a string
 */
char *(dwa_tostru)(char *s, dwa_t x, int base)
{
    static char buf[SIZE*8 + 1];

    int i, j;
    char *p = (s)? s: buf;

    assert(base >= 2 && base <= 36);

    i = 0, j = SIZE;
    do {
        unsigned long r = dwa_touint(quot(x, base, 1));
        x = quot(x, base, 0);
        p[i++] = map[r];
        while (j > 1 && x.u.v[j-1] == 0)
            j--;
    } while(j > 1 || x.u.v[0] != 0);
    p[i] = '\0';
    for (j = 0; j < --i; j++) {
        char c = p[j];
        p[j] = p[i];
        p[i] = c;
    }

    return p;
}


/*
 *  converts a signed double-word integer to a string
 */
char *(dwa_tostr)(char *s, dwa_t x, int base)
{
    static char buf[SIZE*8 + 1];

    if (!s)
        s = buf;
    s[0] = '-';

    if (sign(x))
        dwa_tostru(s+1, dwa_neg(x), base);
    else
        dwa_tostru(s, x, base);

    return s;
}


#define hprefix() (p[0] == '0' && (p[1] == 'x' || p[1] == 'X') && isxdigit(p[2]))
#define valid()   ((q = strchr(map, tolower(*p))) != NULL && q-map < base)

/*
 *  converts a string to a double-word integer
 */
dwa_t (dwa_fromstr)(const char *str, int base, char **end)
{
    int s = 0;
    dwa_t t = { 0, };
    const char *q;
    unsigned char *p = (unsigned char *)str;

    assert(p);
    assert(base == 0 || (base >= 2 && base <= 36));

    while (isspace(*p))
        p++;

    if (*p == '-')
        p++, s = 1;
    else if (*p == '+')
        p++;
    if (base == 0)
        base = (*p != '0')? 10:
               (hprefix())? (p+=2, 16): 8;
    else if (base == 16 && hprefix())
        p += 2;

    if (valid()) {
        do {
            if (prod(SIZE, t.u.v, t.u.v, base) > 0)
                break;
            t = dwa_add(t, dwa_fromuint(q - map));
            p++;
        } while(valid());
    } else
        p = (unsigned char *)str;

    if (end)
        *end = (char *)p;
    return (s)? dwa_neg(t): t;
}

#undef valid
#undef hprefix


/* conversion from and to floating-point */

/*
 *  converts a floating-point to a double-word integer;
 *  intended to work with radix 2
 */
dwa_t (dwa_fromfp)(long double v)
{
    int s, i;
    dwa_t t = { 0, };

    if (v < 0) {
        s = 1;
        v = -v;
    } else
        s = 0;

    for (i = 0; i < SIZE && v >= 1.0; i++) {
        t.u.v[i] = fmodl(v, BASE);
        v /= BASE;
    }

    if (v >= 1.0) {    /* overflow */
        if (s) {
            t.u.v[SIZE-1] = 0x80;
            memset(t.u.v, 0, SIZE-1);
        } else
            memset(t.u.v, 0xff, SIZE);
        return t;
    }
    return (s)? dwa_neg(t): t;
}


/*
 *  converts an unsigned double-word integer to a floating-point;
 *  intended to work with radix 2
 */
long double (dwa_tofpu)(dwa_t x)
{
    int i;
    long double v = 0.0;

    for (i = SIZE-1; i >= 0; i--)
        v = v*BASE + x.u.v[i];

    return v;
}


/*
 *  converts a signed double-word integer to a floating-point
 */
long double (dwa_tofp)(dwa_t x)
{
    return (sign(x))? -dwa_tofpu(dwa_neg(x)): dwa_tofpu(x);
}


#if 0    /* test code */
#include <stdio.h>       /* puts, printf */
#include <cdsl/dwa.h>    /* dwa_t, dwa_lsh, dwa_tostru */


int main(void)
{
    dwa_t t1 = dwa_fromuint(1);
    dwa_t t2;
    char *p;
    const char *q;

    puts(dwa_tostru(NULL, dwa_fromuint(0), 10));              /* 0 */
    puts(dwa_tostru(NULL, dwa_fromuint(1), 10));              /* 1 */
    puts(dwa_tostru(NULL, dwa_fromuint(0xffffffff), 10));     /* 4294967295 */
    puts(dwa_tostru(NULL, dwa_fromuint(-2), 10));             /* 4294967294 */
    puts(dwa_tostr(NULL, dwa_fromuint(-2), 10));              /* 4294967294 */
    puts(dwa_tostru(NULL, dwa_fromint(0), 10));               /* 0 */
    puts(dwa_tostru(NULL, dwa_fromint(-1), 10));              /* 18446744073709551615 */
    puts(dwa_tostr(NULL, dwa_fromint(-1), 10));               /* -1 */
    puts(dwa_tostr(NULL, dwa_fromint(-2), 10));               /* -2 */
    puts(dwa_tostr(NULL, dwa_fromint(-2147483647-1), 10));    /* -2147483648 */

    printf("\n%lu\n", dwa_touint(dwa_fromuint(0)));            /* 0 */
    printf("%lu\n", dwa_touint(dwa_fromuint(1)));              /* 1 */
    printf("%lu\n", dwa_touint(dwa_fromuint(0xffffffff)));     /* 4294967295 */
    printf("%ld\n", dwa_toint(dwa_fromint(-1)));               /* -1 */
    printf("%ld\n", dwa_toint(dwa_fromint(-2)));               /* -2 */
    printf("%ld\n", dwa_toint(dwa_fromint(-2147483647-1)));    /* -2147483648 */

    t1 = dwa_fromuint(0);
    printf("\n%s\n", dwa_tostru(NULL, dwa_addu(t1, t1), 10));    /* 0 */
    t1 = dwa_fromuint(256);
    puts(dwa_tostru(NULL, dwa_addu(t1, t1), 10));                /* 512 */
    t1 = dwa_fromuint(0xffffffff);
    puts(dwa_tostru(NULL, dwa_addu(t1, t1), 10));                /* 8589934590 */
    t2 = dwa_fromuint(2);
    puts(dwa_tostru(NULL, dwa_mulu(t1, t2), 10));                /* 8589934590 */
    t1 = dwa_fromint(1), t2 = dwa_fromint(-1);
    puts(dwa_tostr(NULL, dwa_add(t1, t2), 10));                  /* 0 */
    t2 = dwa_fromint(-2);
    puts(dwa_tostr(NULL, dwa_add(t1, t2), 10));                  /* -1 */

    t1 = dwa_fromuint(1), t2 = dwa_fromuint(2);
    printf("\n%s\n", dwa_tostru(NULL, dwa_subu(t1, t1), 10));    /* 0 */
    puts(dwa_tostru(NULL, dwa_subu(t1, t2), 10));                /* 18446744073709551615 */
    puts(dwa_tostru(NULL, dwa_subu(t2, t1), 10));                /* 1 */
    t1 = dwa_fromuint(512), t2 = dwa_fromuint(513);
    puts(dwa_tostru(NULL, dwa_subu(t1, t2), 10));                /* 18446744073709551615 */
    puts(dwa_tostru(NULL, dwa_subu(t2, t1), 10));                /* 1 */
    t1 = dwa_fromuint(1), t2 = dwa_fromuint(2);
    puts(dwa_tostr(NULL, dwa_sub(t1, t1), 10));                  /* 0 */
    puts(dwa_tostr(NULL, dwa_sub(t1, t2), 10));                  /* -1 */
    puts(dwa_tostr(NULL, dwa_sub(t2, t1), 10));                  /* 1 */
    t2 = dwa_fromint(-2147483647);
    puts(dwa_tostr(NULL, dwa_sub(t2, t1), 10));                  /* -2147483648 */
    puts(dwa_tostr(NULL, dwa_subu(t1, t2), 10));                 /* 2147483648 */

    t1 = dwa_fromuint(0), t2 = dwa_fromint(-1);
    printf("\n%s\n", dwa_tostr(NULL, dwa_mul(t1, t2), 10));     /* 0 */
    puts(dwa_tostr(NULL, dwa_mul(t2, t2), 10));                 /* 1 */
    t1 = dwa_fromint(-2);
    puts(dwa_tostr(NULL, dwa_mul(t1, t1), 10));                 /* 4 */
    t1 = dwa_fromuint(0xffffffff);
    puts(dwa_tostru(NULL, dwa_mulu(t1, t1), 10));               /* 18446744065119617025 */
    puts(dwa_tostru(NULL, dwa_mulu(t1, t1), 16));               /* fffffffe00000001 */
    t1 = dwa_mulu(dwa_fromuint(0x80000000), dwa_fromint(2));
    puts(dwa_tostru(NULL, dwa_mulu(t1, t1), 10));               /* 0 */
    t1 = dwa_fromuint(0x80000000);
    puts(dwa_tostr(NULL, dwa_mul(t1, dwa_fromint(-1)), 10));    /* -2147483648 */

    t1 = dwa_fromuint(0); t2 = dwa_fromuint(1);
    printf("\n%s\n", dwa_tostru(NULL, dwa_divu(t1, t2, 0), 10));    /* 0 */
    puts(dwa_tostru(NULL, dwa_divu(t2, t1, 0), 10));                /* 0 */
    t1 = dwa_fromuint(0x80000000), t2 = dwa_fromint(-1);
    puts(dwa_tostr(NULL, dwa_div(t1, t2, 0), 10));                  /* -2147483648 */
    t1 = dwa_fromint(-2147483647-1);
    puts(dwa_tostr(NULL, dwa_div(t1, t2, 0), 10));                  /* 2147483648 */
    t1 = dwa_fromint(123456789), t2 = dwa_fromint(999);
    t1 = dwa_mulu(t1, t2);
    puts(dwa_tostru(NULL, t1, 10));                                 /* 123333332211 */
    puts(dwa_tostru(NULL, dwa_divu(t1, t2, 0), 10));                /* 123456789 */
    puts(dwa_tostr(NULL, dwa_div(t1, dwa_neg(t2), 0), 10));         /* -123456789 */
    t1 = dwa_fromuint(3), t2 = dwa_fromuint(2);
    puts(dwa_tostru(NULL, dwa_divu(t1, t2, 0), 10));                /* 1 */
    puts(dwa_tostr(NULL, dwa_div(dwa_neg(t1), t2, 0), 10));         /* -1 */
    puts(dwa_tostr(NULL, dwa_div(t1, dwa_neg(t2), 0), 10));         /* -1 */

    t1 = dwa_mulu(dwa_fromint(123456789),  dwa_fromint(999));
    t2 = dwa_fromint(1000);
    printf("\n%s\n", dwa_tostru(NULL, dwa_divu(t1, t2, 1), 10));    /* 211 */
    puts(dwa_tostr(NULL, dwa_div(t1, dwa_neg(t2), 1), 10));         /* 211 */
    puts(dwa_tostr(NULL, dwa_div(dwa_neg(t1), t2, 1), 10));         /* -211 */
    t1 = dwa_fromuint(3), t2 = dwa_fromuint(2);
    puts(dwa_tostru(NULL, dwa_divu(t1, t2, 1), 10));                /* 1 */
    puts(dwa_tostr(NULL, dwa_div(dwa_neg(t1), t2, 1), 10));         /* -1 */
    puts(dwa_tostr(NULL, dwa_div(t1, dwa_neg(t2), 1), 10));         /* 1 */
    t1 = dwa_fromint(123456789), t2 = dwa_fromint(9999);
    t1 = dwa_addu(dwa_mulu(t1, t2), dwa_fromuint(1));
    puts(dwa_tostr(NULL, dwa_divu(t1, t2, 1), 10));                 /* 1 */

    t1 = dwa_lsh(dwa_fromuint(1), 31);
    printf("\n%s\n", dwa_tostru(NULL, t1, 10));    /* 2147483648 */
    puts(dwa_tostr(NULL, t1, 10));                 /* 2147483648 */
    puts(dwa_tostru(NULL, t1, 16));                /* 80000000 */
    t1 = dwa_lsh(t1, 32);
    t2 = dwa_lsh(dwa_fromint(1), 63);
    puts(dwa_tostru(NULL, t1, 10));                /* 9223372036854775808 */
    puts(dwa_tostru(NULL, t2, 10));                /* 9223372036854775808 */
    puts(dwa_tostru(NULL, t1, 16));                /* 8000000000000000 */
    t1 = dwa_rshl(t1, 1);
    puts(dwa_tostru(NULL, t1, 16));                /* 4000000000000000 */
    t1 = dwa_rshl(t1, 33);
    puts(dwa_tostru(NULL, t1, 16));                /* 20000000 */
    t1 = dwa_lsh(t1, 34);
    puts(dwa_tostru(NULL, t1, 16));                /* 8000000000000000 */
    t1 = dwa_rsha(t1, 10);
    puts(dwa_tostru(NULL, t1, 16));                /* ffe0000000000000 */
    t1 = dwa_rsha(t1, 23);
    puts(dwa_tostru(NULL, t1, 16));                /* ffffffffc0000000 */

    t2 = t1;
    printf("\n%s\n", dwa_tostru(NULL, dwa_bit(t1, t2, 1), 16));    /* 0 */
    t1 = dwa_fromuint(0xFFFFFFFFu);
    puts(dwa_tostru(NULL, dwa_bit(t1, t2, 2), 16));                /* ffffffffffffffff */
    t1 = dwa_lsh(t1, 33);
    puts(dwa_tostru(NULL, dwa_bit(t1, t2, 0), 16));                /* fffffffe00000000 */
    t1 = dwa_fromuint(123456789), t2 = dwa_fromint(1);
    t1 = dwa_mulu(t1, t1);
    puts(dwa_tostr(NULL, dwa_add(dwa_bcom(t1), t2), 10));          /* -15241578750190521 */
    puts(dwa_tostr(NULL, dwa_neg(t1), 10));                        /* -15241578750190521 */

    t1 = dwa_fromuint(123456789), t2 = dwa_fromuint(123456788);
    printf("\n%d\n", dwa_cmpu(t1, t2));                             /* > */
    printf("%d\n", dwa_cmpu(t1, dwa_addu(t2, dwa_fromuint(1))));    /* == */
    printf("%d\n", dwa_cmpu(t2, t1));                               /* < */
    printf("%d\n", dwa_cmpu(t1, dwa_neg(t2)));                      /* < */
    printf("%d\n", dwa_cmpu(dwa_neg(t1), t2));                      /* > */
    printf("%d\n", dwa_cmp(t1, dwa_neg(t2)));                       /* > */
    printf("%d\n", dwa_cmp(dwa_neg(t1), t2));                       /* < */
    t1 = dwa_neg(t1), t2 = dwa_neg(t2);
    printf("%d\n", dwa_cmp(t1, t2));                                /* < */
    printf("%d\n", dwa_cmp(t2, t1));                                /* > */

    printf("\n%s\n", dwa_tostru(NULL, t1, 10));                        /* 18446744073586094827 */
    printf("%Lf\n", dwa_tofpu(t1));                                    /* 18446744073586094827.0 */
    printf("%Lf\n", dwa_tofpu(dwa_fromint(1)));                        /* 1.0 */
    puts(dwa_tostru(NULL, dwa_fromfp(3.141592), 10));                  /* 3 */
    puts(dwa_tostru(NULL, dwa_fromfp(0.999999), 10));                  /* 0 */
    puts(dwa_tostru(NULL, dwa_fromfp(9223372036854775808.0), 10));     /* 9223372036854775808 */
    puts(dwa_tostru(NULL, dwa_fromfp(18446744073709551615.0), 10));    /* 18446744073709551615 */
    puts(dwa_tostru(NULL, dwa_fromfp(18446744073709551610.0), 10));    /* 18446744073709551615 */
    puts(dwa_tostr(NULL, dwa_fromfp(-3.141592), 10));                  /* -3 */
    puts(dwa_tostr(NULL, dwa_fromfp(-0.999999), 10));                  /* 0 */
    puts(dwa_tostr(NULL, dwa_fromfp(9007199254740991.0), 10));         /* 9007199254740991 */
    puts(dwa_tostr(NULL, dwa_fromfp(9007199254740992.0), 10));         /* 9007199254740992 */
    puts(dwa_tostr(NULL, dwa_fromfp(9007199254740993.0), 10));         /* 9007199254740992 */
    puts(dwa_tostr(NULL, dwa_fromfp(-9223372036854775808.0), 10));     /* -9223372036854775808 */
    puts(dwa_tostr(NULL, dwa_fromfp(-18446744073709551615.0), 10));    /* -9223372036854775808 */
    puts(dwa_tostr(NULL, dwa_fromfp(-18446744073709551610.0), 10));    /* -9223372036854775808 */

    q = dwa_tostr(NULL, dwa_fromstr("+1234567890", 0, &p), 10);
    printf("\n%s:%s\n", q, p);                                                    /* 1234567890: */
    q = dwa_tostr(NULL, dwa_fromstr("1234567891234567890a", 0, &p), 10);
    printf("%s:%s\n", q, p);                                            /* 1234567891234567890:a */
    q = dwa_tostr(NULL, dwa_fromstr(" \t  -1234567891234567890a", 0, NULL), 10);
    printf("%s:%s\n", q, p);                                           /* -1234567891234567890:a */
    q = dwa_tostru(NULL, dwa_fromstr("0Xffffffffffffffffg", 0, &p), 10);
    printf("%s:%s\n", q, p);                                           /* 18446744073709551615:g */
    q = dwa_tostr(NULL, dwa_fromstr("ffffffffffffffffg", 16, &p), 10);
    printf("%s:%s\n", q, p);                                                             /* -1:g */
    q = dwa_tostr(NULL, dwa_fromstr("ffffffffffffffff!", 16, &p), 10);
    printf("%s:%s\n", q, p);                                                             /* -1:! */
    q = dwa_tostr(NULL, dwa_fromstr("00008000000000000000g", 16, &p), 10);
    printf("%s:%s\n", q, p);                                           /* -9223372036854775808:g */
    q = dwa_tostr(NULL, dwa_fromstr("-9223372036854775808+", 0, &p), 10);
    printf("%s:%s\n", q, p);                                           /* -9223372036854775808:+ */
    q = dwa_tostru(NULL, dwa_fromstr("  07777777777777777777778", 0, &p), 10);
    printf("%s:%s\n", q, p);                                            /* 9223372036854775807:8 */
    q = dwa_tostru(NULL, dwa_fromstr("000111111111111111111111111111111112", 2, &p), 10);
    printf("%s:%s\n", q, p);                                                     /* 4294967295:2 */
    q = dwa_tostru(NULL, dwa_fromstr("000ZZZZZZZZZZZ", 36, &p), 36);
    printf("%s:%s\n", q, p);                                                     /* zzzzzzzzzzz: */
    q = dwa_tostru(NULL, dwa_fromstr("fffffffffffffffff!", 16, &p), 10);
    printf("%s:%s\n", q, p);                                          /* 18446744073709551600:f! */
    q = dwa_tostru(NULL, dwa_fromstr("0z", 0, &p), 10);
    printf("%s:%s\n", q, p);                                                              /* 0:z */
    q = dwa_tostru(NULL, dwa_fromstr("0xz", 0, &p), 10);
    printf("%s:%s\n", q, p);                                                             /* 0:xz */
    q = dwa_tostru(NULL, dwa_fromstr("0xz", 16, &p), 10);
    printf("%s:%s\n", q, p);                                                             /* 0:xz */
    q = dwa_tostru(NULL, dwa_fromstr(" +", 0, &p), 10);
    printf("%s:%s\n", q, p);                                                             /* 0: + */
    q = dwa_tostru(NULL, dwa_fromstr(" +z", 0, &p), 10);
    printf("%s:%s\n", q, p);                                                            /* 0: +z */
    q = dwa_tostru(NULL, dwa_fromstr(" +0X", 0, &p), 10);
    printf("%s:%s\n", q, p);                                                              /* 0:X */

    return 0;
}
#endif    /* disabled */

/* end of dwa.c */
