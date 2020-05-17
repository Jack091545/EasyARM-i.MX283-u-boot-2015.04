/*
 *  crt0 - C-runtime startup Code for ARM U-Boot
 *
 *  Copyright (c) 2012  Albert ARIBAUD <albert.u.boot@aribaud.net>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <asm-offsets.h>
#include <linux/linkage.h>

/*
 * This file handles the target-independent stages of the U-Boot
 * start-up where a C runtime environment is needed. Its entry point
 * is _main and is branched into from the target's start.S file.
 *
 * _main execution sequence is:
 *
 * 1. Set up initial environment for calling board_init_f().
 *    This environment only provides a stack and a place to store
 *    the GD ('global data') structure, both located in some readily
 *    available RAM (SRAM, locked cache...). In this context, VARIABLE
 *    global data, initialized or not (BSS), are UNAVAILABLE; only
 *    CONSTANT initialized data are available.
 *
 * 2. Call board_init_f(). This function prepares the hardware for
 *    execution from system RAM (DRAM, DDR...) As system RAM may not
 *    be available yet, , board_init_f() must use the current GD to
 *    store any data which must be passed on to later stages. These
 *    data include the relocation destination, the future stack, and
 *    the future GD location.
 *
 * (the following applies only to non-SPL builds)
 *
 * 3. Set up intermediate environment where the stack and GD are the
 *    ones allocated by board_init_f() in system RAM, but BSS and
 *    initialized non-const data are still not available.
 *
 * 4. Call relocate_code(). This function relocates U-Boot from its
 *    current location into the relocation destination computed by
 *    board_init_f().
 *
 * 5. Set up final environment for calling board_init_r(). This
 *    environment has BSS (initialized to 0), initialized non-const
 *    data (initialized to their intended value), and stack in system
 *    RAM. GD has retained values set by board_init_f(). Some CPUs
 *    have some work left to do at this point regarding memory, so
 *    call c_runtime_cpu_setup.
 *
 * 6. Branch to board_init_r().
 */

/*
 * entry point of crt0 sequence
 */

ENTRY(_main)

/*
 * Set up initial C runtime environment and call board_init_f(0).
 */


/* 			memory map 							*/
/* ---------0x0001FFFF ------------------------ */
/*			GD_SIZE = 176(maybe not used)		*/
/* ---------0x0001FF50 ------------------------ */
/*			GD_SIZE = 176	 					*/
/* ---------0x0001FEA0 sp --------------------- */



#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_STACK)
	ldr	sp, =(CONFIG_SPL_STACK)
#else
	/* 从这里开始执行 CONFIG_SYS_INIT_SP_ADDR = 0x0001ff50 */
	ldr	sp, =(CONFIG_SYS_INIT_SP_ADDR)
#endif
	/* sp &= ~(0x07) */
	bic	sp, sp, #7	/* 8-byte alignment for ABI compliance */
	/* r2 = sp = 0x0001ff50 */
	mov	r2, sp
	/* sp = sp - GD_SIZE(176) = 0x0001FEA0 */
	sub	sp, sp, #GD_SIZE	/* allocate one GD above SP */
	/* sp &= ~(0x07), sp = 0x0001FEA0 */
	bic	sp, sp, #7	/* 8-byte alignment for ABI compliance */
	/* r9 = sp = 0x0001FEA0 */
	mov	r9, sp		/* GD is above SP */
	/* r1 = sp = 0x0001FEA0 */
	mov	r1, sp
	/* r0 = 0 */
	mov	r0, #0
clr_gd:
	/* r1 = 0x0001FEA0, r2 = 0x0001ff50  */
	cmp	r1, r2			/* while not at end of GD */
	strlo	r0, [r1]		/* clear 32-bit GD word */
	addlo	r1, r1, #4		/* move to next */
	blo	clr_gd
