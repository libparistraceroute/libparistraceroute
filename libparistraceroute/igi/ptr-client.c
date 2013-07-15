/*
 * Copyright (c) 2006
 * Ningning Hu and the Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#include "setsignal.h"
#include "common.h"

#define PhaseSleep	(0)
#define ABS(a)     (((a) < 0) ? -(a) : (a))

extern char *optarg;               // Option currently parsed
extern int optind,                 // Index of the destination IP in argv
           opterr,
           optopt;

const char * version = "2.1";

// IGI also support -h (help)
typedef struct {
    size_t delay_num;        // OPTION Initialized with -l and in fast probing : ??? If 0, this provokes fast probing.
    size_t packet_size;      // OPTION Initialized with -s : set the length of the probing packets in byte [500B] (payload size)
    size_t phase_num;        // OPTION Initialized with -k : the number of train probed for each source gap
    size_t probe_num;        // OPTION Initialized with -n : set the number of probing packets in each train [60]
    FILE * trace_fp;         // OPTION Initialized with -f : dump packet-level trace into trace_file
    bool verbose;            // OPTION Initialized with -v : verbose mode
    bool debug;              // OPTION Initialized with -d : debug mode
    uint16 dst_port;         // OPTION Initialized with -p
} option_igi_t;

const static option_igi_t option_igi_default = {
    .delay_num = 0,
    .packet_size = PacketSize,
    .phase_num = 3,
    .probe_num = ProbeNum,
    .trace_fp = NULL,
    .verbose = 0,
    .debug = 0,
    .dst_port = START_PORT,
};

struct trace_item {
    int probe_num, packet_size, delay_num;

    double send_times[MaxProbeNum];
    struct pkt_rcd_t rcv_record[MaxProbeNum];
    int record_count;

    double avg_src_gap;
    double avg_dst_gap;

    double b_bw, c_bw, a_bw, ptr;

    struct trace_item * next;
};


// TODO this is our igi_data_t
// <<<<<
double b_bw = 0,                   // Initialized in fast_probing thanks to get_bottleneck_bw
       competing_bw,
       PTR_bw,
       a_bw,
       c_bw[MaxRepeat];

int probing_phase_count = 0;       // Updated by one_phase_probing
double probing_start_time = 0,     // Initialized in fast_probing with the current timestamp
       probing_end_time = 0;

uint16 src_port = 0,
       probing_port;

struct sockaddr_in probing_server,
                   probing_server2;

//RETSIGTYPE (*oldhandler)(int); // initialize in init_connection.

double src_gap_sum = 0; // entry value
double avg_src_gap; // entry value
struct pkt_rcd_t dst_gap[MaxProbeNum];
double dst_gap_sum;
double avg_dst_gap;
int    dst_gap_count,
       src_gap_count;
int    total_count;
double send_times[MaxProbeNum];    // Sending time of each probe (TODO this is our probe->delay)
//double tlt_src_gap,                // Sum of interdelays sending times  (us)
//       tlt_dst_gap;                // Sum of interdelays received times (us)

/* keep the data from dst machine, 128 is for other control string */
char msg_buf[sizeof(struct pkt_rcd_t) * MaxProbeNum + 128];
int  msg_len;

// trace item: record the item used to dump out into trace file
int control_sock,                  // TCP socket, initialized in init_connection
    probing_sock;                  // UDP socket, initialized in init_sockets

struct trace_item * trace_list = NULL;
struct trace_item * trace_tail = NULL;
struct trace_item * cur_trace = NULL;
// >>>>>

/**
 * TODO use our option module
 * \brief Usage message
 */

void Usage()
{
    printf("IGI/PTR-%s client usage:\n\n", version);
    printf("\tptr-client [-n probe_num] [-s packet_size] [-p dst_port]\n");
    printf("\t           [-k repeat_num] [-f trace_file] [-vdh] dst_address\n\n");
    printf("\t-n      set the number of probing packets in each train [60]\n");
    printf("\t-s      set the length of the probing packets in byte [500B]\n");
    printf("\t-p      indicate the dst machine's listening port [10241]\n");
    printf("\t        This is optional, it can itself search for the port\n");
    printf("\t        that the igi_server is using.\n");
    printf("\t-k      the number of train probed for each source gap [3]\n");
    printf("\t-f      dump packet-level trace into trace_file\n");
    printf("\t-v      verbose mode.\n");
    printf("\t-d      debug mode.\n");
    printf("\t-h      print this message.\n");
    printf("dst_address     can be either an IP address or a hostname\n\n");
    exit(1);
}

/**
 * TODO to keep
 * \brief Combine the sec & usec of record into a real number
 * \param record A pkt_rcd_t instance
 * \return The corresponding timestamp
 */

double get_rcd_time(struct pkt_rcd_t record) {
    return (record.sec + (double) record.u_sec / 1000000);
}

/**
 * TODO to keep
 * \brief Dump out all the trace packet time stamps
 */

