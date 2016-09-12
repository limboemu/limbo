// Misc function and variable declarations.
#ifndef __STACKS_H
#define __STACKS_H

#include "types.h" // u32

#define CALL32SMM_CMDID    0xb5
#define CALL32SMM_ENTERID  0x1234
#define CALL32SMM_RETURNID 0x5678

// stacks.c
extern int HaveSmmCall32;
u32 __call32(void *func, u32 eax, u32 errret);
#define call32(func, eax, errret) ({                            \
        extern void _cfunc32flat_ ##func (void);                \
        __call32( _cfunc32flat_ ##func , (u32)(eax), (errret)); \
    })
extern u8 ExtraStack[], *StackPos;
u32 __stack_hop(u32 eax, u32 edx, void *func);
#define stack_hop(func, eax, edx)               \
    __stack_hop((u32)(eax), (u32)(edx), (func))
u32 __stack_hop_back(u32 eax, u32 edx, void *func);
#define stack_hop_back(func, eax, edx) ({                               \
        extern void _cfunc16_ ##func (void);                            \
        __stack_hop_back((u32)(eax), (u32)(edx), _cfunc16_ ##func );    \
    })
int on_extra_stack(void);
struct bregs;
void farcall16(struct bregs *callregs);
void farcall16big(struct bregs *callregs);
void __call16_int(struct bregs *callregs, u16 offset);
#define call16_int(nr, callregs) do {                           \
        extern void irq_trampoline_ ##nr (void);                \
        __call16_int((callregs), (u32)&irq_trampoline_ ##nr );  \
    } while (0)
void reset(void);
extern struct thread_info MainThread;
struct thread_info *getCurThread(void);
void yield(void);
void yield_toirq(void);
void thread_setup(void);
int threads_during_optionroms(void);
void run_thread(void (*func)(void*), void *data);
void wait_threads(void);
struct mutex_s { u32 isLocked; };
void mutex_lock(struct mutex_s *mutex);
void mutex_unlock(struct mutex_s *mutex);
void start_preempt(void);
void finish_preempt(void);
int wait_preempt(void);
void check_preempt(void);
u32 __call32_params(void *func, u32 eax, u32 edx, u32 ecx, u32 errret);
#define call32_params(func, eax, edx, ecx, errret) ({                   \
        extern void _cfunc32flat_ ##func (void);                        \
        __call32_params( _cfunc32flat_ ##func , (u32)(eax), (u32)(edx)  \
                        , (u32)(ecx), (errret));                        \
    })

// Inline functions

// Check if a call to stack_hop_back is needed.
static inline int
need_hop_back(void)
{
    return !MODESEGMENT || on_extra_stack();
}

#endif // stacks.h