#if defined(CONFIG_SYS_MALLOC_F_LEN)
	sub	sp, sp, #CONFIG_SYS_MALLOC_F_LEN
	str	sp, [r9, #GD_MALLOC_BASE]
#endif
	/* mov r0, #0 not needed due to above code */
	bl	board_init_f

#if ! defined(CONFIG_SPL_BUILD)

/*
 * Set up intermediate environment (new sp and gd) and call
 * relocate_code(addr_moni). Trick here is that we'll return
 * 'here' but relocated.
 */

	/* 			memory map 							*/
	/* ---------0x48000000 = ram_top -------------- */
	/* ---------0x47FF4000 ------------------------ */
	/*			TLB	= 4 * 4096	 					*/
	/* ---------0x47ff0000 = arch.tlb_addr --------	*/
	/* 			u-boot = 780364(maybe not) 			*/
	/* ---------0x47F39000 = relocaddr ------------ */
	/* 			malloc + env size = 0x404000 		*/
	/* ---------0x47B35000 ------------------------	*/
	/* 			board info = 80						*/
	/* ---------0x47B34FB0 ------------------------ */
	/* 			global data = 176					*/			
	/* ---------0x47B34F00 ------------------------ */
	/* ---------0x47B34EF0 = irq_sp --------------- */
	/* ---------0x47B34EE0 = start_addr_sp -------- */
	/*			stack								*/

	/* sp = gd->start_addr_sp = 0x47B34EE0 */
	ldr	sp, [r9, #GD_START_ADDR_SP]	
	/* sp = = 0x47B34EE0 */
	bic	sp, sp, #7	/* 8-byte alignment for ABI compliance */

	/* gd = r9 = gd->bd = 0x47B34FB0 */
	ldr	r9, [r9, #GD_BD]
	/* gd = r9 = 0x47B34F00 */
	sub	r9, r9, #GD_SIZE		/* new GD is below bd, r9 = 0x47b34f00 */
	
	adr	lr, here
	/* r0 = 7F39000 */
	ldr	r0, [r9, #GD_RELOC_OFF]		/* r0 = gd->reloc_off */
	/* here地址 + 0x7F3900 */
	add	lr, lr, r0
	/* R0 = 0x47F39000 */
	ldr	r0, [r9, #GD_RELOCADDR]		/* r0 = gd->relocaddr */
	b	relocate_code
here:
/*
 * now relocate vectors
 */

	bl	relocate_vectors

/* Set up final (full) environment */

	bl	c_runtime_cpu_setup	/* we still call old routine here */
#endif
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_FRAMEWORK)
# ifdef CONFIG_SPL_BUILD
	/* Use a DRAM stack for the rest of SPL, if requested */
	bl	spl_relocate_stack_gd
	cmp	r0, #0
	movne	sp, r0
# endif
	ldr	r0, =__bss_start	/* this is auto-relocated! */

#ifdef CONFIG_USE_ARCH_MEMSET
	ldr	r3, =__bss_end		/* this is auto-relocated! */
	mov	r1, #0x00000000		/* prepare zero to clear BSS */

	subs	r2, r3, r0		/* r2 = memset len */
	bl	memset
#else
	ldr	r1, =__bss_end		/* this is auto-relocated! */
	mov	r2, #0x00000000		/* prepare zero to clear BSS */

clbss_l:cmp	r0, r1			/* while not at end of BSS */
	strlo	r2, [r0]		/* clear 32-bit BSS word */
	addlo	r0, r0, #4		/* move to next */
	blo	clbss_l
#endif

#if ! defined(CONFIG_SPL_BUILD)
	bl coloured_LED_init
	bl red_led_on
#endif
	/* call board_init_r(gd_t *id, ulong dest_addr) */
	mov     r0, r9                  /* gd_t */
	ldr	r1, [r9, #GD_RELOCADDR]	/* dest_addr */
	/* call board_init_r */
	ldr	pc, =board_init_r	/* this is auto-relocated! */

	/* we should not return here. */
#endif


#if 0
test_hang:
	b test_hang


print_r9:
	push {lr}
	ldr r0, =pstr_r9
	mov r1, r9
	bl printf
	pop {pc}



print_r9_content:
	push {lr}
	ldr r0, =pstr_r9_content
	ldr r1, [r9, #GD_BD]
	bl printf
	pop {pc}

pstr_r9:
	.asciz "r9 = %08x\n"

pstr_r9_content:
	.asciz "[r9] = %08x\n"
#endif

ENDPROC(_main)