void dump_trace(option_igi_t * option_igi)
{
    struct trace_item * p;
    int    i, index;
    FILE * trace_fp = option_igi->trace_fp;

    if (trace_fp == NULL) {
        printf("-w not specified, no trace to dump\n");
        return;
    }

    /* first dump out the summary data */
    p = trace_list;
    index = 0;
    fprintf(trace_fp, "\n%%probe_num packet_size delay_num avg_src_gap arv_dst_gap b_bw c_bw a_bw ptr\n");
    fprintf(trace_fp, "summary_data = [\n");
    while (p != NULL) {
        index ++;
        fprintf(trace_fp,"%2d %4d %5d %f %f %12.3f %12.3f %12.3f %12.3f\n",
                p->probe_num,
                p->packet_size,
                p->delay_num,
                p->avg_src_gap,
                p->avg_dst_gap,
                p->b_bw,
                p->c_bw,
                p->a_bw,
                p->ptr);
        p = p->next;
    }
    fprintf(trace_fp, "];\n");
    fprintf(trace_fp, "phase_num = %d; \n\n", index);

    // then the detail packet trace
    p = trace_list;
    index = 0;
    while (p != NULL) {
        index ++;

        fprintf(trace_fp, "send_time_%d = [\n", index);
        for (i=1; i<p->probe_num; i++) {
            fprintf(trace_fp, "%f ",
                    p->send_times[i] - p->send_times[i-1]);
        }
        fprintf(trace_fp, "];\n");
        fprintf(trace_fp, "send_array_size(%d) = %d; \n\n",
                index, p->probe_num - 1);


        fprintf(trace_fp, "recv_time_%d = [\n", index);

        for (i=0; i<p->record_count-1; i++) {
            fprintf(trace_fp, "%f %d ",
                    get_rcd_time(p->rcv_record[i+1]) -
                    get_rcd_time(p->rcv_record[i]),
                    p->rcv_record[i+1].seq);
        }
        fprintf(trace_fp, "];\n");
        fprintf(trace_fp, "recv_array_size(%d) = %d; \n\n",
                index, p->record_count - 1);

        p = p->next;
    }
}


/**
 * TODO This is our igi_data_free.
 * \brief Make a clean exit on interrupts
 * \param signo Signal ID (unused)
 */

//RETSIGTYPE cleanup(int signo)
RETSIGTYPE cleanup(option_igi_t * option_igi)
{
    FILE * trace_fp = option_igi->trace_fp;

    if (trace_fp != NULL) {
        dump_trace(option_igi);
        fclose(trace_fp);
    }

    close(probing_sock); // TODO this will disappear (socketpool)
    close(control_sock);

    // Free trace list
    if (trace_list != NULL) {
        struct trace_item * pre, * p;
        p = trace_list;

        while (p != NULL) {
            pre = p;
            p = p->next;
            free(pre);
        }
    }

    exit(0);
}

/**
 * TODO this is our ALGORITHM_FAILURE, quit() are replaced by pt_algorithm_throw(ALGORITHM_FAILURE)
 * \brief Quit abruptly the algorithm
 */

void quit(option_igi_t * option_igi)
{
    fflush(stdout);
    fflush(stderr);
    //cleanup(1);
    cleanup(option_igi);
    exit(-1); // TODO this will disappear
}

/**
 * TODO replace by get_timestamp (common.h)
 * \brief Get the current time
 * \return The current time
 */

double get_time(option_igi_t * option_igi)
{
    struct timeval tv;
    struct timezone tz;
    double cur_time;

    if (gettimeofday(&tv, &tz) < 0) {
        perror("get_time() fails, exit\n");
        quit(option_igi);
    }

    cur_time = (double) tv.tv_sec + ((double) tv.tv_usec/ (double) 1000000.0);
    return cur_time;
}

/* find the delay number which can set the src_gap exactly as "gap" */
int get_delay_num(option_igi_t * option_igi, double gap)
{
#define Scale 		(10)
    int lower, upper, mid;
    double s_time, e_time, tmp;
    int k;

    lower = 0;
    upper = 16;
    tmp = 133333;

    /* search for upper bound */
    s_time = e_time = 0;
    while (e_time - s_time < gap * Scale) {
        s_time = get_time(option_igi);
        for (k=0; k<upper * Scale; k++) {
            tmp = tmp * 7;
            tmp = tmp / 13;
        }
        e_time = get_time(option_igi);

        upper *= 2;
    }

    /* binary search for delay_num */
    mid = (int)(upper + lower) / 2;
    while (upper - lower > 20) {
        s_time = get_time(option_igi);
        for (k=0; k<mid * Scale; k++) {
            tmp = tmp * 7;
            tmp = tmp / 13;
        }
        e_time = get_time(option_igi);

        if (e_time - s_time > gap * Scale)
            upper = mid;
        else
            lower = mid;

        mid = (int)(upper + lower) / 2;
    }

    return mid;
}

