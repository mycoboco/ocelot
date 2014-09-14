/*
 *  configuration (cel)
 */

#ifndef CONF_H
#define CONF_H


/* represents an element of a configuration description table */
typedef struct conf_t {
    char *var;       /* section name and variable name */
    int type;        /* type of variable */
    char *defval;    /* default value */
} conf_t;

/* defines enum constants for types of values */
enum {
    CONF_TYPE_NO,      /* cannot have type (not used in this library) */
    CONF_TYPE_BOOL,    /* has boolean (int) type */
    CONF_TYPE_INT,     /* has integer (long) type */
    CONF_TYPE_UINT,    /* has unsigned integer (unsigned long) type */
    CONF_TYPE_REAL,    /* has floating-point (double) type */
    CONF_TYPE_STR      /* has string (char *) type */
};

/* defines enum constants for error codes */
enum {
    CONF_ERR_OK,        /* everything is okay */
    CONF_ERR_FILE,      /* file not found */
    CONF_ERR_IO,        /* I/O error occurred */
    CONF_ERR_SPACE,     /* space in section/variable name */
    CONF_ERR_CHAR,      /* invalid character encountered */
    CONF_ERR_LINE,      /* invalid line encountered */
    CONF_ERR_BSLASH,    /* no following line for slicing */
    CONF_ERR_SEC,       /* section not found */
    CONF_ERR_VAR,       /* variable not found */
    CONF_ERR_TYPE,      /* data type mismatch */
    CONF_ERR_MAX        /* number of error codes */
};

/* defines masks for control options */
enum {
    CONF_OPT_CASE = 0x01,                 /* case-sensitive variable/section name */
    CONF_OPT_ESC  = CONF_OPT_CASE << 1    /* supports escape sequence in quoted value */
};


int conf_preset(const conf_t *, int);
size_t conf_init(FILE *, int);
void conf_free(void);
void conf_hashreset(void);
const void *conf_conv(const char *, int);
const void *conf_get(const char *);
int conf_getbool(const char *, int);
long conf_getint(const char *, long);
unsigned long conf_getuint(const char *, unsigned long);
double conf_getreal(const char *, double);
const char *conf_getstr(const char *);
int conf_set(const char *, const char *);
int conf_section(const char *);
int conf_errcode(void);
const char *conf_errstr(int);


#endif    /* CONF_H */

/* end of conf.h */
