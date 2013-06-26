#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "optparse.h"

#define EMPTY(s) (!(s) || !*(s))

static struct {
    int width, maxhelppos, indent;
    const char *helppfx;
    char sf[3]; /* Initialised to 0 from here on. */
    const char *prog, *usage, *helplf;
    char helpsf, **argv;
    struct opt_spec *opts, *curr, header;
    int opts1st, helppos;
} globals = {79, 24, 2, "  ", "-?"};

void opt_basename(char *fn, char sep)
{
    char *loc;

    if (!sep)
        sep = '/';
    loc = strrchr(fn, sep);
    if (!loc)
        return;
    if (!loc[1]) {
        *loc = 0;
        loc = strrchr(fn, sep);
        *strchr(fn, 0) = sep;
        if (!loc)
            return;
    }
    while ((*fn++ = *++loc))
        ;
}

void opt_config(int width, int max_help_pos,
                int indent, const char *separator)
{
    if (width > 0)
        globals.width = width;
    if (max_help_pos > 0)
        globals.maxhelppos = max_help_pos;
    if (indent >= 0)
        globals.indent = indent;
    if (!EMPTY(separator))
        globals.helppfx = separator;
}

void opt_options1st(void)
{
    globals.opts1st = 1;
}

static void print1opt(struct opt_spec *o, FILE *f, int helppos)
{
    int i, pos, len, helpwidth;
    const char *mv, *a, *b, *c;

    if (!(c = o->help))
        return;
    if (o->action == opt_text && EMPTY(o->lf)) {
        helppos = strspn(c, " ");
        pos = 0;
    }
    else {
        pos = globals.indent + strspn(c, " ");
        fprintf(f, "%*s", pos, "");
        mv = o->metavar ? o->metavar : "";
        len = strlen(mv);
        if (!EMPTY(o->sf)) {
            fprintf(f, "-%c%s", o->sf[0], mv);
            pos += 2 + len;
            for (i = 1; o->sf[i]; ++i) {
                fprintf(f, ", -%c%s", o->sf[i], mv);
                pos += 4 + len;
            }
            if (!EMPTY(o->lf)) {
                fputs(", ", f);
                pos += 2;
            }
        }
        if (!EMPTY(o->lf)) {
            fprintf(f, "%s", o->lf);
            pos += strlen(o->lf);
            if (len) {
                if (mv[0] != ' ') {
                    putc('=', f);
                    ++pos;
                }
                fputs(mv, f);
                pos += len;
            }
        }
        len = strlen(globals.helppfx);
        if (!helppos)
            helppos = pos + len <= globals.maxhelppos
                ? pos + len : globals.maxhelppos;
        if (pos + len > helppos) {
            putc('\n', f);
            pos = 0;
        }
        fprintf(f, "%*s", helppos - pos, globals.helppfx);
        pos = helppos;
    }
    helpwidth = globals.width - helppos;
    for (;;) {
        while (*c == ' ')
            ++c;
        if (!*c)
            break;
        if (*c == '\n') {
            putc(*c++, f);
            pos = 0;
            continue;
        }
        for (a = b = c; ; ) {
            while (*++c && *c != '\n' && *c != ' ')
                ;
            if (c - a > helpwidth) {
                c = b;
                break;
            }
            b = c;
            while (*c == ' ')
                ++c;
            if (!*c)
                break;
            if (*c == '\n') {
                ++c;
                break;
            }
        }
        if (b == a) {
            fprintf(f, "%*s%s\n", helppos - pos, "", a);
            pos = 0;
            break;
        }
        fprintf(f, "%*s%.*s\n", helppos - pos, "", (int)(b - a), a);
        pos = 0;
    }
    if (pos)
        putc('\n', f);
}

static struct opt_spec *sethelppos(struct opt_spec *opts)
{
    int pos, mvlen;

    globals.helppos = 0;
    for ( ; opts->action; ++opts) {
        if (opts->action == opt_text) {
            if (opts->data) {
                opts->data = NULL;
                return opts;
            }
            if (EMPTY(opts->lf))
                continue;
        }
        if (!opts->help)
            continue;
        pos = globals.indent + strspn(opts->help, " ")
            + strlen(globals.helppfx);
        mvlen = opts->metavar ? strlen(opts->metavar) : 0;
        if (EMPTY(opts->sf)) {
            assert(!EMPTY(opts->lf));
            pos += strlen(opts->lf);
            if (mvlen)
                pos += mvlen + (opts->metavar[0] != ' ');
        }
        else {
            pos += strlen(opts->sf) * (mvlen + 4) - 2;
            if (!EMPTY(opts->lf)) {
                pos += strlen(opts->lf) + 2;
                if (mvlen)
                    pos += mvlen + (opts->metavar[0] != ' ');
            }
        }
        if (pos > globals.maxhelppos)
            globals.helppos = globals.maxhelppos;
        else if (pos > globals.helppos)
            globals.helppos = pos;
    }
    return opts;
}

