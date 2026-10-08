/* probe_dvi.c - $GETDVI items for the terminal (SYS$COMMAND, SYS$OUTPUT)
   and for every disk found by $DEVICE_SCAN.  */

#include <stdio.h>
#include <string.h>
#include <descrip.h>
#include <starlet.h>
#include <dvidef.h>
#include <dvsdef.h>
#include <dcdef.h>
#include <ssdef.h>
#include <iledef.h>

enum kind { K_STR, K_NUM, K_HEX };
struct item { const char *name; int code; enum kind kind; };
#define IT(n, k) { #n, DVI$_##n, k }

static const struct item term_items[] = {
  IT(FULLDEVNAM, K_STR), IT(DEVNAM, K_STR), IT(DEVCLASS, K_NUM),
  IT(DEVTYPE, K_NUM), IT(DEVBUFSIZ, K_NUM), IT(TT_PAGE, K_NUM),
  IT(DEVDEPEND, K_HEX), IT(DEVDEPEND2, K_HEX), IT(TT_ACCPORNAM, K_STR),
  IT(TT_PHYDEVNAM, K_STR), IT(TT_ANSICRT, K_NUM), IT(TT_DECCRT, K_NUM),
  IT(TT_DECCRT2, K_NUM), IT(TT_DECCRT3, K_NUM), IT(TT_DECCRT4, K_NUM),
  IT(TT_REGIS, K_NUM), IT(TT_SIXEL, K_NUM), IT(TT_AVO, K_NUM),
  IT(TT_EIGHTBIT, K_NUM), IT(TRM, K_NUM), IT(MBX, K_NUM), IT(SPL, K_NUM),
};

static const struct item disk_items[] = {
  IT(FULLDEVNAM, K_STR), IT(MNT, K_NUM), IT(LOGVOLNAM, K_STR),
  IT(VOLNAM, K_STR), IT(MAXBLOCK, K_NUM), IT(FREEBLOCKS, K_NUM),
  IT(VOLSIZE, K_NUM), IT(CLUSTER, K_NUM), IT(ACPTYPE, K_NUM),
  IT(DEVTYPE, K_NUM), IT(MEDIA_NAME, K_STR), IT(SHDW_MASTER, K_NUM),
  IT(SHDW_MEMBER, K_NUM), IT(REMOTE_DEVICE, K_NUM), IT(VOLUME_EXTEND_QUANTITY, K_NUM),
};

static void
desc_init (struct dsc$descriptor_s *d, const char *s)
{
  d->dsc$w_length = (unsigned short) strlen (s);
  d->dsc$b_dtype = DSC$K_DTYPE_T;
  d->dsc$b_class = DSC$K_CLASS_S;
  d->dsc$a_pointer = (char *) s;
}

static void
show (const char *dev, const struct item *items, size_t n)
{
  struct dsc$descriptor_s d;
  size_t i;

  desc_init (&d, dev);
  for (i = 0; i < n; i++)
    {
      unsigned char buf[256];
      unsigned short len = 0, iosb[4];
      ile3 il[2];
      int st;

      memset (buf, 0, sizeof buf);
      memset (il, 0, sizeof il);
      il[0].ile3$w_length = items[i].kind == K_STR ? sizeof buf - 1 : 8;
      il[0].ile3$w_code = items[i].code;
      il[0].ile3$ps_bufaddr = buf;
      il[0].ile3$ps_retlen_addr = &len;
      st = sys$getdviw (0, 0, &d, il, (void *) iosb, 0, 0, 0);
      if (st & 1)
        st = iosb[0];
      if (!(st & 1))
        {
          printf ("DVI %s %-22s status %%X%08X\n", dev, items[i].name, st);
          continue;
        }
      if (items[i].kind == K_STR)
        printf ("DVI %s %-22s \"%.*s\"\n", dev, items[i].name, (int) len, buf);
      else
        {
          unsigned __int64 v = 0;
          memcpy (&v, buf, len > 8 ? 8 : len);
          if (items[i].kind == K_HEX)
            printf ("DVI %s %-22s %%X%08llX\n", dev, items[i].name, v);
          else
            printf ("DVI %s %-22s %llu\n", dev, items[i].name, v);
        }
    }
}

int
main (void)
{
  char name[65];
  unsigned short len;
  unsigned int devclass = DC$_DISK;
  unsigned __int64 ctx = 0;
  struct dsc$descriptor_s nd;
  ile3 sel[2];
  int st, n = 0;

  show ("SYS$COMMAND", term_items, sizeof term_items / sizeof term_items[0]);
  show ("SYS$OUTPUT", term_items, sizeof term_items / sizeof term_items[0]);
  show ("SYS$SYSDEVICE", disk_items, sizeof disk_items / sizeof disk_items[0]);

  memset (sel, 0, sizeof sel);
  sel[0].ile3$w_length = sizeof devclass;
  sel[0].ile3$w_code = DVS$_DEVCLASS;
  sel[0].ile3$ps_bufaddr = &devclass;
  nd.dsc$w_length = sizeof name - 1;
  nd.dsc$b_dtype = DSC$K_DTYPE_T;
  nd.dsc$b_class = DSC$K_CLASS_S;
  nd.dsc$a_pointer = name;
  for (;;)
    {
      len = 0;
      st = sys$device_scan (&nd, &len, 0, sel, &ctx);
      if (st == SS$_NOMOREDEV || st == SS$_NOSUCHDEV)
        break;
      if (!(st & 1))
        {
          printf ("SCAN status %%X%08X\n", st);
          break;
        }
      name[len] = 0;
      n++;
      show (name, disk_items, sizeof disk_items / sizeof disk_items[0]);
    }
  printf ("SCAN %d disk devices\n", n);
  return 0;
}
