#ifndef PPC_CONTEXT_H
#define PPC_CONTEXT_H

struct context {
#define SP_LOC(ctx) (&(ctx)->sp)
    unsigned long _sp;
    unsigned long return_addr;
    unsigned long sp;
    unsigned long pc;
    /* General registers */
    unsigned long regs[34];
#define REG_R3 3
#define REG_R5 8
#define REG_R6 9
#define REG_R7 10
    /* Flags */
    /* Optional stack contents */
    unsigned long param[0];
};

/* Create a new context in the given stack */
struct context *
init_context(uint8_t *stack, uint32_t stack_size, int num_param);

/* Switch context */
struct context *switch_to(struct context *);

/* Holds physical address of boot context */
extern unsigned long __boot_ctx;

/* This can always be safely used to refer to the boot context */
#define boot_ctx ((struct context *) phys_to_virt(__boot_ctx))

#endif /* PPC_CONTEXT_H */
