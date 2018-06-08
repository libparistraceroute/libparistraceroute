#include "config.h"
#include "use.h"    // USE_*

#include <assert.h> // assert
#include <stdarg.h> // va_*
#include <stdlib.h> // malloc, free
#include <string.h> // strcmp

#include "bits.h"   // bits_*
#include "filter.h" // filter_t

filter_t * filter_create(const char * expr1, ...) {
    const char * expr;
    filter_t   * filter;
    va_list      args;

    if (!(filter = list_create(NULL, fprintf))) goto ERR_MALLOC;

    va_start(args, expr1);
    for (expr = expr1; expr; expr = va_arg(args, const char *)) {
        if (!list_push_element(filter, (char *) expr)) {
            goto ERR_LIST_PUSH_ELEMENT;
        }
    }
    va_end(args);
    return filter;

ERR_LIST_PUSH_ELEMENT:
    filter_free(filter);
ERR_MALLOC:
    return filter;
}

void filter_free(filter_t * filter) {
    list_free(filter);
}

static bool filter_parse_fieldname(
    const char * fieldname,
    char       * buffer_protocol,
    size_t       buffer_protocol_size,
    char       * buffer_field,
    size_t       buffer_field_size
) {
    const char * s;
    char       * t;
    size_t       dot_index, i;

    // Extract protocol name
    dot_index = 0;
    t = buffer_protocol;
    for (s = fieldname, dot_index = 0; *s && *s != '.'; dot_index++) {
        if (!s || dot_index == buffer_protocol_size) return false;
        *t++ = *s++;
    }
    if (dot_index == buffer_protocol_size) return false;
    *t = '\0';

    // Extract field name
    if (!*s) return false;
    t = buffer_field;
    for (s = s + 1, i = 0; *s; i++) {
        if (i == buffer_field_size) return false;
        *t++ = *s++;
    }
    if (i == buffer_field_size) return false;
    *t = '\0';

    return true;
}

bool filter_iter(
    const filter_t * filter,
    const probe_t  * probe,
    bool (*callback)(
        const filter_t         * filter,
        const probe_t          * probe,
        const layer_t          * layer,
        const protocol_field_t * protocol_field,
        void                   * user_data
   ),
   void * user_data
) {
    list_cell_t * cur;
    const char  * fieldname;
    char          protocol_name[20];
    char          field_name[20];
    layer_t     * layer;
    size_t        i,
                  depth = 0,
                  num_layers = probe_get_num_layers(probe);

    for (cur = filter->head; cur; cur = cur->next) {
        // Extract from the current peace of filter the protocol and the fieldname
        // e.g. "ipv4.icmpv4" --> "ipv4" "icmpv4"
        fieldname = cur->element;
        if (!filter_parse_fieldname(
            fieldname,
            protocol_name, sizeof(protocol_name),
            field_name,    sizeof(field_name)
        )) {
            fprintf(stderr, "filter_iter: cannot parse '%s'\n", fieldname);
        }

        // Find the corresponding layer, if any.
        layer = NULL;
        for (i = depth; i < num_layers; i++) {
            if (!strcmp(probe_get_protocol_name(probe, i), protocol_name)) {
                layer = probe_get_layer(probe, i);
                break;
            }
        }

        if (!layer) {
            //fprintf(stderr, "filter_get_matching_size_in_bits: '%s' not found in this probe:\n", protocol_name);
            //probe_fprintf(stderr, probe);
            return false; // abort
        }

        // Fieldnames appear in the same order in the packet and in the filters.
        depth = i;

        // Find the field, if any.
        const protocol_field_t * protocol_field = layer_get_protocol_field(layer, field_name);
        if (!protocol_field) {
            //fprintf(stderr, "filter_get_matching_size_in_bits: '%s' protocol does not have field name '%s'\n", protocol_name, field_name);
            return false; // abort
        } else {
            if (!callback(filter, probe, layer, protocol_field, user_data)) break;
        }
    }
    return true;
}

void filter_dump(const filter_t * filter) {
    list_dump(filter);
}

void filter_fprintf(FILE * out, const filter_t * filter) {
    list_fprintf(out, filter);
}

//---------------------------------------------------------------------------
// filter_get_matching_size_in_bits
//---------------------------------------------------------------------------

static bool filter_size_in_bits_callback(
    const filter_t         * filter,
    const probe_t          * probe,
    const layer_t          * layer,
    const protocol_field_t * protocol_field,
    void                   * _user_data
) {
    size_t * user_data = (size_t *) _user_data;
    *user_data += protocol_field_get_size_in_bits(protocol_field);
    return true; // continue until having processed each field of the metafield.
}

size_t filter_get_matching_size_in_bits(const filter_t * filter, const probe_t * probe) {
    size_t size_in_bits = 0;
    return filter_iter(filter, probe, filter_size_in_bits_callback, &size_in_bits) ?
        size_in_bits : 0;
}

bool filter_matches(const filter_t * filter, const probe_t * probe) {
    return filter_get_matching_size_in_bits(filter, probe) > 0;
}

//---------------------------------------------------------------------------
// filter_read
// TODO: factorize filter_{read|write}_callback
//---------------------------------------------------------------------------

typedef struct filter_read_data_s {
    // Input buffer
    size_t    num_bits_processed;

    // Output buffer
    size_t    num_bits_available;
    uint8_t * output_buffer;

#ifdef USE_BITS
    uint8_t   input_offset;
    uint8_t   output_offset;
#endif
} filter_read_data_t;

