;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.0 #15242 (Linux)
;--------------------------------------------------------
	.module watchdog
	
	.optsdcc -mmcs51 --model-small
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _watchdog_pet_count
	.globl _watchdog_last_worker_seq
	.globl _watchdog_last_kernel_seq
	.globl _kernel_pet_seq
	.globl _worker_state
	.globl _worker_heartbeat
	.globl _kernel_heartbeat
	.globl _reset_reason
	.globl _system_flags
	.globl _system_stage
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area RSEG    (ABS,DATA)
	.org 0x0000
;--------------------------------------------------------
; special function bits
;--------------------------------------------------------
	.area RSEG    (ABS,DATA)
	.org 0x0000
;--------------------------------------------------------
; overlayable register banks
;--------------------------------------------------------
	.area REG_BANK_0	(REL,OVR,DATA)
	.ds 8
;--------------------------------------------------------
; internal ram data
;--------------------------------------------------------
	.area DSEG    (DATA)
_main_last_worker_10000_8:
	.ds 4
_main_kernel_timeout_10000_8:
	.ds 2
_main_worker_timeout_10000_8:
	.ds 2
;--------------------------------------------------------
; overlayable items in internal ram
;--------------------------------------------------------
;--------------------------------------------------------
; Stack segment in internal ram
;--------------------------------------------------------
	.area SSEG
__start__stack:
	.ds	1

;--------------------------------------------------------
; indirectly addressable internal ram data
;--------------------------------------------------------
	.area ISEG    (DATA)
;--------------------------------------------------------
; absolute internal ram data
;--------------------------------------------------------
	.area IABS    (ABS,DATA)
	.area IABS    (ABS,DATA)
;--------------------------------------------------------
; bit data
;--------------------------------------------------------
	.area BSEG    (BIT)
;--------------------------------------------------------
; paged external ram data
;--------------------------------------------------------
	.area PSEG    (PAG,XDATA)
;--------------------------------------------------------
; uninitialized external ram data
;--------------------------------------------------------
	.area XSEG    (XDATA)
_system_stage	=	0x0008
_system_flags	=	0x000c
_reset_reason	=	0x0010
_kernel_heartbeat	=	0x0020
_worker_heartbeat	=	0x0024
_worker_state	=	0x0028
_kernel_pet_seq	=	0x0030
_watchdog_last_kernel_seq	=	0x0034
_watchdog_last_worker_seq	=	0x0038
_watchdog_pet_count	=	0x003c
;--------------------------------------------------------
; absolute external ram data
;--------------------------------------------------------
	.area XABS    (ABS,XDATA)
;--------------------------------------------------------
; initialized external ram data
;--------------------------------------------------------
	.area XISEG   (XDATA)
	.area HOME    (CODE)
	.area GSINIT0 (CODE)
	.area GSINIT1 (CODE)
	.area GSINIT2 (CODE)
	.area GSINIT3 (CODE)
	.area GSINIT4 (CODE)
	.area GSINIT5 (CODE)
	.area GSINIT  (CODE)
	.area GSFINAL (CODE)
	.area CSEG    (CODE)
;--------------------------------------------------------
; interrupt vector
;--------------------------------------------------------
	.area HOME    (CODE)
__interrupt_vect:
	ljmp	__sdcc_gsinit_startup
; restartable atomic support routines
	.ds	5
sdcc_atomic_exchange_rollback_start::
	nop
	nop
sdcc_atomic_exchange_pdata_impl:
	movx	a, @r0
	mov	r3, a
	mov	a, r2
	movx	@r0, a
	sjmp	sdcc_atomic_exchange_exit
	nop
	nop
sdcc_atomic_exchange_xdata_impl:
	movx	a, @dptr
	mov	r3, a
	mov	a, r2
	movx	@dptr, a
	sjmp	sdcc_atomic_exchange_exit
sdcc_atomic_compare_exchange_idata_impl:
	mov	a, @r0
	cjne	a, ar2, .+#5
	mov	a, r3
	mov	@r0, a
	ret
	nop
sdcc_atomic_compare_exchange_pdata_impl:
	movx	a, @r0
	cjne	a, ar2, .+#5
	mov	a, r3
	movx	@r0, a
	ret
	nop
sdcc_atomic_compare_exchange_xdata_impl:
	movx	a, @dptr
	cjne	a, ar2, .+#5
	mov	a, r3
	movx	@dptr, a
	ret
sdcc_atomic_exchange_rollback_end::

