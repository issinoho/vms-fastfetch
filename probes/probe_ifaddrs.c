/* probe_ifaddrs.c - does the TCP/IP stack offer getifaddrs() (IPv4, IPv6, MAC)?  */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

int
main (void)
{
  struct ifaddrs *ifa, *p;

  if (getifaddrs (&ifa) != 0)
    {
      perror ("IFA getifaddrs");
      return 0;
    }
  for (p = ifa; p; p = p->ifa_next)
    {
      char addr[64] = "-";
      int fam = p->ifa_addr ? p->ifa_addr->sa_family : -1;
      if (fam == AF_INET)
        inet_ntop (AF_INET, &((struct sockaddr_in *) p->ifa_addr)->sin_addr, addr, sizeof addr);
#ifdef AF_INET6
      else if (fam == AF_INET6)
        inet_ntop (AF_INET6, &((struct sockaddr_in6 *) p->ifa_addr)->sin6_addr, addr, sizeof addr);
#endif
      printf ("IFA %-8s family %2d flags %%X%04X addr %s\n", p->ifa_name, fam,
              (unsigned) p->ifa_flags, addr);
    }
  freeifaddrs (ifa);
  return 0;
}
