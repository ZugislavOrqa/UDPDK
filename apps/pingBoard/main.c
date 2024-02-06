//
// Created by leoll2 on 10/4/20.
// Copyright (c) 2020 Leonardo Lai. All rights reserved.
//
// Options:
//  -f <func>  : function ('ping' or 'pong')
//

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#include <udpdk_api.h>

#define PORT_PING   42000
#define PORT_PONG   42000
#define IP_PONG     "192.68.42.101"
#define MAX_SAMPLES 1000
#define PING_PKT_S 64


struct ping_pkt {
    struct icmphdr hdr;
    char msg[PING_PKT_S - sizeof(struct icmphdr)];
};


typedef enum {PING, PONG} app_mode;

static app_mode mode = PING;
static volatile int app_alive = 1;
static int log_enabled = 0;
static char *log_file;
static FILE *log;
static unsigned delay = 1000000;
static unsigned samples[MAX_SAMPLES];
static unsigned n_samples = 0;
static const char *progname;

unsigned short checksum(void* b, int len) {
    unsigned short* buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

static void signal_handler(int signum)
{
    printf("Caught signal %d in pingpong main process\n", signum);
    udpdk_interrupt(signum);
    app_alive = 0;
}

static void ping_body(void)
{
    struct sockaddr_in servaddr, destaddr;
    struct timespec ts, ts_msg, ts_now;
    int n;

    printf("PING mode\n");

    // Open log file
    if (log_enabled) {
        log = fopen(log_file, "w");
        if (log == NULL) {
            printf("Error opening log file: %s\n", log_file);
        }
    }

    // Create a socket
    int sock;
    if ((sock = udpdk_socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "Ping: socket creation failed");
        return;
    }
    // Bind it
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT_PING);
    if (udpdk_bind(sock, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "bind failed");
        return;
    }

    char rbuffer[128];
    struct ping_pkt* r_pckt;
 
    struct ping_pkt pckt;
    struct sockaddr_in r_addr;
    struct timespec time_start, time_end, tfs, tfe;
    long double rtt_msec = 0, total_msec = 0;
    struct timeval tv_out;
    tv_out.tv_sec = 15;
    tv_out.tv_usec = 0;
    int i=0, msg_count = 0;
 
    clock_gettime(CLOCK_MONOTONIC, & tfs);

    while (app_alive) {

        // filling packet
        bzero(& pckt, sizeof(pckt));
 
        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = getpid();
 
        memset(&pckt,sizeof(pckt),0);
 
        pckt.msg[i] = 0;
        pckt.hdr.un.echo.sequence = msg_count++;
        pckt.hdr.checksum
            = checksum(& pckt, sizeof(pckt));

        // Send ping
        if (!log_enabled)
            printf("Sending ping\n");
        destaddr.sin_family = AF_INET;
        destaddr.sin_addr.s_addr = inet_addr(IP_PONG);
        destaddr.sin_port = htons(PORT_PONG);
        clock_gettime(CLOCK_REALTIME, &ts);
        udpdk_sendto(sock, &pckt, sizeof(pckt), 0,
                (const struct sockaddr *) &destaddr, sizeof(destaddr));

        // Get pong response
        n = udpdk_recvfrom(sock, &r_pckt, sizeof(r_pckt), 0, NULL, NULL);
        if (n > 0) {
            clock_gettime(CLOCK_REALTIME, &ts_now);
            ts.tv_sec = ts_now.tv_sec - ts_msg.tv_sec;
            ts.tv_nsec = ts_now.tv_nsec - ts_msg.tv_nsec;
            if (ts.tv_nsec < 0) {
                ts.tv_nsec += 1000000000;
                ts.tv_sec--;
            }
            if (log_enabled) {
                samples[n_samples++] = (int)ts.tv_sec * 1000000000 + (int)ts.tv_nsec;
            } else {
                printf("Received pong; delta = %d.%09d seconds\n", (int)ts.tv_sec, (int)ts.tv_nsec);
            }
        }

        usleep(delay);
    }
}

static void pong_body(void)
{
    char rbuffer[128];
    struct ping_pkt* r_pckt;
    struct ping_pkt pckt;
        // filling packet
    bzero(& pckt, sizeof(pckt));

    pckt.hdr.type = ICMP_ECHO;
    pckt.hdr.un.echo.id = getpid();

    pckt.msg[0] = 0;
    pckt.hdr.un.echo.sequence = 0;
    pckt.hdr.checksum
        = checksum(& pckt, sizeof(pckt));

    int sock, n;
    struct sockaddr_in servaddr, cliaddr;
    struct timespec ts_msg;

    printf("PONG mode\n");

    // Create a socket
    if ((sock = udpdk_socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "Pong: socket creation failed");
        return;
    }
    // Bind it
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT_PONG);
    if (udpdk_bind(sock, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "Pong: bind failed");
        return;
    }

    while (app_alive) {
        // Bounce incoming packets
        int len = sizeof(cliaddr);
        n = udpdk_recvfrom(sock, rbuffer, sizeof(rbuffer), 0, ( struct sockaddr *) &cliaddr, &len);
        printf("VALUE FROM RECVFROM IS %d bytes\n", n);
        printf("Payload was %s\n", rbuffer);
        if (n > 0) {
            n = udpdk_sendto(sock, &pckt, sizeof(pckt), 0, (const struct sockaddr *) &cliaddr, len);
            printf("VALUE FROM SENDTO IS %d bytes\n", n);
        }
    }
}

static void usage(void)
{
    printf("%s -c CONFIG -f FUNCTION \n"
            " -c CONFIG: .ini configuration file\n"
            " -f FUNCTION: 'ping' or 'pong'\n"
            " -d DELAY: delay (microseconds) between two ping invocations\n"
            , progname);
}


static int parse_app_args(int argc, char *argv[])
{
    int c;

    progname = argv[0];

    while ((c = getopt(argc, argv, "c:f:d:l:")) != -1) {
        switch (c) {
            case 'c':
                // this is for the .ini cfg file needed by DPDK, not by the app
                break;
            case 'f':
                if (strcmp(optarg, "ping") == 0) {
                    mode = PING;
                } else if (strcmp(optarg, "pong") == 0) {
                    mode = PONG;
                } else {
                    fprintf(stderr, "Unsupported function %s (must be 'ping' or 'pong')\n", optarg);
                    return -1;
                }
                break;
            case 'd':
                delay = atoi(optarg);
                break;
            case 'l':
                log_enabled = 1;
                log_file = strdup(optarg);
                log = fopen(log_file, "w");
                if (log == NULL) {
                    printf("Error opening log file: %s\n", log_file);
                }
                break;
            default:
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                usage();
                return -1;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int retval;

    // Register signals for shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize UDPDK
    retval = udpdk_init(argc, argv);
    if (retval < 0) {
        goto pingpong_end;
        return -1;
    }
    printf("App: UDPDK Intialized\n");

    // Parse app-specific arguments
    printf("Parsing app arguments...\n");
    retval = parse_app_args(argc, argv);
    if (retval != 0) {
        goto pingpong_end;
        return -1;
    }


    if (mode == PING) {
        ping_body();
    } else {
        pong_body();
    }

pingpong_end:
    if (log_enabled) {
        printf("Dumping %d samples to log...\n", n_samples);
        for (unsigned i = 0; i < n_samples; ++i)
            fprintf(log, "%d\n", samples[i]);
        printf("Closing log...\n");
        fclose(log);
    }
    udpdk_interrupt(0);
    udpdk_cleanup();
    return 0;
}
