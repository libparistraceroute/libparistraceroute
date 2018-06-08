#include "config.h"
#include "use.h"    // USE_*

#include <string.h>
#include <stdlib.h>
//#include <search.h>

#include "filter.h"    // filter_t
#include "metafield.h"

/*
static void * metafields_root = NULL;

static int metafield_compare(
    const metafield_t * metafield1,
    const metafield_t * metafield2
) {
    return strcmp(metafield1->name, metafield2->name);
}

metafield_t* metafield_search(const char * name)
{
    metafield_t **metafield, search;

    if (!name) return NULL;
    search.name = name;
    metafield = tfind(&search, &metafields_root, (ELEMENT_COMPARE) metafield_compare);

    return metafield ? *metafield : NULL;
}

void metafield_register(metafield_t * metafield)
{
    // Process the patterns
    // XXX

    // Insert the metafield in the tree if the keys does not yet exist
    tsearch(metafield, &metafields_root, (ELEMENT_COMPARE) metafield_compare);
}
*/

metafield_t * metafield_create(const char * name) {
    metafield_t * metafield = NULL;
    if (!(metafield = malloc(sizeof(metafield_t)))) goto ERR_MALLOC;
    if (!(metafield->filters = list_create(filter_free, filter_fprintf))) goto ERR_LIST_CREATE;
    metafield->name = name;
    return metafield;

ERR_LIST_CREATE:
    free(metafield);
ERR_MALLOC:
    return NULL;
}

void metafield_free(metafield_t * metafield) {
    if (metafield) {
        if (metafield->filters) list_free(metafield->filters);
        free(metafield);
    }
}

void metafield_fprintf(FILE * out, const metafield_t * metafield) {
    const filter_t * filter;
    list_cell_t    * cur;

    fprintf(out, "metafield {\n\tname = %s,\n\tfilters = {\n", metafield->name);
    for (cur = metafield->filters->head; cur; cur = cur->next) {
        filter = cur->element;
        fprintf(out, "\t\t");
        filter_fprintf(out, filter);
        fprintf(out, "\n");
    }
    fprintf(out, "\t}\n}");
}

void metafield_dump(const metafield_t * metafield) {
    metafield_fprintf(stdout, metafield);
    printf("\n");
}

bool metafield_add_filter(metafield_t * metafield, filter_t * filter) {
    return list_push_element(metafield->filters, filter);
}

filter_t * metafield_find_filter(const metafield_t * metafield, const probe_t * probe) {
    const filter_t * filter;
    list_cell_t    * cur;

    for (cur = metafield->filters->head; cur; cur = cur->next) {
        filter = cur->element;
        if (filter_matches(filter, probe)) break;
    }

    return cur ? cur->element : NULL;
}

size_t metafield_get_matching_size_in_bits(const metafield_t * metafield, const probe_t * probe) {
    const filter_t * filter = metafield_find_filter(metafield, probe);
    return filter ? filter_get_matching_size_in_bits(filter, probe) : 0;
}

bool metafield_read(const metafield_t * metafield, const probe_t * probe, uint8_t * buffer, size_t num_bits) {
    const filter_t * filter = metafield_find_filter(metafield, probe);
    return filter ? filter_read(filter, probe, buffer, num_bits) : false;
}

bool metafield_write(const metafield_t * metafield, const probe_t * probe, uint8_t * buffer, size_t num_bits) {
    const filter_t * filter = metafield_find_filter(metafield, probe);
    return filter ? filter_write(filter, probe, buffer, num_bits) : false;
}


//---------------------------------------------------------------------------
// flow_id
//---------------------------------------------------------------------------

metafield_t * metafield_make_flow_id() {
    metafield_t * metafield;
    filter_t    * filter1,
                * filter2,
                * filter3,
                * filter4;

    if (!(metafield = metafield_create("flow_id"))) {
        goto ERR_METAFIELD_CREATE;
    }

#ifdef USE_IPV6
    // Flow IPv6
    if (!(filter1 = filter_create("ipv6.flow_label", NULL))) goto ERR_FILTER_CREATE;
    metafield_add_filter(metafield, filter1);

    // TODO ICMPv6
#endif

#ifdef USE_IPV4
    // Flow IPv4/UDP
    if (!(filter2 = filter_create("ipv4.protocol", "ipv4.src_ip", "ipv4.dst_ip", "tcp.src_port", "tcp.dst_port", NULL))) goto ERR_FILTER_CREATE;
    metafield_add_filter(metafield, filter2);

    // Flow IPv4/TCP
    if (!(filter3 = filter_create("ipv4.protocol", "ipv4.src_ip", "ipv4.dst_ip", "udp.src_port", "udp.dst_port", NULL))) goto ERR_FILTER_CREATE;
    metafield_add_filter(metafield, filter3);

    // Flow ICMP
    if (!(filter4 = filter_create("ipv4.protocol", "ipv4.src_ip", "ipv4.dst_ip", "icmpv4.code", "icmpv4.checksum", NULL))) goto ERR_FILTER_CREATE;
    metafield_add_filter(metafield, filter4);
#endif

    return metafield;

ERR_FILTER_CREATE:
    // Calling metafield_free will release filter1 ... filter4
    metafield_free(metafield);
ERR_METAFIELD_CREATE:
    return NULL;
}


