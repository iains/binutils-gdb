#ifndef __PPC_DARWIN_THREAD_STATE_H__
#define __PPC_DARWIN_THREAD_STATE_H__

#define PPC_DARWIN_THREAD_STATE 1
#define PPC_DARWIN_THREAD_FPSTATE 2
#define PPC_DARWIN_THREAD_VPSTATE 4
#define PPC_DARWIN_THREAD_STATE_64 5

struct ppc_darwin_thread_state
{
  unsigned int srr0;            /* program counter */
  unsigned int srr1;            /* machine state register */

  unsigned int gpregs[32];

  unsigned int cr;              /* condition register */
  unsigned int xer;             /* integer exception register */
  unsigned int lr;              /* link register */
  unsigned int ctr;
  unsigned int mq;

  unsigned int vrsave;          /* vector save register */
};

typedef struct ppc_darwin_thread_state ppc_darwin_thread_state_t;

struct ppc_darwin_thread_fpstate
{

  double fpregs[32];

  unsigned int fpscr_pad;       /* fpscr is 64 bits; first 32 are unused */
  unsigned int fpscr;           /* floating point status register */
};

typedef struct ppc_darwin_thread_fpstate ppc_darwin_thread_fpstate_t;

/* Make sure the structures stays how we define them. */
#pragma pack (push, 4)	       

struct ppc_darwin_thread_state_64
{
  unsigned long long srr0;      /* program counter */
  unsigned long long srr1;      /* machine state register */

  unsigned long long gpregs[32];

  unsigned int cr;              /* condition register */
  unsigned long long xer;       /* integer exception register */
  unsigned long long lr;        /* link register */
  unsigned long long ctr;

  unsigned int vrsave;          /* vector save register */
};

typedef struct ppc_darwin_thread_state_64 ppc_darwin_thread_state_64_t;

struct ppc_darwin_thread_vpstate
{
#if defined(__LP64__)
  unsigned int	save_vr[32][4];
  unsigned int	save_vscr[4];
#else
  unsigned long	save_vr[32][4];
  unsigned long	save_vscr[4];
#endif
  unsigned int save_pad5[4];
  unsigned int save_vrvalid;    /* vrs that have been saved */
  unsigned int save_pad6[7];
} __attribute__((aligned(16)));

typedef struct ppc_darwin_thread_vpstate ppc_darwin_thread_vpstate_t;

#pragma pack (pop)

#define PPC_DARWIN_THREAD_STATE_COUNT \
  (sizeof (ppc_darwin_thread_state_t) / sizeof (unsigned int))

#define PPC_DARWIN_THREAD_STATE_64_COUNT \
  (sizeof (ppc_darwin_thread_state_64_t) / sizeof (unsigned int))

#define PPC_DARWIN_THREAD_FPSTATE_COUNT \
  (sizeof (ppc_darwin_thread_fpstate_t) / sizeof (unsigned int))

#define PPC_DARWIN_THREAD_VPSTATE_COUNT \
  (sizeof (ppc_darwin_thread_vpstate_t) / sizeof (unsigned int))

#endif /* __PPC_DARWIN_THREAD_STATE_H__ */
