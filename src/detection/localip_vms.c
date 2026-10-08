/* localip_vms.c - IPv4 addresses from getifaddrs() (D6, VSI TCP/IP).  */

#include "detection/detection.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>

const char *
ffDetectLocalIps (FFlist *ips)
{
  struct ifaddrs *all, *p;

  if (getifaddrs (&all) != 0)
    return "getifaddrs() failed";
  for (p = all; p; p = p->ifa_next)
    {
      FFLocalIpResult *r;
      char addr[INET_ADDRSTRLEN];
      unsigned prefix = 0;

      if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET
          || !(p->ifa_flags & IFF_UP) || (p->ifa_flags & IFF_LOOPBACK))
        continue;
      if (!inet_ntop (AF_INET, &((struct sockaddr_in *) p->ifa_addr)->sin_addr, addr, sizeof addr))
        continue;
      if (p->ifa_netmask)
        {
          uint32_t m = ntohl (((struct sockaddr_in *) p->ifa_netmask)->sin_addr.s_addr);
          while (m & 0x80000000u)
            prefix++, m <<= 1;
        }
      r = ffListAdd (ips);
      ffStrbufInitS (&r->interface, p->ifa_name);
      ffStrbufInitS (&r->address, addr);
      if (prefix)
        ffStrbufAppendF (&r->address, "/%u", prefix);
    }
  freeifaddrs (all);
  return ips->length ? NULL : "No IPv4 address on an interface that is up";
}
