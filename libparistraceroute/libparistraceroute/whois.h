#ifndef WHOIS_H
#define WHOIS_H

#include <stdint.h>		// uint32_t
#include <stdbool.h>	// bool

#include "address.h"	// address_t

/**
 * \brief Default callback for whois_* function.
 * \param pdata Pointer passed as 'pdata' to the whois_* function.
 * \param line The current fetched line of the whois reply.
 * \return Always true (which means: continue to read the whois reply.)
 */

bool whois_callback_print(void * pdata, const char * line);

/**
 * \brief Perform a whois query.
 * \sa also whois().
 * \param server_address The address of the whois server.
 *    If the server_address is unknown you can either
 *    - call whois_find_server(), and then whois_query().
 *    - call whois() which wraps these both functions.
 * \param queried_address The queried IP address.
 * \param callback Function called back whenever a line
 *    from the whois reply is fetched. It receives as parameter
 *    pdata and the current fetched line. whois next line
 *    is fetched iff it returns true.
 * \param pdata A pointer to a structure passed to the callback.
 *    If the whois_query must populate a structure, you could
 *    typically pass its address an populate it using the
 *    callback.
 * \example whois_query(server_address, queried_address, whois_callback_print, stdout);
 * \return true iff successful.
 */

bool whois_query(
	const address_t * server_address,
	const address_t * queried_address,
	bool (*callback)(void *, const char *),
	void            * pdata
);

/**
 * \brief Find the whois server related to a given IP address.
 * \param queried_address The queried IP address.
 * \param whois_server A preallocated buffer, which will store the
 *     FQDN of the whois server.
 * \return true iff successful.
 */

bool whois_find_server(
	const address_t * queried_address,
	char            * whois_server
);

/**
 * \brief Perform a whois query (whois_find_server + whois_query).
 * \param queried_address The queried IP address.
 * \param callback Function called back whenever a line
 *    from the whois reply is fetched. It receives as parameter
 *    pdata and the current fetched line. whois next line
 *    is fetched iff it returns true.
 * \param pdata A pointer to a structure passed to the callback.
 *    If the whois_query must populate a structure, you could
 *    typically pass its address an populate it using the
 *    callback.
 * \return true iff successful.
 */

bool whois(
	const address_t * queried_address,
	bool (*callback)(void *, const char *),
	void            * pdata
);

/**
 * \brief Perform a whois query (whois_find_server + whois_query).
 * \example
    uint32_t asn;
    address_t address;
    address_from_string(AF_INET, "193.164.11.22", &address);
    whois_get_asn(&queried_address, &asn, CACHE_ENABLED);
 * \param queried_address The queried IP address.
 * \param asn The address of an uint32_t, where the ASN will be
 *    written (iff successful).
 * \param mask A value among CACHE_ENABLED, CACHE_WRITE,
 *    CACHE_DISABLED, CACHE_WRITE
 * \return true iff the asn has been successfully retrieved.
 */

bool whois_get_asn(
	const address_t * queried_address,
	uint32_t        * asn,
	int               mask
);

#endif