static void printopts(FILE *f)
{
    struct opt_spec *o, *o2;

    print1opt(&globals.header, f, 0);
    o = globals.opts;
    while (o->action) {
        o2 = sethelppos(o);
        while (o != o2)
            print1opt(o++, f, globals.helppos);
    }
}

const char *opt_name(void)
{
    return globals.sf[1] ? globals.sf : globals.curr->lf;
}

void opt_err_pfx(void)
{
    fprintf(stderr, "%s: ", globals.prog);
}

void opt_err_sfx(void)
{
    putc('\n', stderr);
    if (globals.curr->help) {
        fputs("option usage:\n", stderr);
        print1opt(globals.curr, stderr, 0);
    }
    exit(EXIT_FAILURE);
}

void opt_err(const char *msg)
{
    opt_err_pfx();
    fprintf(stderr, msg, opt_name());
    opt_err_sfx();
}

int opt_text(char *arg, void *data)
{
    assert(0);
    return 0;
}

int opt_help(char *arg, void *data)
{
    assert(!arg);
    printf(globals.usage, globals.prog);
    putchar('\n');
    printopts(stdout);
    exit(0);
}

int opt_version(char *arg, void *data)
{
    assert(!arg && data);
    puts((char *)data);
    exit(0);
}

int opt_stop(char *arg, void *data)
{
    if (arg)
        opt_store_str(arg, data);
    else if (data)
        *(int *)data = 1;
    return 1;
}

int opt_store_0(char *arg, void *data)
{
    assert(!arg && data);
    *(int *)data = 0;
    return 0;
}

int opt_store_1(char *arg, void *data)
{
    assert(!arg && data);
    *(int *)data = 1;
    return 0;
}

int opt_incr(char *arg, void *data)
{
    assert(!arg && data);
    ++*(int *)data;
    return 0;
}

int opt_store_char(char *arg, void *data)
{
    assert(arg && data);
    if (!arg[0] || arg[1])
        opt_err("the value of %s must be a single character");
    *(char *)data = arg[0];
    return 0;
}


int opt_store_int(char *arg, void *data)
{
    char *end;
    long val;

    assert(arg && data);
    errno = 0;
    val = strtol(arg, &end, 10);
    if (end == arg || end[0])
        opt_err("the value of %s must be an integer");
    if (errno == ERANGE || val < INT_MIN || val > INT_MAX)
        opt_err("the value of %s is too large");
    *(int *)data = val;
    return 0;
}

int opt_store_int_lim(char *arg, void *data)
{
    char *end;
    long val;
    int *p = data;

    assert(arg && data);
    errno = 0;
    val = strtol(arg, &end, 10);
    if (end == arg || end[0])
        opt_err("the value of %s must be an integer");
    if (errno == ERANGE || val < p[1] || val > p[2]) {
        opt_err_pfx();
        fprintf(stderr, "the value of %s must be in the range %d to %d",
                opt_name(), p[1], p[2]);
        opt_err_sfx();
    }
    p[0] = val;
    return 0;
}

int opt_store_int_lim_en(char *arg, void *data)
{
    char *end;
    long val;
    int *p = data;

    assert(arg && data);
    errno = 0;
    val = strtol(arg, &end, 10);
    if (end == arg || end[0])
        opt_err("the value of %s must be an integer");
    if (errno == ERANGE || val < p[1] || val > p[2]) {
        opt_err_pfx();
        fprintf(stderr, "the value of %s must be in the range %d to %d",
                opt_name(), p[1], p[2]);
        opt_err_sfx();
    }
    p[0] = val;
    p[3] = 1;
    return 0;
}


int opt_store_double(char *arg, void *data)
{
    char *end;
    double val;

    assert(arg && data);
    errno = 0;
    val = strtod(arg, &end);
    if (end == arg || end[0])
        opt_err("the value of %s must be a number");
    if (errno == ERANGE)
        opt_err("the value of %s is too large");
    *(double *)data = val;
    return 0;
}

