#include "generator.h"

#include <string.h>         // strcmp(), memcpy ...
#include <search.h>         // tfind(), ...
#include <stdio.h>

#include "common.h"         // ELEMENT_COMPARE

// static ?
void * generators_root;


static int generator_compare(
    const generator_t * generator1,
    const generator_t * generator2
) {
    return strcmp(generator1->name, generator2->name);
}

static field_t * generator_get_field(const generator_t * generator, const char * key) {
    field_t * field;

    if (generator->fields) {
        for (field = generator->fields[0]; field->key; field++) {
            if (strcmp(field->key, key) == 0) {
                return field;
            }
        }
    }
    return NULL;
}

generator_t * generator_create_by_name(const char * name)
{
    size_t              size, i , num_fields;
    generator_t       * generator;
    const generator_t * search;

    if (!(search = generator_search(name))) {
        fprintf(stderr, "Unknown generator %s", name);
        goto ERR_SEARCH;
    }

    size = generator_get_size(search);
    if (!(generator = calloc(1, size))) goto ERR_CALLOC;
    memcpy(generator, search, size);
    num_fields = generator->num_fields;
    if (!(generator->fields = malloc(num_fields * sizeof(field_t *)))) goto ERR_MALLOC;
    for (i = 0; i < num_fields; ++i) {
        if (!(generator->fields[i] = field_dup((const field_t *)(search->fields + i)))) goto ERR_DUP;
    }
    return generator;

ERR_DUP:
    free(generator->fields);
ERR_MALLOC:
    generator_free(generator);
ERR_CALLOC:
ERR_SEARCH:
    return NULL;
}

void generator_free(generator_t * generator) {
    size_t i, num_fields = generator_get_num_fields(generator);
    if (generator) {
        for (i = 0; i < num_fields; ++i) {
            if (generator->fields[i]) field_free(generator->fields[i]);
        }
        free(generator->fields);
        free(generator);
    }
}

size_t generator_get_size(const generator_t * generator) {
    return generator->size;
}

generator_t * generator_dup(const generator_t * generator) {
    generator_t * gdup;
    size_t        size = generator_get_size(generator);

    if (!(gdup = malloc(size))) goto ERR_MALLOC;
    memcpy(gdup, generator, size);
    return gdup;

ERR_MALLOC:
    return NULL;
}

bool generator_set_field(generator_t * generator, field_t * field) {
    size_t    i, num_fields = generator->num_fields;
    field_t * field_to_update;

    for (i = 0; i < num_fields; i++) {
        field_to_update = generator->fields[i];
        if (field_match(field_to_update, field)) {
            return field_set_value(field_to_update, &field->value);
        }
    }
    return false;
}

bool generator_set_field_and_free(generator_t * generator, field_t * field)
{
    bool ret = false;

    if (field) {
        ret = generator_set_field(generator, field);
        field_free(field);
    }
    return ret;
}

void generator_dump(const generator_t * generator) {
    size_t i, num_fields = generator_get_num_fields(generator);

    printf("*** GENERATOR : %s ***\n", generator->name);
    for (i = 0; i < num_fields; ++i) {
        printf("\t%s : ", field_get_key(generator->fields[i]) );
        field_dump(generator->fields[i]);
        printf("\n");
    }
}

size_t generator_get_num_fields(const generator_t * generator) {
    return generator->num_fields;
}

bool generator_extract_value(const generator_t * generator, const char * key, void * value) {
    field_t * field;

    if (!(field = generator_get_field(generator, key))) return false;
    memcpy(value, &field->value, field_get_size(field));
    return true;
}

double generator_get_value(const generator_t * generator) {
    return generator->value;
}

double generator_next_value(generator_t * generator) {
    generator->value = generator->get_next_value(generator);
    return generator->value;
}

const generator_t * generator_search(const char * name)
{
    generator_t ** generator, search;

    if (!name) return NULL;
    search.name = name;
    generator = tfind(&search, &generators_root, (ELEMENT_COMPARE) generator_compare);

    return generator ? *generator : NULL;
}

void generator_register(generator_t * generator)
{
    // Insert the generator in the tree if the keys does not exist yet
    tsearch(generator, &generators_root, (ELEMENT_COMPARE) generator_compare);
}
