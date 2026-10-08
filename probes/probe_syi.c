/* probe_syi.c - print the $GETSYI items vms-fastfetch may use.

   Each item is fetched in its own $GETSYIW call, so one unsupported item
   does not hide the others.  Items missing from this node's <syidef.h>
   are reported as such rather than breaking the compile.  */

#include <stdio.h>
#include <string.h>
#include <descrip.h>
#include <starlet.h>
#include <syidef.h>
#include <ssdef.h>
#include <iledef.h>

enum kind { K_STR, K_NUM, K_TIME, K_HEX };

struct item { const char *name; int code; enum kind kind; };

#define IT(n, k) { #n, SYI$_##n, k }
#define NO(n)    { #n, -1, K_NUM }

static const struct item items[] = {
#ifdef SYI$_VERSION
  IT(VERSION, K_STR),
#else
  NO(VERSION),
#endif
#ifdef SYI$_ARCH_NAME
  IT(ARCH_NAME, K_STR),
#else
  NO(ARCH_NAME),
#endif
#ifdef SYI$_ARCH_TYPE
  IT(ARCH_TYPE, K_NUM),
#else
  NO(ARCH_TYPE),
#endif
#ifdef SYI$_HW_NAME
  IT(HW_NAME, K_STR),
#else
  NO(HW_NAME),
#endif
#ifdef SYI$_HW_MODEL
  IT(HW_MODEL, K_NUM),
#else
  NO(HW_MODEL),
#endif
#ifdef SYI$_NODENAME
  IT(NODENAME, K_STR),
#else
  NO(NODENAME),
#endif
#ifdef SYI$_SCSNODE
  IT(SCSNODE, K_STR),
#else
  NO(SCSNODE),
#endif
#ifdef SYI$_NODE_HWVERS
  IT(NODE_HWVERS, K_HEX),
#else
  NO(NODE_HWVERS),
#endif
#ifdef SYI$_SYSTEM_RIGHTS
  IT(SYSTEM_RIGHTS, K_HEX),
#else
  NO(SYSTEM_RIGHTS),
#endif
#ifdef SYI$_BOOTTIME
  IT(BOOTTIME, K_TIME),
#else
  NO(BOOTTIME),
#endif
#ifdef SYI$_SID
  IT(SID, K_HEX),
#else
  NO(SID),
#endif
#ifdef SYI$_XSID
  IT(XSID, K_HEX),
#else
  NO(XSID),
#endif
#ifdef SYI$_CPU
  IT(CPU, K_NUM),
#else
  NO(CPU),
#endif
#ifdef SYI$_CPUTYPE
  IT(CPUTYPE, K_NUM),
#else
  NO(CPUTYPE),
#endif
#ifdef SYI$_REAL_CPUTYPE
  IT(REAL_CPUTYPE, K_NUM),
#else
  NO(REAL_CPUTYPE),
#endif
#ifdef SYI$_PROCESSOR_TYPE
  IT(PROCESSOR_TYPE, K_NUM),
#else
  NO(PROCESSOR_TYPE),
#endif
#ifdef SYI$_CPU_FREQ
  IT(CPU_FREQ, K_NUM),
#else
  NO(CPU_FREQ),
#endif
#ifdef SYI$_CPU_SPEED
  IT(CPU_SPEED, K_NUM),
#else
  NO(CPU_SPEED),
#endif
#ifdef SYI$_ACTIVECPU_CNT
  IT(ACTIVECPU_CNT, K_NUM),
#else
  NO(ACTIVECPU_CNT),
#endif
#ifdef SYI$_AVAILCPU_CNT
  IT(AVAILCPU_CNT, K_NUM),
#else
  NO(AVAILCPU_CNT),
#endif
#ifdef SYI$_POTENTIALCPU_CNT
  IT(POTENTIALCPU_CNT, K_NUM),
#else
  NO(POTENTIALCPU_CNT),
#endif
#ifdef SYI$_PRESENTCPU_CNT
  IT(PRESENTCPU_CNT, K_NUM),
#else
  NO(PRESENTCPU_CNT),
#endif
#ifdef SYI$_PRIMARY_CPUID
  IT(PRIMARY_CPUID, K_NUM),
#else
  NO(PRIMARY_CPUID),
#endif
#ifdef SYI$_CPUCAP_MASK
  IT(CPUCAP_MASK, K_HEX),
#else
  NO(CPUCAP_MASK),
#endif
#ifdef SYI$_HP_ACTIVE_CPU_CNT
  IT(HP_ACTIVE_CPU_CNT, K_NUM),
#else
  NO(HP_ACTIVE_CPU_CNT),
#endif
#ifdef SYI$_HP_CORE_CNT
  IT(HP_CORE_CNT, K_NUM),
#else
  NO(HP_CORE_CNT),
#endif
#ifdef SYI$_HP_ACTIVE_SP_CNT
  IT(HP_ACTIVE_SP_CNT, K_NUM),
#else
  NO(HP_ACTIVE_SP_CNT),
#endif
#ifdef SYI$_MEMSIZE
  IT(MEMSIZE, K_NUM),
#else
  NO(MEMSIZE),
#endif
#ifdef SYI$_PAGE_SIZE
  IT(PAGE_SIZE, K_NUM),
#else
  NO(PAGE_SIZE),
#endif
#ifdef SYI$_FREE_PAGES
  IT(FREE_PAGES, K_NUM),
#else
  NO(FREE_PAGES),
#endif
#ifdef SYI$_USED_PAGES
  IT(USED_PAGES, K_NUM),
#else
  NO(USED_PAGES),
#endif
#ifdef SYI$_MODIFIED_PAGES
  IT(MODIFIED_PAGES, K_NUM),
#else
  NO(MODIFIED_PAGES),
#endif
#ifdef SYI$_PAGEFILE_PAGE
  IT(PAGEFILE_PAGE, K_NUM),
#else
  NO(PAGEFILE_PAGE),
#endif
#ifdef SYI$_PAGEFILE_FREE
  IT(PAGEFILE_FREE, K_NUM),
#else
  NO(PAGEFILE_FREE),
#endif
#ifdef SYI$_SWAPFILE_PAGE
  IT(SWAPFILE_PAGE, K_NUM),
#else
  NO(SWAPFILE_PAGE),
#endif
#ifdef SYI$_SWAPFILE_FREE
  IT(SWAPFILE_FREE, K_NUM),
#else
  NO(SWAPFILE_FREE),
#endif
#ifdef SYI$_TOTAL_PAGEFILES
  IT(TOTAL_PAGEFILES, K_NUM),
#else
  NO(TOTAL_PAGEFILES),
#endif
#ifdef SYI$_TOTAL_SWAPFILES
  IT(TOTAL_SWAPFILES, K_NUM),
#else
  NO(TOTAL_SWAPFILES),
#endif
#ifdef SYI$_CLUSTER_MEMBER
  IT(CLUSTER_MEMBER, K_NUM),
#else
  NO(CLUSTER_MEMBER),
#endif
#ifdef SYI$_CLUSTER_NODES
  IT(CLUSTER_NODES, K_NUM),
#else
  NO(CLUSTER_NODES),
#endif
#ifdef SYI$_CLUSTER_VOTES
  IT(CLUSTER_VOTES, K_NUM),
#else
  NO(CLUSTER_VOTES),
#endif
#ifdef SYI$_CLUSTER_QUORUM
  IT(CLUSTER_QUORUM, K_NUM),
#else
  NO(CLUSTER_QUORUM),
#endif
#ifdef SYI$_VOTES
  IT(VOTES, K_NUM),
#else
  NO(VOTES),
#endif
#ifdef SYI$_EXPECTED_VOTES
  IT(EXPECTED_VOTES, K_NUM),
#else
  NO(EXPECTED_VOTES),
#endif
#ifdef SYI$_MAXPROCESSCNT
  IT(MAXPROCESSCNT, K_NUM),
#else
  NO(MAXPROCESSCNT),
#endif
#ifdef SYI$_SERIAL_NUMBER
  IT(SERIAL_NUMBER, K_STR),
#else
  NO(SERIAL_NUMBER),
#endif
#ifdef SYI$_PRODUCT_NAME
  IT(PRODUCT_NAME, K_STR),
#else
  NO(PRODUCT_NAME),
#endif
#ifdef SYI$_VIRTUAL_MACHINE
  IT(VIRTUAL_MACHINE, K_NUM),
#else
  NO(VIRTUAL_MACHINE),
#endif
#ifdef SYI$_HYPERVISOR_VENDOR
  IT(HYPERVISOR_VENDOR, K_STR),
#else
  NO(HYPERVISOR_VENDOR),
#endif
#ifdef SYI$_HYPERVISOR_NAME
  IT(HYPERVISOR_NAME, K_STR),
#else
  NO(HYPERVISOR_NAME),
#endif
#ifdef SYI$_HYPERVISOR_VERSION
  IT(HYPERVISOR_VERSION, K_STR),
#else
  NO(HYPERVISOR_VERSION),
#endif
#ifdef SYI$_GALAXY_ID
  IT(GALAXY_ID, K_HEX),
#else
  NO(GALAXY_ID),
#endif
#ifdef SYI$_PFCDEFAULT
  IT(PFCDEFAULT, K_NUM),
#else
  NO(PFCDEFAULT),
#endif
};

