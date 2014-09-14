/*
 *  option (cel)
 */

#ifndef OPT_H
#define OPT_H


/*
 *  represents an element of an option description table
 */
typedef struct opt_t {
    const char *lopt;    /* long-named option */
    int sopt;            /* short-named option */
    int *flag;           /* flag varible or information about additional argument */
    int arg;             /* value for flag variable or type of additional argument */
} opt_t;

/*
 *  provides string-integer pairs for opt_val()
 */
typedef struct opt_val_t {
    const char *str;    /* string to match */
    int val;            /* corresponding integral value */
} opt_val_t;

/* types of argument conversion */
enum {
    OPT_TYPE_NO,      /* cannot have type */
    OPT_TYPE_BOOL,    /* has boolean (int) type */
    OPT_TYPE_INT,     /* has integer (long) type */
    OPT_TYPE_UINT,    /* has unsigned integer (unsigned long) type */
    OPT_TYPE_REAL,    /* has floating-point (double) type */
    OPT_TYPE_STR      /* has string (char *) type */
};

/* controls behavior of opt_val() */
enum {
    OPT_CMP_NORMSPC = 1,                       /* considers '_' and '-' equivalent to a space */
    OPT_CMP_CASEIN  = OPT_CMP_NORMSPC << 1     /* performs case-insensitive comparison */
};


/* provides unique address for OPT_ARG_ macros */
extern int opt_arg_req, opt_arg_no, opt_arg_opt;


const char *opt_init(const opt_t *, int *, char **[], const void **, const char *, int);
int opt_parse(void);
int opt_val(opt_val_t *, const char *, int);
void opt_abort(void);
const char *opt_errmsg(int);
void opt_free(void);


/* describes option-arguments */
#define OPT_ARG_REQ (&opt_arg_req)    /* mandatory argument */
#define OPT_ARG_NO  (&opt_arg_no)     /* no argument taken */
#define OPT_ARG_OPT (&opt_arg_opt)    /* optional argument */


#endif    /* OPT_H */

/* end of opt.h */