void dump_bandwidth(option_igi_t * option_igi, double tlt_dst_gap)
{
    FILE * trace_fp = option_igi->trace_fp;

    printf("\nPTR: %7.3f Mpbs (suggested)\n", PTR_bw / 1000000);
    printf("IGI: %7.3f Mpbs\n", a_bw / 1000000);
    printf("Probing uses %.3f seconds, %d trains, ending at a gap value of %d us.\n", probing_end_time - probing_start_time,
            probing_phase_count,
            (int)(tlt_dst_gap * 1000000));

    if (trace_fp != NULL) {
        fprintf(trace_fp, "%%Bottleneck Bandwidth: %7.3f Mbps\n", b_bw / 1000000);
        fprintf(trace_fp, "%%Competing  Bandwidth: %7.3f Mpbs\n", competing_bw / 1000000);
        fprintf(trace_fp, "%%Packet Transmit Rate: %7.3f Mpbs\n", PTR_bw / 1000000);
        fprintf(trace_fp, "%%Available  Bandwidth: %7.3f Mpbs\n", a_bw / 1000000);
    }
}

/**
 * TODO We will use our socketpool
 * \brief Create/bind the UDP socket used to emit packet toward the IGI server
 * \param dst_ip The IP address of the IGI server
 */

int init_sockets(option_igi_t * option_igi, uint32 dst_ip)
{
    struct sockaddr_in client;

    probing_sock = socket(AF_INET, SOCK_DGRAM, 0);

    probing_server.sin_family = AF_INET;
    probing_server.sin_addr.s_addr = dst_ip;
    probing_server.sin_port = htons(probing_port);

    /* used for sending the tail "junk" packet */
    probing_server2.sin_family = AF_INET;
    probing_server2.sin_addr.s_addr = dst_ip;
    probing_server2.sin_port = htons(probing_port+1);

    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_family = AF_INET;
    client.sin_port = htons(0);

    if (bind(probing_sock, (const struct sockaddr *) &client,
                sizeof(client)) < 0) {
        fprintf(stderr, "server: Unable to bind local address.\n");
        quit(option_igi);
    }

#ifndef LINUX
    if (fcntl(probing_sock, F_SETFL, O_NDELAY) < 0) {
#else
    if (fcntl(probing_sock, F_SETFL, FNDELAY) < 0) {
#endif
        printf("fcntl fail \n");
        quit(option_igi);
    }

    return 1;
}

/**
 * TODO We'll rewrite this by using address_t type and address_* functions.
 * \brief Fill name, ip, ip_str according to a string value
 * \param string It may be an IPv4 address or a FQDN
 * \param name Points to a pre-allocated buffer in order to store the
 * corresponding hostname. Pass NULL if not interested in.
 * \param ip Points to a pre-allocated IPv4 (binary format). Pass NULL if not
 * interested in.
 * \param ip_str Points to a pre-allocated buffer in order to store the IPv4
 * (string format). Pass NULL if not interested in.
 * \return 0 iif successful, a negative value otherwise.
 */

int get_host_info(char *string, char *name, uint32_t *ip, char *ip_str)
{
    struct hostent *hp;
    struct in_addr in_a;
    uint32_t tmp_ip;

#ifdef SUN
    if (inet_addr(string) == INADDR_BROADCAST) {
#else
    if (inet_addr(string) == INADDR_NONE) {
        // "string" is not an IP address
#endif
        // not interested in IP
        if (!ip && !ip_str) {
            if (name) {
                strcpy(name, string);
            }
            return 0;
        }

        // find hostname
        if ((hp = gethostbyname(string)) == NULL) {
            printf("ERROR: couldn't obtain address for %s\n", string);
            return -1;
        } else {
            if (name) {
                strncpy(name, hp->h_name, MAXHOSTNAMELEN-1);
            }
            if (ip) {
                memcpy((void *)ip, (void *)hp->h_addr, hp->h_length);
            }
            if (ip_str) {
                in_a.s_addr = *ip;
                strcpy(ip_str, inet_ntoa(in_a));
            }
        }

    } else { // string is an IP

        // not interested in name
        if (ip_str) strcpy(ip_str, string);

        tmp_ip = inet_addr(string);
        if (ip) *ip = tmp_ip;

        if (name) {
            if ((hp = gethostbyaddr((char *)&tmp_ip, sizeof(tmp_ip), AF_INET)) == NULL)
                strncpy(name, string, MAXHOSTNAMELEN - 1);
            else
                strncpy(name, hp->h_name, MAXHOSTNAMELEN - 1);
        }
    }
    return 0;
}

/**
 * TODO We keep this function.
 * Parse commands sent by the server.
 *    READY dst_port
 * \param control_sock The TCP socket.
 * \param str A preallocated string where we write the parameter carried by the command (e.g. dst_port)
 */

// the begining of the msg_buf should have the form $**$, return the string
// simplify using index()
void get_item(option_igi_t * option_igi, int control_sock, char *str)
{
    int i, j, cur_len;
    bool debug = option_igi->debug;

    // Check if two $ already exit
    if (msg_len > 0) {
        if (msg_buf[0] != '$') {
            printf("get unknown msg, it will mess up everything, exit\n");
            quit(option_igi);
        }
        i = 1;
        while ((i<msg_len) && (msg_buf[i] != '$')) {
            i++;
        }
        if (i < msg_len) {
            strncpy(str, msg_buf + 1, i - 1);
            str[i - 1] = 0;
            goto OUT;
        }
    }

    // Read until we get two $
    while (1) {
        if (debug) {
            printf("msg_len = %d  ", msg_len);
        }
        cur_len = read(control_sock, msg_buf+msg_len, sizeof(msg_buf) - msg_len);
        // Guard against control channel breaks
        if (cur_len <= 0) {
            printf("read failed in get_item(option_igi, )\n");
            quit(option_igi);
        }

        msg_len += cur_len;
        msg_buf[msg_len] = 0;

        i = 1;
        while ((i<msg_len) && (msg_buf[i] != '$')) {
            i ++;
        }
        if (i < msg_len) {
            strncpy(str, msg_buf + 1, i - 1);
            str[i - 1] = 0;
            break;
        }
    }

OUT:
    // get out the string and move the buf ahead, with i points
    // to the 2nd $
    i++;
    for (j = i; j < msg_len; j++)
        msg_buf[j - i] = msg_buf[j];
    msg_len -= i;
}

/**
 * \brief Establish the TCP connection between the IGI client and the IGI server.
 *    Send START message, wait for READY message.
 *    Prepare the UDP socket.
 *    Initialize src_* and dst_* global variables.
 */

void init_connection(option_igi_t * option_igi, char *dst)
{
    uint32_t src_ip, dst_ip;
    char src[MAXHOSTNAMELEN];          // Source IP, initialized in init_connection
    char dst_ip_str[16],               // Destination IP, string format, initialized in init_connection
         src_ip_str[16];               // Source IP, string format, initialized in init_connection
    char dst_hostname[MAXHOSTNAMELEN], // Destination hostname, initialized in init_connection
         src_hostname[MAXHOSTNAMELEN]; // Source hostname, initialized in init_connection
    bool verbose = option_igi->verbose;
    size_t probe_num = option_igi->probe_num;

    struct sockaddr_in server, src_addr;
    socklen_t src_size;
    int result, msg_len;
    char cur_str[16], msg_buf[64];

    // Setup signal handler for cleaning up
    /*
       // TODO
    (void) setsignal(SIGTERM, cleanup);
    (void) setsignal(SIGINT, cleanup);
    if ((oldhandler = setsignal(SIGHUP, cleanup)) != SIG_DFL)
        (void) setsignal(SIGHUP, oldhandler);
        */

    // Get hostname and IP address of target host
    if (get_host_info(dst, dst_hostname, &dst_ip, dst_ip_str) < 0) {
        quit(option_igi);
    }

    server.sin_addr.s_addr = dst_ip;
    server.sin_family = AF_INET;

    // Search for the right dst port to connect with
    control_sock = socket(AF_INET, SOCK_STREAM, 0);
    ++(option_igi->dst_port);
    server.sin_port = htons(option_igi->dst_port);
    result = connect(control_sock, (struct sockaddr *)& server, sizeof(server));

    while ((result < 0) && (option_igi->dst_port < END_PORT)) {
        close(control_sock);
        control_sock = socket(AF_INET, SOCK_STREAM, 0);
        ++(option_igi->dst_port);
        server.sin_port = htons(option_igi->dst_port);
        result = connect(control_sock, (struct sockaddr *)& server, sizeof(server));
        if (verbose)
            printf("dst_port = %d\n", option_igi->dst_port);
    }

    if (result < 0) {
        perror("no dst port found, connection fails\n");
        quit(option_igi);
    }

    // Get hostname and IP address of source host

    src_size = sizeof(struct sockaddr);
    getsockname(control_sock, (struct sockaddr *)&src_addr, &src_size);
    strcpy(src, inet_ntoa(src_addr.sin_addr));

    // TODO We don't care about this, this is set by ipv4_get_default_src_ip
    if (get_host_info(src, src_hostname, &src_ip, src_ip_str) < 0) {
        printf("get source address info fails, exit\n");
        quit(option_igi);
    }

    if (verbose) {
        printf("src addr: %s\n", src_ip_str);
        printf("dst addr: %s\n", dst_ip_str);
    }

    // Send START message
    sprintf(msg_buf, "$START$%ld$", probe_num);
    msg_len = strlen(msg_buf);
    write(control_sock, msg_buf, msg_len);

    // Wait for READY ack
    get_item(option_igi, control_sock, cur_str);
    if (verbose) {
        printf("we get str: %s\n", cur_str);
    }
    if (strncmp(cur_str, "READY", 5) != 0) {
        printf("get unknow str when waiting for READY from dst, exit\n");
        quit(option_igi);
    }

    printf("The server has sent the following message: %s\n", cur_str); // TODO debug
    get_item(option_igi, control_sock, cur_str);
    if (verbose) {
        printf("probing_port = %s\n", cur_str);
    }
    probing_port = atoi(cur_str);

    if (init_sockets(option_igi, dst_ip) == 0) {
        perror("init sockets failed");
        quit(option_igi);
    }
}


/**
 * TODO TO change use probe_t and pt_send_probe
 * \brief Send a train of probe
 *   Send out probe_num packets, each packet is packet_size bytes,
 *   and the inital gap is set using delay_num
 * \param probe_num The number of probing packets in this train
 * \param packet_size The UDP payload size.
 * \param delay_num See -l option
 * \param sent_times Sending times
 */

void send_packets(double *sent_times, option_igi_t * option_igi)
{
    int i, k;
    double tmp = 133333.0003333;
    char send_buf[4096];
    size_t delay_num = option_igi->delay_num,
           packet_size = option_igi->packet_size,
           probe_num = option_igi->probe_num;

    // Send out probing packets
    for (i = 0; i< probe_num - 1; i++) {
        // todo: the middle send_times are not useful any more, since
        // we don't use sanity-check
        sent_times[i] = get_time(option_igi);
        send_buf[0] = i;
        sendto(probing_sock, send_buf, packet_size, 0, (struct sockaddr *)&(probing_server), sizeof(probing_server));

        // gap generation
        for (k = 0; k < delay_num; k++) {
            tmp = tmp * 7;
            tmp = tmp / 13;
        }
    }

    // The last packets
    send_buf[0] = i;
    sendto(probing_sock, send_buf, packet_size, 0, (struct sockaddr *)&(probing_server), sizeof(probing_server));

    sendto(probing_sock, send_buf, 40, 0, (struct sockaddr *)&(probing_server2), sizeof(probing_server2));
    sent_times[probe_num - 1] = get_time(option_igi);
}

/**
 * \brief Get dst gap_sum and gap_count from the records. It sums the inter-delay
 *   between each probe packet and computes the corresponding arithmetic mean.
 * \param rcv_record A set of pkt_rcd_t instances (TODO this is close to our probe_reply_t)
 * \param count The number of pkt_rcd_t instances.
 * \param gap_count Poiints to a pre-allocated int which will store the gap_count.
 * \return The arithmetic mean of each interdelays.
 */

double get_dst_sum(struct pkt_rcd_t *rcv_record, int count, int *gap_count)
{
    double gap_sum, time1, time2;
    int i;

    gap_sum = 0;
    *gap_count = 0;
    for (i = 0; i < count - 1; i++) {
        // Only consider those gaps composed by two in-order packets
        if (rcv_record[i + 1].seq - rcv_record[i].seq == 1) {
            time1 = get_rcd_time(rcv_record[i+1]);
            time2 = get_rcd_time(rcv_record[i]);
            gap_sum += (time1 - time2);
            (*gap_count)++;
        }
    }

    return gap_sum;
}

/**
 * \brief Get dst gap trace data through control channel
 * \param rcv_record
 * \return 0 means fails in reading dst's data
 */

int get_dst_gaps(option_igi_t * option_igi, struct pkt_rcd_t * rcv_record)
{
    bool verbose = option_igi->verbose,
         debug   = option_igi->debug;
    FILE * trace_fp = option_igi->trace_fp;
    int i, cur_len, data_size, total_count, ptr;
    char num_str[32];

    get_item(option_igi, control_sock, num_str);
    data_size = atoi(num_str); // this depends on stdlib.h
    total_count = data_size / sizeof(struct pkt_rcd_t);
    if (verbose) {
        printf("from dst: data_size = %d total_count = %d \n", data_size, total_count);
    }

    // Get the data package
    while (msg_len < data_size) {
        cur_len = read(control_sock, msg_buf+msg_len, sizeof(msg_buf) - msg_len);
        if (cur_len <= 0) {
            printf("read failed in get_dst_gaps(option_igi, )\n");
            quit(option_igi);
        }
        msg_len += cur_len;
    }

    // Split out the record
    ptr = 0;
    for (i=0; i<total_count; i++) {
        memcpy((void *)&rcv_record[i], (const void *)(msg_buf + ptr), sizeof(struct pkt_rcd_t));

        rcv_record[i].sec = ntohl(rcv_record[i].sec);
        rcv_record[i].u_sec = ntohl(rcv_record[i].u_sec);
        rcv_record[i].seq = ntohl(rcv_record[i].seq);

        if (debug)
            printf("%d %d %d \n", rcv_record[i].sec, rcv_record[i].u_sec, rcv_record[i].seq);
        ptr += sizeof(struct pkt_rcd_t);
    }
    msg_len -= data_size;

    if (trace_fp != NULL) {
        memcpy(cur_trace->rcv_record, rcv_record, data_size);
        cur_trace->record_count = total_count;
    }

    return total_count;
}

/**
 *
 */

// here we use some a similar technique with nettimer, but the
// implementation is much simpler
double get_bottleneck_bw(struct pkt_rcd_t *rcv_record, int count, option_igi_t * option_igi)
{
    bool debug = option_igi->debug;
    size_t packet_size = option_igi->packet_size;

    double gaps[MaxProbeNum];
    int gap_count;

    struct bin_item {
        int value;
        int count;
    } bin[MaxProbeNum];
    int bin_count;

    double lower_time, upper_time;
    int max_index, max_count, max_value, cur_value, i, j;

    double gap_sum;
    int sum_count;

    // Get valid dst gaps
    gap_count = 0;
    for (i = 0; i < count-1; i++) {
        if (rcv_record[i + 1].seq - rcv_record[i].seq == 1) {
            gaps[gap_count] = get_rcd_time(rcv_record[i + 1]) - get_rcd_time(rcv_record[i]);
            gap_count ++;
        }
    }

    // Use 25us bins to go through all the gaps
    bin_count = 0;
    for (i = 0; i < gap_count; i++) {
        cur_value = (int)(gaps[i] / BinWidth);
        j = 0;
        while ((j < bin_count) && (cur_value != bin[j].value))  {
            j++;
        }

        if (j == bin_count) {
            bin[bin_count].value = cur_value;
            bin[bin_count].count = 1;
            bin_count ++;
        } else {
            bin[j].count ++;
        }
    }

    // Find out the biggest bin
    max_index = 0; max_count = bin[0].count;
    for (i=1; i<bin_count; i++) {
        if (bin[i].count > max_count) {
            max_count = bin[i].count;
            max_index = i;
        }
    }
    max_value = bin[max_index].value;
    lower_time = max_value * BinWidth;
    upper_time = (max_value + 1) * BinWidth;

    // See whether the adjacent two bins also have some items
    for (i = 0; i < bin_count; i++) {
        if (bin[i].value == max_value + 1)
            upper_time += BinWidth;
        if (bin[i].value == max_value - 1)
            lower_time -= BinWidth;
    }

    if (debug) {
        printf("get_bottleneck_bw: %f %f \n", lower_time, upper_time);
    }

    // average them
    gap_sum = 0;
    sum_count = 0;
    for (i =0; i < gap_count; i++) {
        if ((gaps[i] >= lower_time) && (gaps[i] <= upper_time)) {
            gap_sum += gaps[i];
            sum_count++;
        }
    }

    return (packet_size * 8 / (gap_sum / sum_count));
}

// Calculate the competing traffic rate using IGO method
double get_competing_bw(struct pkt_rcd_t *rcv_record, double avg_src_gap, int count, double b_bw, option_igi_t * option_igi)
{
    bool debug = option_igi->debug;
    size_t packet_size = option_igi->packet_size;
    double b_gap, cur_gap, m_gap;
    double gap_sum, inc_gap_sum;
    int i;

    b_gap = packet_size * 8 / b_bw;
    m_gap = MAX(b_gap, avg_src_gap);

    gap_sum = inc_gap_sum = 0;
    for (i=0; i<count-1; i++) {
        /* only consider those gaps composed by two in-order packets */
        if (rcv_record[i+1].seq - rcv_record[i].seq == 1) {
            cur_gap = get_rcd_time(rcv_record[i+1])
                - get_rcd_time(rcv_record[i]);
            gap_sum += cur_gap;

            if (cur_gap > m_gap + 0.000005)
                inc_gap_sum += (cur_gap - b_gap);
        }
    }

    if (debug) {
        printf("b_gap = %f %f %f \n", b_gap, inc_gap_sum, gap_sum);
    }

    if (gap_sum == 0) {
        printf("\nNo valid trace in the last phase probing\n");
        printf("You may want to try again. Exit\n");
        quit(option_igi);
    }

    return (inc_gap_sum * b_bw / gap_sum);
}

/**
 * \brief Return sum of the "valid" src gaps
 * \param times The times when the probe packet are sent
 * \return The sum of each interdelay (= gap) of probe emission
 */

double get_src_sum(option_igi_t * option_igi, double * times)
{
    size_t probe_num = option_igi->probe_num;
    int i;
    double sum;

    sum = 0;
    for (i = 0; i < probe_num - 1; i++)
        sum += (times[i+1] - times[i]);
    return sum;
}

void get_bandwidth(option_igi_t * option_igi)
{
    if (dst_gap_count == 0)
        competing_bw = PTR_bw = a_bw = 0;
    else {
        if (b_bw < 1)
            b_bw = get_bottleneck_bw((struct pkt_rcd_t *)&dst_gap, total_count, option_igi);
        competing_bw = get_competing_bw((struct pkt_rcd_t *)&dst_gap, avg_src_gap, total_count, b_bw, option_igi);
    }
}


/**
 * one complete probing procedure, which includes
 * 1) send out packets with (probe_num, packet_size, delay_num)
 * 2) retrieve the dst gaps values from dst machine
 * 3) filter out those bad trace data and compute the src gap & dst
 *    gap summary and average
 */

void one_phase_probing(option_igi_t * option_igi, double * tlt_src_gap, double * tlt_dst_gap)
{
    bool verbose = option_igi->verbose,
         debug = option_igi->debug;
    size_t probe_num = option_igi->probe_num,
           packet_size = option_igi->packet_size,
           delay_num = option_igi->delay_num;
    FILE * trace_fp = option_igi->trace_fp;

    if (verbose) {
        printf("\nprobe_num = %ld packet_size = %ld delay_num = %ld\n",
                probe_num, packet_size, delay_num);
    }

    // probing
    send_packets(send_times, option_igi);
    if (trace_fp) {
        // create a new trace item
        cur_trace = malloc(sizeof(struct trace_item));
        cur_trace->next = NULL;

        if (trace_list == NULL) {
            trace_list = cur_trace;
            trace_tail = cur_trace;
        } else {
            trace_tail->next = cur_trace;
            trace_tail = cur_trace;
        }
    }

    // Get dst gap logs
    total_count = get_dst_gaps(option_igi, (struct pkt_rcd_t *)&dst_gap);

    // Compute avg_src_gap
    src_gap_count = probe_num - 1;
    if (debug) {
        printf("src_gap_count = %d \n", src_gap_count);
    }

    src_gap_sum = get_src_sum(option_igi, send_times);
    if (src_gap_count) {
        avg_src_gap = src_gap_sum / src_gap_count;
    } else {
        avg_src_gap = 0;
    }

    // Compute avg_dst_gap
    dst_gap_sum = get_dst_sum((struct pkt_rcd_t *)&dst_gap, total_count, &dst_gap_count);
    if (debug) {
        printf("%d dst_gap_count = %d \n", total_count, dst_gap_count);
    }

    if (dst_gap_count != 0) {
        avg_dst_gap = dst_gap_sum / dst_gap_count;
    } else {
        avg_dst_gap = 0;
    }

    // record total src gaps
    *tlt_src_gap += avg_src_gap;
    *tlt_dst_gap += avg_dst_gap;

    if (verbose) {
        printf("gaps (us): %5.0f %5.0f | %5.0f %5.0f\n",
            *tlt_src_gap * 1000000,
            *tlt_dst_gap * 1000000,
            avg_src_gap  * 1000000,
            avg_dst_gap  * 1000000
        );
    }

    if (trace_fp) {
        double tmp1, tmp2, tmp3;

        cur_trace->probe_num = probe_num;
        cur_trace->packet_size = packet_size;
        cur_trace->delay_num = delay_num;

        memcpy(cur_trace->send_times, send_times, sizeof(double) * probe_num);
        cur_trace->avg_src_gap = avg_src_gap;
        cur_trace->avg_dst_gap = avg_dst_gap;

        // we can't use get_bandwidth() here since we want to record
        // the current data, which may be wrong value
        if (dst_gap_count == 0) {
            cur_trace->b_bw = 0;
        } else {
            cur_trace->b_bw = get_bottleneck_bw((struct pkt_rcd_t *)&dst_gap, total_count, option_igi);
        }

        tmp1 = PTR_bw;
        tmp2 = competing_bw;
        tmp3 = a_bw;
        get_bandwidth(option_igi);
        cur_trace->ptr = PTR_bw;
        cur_trace->c_bw = competing_bw;
        cur_trace->a_bw = a_bw;
        PTR_bw = tmp1;
        competing_bw = tmp2;
        a_bw = tmp3;
    }

    probing_phase_count++;
}

/**
 * TODO to keep
 * Initialize tlt_src_gap, tlt_dst_gap, a_bw, b_bw, c_bw, PTR_bw
 * \param n The phase number
 */

void n_phase_probing(int n, option_igi_t * option_igi)
{
    bool verbose = option_igi->verbose;
    size_t packet_size = option_igi->packet_size,
           phase_num   = option_igi->phase_num;
    int i, j, k;

    double tlt_src_gap = 0,
           tlt_dst_gap = 0;

    for (i = 0; i < n; i++) {
        one_phase_probing(option_igi, &tlt_src_gap, &tlt_dst_gap);
        get_bandwidth(option_igi);

        /* insert, so as to be able to pick the median easily */
        j = 0;
        while (j < i && c_bw[j] < competing_bw) {
            j++;
        }
        for (k = i - 1; k >= j; k--) {
            c_bw[k + 1] = c_bw[k];
        }
        c_bw[j] = competing_bw;
    }

    PTR_bw = packet_size * 8 * phase_num / tlt_dst_gap;
    a_bw = b_bw - c_bw[(int) (n / 2)];

    if (verbose) {
        printf("------------------------------------------------------\n");
    }
}

int gap_comp(option_igi_t * option_igi, double dst_gap, double src_gap)
{
    bool verbose = option_igi->verbose;
    size_t phase_num = option_igi->phase_num;
    double delta = 0.05;

    if (dst_gap < src_gap / (1 + delta)) {
        if (verbose)
            printf("smaller dst_gap, considered to be equal \n");
        return 0;
    }

    if (dst_gap > src_gap + 0.000005 * phase_num)
        return 1;
    else
        return 0;
}

/**
 * \brief Estimate IGI and PTR value (fast probing)
 */

void fast_probing(option_igi_t * option_igi)
{
    double tlt_src_gap = 0,
           tlt_dst_gap = 0;
    bool   verbose = option_igi->verbose;
    int    double_check = 0,
           first = 1,
           tmp_num;
    option_igi->delay_num = 0;
    double interval,
           saved_ptr,
           saved_abw,
           pre_gap = 0;

    probing_start_time = get_time(option_igi);

    n_phase_probing(option_igi->phase_num, option_igi);
    b_bw = get_bottleneck_bw((struct pkt_rcd_t *) &dst_gap, total_count, option_igi);

    option_igi->delay_num = get_delay_num(option_igi, avg_dst_gap);
    interval = (double) option_igi->delay_num / 4;
    option_igi->delay_num /= 2;

    while (1) {
        if (gap_comp(option_igi, tlt_dst_gap, tlt_src_gap) == 0) {
            if (double_check) {
                tlt_dst_gap = pre_gap;
                break;
            } else {
                pre_gap = tlt_dst_gap;
                double_check = 1;
            }
        } else {
            if (double_check && verbose) {
                printf("DISCARD: gap = %d\n", (int)(tlt_dst_gap * 1000000));
            }
            double_check = 0;
        }

        if (!first && (tlt_dst_gap) >= 1.5 * (tlt_src_gap)) {
            tmp_num = get_delay_num(option_igi, (tlt_src_gap + tlt_dst_gap) / (2 * total_count));
            if (tmp_num > option_igi->delay_num) {
                option_igi->delay_num = tmp_num;
            }
        }
        option_igi->delay_num = (size_t)(option_igi->delay_num + interval);

        // todo: need a better way to deal with them
        saved_ptr = PTR_bw;
        saved_abw = a_bw;
        n_phase_probing(option_igi->phase_num, option_igi);
        first = 0;
    }
    PTR_bw = saved_ptr;
    a_bw = saved_abw;

    probing_end_time = get_time(option_igi);
    dump_bandwidth(option_igi, tlt_dst_gap);
}

int main(int argc, char *argv[])
{
    int opt;
    option_igi_t option_igi;
    char dst[MAXHOSTNAMELEN];  // Destination IP or hostname, set by the user
    double tlt_src_gap = 0,
           tlt_dst_gap = 0;
    memcpy(&option_igi, &option_igi_default, sizeof(option_igi_t));

    // Parse options
    while ((opt = getopt(argc, argv, "k:l:n:s:p:f:dhv")) != EOF) {
        switch ((char)opt) {
            case 'k':
                option_igi.phase_num = atoi(optarg);
                if (option_igi.phase_num > MaxRepeat) {
                    option_igi.phase_num = MaxRepeat;
                    printf("phase_num is too large, reset as %d\n", MaxRepeat);
                }
                break;
            case 'l':
                option_igi.delay_num = atoi(optarg);
                break;
            case 'n':
                option_igi.probe_num = atoi(optarg);
                if (option_igi.probe_num > MaxProbeNum) {
                    option_igi.probe_num = MaxProbeNum;
                    printf("probe_num is too large, reset as %d\n", MaxProbeNum);
                }
                break;
            case 's':
                option_igi.packet_size = atoi(optarg);
                break;
            case 'p':
                option_igi.dst_port = atoi(optarg);
                break;
            case 'f':
                if ((option_igi.trace_fp = fopen(optarg, "w")) == NULL) {
                    printf("tracefile open fails, exits\n");
                    exit(1);
                }
                break;

            case 'd':
                option_igi.debug = true;
                break;
            case 'v':
                option_igi.verbose = true;
                break;
            case 'h':
            default:
                Usage();
        }
    }

    // Retrieve dst_ip
    switch (argc - optind) {
        case 1:
            strcpy(dst, argv[optind]);
            break;
        default:
            Usage();
    }

    init_connection(&option_igi, dst);

    // Allow dst 2 seconds to start up the packet filter
    sleep(2);

    // Probing
    if (option_igi.delay_num > 0) {
        one_phase_probing(&option_igi, &tlt_src_gap, &tlt_dst_gap);
        get_bandwidth(&option_igi);
        dump_bandwidth(&option_igi, tlt_dst_gap);
    } else {
        // Default behaviour
        fast_probing(&option_igi);
    }

    // Finishing
   // cleanup(1);
    cleanup(&option_igi);
    return 0;
}
