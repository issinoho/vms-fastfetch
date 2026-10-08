/* probe_net.c - enumerate IPv4 interfaces with SIOCGIFCONF / SIOCGIFFLAGS.
   Built with and without _SOCKADDR_LEN (4.4BSD sockaddr with sa_len).  */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

int
main (void)
{
  char buf[8192];
  struct ifconf ifc;
  char *p, *end;
  int s, n = 0;

#ifdef _SOCKADDR_LEN
  printf ("NET build: _SOCKADDR_LEN\n");
#else
  printf ("NET build: plain sockaddr\n");
#endif
  printf ("NET sizeof(struct ifreq) %u sizeof(struct sockaddr) %u\n",
          (unsigned) sizeof (struct ifreq), (unsigned) sizeof (struct sockaddr));
  s = socket (AF_INET, SOCK_DGRAM, 0);
  if (s < 0)
    {
      perror ("NET socket");
      return 0;
    }
  ifc.ifc_len = sizeof buf;
  ifc.ifc_buf = buf;
  if (ioctl (s, SIOCGIFCONF, (char *) &ifc) < 0)
    {
      perror ("NET SIOCGIFCONF");
      close (s);
      return 0;
    }
  printf ("NET ifc_len %d\n", ifc.ifc_len);
  p = buf;
  end = buf + ifc.ifc_len;
  while (p < end)
    {
      struct ifreq *ifr = (struct ifreq *) p;
      struct ifreq fr;
      size_t step = sizeof (struct ifreq);
      char addr[64] = "?";

#ifdef _SOCKADDR_LEN
      if (ifr->ifr_addr.sa_len > sizeof (struct sockaddr))
        step = sizeof ifr->ifr_name + ifr->ifr_addr.sa_len;
#endif
      if (ifr->ifr_addr.sa_family == AF_INET)
        strcpy (addr, inet_ntoa (((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr));
      memset (&fr, 0, sizeof fr);
      strncpy (fr.ifr_name, ifr->ifr_name, sizeof fr.ifr_name);
      if (ioctl (s, SIOCGIFFLAGS, (char *) &fr) < 0)
        fr.ifr_flags = -1;
      printf ("NET if %-8.*s family %d addr %-16s flags %%X%04X%s%s\n",
              (int) sizeof ifr->ifr_name, ifr->ifr_name, ifr->ifr_addr.sa_family,
              addr, (unsigned) fr.ifr_flags & 0xFFFF,
              (fr.ifr_flags & IFF_UP) ? " UP" : "",
              (fr.ifr_flags & IFF_LOOPBACK) ? " LOOPBACK" : "");
      n++;
      p += step;
    }
  printf ("NET %d entries\n", n);
  close (s);
  return 0;
}