static bool filter_read_callback(
    const filter_t         * filter,
    const probe_t          * probe,
    const layer_t          * layer,
    const protocol_field_t * protocol_field,
    void                   * _user_data
) {
    bool                 no_offset = true;
    size_t               field_segment_size_in_bits;
    uint8_t            * field_segment;
    filter_read_data_t * user_data = (filter_read_data_t *) _user_data;

    if (!(field_segment = layer_get_field_segment(layer, protocol_field->key))) {
        fprintf(
            stderr,
            "filter_read_callback: in layer '%s': cannot find '%s' field\n",
            layer->protocol->name,
            protocol_field->key
        );
        return false;
    }

    field_segment_size_in_bits = protocol_field_get_size_in_bits(protocol_field);
    assert(field_segment_size_in_bits <= user_data->num_bits_available);
    if (no_offset && (field_segment_size_in_bits % 8 == 0)) {
        memcpy(user_data->output_buffer, field_segment, field_segment_size_in_bits / 8);
        user_data->num_bits_processed += field_segment_size_in_bits;
        user_data->num_bits_available -= field_segment_size_in_bits;
        user_data->output_buffer      += field_segment_size_in_bits / 8;
    } else {
        fprintf(stderr, "filter_read_callback: unaligned field: not yet implemented\n");
    }

    return true; // continue until having processed each field of the filter.
}

bool filter_read(const filter_t * filter, const probe_t * probe, uint8_t * output_buffer, size_t num_bits) {
    bool success;
    size_t filter_size_in_bits;
    filter_read_data_t user_data = {
        .num_bits_processed = 0,
        .num_bits_available = num_bits,
        .output_buffer      = output_buffer,

    };

    // The probe does not match any filter supplied by the filter.
    if (!filter) return false;

    // Check whether the buffer is large enough to store the data.
    filter_size_in_bits = filter_get_matching_size_in_bits(filter, probe);
    if (filter_size_in_bits > num_bits) {
        fprintf(stderr, "filter_read: buffer too small (size in bits: %zu, required: %zu)\n", num_bits, filter_size_in_bits);
        return false;
    }

    // Extract
    success = filter_iter(filter, probe, filter_read_callback, &user_data);
    assert(success);

    return true;
}

//---------------------------------------------------------------------------
// filter_write
//---------------------------------------------------------------------------

typedef struct filter_write_data_s {
    // Input buffer
    size_t          num_bits_processed;
    const uint8_t * input_buffer;

    // Output (metafield bits)
    size_t          num_bits_available;

#ifdef USE_BITS
    uint8_t         input_offset;
    uint8_t         output_offset;
#endif
} filter_write_data_t;

static bool filter_write_callback(
    const filter_t         * filter,
    const probe_t          * probe,
    const layer_t          * layer,
    const protocol_field_t * protocol_field,
    void                   * _user_data
) {
    bool                  no_offset = true;
    size_t                field_segment_size_in_bits;
    uint8_t             * field_segment;
    filter_write_data_t * user_data = (filter_write_data_t *) _user_data;

    if (!(field_segment = layer_get_field_segment(layer, protocol_field->key))) {
        fprintf(
            stderr,
            "filter_write_callback: in layer '%s': cannot find '%s' field\n",
            layer->protocol->name,
            protocol_field->key
        );
        return false;
    }

    field_segment_size_in_bits = protocol_field_get_size_in_bits(protocol_field);
    if (no_offset && (field_segment_size_in_bits % 8 == 0)) {
        assert(field_segment_size_in_bits <= user_data->num_bits_available);
        memcpy(field_segment, user_data->input_buffer, field_segment_size_in_bits / 8);
    } else {
        if (!(bits_write(
            field_segment,              user_data->output_offset,
            user_data->input_buffer,    user_data->input_offset,
            field_segment_size_in_bits
        ))) {
            fprintf(stderr, "filter_write_callback: bits_write failed");
            return false;
        }
    }

    // Update stats
    user_data->num_bits_processed += field_segment_size_in_bits;
    user_data->num_bits_available -= field_segment_size_in_bits;

    // Update pointers and offsets
#ifdef USE_BITS
    user_data->input_offset += field_segment_size_in_bits;
    if (user_data->input_offset >= 8) {
        user_data->input_buffer += user_data->input_offset / 8;
        user_data->input_offset %= 8;
    }

    user_data->output_offset += field_segment_size_in_bits;
    if (user_data->output_offset >= 8) {
        user_data->output_offset %= 8;
    }
#else
    user_data->input_buffer += field_segment_size_in_bits / 8;
#endif

    return true; // continue until having processed each field of the filter.
}

bool filter_write(const filter_t * filter, const probe_t * probe, const uint8_t * input_buffer, size_t num_bits) {
    bool success;
    size_t filter_size_in_bits;
    filter_write_data_t user_data = {
        .num_bits_processed = 0,
        .num_bits_available = num_bits,
#ifdef USE_BITS
        .input_offset       = 0,
        .output_offset      = 0,
#endif
        .input_buffer       = input_buffer,
    };

    // The probe does not match any filter supplied by the filter.
    if (!filter) return false;

    // Check whether the packet is large enough to store the data.
    filter_size_in_bits = filter_get_matching_size_in_bits(filter, probe);
    if (num_bits > filter_size_in_bits) {
        fprintf(stderr, "filter_write: buffer too large (size in bits: %zu, maximum size: %zu)\n", num_bits, filter_size_in_bits);
        return false;
    }

    // Write
    success = filter_iter(filter, probe, filter_write_callback, &user_data);
    assert(success);

    return user_data.num_bits_processed == num_bits;
}