int opt_store_double_lim(char *arg, void *data)
{
    char *end;
    double val;
    double *p = data;

    assert(arg && data);
    errno = 0;
    val = strtod(arg, &end);
    if (end == arg || end[0])
        opt_err("the value of %s must be a number");
    if (errno == ERANGE || val < p[1] || val > p[2]) {
        opt_err_pfx();
        fprintf(stderr, "the value of %s must be in the range %g to %g",
                opt_name(), p[1], p[2]);
        opt_err_sfx();
    }
    p[0] = val;
    return 0;
}

int opt_store_str(char *arg, void *data) {
    struct opt_str *s = data;

    assert(arg && data);
    s->s = arg;
    s->s0 = arg[0];
    return 0;
}

int opt_store_int_2(char *arg, void *data){
    char     * tab[3];
    char     * end;
    int        i = 0;
    unsigned * p = data;
    unsigned   val1, val2;

    assert(arg && data);
    tab[i] = strtok(arg, ",");
    if (!tab[i]) {
         opt_err("this option requires two values seperated by ','");
    }

    while (tab[i] != NULL) {
        i++;
        tab[i] = strtok(NULL,",");
        if (i == 3 ) {
         opt_err("this option requires two values seperated by ','");
        }
        if (i < 2 && !tab[i]) {
         opt_err("this option requires two values seperated by ','");
        }
    }

    ////
    if (i != 2) {
        opt_err("this option requires two values seperated by ','");
    }
    /////

    val1 = strtol(tab[0], &end, 10);
    errno = 0;
    if (end == tab[0] || end[0])
        opt_err("the first value of %s must be an integer");
    if (errno == ERANGE || val1 < p[1] || val1 > p[2]) {
        opt_err_pfx();
        fprintf(stderr, "the first value of %s must be in the range %d to %d",
                opt_name(), p[1], p[2]);
        opt_err_sfx();
    }
    p[0] = val1;

     val2 = strtol(tab[1], &end, 10);
    errno = 0;
      if (end == tab[0] || end[0])
        opt_err("the second value of %s must be an integer");
    if (errno == ERANGE || val2 < p[4] || val2 > p[5]) {
        opt_err_pfx();
        fprintf(stderr, "the second value of %s must be in the range %d to %d",
                opt_name(), p[4], p[5]);
        opt_err_sfx();
    }
    p[3] = val2;
    p[6] = 1;
    return 0;
 }





static void badchoice(const char **choices, const char *arg)
{
    int i;

    for (i = 0; !EMPTY(choices[i]); ++i)
        ;
    if (choices[i]) {
        const char *temp = choices[i];
        choices[i] = choices[0];
        choices[0] = temp;
        return;
    }
    opt_err_pfx();
    fprintf(stderr, "invalid choice '%s' for option %s",
            arg, opt_name());
    opt_err_sfx();
}

int opt_store_choice(char *arg, void *data)
{
    const char **choices = data, *temp;
    int i;

    assert(arg && data);
    for (i = 0; choices[i] && strcmp(arg, choices[i]) != 0; ++i)
        ;
    if (!choices[i]) {
        badchoice(choices, arg);
        return 0;
    }
    temp = choices[i];
    choices[i] = choices[0];
    choices[0] = temp;
    return 0;
}

int opt_store_choice_abbr(char *arg, void *data)
{
    const char **choices = data, *temp;
    int i, j;

    assert(arg && data);
    for (i = 0; choices[i] && (strstr(choices[i], arg) != choices[i]
                               || !choices[i][0]); ++i)
        ;
    if (!choices[i]) {
        badchoice(choices, arg);
        return 0;
    }
    for (j = i + 1; choices[j] && (strstr(choices[j], arg) != choices[j]
                                   || !choices[j][0]); ++j)
        ;
    if (choices[j]) {
        opt_err_pfx();
        fprintf(
            stderr, "ambiguous choice '%s' for option %s\n%*s(%s",
            arg,
            opt_name(),
            (char *) strlen(globals.prog) + 2,
            "",
            choices[i]
        );
        for (;;) {
            i = j;
            for (j = i + 1;
                 choices[j] && (strstr(choices[j], arg) != choices[j]
                                || !choices[j][0]); ++j)
                ;
            if (!choices[j]) {
                fprintf(stderr, " or %s?)", choices[i]);
                opt_err_sfx();
            }
            fprintf(stderr, ", %s", choices[i]);
        }
    }
    temp = choices[i];
    choices[i] = choices[0];
    choices[0] = temp;
    return 0;
}