sdcc_atomic_exchange_gptr_impl::
	jnb	b.6, sdcc_atomic_exchange_xdata_impl
	mov	r0, dpl
	jb	b.5, sdcc_atomic_exchange_pdata_impl
sdcc_atomic_exchange_idata_impl:
	mov	a, r2
	xch	a, @r0
	mov	dpl, a
	ret
sdcc_atomic_exchange_exit:
	mov	dpl, r3
	ret
sdcc_atomic_compare_exchange_gptr_impl::
	jnb	b.6, sdcc_atomic_compare_exchange_xdata_impl
	mov	r0, dpl
	jb	b.5, sdcc_atomic_compare_exchange_pdata_impl
	sjmp	sdcc_atomic_compare_exchange_idata_impl
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area HOME    (CODE)
	.area GSINIT  (CODE)
	.area GSFINAL (CODE)
	.area GSINIT  (CODE)
	.globl __sdcc_gsinit_startup
	.globl __sdcc_program_startup
	.globl __start__stack
	.globl __mcs51_genXINIT
	.globl __mcs51_genXRAMCLEAR
	.globl __mcs51_genRAMCLEAR
	.area GSFINAL (CODE)
	ljmp	__sdcc_program_startup
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area HOME    (CODE)
	.area HOME    (CODE)
__sdcc_program_startup:
	ljmp	_main
;	return from main will return to caller
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area CSEG    (CODE)
;------------------------------------------------------------
;Allocation info for local variables in function 'pet_hw'
;------------------------------------------------------------
;	watchdog.c:18: static void pet_hw(void) {
;	-----------------------------------------
;	 function pet_hw
;	-----------------------------------------
_pet_hw:
	ar7 = 0x07
	ar6 = 0x06
	ar5 = 0x05
	ar4 = 0x04
	ar3 = 0x03
	ar2 = 0x02
	ar1 = 0x01
	ar0 = 0x00
;	watchdog.c:19: watchdog_pet_count++;
	mov	dptr,#_watchdog_pet_count
	movx	a,@dptr
	mov	r4,a
	inc	dptr
	movx	a,@dptr
	mov	r5,a
	inc	dptr
	movx	a,@dptr
	mov	r6,a
	inc	dptr
	movx	a,@dptr
	mov	r7,a
	mov	dptr,#_watchdog_pet_count
	mov	a,#0x01
	add	a, r4
	movx	@dptr,a
	clr	a
	addc	a, r5
	inc	dptr
	movx	@dptr,a
	clr	a
	addc	a, r6
	inc	dptr
	movx	@dptr,a
	clr	a
	addc	a, r7
	inc	dptr
	movx	@dptr,a
;	watchdog.c:20: }
	ret
;------------------------------------------------------------
;Allocation info for local variables in function 'do_reset'
;------------------------------------------------------------
;reason        Allocated to registers r4 r5 r6 r7 
;------------------------------------------------------------
;	watchdog.c:22: static void do_reset(uint32_t reason) {
;	-----------------------------------------
;	 function do_reset
;	-----------------------------------------
_do_reset:
	mov	r4,dpl
	mov	r5,dph
	mov	r6,b
	mov	r7,a
;	watchdog.c:23: reset_reason = reason;
	mov	dptr,#_reset_reason
	mov	a,r4
	movx	@dptr,a
	mov	a,r5
	inc	dptr
	movx	@dptr,a
	mov	a,r6
	inc	dptr
	movx	@dptr,a
	mov	a,r7
	inc	dptr
	movx	@dptr,a
;	watchdog.c:24: system_flags |= 0x80000000UL;
	mov	dptr,#_system_flags
	movx	a,@dptr
	inc	dptr
	movx	a,@dptr
	inc	dptr
	movx	a,@dptr
	inc	dptr
	movx	a,@dptr
	orl	acc,#0x80
	movx	@dptr,a
00102$:
;	watchdog.c:26: pet_hw();
	lcall	_pet_hw
;	watchdog.c:28: }
	sjmp	00102$
