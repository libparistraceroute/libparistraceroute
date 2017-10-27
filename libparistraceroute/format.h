#ifndef FORMAT
#define FORMAT
#include <stddef.h>

typedef enum format_t {
    FORMAT_DEFAULT,
    FORMAT_JSON,
    FORMAT_XML
} format_t;

const char * format_names[] = {
    "default", // default value
    "json",
    "xml",
    NULL
};

#endif
