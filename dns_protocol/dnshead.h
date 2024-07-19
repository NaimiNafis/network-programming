/* dnshead.h */
#ifndef DNSHEAD_H
#define DNSHEAD_H

#include <stdint.h>

#define HFIXEDSZ 12 // DNS header size

typedef struct {
    uint16_t id; // identification number

    uint8_t rd :1; // recursion desired
    uint8_t tc :1; // truncated message
    uint8_t aa :1; // authoritative answer
    uint8_t opcode :4; // purpose of message
    uint8_t qr :1; // query/response flag

    uint8_t rcode :4; // response code
    uint8_t cd :1; // checking disabled
    uint8_t ad :1; // authenticated data
    uint8_t unused :1; // reserved for future use
    uint8_t ra :1; // recursion available

    uint16_t qdcount; // number of question entries
    uint16_t ancount; // number of answer entries
    uint16_t nscount; // number of authority entries
    uint16_t arcount; // number of resource entries
} HEADER;

#endif // DNSHEAD_H