;------------------------------------------------------------
;Allocation info for local variables in function 'main'
;------------------------------------------------------------
;last_pet      Allocated to registers r4 r5 r6 r7 
;last_worker   Allocated with name '_main_last_worker_10000_8'
;kernel_timeout Allocated with name '_main_kernel_timeout_10000_8'
;worker_timeout Allocated with name '_main_worker_timeout_10000_8'
;------------------------------------------------------------
;	watchdog.c:30: void main(void) {
;	-----------------------------------------
;	 function main
;	-----------------------------------------
_main:
;	watchdog.c:31: uint32_t last_pet = kernel_pet_seq;
	mov	dptr,#_kernel_pet_seq
	movx	a,@dptr
	mov	r4,a
	inc	dptr
	movx	a,@dptr
	mov	r5,a
	inc	dptr
	movx	a,@dptr
	mov	r6,a
	inc	dptr
	movx	a,@dptr
	mov	r7,a
;	watchdog.c:32: uint32_t last_worker = worker_heartbeat;
	mov	dptr,#_worker_heartbeat
	movx	a,@dptr
	mov	_main_last_worker_10000_8,a
	inc	dptr
	movx	a,@dptr
	mov	(_main_last_worker_10000_8 + 1),a
	inc	dptr
	movx	a,@dptr
	mov	(_main_last_worker_10000_8 + 2),a
	inc	dptr
	movx	a,@dptr
	mov	(_main_last_worker_10000_8 + 3),a
;	watchdog.c:33: uint16_t kernel_timeout = 0;
	clr	a
	mov	_main_kernel_timeout_10000_8,a
	mov	(_main_kernel_timeout_10000_8 + 1),a
;	watchdog.c:34: uint16_t worker_timeout = 0;
	mov	_main_worker_timeout_10000_8,a
	mov	(_main_worker_timeout_10000_8 + 1),a
;	watchdog.c:36: watchdog_last_kernel_seq = last_pet;
	mov	dptr,#_watchdog_last_kernel_seq
	mov	a,r4
	movx	@dptr,a
	mov	a,r5
	inc	dptr
	movx	@dptr,a
	mov	a,r6
	inc	dptr
	movx	@dptr,a
	mov	a,r7
	inc	dptr
	movx	@dptr,a
;	watchdog.c:37: watchdog_last_worker_seq = last_worker;
	mov	dptr,#_watchdog_last_worker_seq
	mov	a,_main_last_worker_10000_8
	movx	@dptr,a
	mov	a,(_main_last_worker_10000_8 + 1)
	inc	dptr
	movx	@dptr,a
	mov	a,(_main_last_worker_10000_8 + 2)
	inc	dptr
	movx	@dptr,a
	mov	a,(_main_last_worker_10000_8 + 3)
	inc	dptr
	movx	@dptr,a
;	watchdog.c:38: watchdog_pet_count = 0;
	mov	dptr,#_watchdog_pet_count
	clr	a
	movx	@dptr,a
	inc	dptr
	movx	@dptr,a
	inc	dptr
	movx	@dptr,a
	inc	dptr
	movx	@dptr,a
;	watchdog.c:40: while (1) {
00113$:
;	watchdog.c:41: if (kernel_pet_seq != last_pet) {
	mov	dptr,#_kernel_pet_seq
	movx	a,@dptr
	mov	r0,a
	inc	dptr
	movx	a,@dptr
	mov	r1,a
	inc	dptr
	movx	a,@dptr
	mov	r2,a
	inc	dptr
	movx	a,@dptr
	mov	r3,a
	mov	a,r0
	cjne	a,ar4,00153$
	mov	a,r1
	cjne	a,ar5,00153$
	mov	a,r2
	cjne	a,ar6,00153$
	mov	a,r3
	cjne	a,ar7,00153$
	sjmp	00104$
00153$:
;	watchdog.c:42: last_pet = kernel_pet_seq;
	mov	dptr,#_kernel_pet_seq
	movx	a,@dptr
	mov	r4,a
	inc	dptr
	movx	a,@dptr
	mov	r5,a
	inc	dptr
	movx	a,@dptr
	mov	r6,a
	inc	dptr
	movx	a,@dptr
	mov	r7,a
;	watchdog.c:43: watchdog_last_kernel_seq = last_pet;
	mov	dptr,#_watchdog_last_kernel_seq
	mov	a,r4
	movx	@dptr,a
	mov	a,r5
	inc	dptr
	movx	@dptr,a
	mov	a,r6
	inc	dptr
	movx	@dptr,a
	mov	a,r7
	inc	dptr
	movx	@dptr,a
;	watchdog.c:44: kernel_timeout = 0;
	clr	a
	mov	_main_kernel_timeout_10000_8,a
	mov	(_main_kernel_timeout_10000_8 + 1),a
;	watchdog.c:45: pet_hw();
	push	ar7
	push	ar6
	push	ar5
	push	ar4
	lcall	_pet_hw
	pop	ar4
	pop	ar5
	pop	ar6
	pop	ar7
	sjmp	00105$
00104$:
;	watchdog.c:46: } else if (++kernel_timeout > 60000u) {
	inc	_main_kernel_timeout_10000_8
	clr	a
	cjne	a,_main_kernel_timeout_10000_8,00154$
	inc	(_main_kernel_timeout_10000_8 + 1)
00154$:
	mov	r2,_main_kernel_timeout_10000_8
	mov	r3,(_main_kernel_timeout_10000_8 + 1)
	clr	c
	mov	a,#0x60
	subb	a,r2
	mov	a,#0xea
	subb	a,r3
	jnc	00105$
;	watchdog.c:47: do_reset(0x80510001UL);
	mov	dptr,#0x0001
	mov	b, #0x51
	mov	a, #0x80
	push	ar7
	push	ar6
	push	ar5
	push	ar4
	lcall	_do_reset
	pop	ar4
	pop	ar5
	pop	ar6
	pop	ar7
00105$:
;	watchdog.c:50: if (worker_heartbeat != last_worker) {
	mov	dptr,#_worker_heartbeat
	movx	a,@dptr
	mov	r0,a
	inc	dptr
	movx	a,@dptr
	mov	r1,a
	inc	dptr
	movx	a,@dptr
	mov	r2,a
	inc	dptr
	movx	a,@dptr
	mov	r3,a
	mov	a,r0
	cjne	a,_main_last_worker_10000_8,00156$
	mov	a,r1
	cjne	a,(_main_last_worker_10000_8 + 1),00156$
	mov	a,r2
	cjne	a,(_main_last_worker_10000_8 + 2),00156$
	mov	a,r3
	cjne	a,(_main_last_worker_10000_8 + 3),00156$
	sjmp	00110$
00156$:
;	watchdog.c:51: last_worker = worker_heartbeat;
	mov	dptr,#_worker_heartbeat
	movx	a,@dptr
	mov	_main_last_worker_10000_8,a
	inc	dptr
	movx	a,@dptr
	mov	(_main_last_worker_10000_8 + 1),a
	inc	dptr
	movx	a,@dptr
	mov	(_main_last_worker_10000_8 + 2),a
	inc	dptr
	movx	a,@dptr
	mov	(_main_last_worker_10000_8 + 3),a
;	watchdog.c:52: watchdog_last_worker_seq = last_worker;
	mov	dptr,#_watchdog_last_worker_seq
	mov	a,_main_last_worker_10000_8
	movx	@dptr,a
	mov	a,(_main_last_worker_10000_8 + 1)
	inc	dptr
	movx	@dptr,a
	mov	a,(_main_last_worker_10000_8 + 2)
	inc	dptr
	movx	@dptr,a
	mov	a,(_main_last_worker_10000_8 + 3)
	inc	dptr
	movx	@dptr,a
;	watchdog.c:53: worker_timeout = 0;
	clr	a
	mov	_main_worker_timeout_10000_8,a
	mov	(_main_worker_timeout_10000_8 + 1),a
	ljmp	00113$
00110$:
;	watchdog.c:54: } else if (worker_state == 3u && ++worker_timeout > 60000u) {
	mov	dptr,#_worker_state
	movx	a,@dptr
	mov	r0,a
	inc	dptr
	movx	a,@dptr
	mov	r1,a
	inc	dptr
	movx	a,@dptr
	mov	r2,a
	inc	dptr
	movx	a,@dptr
	mov	r3,a
	cjne	r0,#0x03,00157$
	cjne	r1,#0x00,00157$
	cjne	r2,#0x00,00157$
	cjne	r3,#0x00,00157$
	sjmp	00158$
00157$:
	ljmp	00113$
00158$:
	inc	_main_worker_timeout_10000_8
	clr	a
	cjne	a,_main_worker_timeout_10000_8,00159$
	inc	(_main_worker_timeout_10000_8 + 1)
00159$:
	mov	r2,_main_worker_timeout_10000_8
	mov	r3,(_main_worker_timeout_10000_8 + 1)
	clr	c
	mov	a,#0x60
	subb	a,r2
	mov	a,#0xea
	subb	a,r3
	jc	00160$
	ljmp	00113$
00160$:
;	watchdog.c:55: do_reset(0x80510002UL);
	mov	dptr,#0x0002
	mov	b, #0x51
	mov	a, #0x80
	push	ar7
	push	ar6
	push	ar5
	push	ar4
	lcall	_do_reset
	pop	ar4
	pop	ar5
	pop	ar6
	pop	ar7
;	watchdog.c:58: }
	ljmp	00113$
	.area CSEG    (CODE)
	.area CONST   (CODE)
	.area XINIT   (CODE)
	.area CABS    (ABS,CODE)
