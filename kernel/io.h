#ifndef __IO_H__
#define __IO_H__

#ifdef __cplusplus
extern "C" {
#endif

struct IDTDescr
{
   unsigned short offset_1; // offset bits 0..15
   unsigned short selector; // a code segment selector in GDT or LDT
   unsigned char zero;      // unused, set to 0
   unsigned char type_attr; // type and attributes, see below
   unsigned short offset_2; // offset bits 16..31
};

static inline void outb(unsigned short port, unsigned char val)
{
   asm volatile ("outb %0,%1"::"a"(val), "Nd" (port));
}

static inline void outw(unsigned short port, unsigned short val)
{
   asm volatile ("outw %0,%1"::"a"(val), "Nd" (port));
}

static inline void outl(unsigned short port, unsigned int val)
{
   asm volatile ("outl %0,%1"::"a"(val), "Nd" (port));
}

static inline unsigned char inb(unsigned short port)
{
   unsigned char ret;
   asm volatile ("inb %1,%0":"=a"(ret):"Nd"(port));
   return ret;
}

static inline unsigned short inw(unsigned short port)
{
   unsigned short ret;
   asm volatile ("inw %1,%0":"=a"(ret):"Nd"(port));
   return ret;
}

static inline unsigned int inl(unsigned short port)
{
   unsigned int ret;
   asm volatile ("inl %1,%0":"=a"(ret):"Nd"(port));
   return ret;
}

static inline void io_wait(void)
{
   asm volatile("outb %%al, $0x80" : : "a"(0));
}

static inline void lidt(volatile void* base, unsigned int size /* limit+1 */)
{
    volatile unsigned int i[2];
    i[0] = (size-1) << 16;
    i[1] = (unsigned int) base;
    asm volatile ("lidt %0" : : "m" (((char*)i)[2]));
}

static inline void lgdt(volatile void *base, unsigned int size) {
    volatile unsigned int i[2];
    i[0] = (size-1) << 16;
    i[1] = (unsigned int) base;
    asm volatile ("lgdt %0" : : "m" (((char*)i)[2]));
}

static inline void cli() { asm volatile ("cli"); }

static inline void sti() { asm volatile ("sti"); }

static inline void irq_unmask(char irq)
{
    unsigned short mask = (0xFFFE<<irq)|(0xFFFE>>(16-irq));
    if (~(unsigned char)(mask)&0xFF)
      outb(0x21, inb(0x21)&(unsigned char)(mask));
    else
      outb(0xa1, inb(0xa1)&(unsigned char)(mask>>8));
}

static inline int irq_getmask()
{
    return inb(0x21)|(inb(0xa1)<<8);
}

static inline void irq_setmask(int mask)
{
   outb(0x21, mask);
   outb(0xa1, mask>>8);
}

static inline int irq_disable()
{
    int i = irq_getmask();
    irq_setmask(0xFFFF);
    return i;
}

static inline int int_disable() // returns 0x200 if ints were enabled
{
    int i;
    asm volatile ("pushf; pop %0; cli":"=g"(i));
    return i & 0x200;
}

static inline void int_restore(int i)
{
    if (i) sti();
}

static inline void eoi(int irq)
{
    if (irq >= 8) outb(0xa0,0x20);
    outb(0x20,0x20);
}

static inline void invlpg(void* pg)
{
    // FIXED BUG!!!
    // was: asm volatile ("invlpg %0"::"m"(pg));
    asm volatile ("invlpg (%0)"::"r"((unsigned)pg));
}

static inline void hlt()
{
    asm volatile ("hlt");
}

static inline void hang()
{
    asm volatile ("0: jmp 0b");
}

static inline void freeze()
{
    cli();
    while (1) hlt();
}
// Spins while value int ptr is set and then sets 1 in the pointer.
static inline void spinlock(int* ptr)
{
    int prev;
    // This seems to be the perfect way to do it.
    // The Intel manual says the LOCK prefix is alway assummed
    // with the xchg instruction with mem op, so we can save a byte by not
    // using the prefix.
    // >> SAME AS XCHG
    do asm volatile ("lock xchgl %0,%1":"=a"(prev):"m"(*ptr),"a"(1)); while (prev);
}

// Sets the value, and returns whether it was previously set.
// Allows multiple threads to wait for a value to be unset
static inline int lock(int* ptr)
{
    int prev;
    // >> SAME AS XCHG
    asm volatile ("lock xchgl %0,%1":"=a"(prev):"m"(*ptr),"a"(1));
    return prev;
}

// Based on https://wiki.osdev.org/CPUID:
#define CPUID_VENDOR_OLDAMD       "AMDisbetter!" /* early engineering samples of AMD K5 processor */
#define CPUID_VENDOR_AMD          "AuthenticAMD"
#define CPUID_VENDOR_INTEL        "GenuineIntel"
#define CPUID_VENDOR_VIA          "CentaurHauls"
#define CPUID_VENDOR_OLDTRANSMETA "TransmetaCPU"
#define CPUID_VENDOR_TRANSMETA    "GenuineTMx86"
#define CPUID_VENDOR_CYRIX        "CyrixInstead"
#define CPUID_VENDOR_CENTAUR      "CentaurHauls"
#define CPUID_VENDOR_NEXGEN       "NexGenDriven"
#define CPUID_VENDOR_UMC          "UMC UMC UMC "
#define CPUID_VENDOR_SIS          "SiS SiS SiS "
#define CPUID_VENDOR_NSC          "Geode by NSC"
#define CPUID_VENDOR_RISE         "RiseRiseRise"
#define CPUID_VENDOR_VORTEX       "Vortex86 SoC"
#define CPUID_VENDOR_VIA2         "VIA VIA VIA "

/*Vendor-strings from Virtual Machines.*/
#define CPUID_VENDOR_VMWARE       "VMwareVMware"
#define CPUID_VENDOR_XENHVM       "XenVMMXenVMM"
#define CPUID_VENDOR_MICROSOFT_HV "Microsoft Hv"
#define CPUID_VENDOR_PARALLELS    " lrpepyh vr"

#define CPUID_FEAT_ECX_SSE3         1 << 0      // streaming SIMD extensions 3 (SSE3)
#define CPUID_FEAT_ECX_PCLMUL       1 << 1
#define CPUID_FEAT_ECX_DTES64       1 << 2
#define CPUID_FEAT_ECX_MONITOR      1 << 3      // monitor/mwait
#define CPUID_FEAT_ECX_DS_CPL       1 << 4      // CPL qualified debug store
#define CPUID_FEAT_ECX_VMX          1 << 5      // virtual machine extensions
#define CPUID_FEAT_ECX_SMX          1 << 6      // safer mode extensions
#define CPUID_FEAT_ECX_EST          1 << 7      // enhanced Intel SpeedStep(R) technology
#define CPUID_FEAT_ECX_TM2          1 << 8      // thermal monitor 2
#define CPUID_FEAT_ECX_SSSE3        1 << 9      // supplemental streaming SIMD extensions 3 (SSSSE3)
#define CPUID_FEAT_ECX_CNXT_ID      1 << 10     // L1 context ID
#define CPUID_FEAT_ECX_FMA          1 << 12
#define CPUID_FEAT_ECX_CMPXCHG16B   1 << 13     // cmpxchg16b available
#define CPUID_FEAT_ECX_xTPR_UPDATE  1 << 14     // xTPR update control
#define CPUID_FEAT_ECX_PDCM         1 << 15     // performance and debug capability
#define CPUID_FEAT_ECX_PCIDE        1 << 17 
#define CPUID_FEAT_ECX_DCA          1 << 18     // memory-mapped device prefetching
#define CPUID_FEAT_ECX_SSE4_1       1 << 19     // SSE4.1
#define CPUID_FEAT_ECX_SSE4_2       1 << 20     // SSE4.2
#define CPUID_FEAT_ECX_x2APIC       1 << 21     // x2APIC available
#define CPUID_FEAT_ECX_MOVBE        1 << 22     // movbe available
#define CPUID_FEAT_ECX_POPCNT       1 << 23     // popcnt available
#define CPUID_FEAT_ECX_AES          1 << 25     
#define CPUID_FEAT_ECX_XSAVE        1 << 26     // xsave/xrstor/xsetbv/xgetbv supported
#define CPUID_FEAT_ECX_OSXSAVE      1 << 27     // xsetbv/xgetbv has been enabled
#define CPUID_FEAT_ECX_AVX          1 << 28

#define CPUID_FEAT_EDX_FPU          1 << 0      // x86 FPU on chip
#define CPUID_FEAT_EDX_VME          1 << 1      // virtual-8086 mode enhancement
#define CPUID_FEAT_EDX_DE           1 << 2      // debugging extensions
#define CPUID_FEAT_EDX_PSE          1 << 3      // page size extensions
#define CPUID_FEAT_EDX_TSC          1 << 4      // timestamp counter
#define CPUID_FEAT_EDX_MSR          1 << 5      // rdmsr/wrmsr
#define CPUID_FEAT_EDX_PAE          1 << 6      // page address extensions
#define CPUID_FEAT_EDX_MCE          1 << 7      // machine check exception
#define CPUID_FEAT_EDX_CX8          1 << 8      // cmpxchg8b supported
#define CPUID_FEAT_EDX_APIC         1 << 9      // APIC on a chip
#define CPUID_FEAT_EDX_SEP          1 << 11     // sysenter/sysexit
#define CPUID_FEAT_EDX_MTRR         1 << 12     // memory type range registers
#define CPUID_FEAT_EDX_PGE          1 << 13     // PTE global bit (PTE_GLOBAL)
#define CPUID_FEAT_EDX_MCA          1 << 14     // machine check architecture
#define CPUID_FEAT_EDX_CMOV         1 << 15     // conditional move/compare instructions
#define CPUID_FEAT_EDX_PAT          1 << 16     // page attribute table
#define CPUID_FEAT_EDX_PSE36        1 << 17     // page size extension
#define CPUID_FEAT_EDX_PSN          1 << 18     // processor serial number
#define CPUID_FEAT_EDX_CLF          1 << 19     // cflush instruction
#define CPUID_FEAT_EDX_DTES         1 << 21     // debug store
#define CPUID_FEAT_EDX_ACPI         1 << 22     // thermal monitor and clock control
#define CPUID_FEAT_EDX_MMX          1 << 23     // MMX technology
#define CPUID_FEAT_EDX_FXSR         1 << 24     // fxsave/fxrstor
#define CPUID_FEAT_EDX_SSE          1 << 25     // SSE extensions
#define CPUID_FEAT_EDX_SSE2         1 << 26     // SSE2 extensions
#define CPUID_FEAT_EDX_SS           1 << 27     // self-snoop
#define CPUID_FEAT_EDX_HTT          1 << 28     // hyper-threading
#define CPUID_FEAT_EDX_TM1          1 << 29     // thermal monitor
#define CPUID_FEAT_EDX_IA64         1 << 30
#define CPUID_FEAT_EDX_PBE          1 << 31     // Pend. Brk. EN.

#define CPUID_GETVENDORSTRING       0
#define CPUID_GETFEATURES           1
#define CPUID_GETTLB                2
#define CPUID_GETSERIAL             3
 
#define CPUID_INTELEXTENDED         0x80000000
#define CPUID_INTELFEATURES         0x80000001
#define CPUID_INTELBRANDSTRING      0x80000002
#define CPUID_INTELBRANDSTRINGMORE  0x80000003
#define CPUID_INTELBRANDSTRINGEND   0x80000004

/** issue a single request to CPUID. Fits 'intel features', for instance.
 */
static inline void cpuid(int code, unsigned *a, unsigned *b, unsigned *c, unsigned *d) {
  // note that even if only "eax" and "edx" are of interest, other registers
  // will be modified by the operation, so we need to tell the compiler about it.
  asm volatile("cpuid":"=a"(*a),"=b"(*b),"=c"(*c),"=d"(*d):"a"(code));
}
 
/** issue a complete request, storing general registers output as a string
 */
static inline int cpuid_string(int code, unsigned where[4]) {
  asm volatile("cpuid":"=a"(*where),"=b"(*(where+1)),
               "=c"(*(where+2)),"=d"(*(where+3)):"a"(code));
  return (int)where[0];
}

static const char * const cpu_string() {
	static char s[16] = "BogusProces!";
	cpuid_string(0, (unsigned*)(s));
	return s;
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
