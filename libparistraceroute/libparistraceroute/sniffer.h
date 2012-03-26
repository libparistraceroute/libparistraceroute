#ifndef SNIFFER_H
#define SNIFFER_H
/**
 * \file sniffer.h
 * \brief Header file : packet sniffer
 *
 * The current implementation is based on raw sockets, but we could envisage a
 * libpcap implementation too
 */

/**
 * \struct sniffer_t
 * \brief Structure representing a packet sniffer
 */
typedef struct {
    int socket;     /*!< Raw socket for listening to the network */
    void (*callback)(void*, void*); /*!< TODO */
} sniffer_t;

/**
 * \brief Creates a new sniffer.
 * \return Pointer to a sniffer_t structure representing a packet sniffer
 */
sniffer_t * sniffer_create(void);

/**
 * \brief Free a sniffer_t structure.
 * \param sniffer Pointer to a sniffer_t structure representing a packet sniffer
 */
void sniffer_free(sniffer_t *sniffer);

/**
 * \brief Return a file description associated to the sniffer.
 * \param sniffer Pointer to a sniffer_t structure representing a packet sniffer 
 */
int sniffer_get_fd(sniffer_t *sniffer);

// TODO
int sniffer_set_callback(sniffer_t *sniffer, void (*callback)(void*, void*));

#endif
