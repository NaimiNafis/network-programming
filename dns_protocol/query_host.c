/*
    query_host.c
*/
#include "mynet.h"
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include "dnshead.h"

#define PORT 53        /* DNS port number */
#define BUFSIZE 512    /* Buffer size */
#define HOSTSIZE 256   /* Maximum hostname length */
#define TIMEOUT_SEC 3  /* Receive timeout in seconds */
#define RETRY   3      /* Number of retries */
#define RETRY_PAUSE 3  /* Retry wait time */

#define NO_ERROR       0
#define OTHER_RESPONSE 1
#define WRONG_PACKET   2
#define SERVER_ERROR   3

#define QSECTAIL_LEN   4
#define ASECTAIL_LEN   10

typedef struct _QSectail {
  uint16_t  qtype;
  uint16_t  qclass;
}  QSectail;

typedef struct _ASectail {
  uint16_t  type;
  uint16_t  class;
  uint32_t  ttl;
  uint16_t  rdlength;
} ASectail;

int dns_request_a(char *buf, char *hostname);
int analize_dnsanswer(char *buf, struct in_addr *ipadrs);
int formalize_qname(char *buf);
char *skip_question(char *payload, int qdcount);
int answersection_a(char *payload, int ancount, struct in_addr *ipadrs);

int main(int argc, char *argv[]) {
  struct sockaddr_in server_adrs;
  struct sockaddr_in from_adrs;
  socklen_t from_len;

  int sock;
  fd_set mask, readfds;
  struct timeval timeout;

  char s_buf[BUFSIZE], r_buf[BUFSIZE], hostname[HOSTSIZE];
  struct in_addr ipadrs;
  int size;
  int retry;

  /* Check arguments and display usage */
  if (argc != 3) {
    fprintf(stderr, "Usage: %s DNS_Server Lookup_Host\n", argv[0]);
    exit(1);
  }

  /* Store server address information in sockaddr_in structure */
  set_sockaddr_in(&server_adrs, argv[1], (in_port_t)PORT);

  /* Hostname to query */
  strncpy(hostname, argv[2], HOSTSIZE);

  /* Create socket in DGRAM mode */
  sock = init_udpclient();

  /* Prepare bit mask */
  FD_ZERO(&mask);
  FD_SET(sock, &mask);

  /* Create the packet */
  size = dns_request_a(s_buf, hostname);

  /* Send the packet to the server and receive the response */
  for (retry = 0; retry < RETRY; retry++) {
    /* Send the string to the server */
    if (sendto(sock, s_buf, size, 0, (struct sockaddr *)&server_adrs, sizeof(server_adrs)) == -1) {
      exit_errmesg("sendto()");
    }

    readfds = mask;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    /* Check for received data */
    if (select(sock + 1, &readfds, NULL, NULL, &timeout) == 0) {
      sleep(RETRY_PAUSE);
      continue;
    }

    /* Receive the packet */
    from_len = sizeof(from_adrs);
    if ((size = recvfrom(sock, r_buf, BUFSIZE - 1, 0, (struct sockaddr *)&from_adrs, &from_len)) == -1) {
      exit_errmesg("recvfrom()");
    }

    /* Parse the received packet */
    if (analize_dnsanswer(r_buf, &ipadrs) == NO_ERROR) {
      printf("%s\n", inet_ntoa(ipadrs));
      break;
    }
  }

  /* Error if the number of retries is exceeded */
  if (retry == RETRY) {
    exit_errmesg("Time out.\n");
  }

  close(sock);  /* Close the socket */

  exit(0);
}

