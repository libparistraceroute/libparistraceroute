
#ifndef OPTPARSE_H
#define OPTPARSE_H

#define OPT_NO_ACTION (void *)0
#define OPT_NO_SF (void *)0
#define OPT_NO_LF (void *)0
#define OPT_NO_METAVAR (void *)0
#define OPT_NO_HELP (void *)0
#define OPT_NO_DATA (void *)0

/*
 * \brief a structure describing a command line option
 * \action the function to be called to handle the data option
 * \sf shortform of the option
 * \action the function to be called to handle the data option
 */
struct opt_spec {
    int (*action)(char *, void *);
    const char *sf, *lf, *metavar, *help;
    void *data;
};

struct opt_str {
    char *s, s0;
};

/*
 * \brief
 * \
 */

void opt_basename(char *fn, char sep);

/**
 * \brief
 * \
 */

void opt_config(int width, int max_help_pos,
                int indent, const char *separator);
/**
 * \brief
 * \
 */

void opt_options1st(void);

/**
 * \brief
 * \
 */

const char * opt_name(void);

/**
 * \brief
 * \
 */

void opt_err_pfx(void);

/**
 * \brief
 * \
 */

void opt_err_sfx(void);

/**
 * \brief
 * \
 */

void opt_err(const char *msg);


/**
 * \brief
 * \
 */

int opt_text(char *, void *);

/**
 * \brief
 * \
 */

int opt_help(char *, void *);

/**
 * \brief
 * \
 */

int opt_version(char *, void *);

/**
 * \brief
 * \
 */

int opt_stop(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_0(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_1(char *, void *);

/**
 * \brief
 * \
 */

int opt_incr(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_char(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_int(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_int_lim(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_int_lim_en(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_double(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_double_lim(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_double_lim_en(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_str(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_int_2(char *,void *);

/**
 * \brief
 * \
 */

int opt_store_choice(char *, void *);

/**
 * \brief
 * \
 */

int opt_store_choice_abbr(char *, void *);

/**
 * \brief
 * \
 */

int opt_parse(const char * usage, struct opt_spec * opts, char **argv);

/**
 * \brief
 * \
 */

char ***opt_remainder(void);

#endif