int
main (void)
{
  size_t i;

  for (i = 0; i < sizeof items / sizeof items[0]; i++)
    {
      const struct item *it = &items[i];
      unsigned char buf[256];
      unsigned short len = 0;
      unsigned short iosb[4];
      ile3 il[2];
      int st;

      if (it->code < 0)
        {
          printf ("SYI %-20s not defined in syidef.h\n", it->name);
          continue;
        }
      memset (buf, 0, sizeof buf);
      il[0].ile3$w_length = it->kind == K_STR ? sizeof buf - 1 : 8;
      il[0].ile3$w_code = it->code;
      il[0].ile3$ps_bufaddr = buf;
      il[0].ile3$ps_retlen_addr = &len;
      memset (&il[1], 0, sizeof il[1]);
      st = sys$getsyiw (0, 0, 0, il, (void *) iosb, 0, 0);
      if (st & 1)
        st = iosb[0];
      if (!(st & 1))
        {
          printf ("SYI %-20s status %%X%08X\n", it->name, st);
          continue;
        }
      switch (it->kind)
        {
        case K_STR:
          /* Trim trailing blanks and NULs; show the length to catch padding.  */
          while (len > 0 && (buf[len - 1] == ' ' || buf[len - 1] == 0))
            len--;
          printf ("SYI %-20s \"%.*s\" (len %u)\n", it->name, (int) len, buf, len);
          break;
        case K_TIME:
          {
            char tbuf[32];
            unsigned short tlen = 0;
            struct dsc$descriptor_s d;
            d.dsc$w_length = sizeof tbuf - 1;
            d.dsc$b_dtype = DSC$K_DTYPE_T;
            d.dsc$b_class = DSC$K_CLASS_S;
            d.dsc$a_pointer = tbuf;
            st = sys$asctim (&tlen, &d, (void *) buf, 0);
            tbuf[(st & 1) ? tlen : 0] = 0;
            printf ("SYI %-20s %s (len %u)\n", it->name, tbuf, len);
          }
          break;
        default:
          {
            unsigned __int64 v = 0;
            memcpy (&v, buf, len > 8 ? 8 : len);
            if (it->kind == K_HEX)
              printf ("SYI %-20s %%X%016llX (len %u)\n", it->name, v, len);
            else
              printf ("SYI %-20s %llu (len %u)\n", it->name, v, len);
          }
          break;
        }
    }
  return 0;
}