static struct opt_spec *findlf(struct opt_spec *opts, const char *arg)
{
    while (opts->action && (opts->action == opt_text
                            || EMPTY(opts->lf)
                            || strstr(opts->lf, arg) != opts->lf))
        ++opts;
    return opts->action ? opts : NULL;
}

static void unknown(const char *arg)
{
    fprintf(stderr, "%s: no such option: %s\n", globals.prog, arg);
    if (globals.helpsf)
        fprintf(stderr, "Try '%s -%c' for more information.\n",
                globals.prog, globals.helpsf);
    else if (globals.helplf)
        fprintf(stderr, "Try '%s %s' for more information.\n",
                globals.prog, globals.helplf);
    else
        printopts(stderr);
    exit(EXIT_FAILURE);
}

int opt_parse(const char *usage, struct opt_spec *opts, char **argv)
{
    char *a, *b, save;
    struct opt_spec *o, *o2;
    int ret = 0, skipping = 0, nh, minh = INT_MAX;

    if (!(globals.prog = argv[0]))
        return 0;
    globals.argv = argv + 1;
    if (EMPTY(usage))
        globals.usage = "usage: %s [options]";
    else
        globals.usage = usage;
    globals.header.action = opt_text;
    if (opts->action == opt_text && EMPTY(opts->lf))
        globals.header.help = opts++->help;
    else
        globals.header.help = "options:";
    globals.opts = opts;
    for (o = opts; o->action; ++o) {
        if (o->action == opt_help) {
            if (EMPTY(o->help))
                o->help = "print this help message and exit";
            if (!EMPTY(o->sf))
                globals.helpsf = o->sf[0];
            else
                globals.helplf = o->lf;
        }
        else if (o->action == opt_version && EMPTY(o->help))
            o->help = "print the version number and exit";
        if (o->action != opt_text && !EMPTY(o->lf)
            && (nh = strspn(o->lf, "-")) < minh)
            minh = nh;
    }
    while ((a = *globals.argv++)) {
        if (!*a) {
            if (globals.opts1st) {
                ++ret;
                skipping = 1;
            }
            continue;
        }
        if (skipping) {
            ++ret;
            continue;
        }
        if (*a == '-' && a[1] == '-' && !a[2]) {
            *a = 0;
            skipping = 1;
            continue;
        }
        if ((nh = strspn(a, "-")) >= minh) {
            b = a + strcspn(a, "=");
            save = *b;
            *b = 0;
            o = b - a < 2 ? NULL : findlf(opts, a);
            if (o) {
                o2 = findlf(o + 1, a);
                if (o2) {
                    fprintf(
                        stderr, "%s: ambiguous option %s\n%*s(%s",
                        globals.prog,
                        a,
                        strlen(globals.prog) + 2,
                        "",
                        o->lf
                    );
                    for (;;) {
                        o = o2;
                        o2 = findlf(o + 1, a);
                        if (!o2) {
                            fprintf(stderr, " or %s?)\n", o->lf);
                            exit(EXIT_FAILURE);
                        }
                        fprintf(stderr, ", %s", o->lf);
                    }
                }
                globals.curr = o;
                globals.sf[1] = 0;
                if (EMPTY(o->metavar)) {
                    if (save)
                        opt_err("option %s does not take a value");
                    skipping = o->action(NULL, o->data);
                }
                else {
                    if (save)
                        ++b;
                    else {
                        b = *globals.argv++;
                        if (!b)
                            opt_err("option %s requires a value");
                    }
                    skipping = o->action(b, o->data);
                    if (!save)
                        *b = 0;
                }
                *a = 0;
                continue;
            }
            if (nh >= 2)
                unknown(a);
            *b = save;
        }
        if (nh == 0 || !a[1]) {
            ++ret;
            skipping = globals.opts1st;
            continue;
        }
        *a = 0;
        while (*++a) {
            globals.sf[1] = *a;
            for (o = opts;
                 o->action && (!o->sf || !strchr(o->sf, *a)); ++o)
                ;
            if (!o->action)
                unknown(globals.sf);
            globals.curr = o;
            if (!EMPTY(o->metavar)) {
                b = a + 1;
                if (!*b) {
                    b = *globals.argv++;
                    if (!b)
                        opt_err("option %s requires a value");
                }
                skipping = o->action(b, o->data);
                if (b != a + 1)
                    *b = 0;
                break;
            }
            if ((skipping = o->action(NULL, o->data)))
                break;
        }
    }
    return ret;
}

char ***opt_remainder(void)
{
    return &globals.argv;
}




