/* dns_vms.c - the resolver configuration of VSI TCP/IP Services.

   TCP/IP publishes it as system logical names: TCPIP$BIND_SERVER000,
   ...001, ... (name servers) and TCPIP$BIND_DOMAIN (default domain).  */

#include "detection/detection.h"

#include <stdio.h>

#include "vms/vmsapi.h"

const char *
ffDetectDNS (FFstrbuf *servers, FFstrbuf *domain)
{
  FFstrbuf v;
  char name[32];
  int i;

  ffStrbufInit (&v);
  for (i = 0; i < 1000; i++)
    {
      snprintf (name, sizeof name, "TCPIP$BIND_SERVER%03d", i);
      if (!ffVmsTrnlnm (name, &v))
        break;
      if (servers->length)
        ffStrbufAppendS (servers, ", ");
      ffStrbufAppend (servers, &v);
    }
  ffStrbufDestroy (&v);
  ffVmsTrnlnm ("TCPIP$BIND_DOMAIN", domain);
  return servers->length ? NULL : "No name servers (TCPIP$BIND_SERVERnnn)";
}