/* Create DNS query message (Type=A) */
int dns_request_a(char *buf, char *hostname) {
  HEADER *packet;
  QSectail *tail;
  int len_qname;

  packet = (HEADER *)buf;

  /* Header */
  packet->id = htons(getpid());
  packet->rd = 1;     /* Recursion desired */
  packet->tc = 0;
  packet->aa = 0;
  packet->opcode = 0; /* Opcode=0: query */
  packet->qr = 0;     /* QR=0: query */
  packet->rcode = 0;
  packet->cd = 0;
  packet->ad = 0;
  packet->unused = 0;
  packet->ra = 0;
  packet->qdcount = htons(1); /* Number of questions */
  packet->ancount = 0;
  packet->nscount = 0;
  packet->arcount = 0;

  /* Question section */
  strncpy(&buf[HFIXEDSZ + 1], hostname, HOSTSIZE); /* HFIXEDSZ is the DNS header size (12 bytes) */
  len_qname = formalize_qname(&buf[HFIXEDSZ]);
  tail = (QSectail *)(buf + HFIXEDSZ + len_qname);
  tail->qtype = htons(1);  /* QType */
  tail->qclass = htons(1); /* QClass */

  return (HFIXEDSZ + len_qname + QSECTAIL_LEN);
}

/* Parse DNS response message (Type=A only) */
int analize_dnsanswer(char *buf, struct in_addr *ipadrs) {
  HEADER *packet;
  char *payload;

  packet = (HEADER *)buf;

  /* Analyze header */
  if (packet->id != htons(getpid())) {  /* Not a packet sent by this process */
    printf("Another packet received.\n");
    return (OTHER_RESPONSE);
  }

  if (packet->qr != 1) { /* Not an answer packet */
    printf("Not answer packet.\n");
    return (WRONG_PACKET);
  }

  if (packet->rcode != 0) {  /* Server-side error */
    printf("Server error!(Rcode=%d)\n", packet->rcode);
    return (SERVER_ERROR);
  }

  if (packet->ancount == 0) { /* No answer section */
    printf("No answer section!\n");
    return (SERVER_ERROR);
  }

  /* Analyze payload */
  payload = buf + HFIXEDSZ;
  payload = skip_question(payload, ntohs(packet->qdcount)); /* Skip question section */
  if (answersection_a(payload, ntohs(packet->ancount), ipadrs) != NO_ERROR) {
    printf("Not answer packet.\n");
    return (WRONG_PACKET);
  }

  return (NO_ERROR);
}

/* Create QName field in question section */
int formalize_qname(char *buf) {
  char *pl, *pd;
  pl = buf;

  do {
    if ((pd = strchr(pl + 1, '.')) == NULL) {
      pd = buf + strlen(buf + 1) + 1;
    }
    *pl = pd - pl - 1;
    pl += *pl + 1;
  } while (*pl != 0);

  return (pl - buf + 1);
}

/* Skip question section */
char *skip_question(char *p, int qdcount) {
  int i;

  for (i = 0; i < qdcount; i++) {
    while (*p != 0) {
      if ((unsigned char)*p >= 0xc0) {
        p++;         /* Compressed */
        break;
      } else {
        p += *p + 1; /* Skip QName */
      }
    }
    p += 5;       /* Skip QType & QClass */
  }

  return (p);
}

/* Parse answer section (Type=A only) */
int answersection_a(char *p, int ancount, struct in_addr *ipadrs) {
  int i;
  ASectail *tail;
  uint16_t type, rdlength;

  for (i = 0; i < ancount; i++) {  /* Loop for the number of answers */

    /* Skip Name */
    while (*p != 0) {
      if ((unsigned char)*p >= 0xc0) { /* Compressed */
        p++;
        break;
      } else {
        p += *p + 1; /* Skip Name */
      }
    }
    p++;

    tail = (ASectail *)p;
    type = ntohs(tail->type);          /* Type field */
    rdlength = ntohs(tail->rdlength);  /* RDLength field */

    p += ASECTAIL_LEN;
    if (type == 1) {  /* Type=1: IP address */
      ipadrs->s_addr = *(in_addr_t *)p;
      break;
    }

    p += rdlength;
  }

  if (type != 1) {
    return (WRONG_PACKET);
  }
  return (NO_ERROR);
}
