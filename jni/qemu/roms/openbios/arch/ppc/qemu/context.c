/*
 * context switching
 * 2003-10 by SONE Takeshi
 */

#include "config.h"
#include "kernel/kernel.h"
#include "context.h"
#include "libopenbios/sys_info.h"

#define MAIN_STACK_SIZE 16384
#define IMAGE_STACK_SIZE 4096*2

#define debug printk

#ifdef CONFIG_PPC_64BITSUPPORT
  #ifdef __powerpc64__
    #define ULONG_SIZE 8
    #define STACKFRAME_MINSIZE 48
    #define STKOFF STACKFRAME_MINSIZE
    #define SAVE_SPACE 320
  #else
    #define ULONG_SIZE 4
    #define STACKFRAME_MINSIZE 16
    #define STKOFF 8
    #define SAVE_SPACE 144
  #endif
#endif

static void start_main(void); /* forward decl. */
void __exit_context(void); /* assembly routine */

unsigned int start_elf(unsigned long entry_point, unsigned long param);
void entry(void);
void of_client_callback(void);

/*
 * Main context structure
 * It is placed at the bottom of our stack, and loaded by assembly routine
 * to start us up.
 */
static struct context main_ctx = {
    .sp = (unsigned long) &_estack - SAVE_SPACE,
    .pc = (unsigned long) start_main,
    .return_addr = (unsigned long) __exit_context,
};

/* This is used by assembly routine to load/store the context which
 * it is to switch/switched.  */
struct context * volatile __context = &main_ctx;

/* Stack for loaded ELF image */
static uint8_t image_stack[IMAGE_STACK_SIZE];

/* Pointer to startup context (physical address) */
unsigned long __boot_ctx;

/*
 * Main starter
 * This is the C function that runs first.
 */
static void start_main(void)
{
    /* Save startup context, so we can refer to it later.
     * We have to keep it in physical address since we will relocate. */
    __boot_ctx = virt_to_phys(__context);

    /* Start the real fun */
    entry();

    /* Returning from here should jump to __exit_context */
    __context = boot_ctx;
}

/* Setup a new context using the given stack.
 */
struct context *
init_context(uint8_t *stack, uint32_t stack_size, int num_params)
{
    struct context *ctx;

    ctx = (struct context *)
	(stack + stack_size - (sizeof(*ctx) + num_params*sizeof(unsigned long)));
    memset(ctx, 0, sizeof(*ctx));

    /* Fill in reasonable default for flat memory model */
    ctx->sp = virt_to_phys(SP_LOC(ctx));
    ctx->return_addr = virt_to_phys(__exit_context);
    
    return ctx;
}

/* Switch to another context. */
struct context *switch_to(struct context *ctx)
{
    volatile struct context *save;
    struct context *ret;
    unsigned int lr;

    debug("switching to new context:\n");
    save = __context;
    __context = ctx;

    asm __volatile__ ("mflr %%r9\n\t"
                      "stw %%r9, %0\n\t"
                      "bl __switch_context\n\t"
                      "lwz %%r9, %0\n\t"
                      "mtlr %%r9\n\t" : "=m" (lr) : "m" (lr) : "%r9" );
    
    ret = __context;
    __context = (struct context *)save;
    return ret;
}

/* Start ELF Boot image */
unsigned int start_elf(unsigned long entry_point, unsigned long param)
{
    struct context *ctx;

    /* According to IEEE 1275, PPC bindings:
     *
     *    MSR = FP, ME + (DR|IR)
     *    r1 = stack (32 K + 32 bytes link area above)
     *    r5 = client interface handler
     *    r6 = address of client program arguments (unused)
     *    r7 = length of client program arguments (unused)
     *
     *      Yaboot and Linux use r3 and r4 for initrd address and size
     */
    
    ctx = init_context(image_stack, sizeof image_stack, 1);
    ctx->pc = entry_point;
    ctx->regs[REG_R5] = (unsigned long)of_client_callback;
    ctx->regs[REG_R6] = 0;
    ctx->regs[REG_R7] = 0;
    ctx->param[0] = param;
   
    ctx = switch_to(ctx);
    return ctx->regs[REG_R3];
}
