
#ifndef OPTPARSE_H
#define OPTPARSE_H

void opt_basename(char *fn, char sep);
void opt_config(int width, int max_help_pos,
                int indent, const char *separator);
void opt_options1st(void);

const char *opt_name(void);
void opt_err_pfx(void);
void opt_err_sfx(void);
void opt_err(const char *msg);

struct opt_str {
    char *s, s0;
};

int opt_text(char *, void *);
int opt_help(char *, void *);
int opt_version(char *, void *);
int opt_stop(char *, void *);
int opt_store_0(char *, void *);
int opt_store_1(char *, void *);
int opt_incr(char *, void *);
int opt_store_char(char *, void *);
int opt_store_int(char *, void *);
int opt_store_int_lim(char *, void *);
int opt_store_double(char *, void *);
int opt_store_double_lim(char *, void *);
int opt_store_str(char *, void *);
int opt_store_choice(char *, void *);
int opt_store_choice_abbr(char *, void *);

struct opt_spec {
    int (*action)(char *, void *);
    const char *sf, *lf, *metavar, *help;
    void *data;
};
#define OPT_NO_ACTION (void *)0
#define OPT_NO_SF (void *)0
#define OPT_NO_LF (void *)0
#define OPT_NO_METAVAR (void *)0
#define OPT_NO_HELP (void *)0
#define OPT_NO_DATA (void *)0

int opt_parse(const char *usage, struct opt_spec *opts, char **argv);
char ***opt_remainder(void);

#endif
