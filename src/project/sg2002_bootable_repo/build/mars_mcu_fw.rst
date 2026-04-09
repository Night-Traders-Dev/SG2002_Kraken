                                      1 ;--------------------------------------------------------
                                      2 ; File Created by SDCC : free open source ISO C Compiler
                                      3 ; Version 4.5.0 #15242 (Linux)
                                      4 ;--------------------------------------------------------
                                      5 	.module watchdog
                                      6 	
                                      7 	.optsdcc -mmcs51 --model-small
                                      8 ;--------------------------------------------------------
                                      9 ; Public variables in this module
                                     10 ;--------------------------------------------------------
                                     11 	.globl _main
                                     12 	.globl _watchdog_pet_count
                                     13 	.globl _watchdog_last_worker_seq
                                     14 	.globl _watchdog_last_kernel_seq
                                     15 	.globl _kernel_pet_seq
                                     16 	.globl _worker_state
                                     17 	.globl _worker_heartbeat
                                     18 	.globl _kernel_heartbeat
                                     19 	.globl _reset_reason
                                     20 	.globl _system_flags
                                     21 	.globl _system_stage
                                     22 ;--------------------------------------------------------
                                     23 ; special function registers
                                     24 ;--------------------------------------------------------
                                     25 	.area RSEG    (ABS,DATA)
      000000                         26 	.org 0x0000
                                     27 ;--------------------------------------------------------
                                     28 ; special function bits
                                     29 ;--------------------------------------------------------
                                     30 	.area RSEG    (ABS,DATA)
      000000                         31 	.org 0x0000
                                     32 ;--------------------------------------------------------
                                     33 ; overlayable register banks
                                     34 ;--------------------------------------------------------
                                     35 	.area REG_BANK_0	(REL,OVR,DATA)
      000000                         36 	.ds 8
                                     37 ;--------------------------------------------------------
                                     38 ; internal ram data
                                     39 ;--------------------------------------------------------
                                     40 	.area DSEG    (DATA)
      000008                         41 _main_last_worker_10000_8:
      000008                         42 	.ds 4
      00000C                         43 _main_kernel_timeout_10000_8:
      00000C                         44 	.ds 2
      00000E                         45 _main_worker_timeout_10000_8:
      00000E                         46 	.ds 2
                                     47 ;--------------------------------------------------------
                                     48 ; overlayable items in internal ram
                                     49 ;--------------------------------------------------------
                                     50 ;--------------------------------------------------------
                                     51 ; Stack segment in internal ram
                                     52 ;--------------------------------------------------------
                                     53 	.area SSEG
      000010                         54 __start__stack:
      000010                         55 	.ds	1
                                     56 
                                     57 ;--------------------------------------------------------
                                     58 ; indirectly addressable internal ram data
                                     59 ;--------------------------------------------------------
                                     60 	.area ISEG    (DATA)
                                     61 ;--------------------------------------------------------
                                     62 ; absolute internal ram data
                                     63 ;--------------------------------------------------------
                                     64 	.area IABS    (ABS,DATA)
                                     65 	.area IABS    (ABS,DATA)
                                     66 ;--------------------------------------------------------
                                     67 ; bit data
                                     68 ;--------------------------------------------------------
                                     69 	.area BSEG    (BIT)
                                     70 ;--------------------------------------------------------
                                     71 ; paged external ram data
                                     72 ;--------------------------------------------------------
                                     73 	.area PSEG    (PAG,XDATA)
                                     74 ;--------------------------------------------------------
                                     75 ; uninitialized external ram data
                                     76 ;--------------------------------------------------------
                                     77 	.area XSEG    (XDATA)
                           000008    78 _system_stage	=	0x0008
                           00000C    79 _system_flags	=	0x000c
                           000010    80 _reset_reason	=	0x0010
                           000020    81 _kernel_heartbeat	=	0x0020
                           000024    82 _worker_heartbeat	=	0x0024
                           000028    83 _worker_state	=	0x0028
                           000030    84 _kernel_pet_seq	=	0x0030
                           000034    85 _watchdog_last_kernel_seq	=	0x0034
                           000038    86 _watchdog_last_worker_seq	=	0x0038
                           00003C    87 _watchdog_pet_count	=	0x003c
                                     88 ;--------------------------------------------------------
                                     89 ; absolute external ram data
                                     90 ;--------------------------------------------------------
                                     91 	.area XABS    (ABS,XDATA)
                                     92 ;--------------------------------------------------------
                                     93 ; initialized external ram data
                                     94 ;--------------------------------------------------------
                                     95 	.area XISEG   (XDATA)
                                     96 	.area HOME    (CODE)
                                     97 	.area GSINIT0 (CODE)
                                     98 	.area GSINIT1 (CODE)
                                     99 	.area GSINIT2 (CODE)
                                    100 	.area GSINIT3 (CODE)
                                    101 	.area GSINIT4 (CODE)
                                    102 	.area GSINIT5 (CODE)
                                    103 	.area GSINIT  (CODE)
                                    104 	.area GSFINAL (CODE)
                                    105 	.area CSEG    (CODE)
                                    106 ;--------------------------------------------------------
                                    107 ; interrupt vector
                                    108 ;--------------------------------------------------------
                                    109 	.area HOME    (CODE)
      000000                        110 __interrupt_vect:
      000000 02 00 4C         [24]  111 	ljmp	__sdcc_gsinit_startup
                                    112 ; restartable atomic support routines
      000003                        113 	.ds	5
      000008                        114 sdcc_atomic_exchange_rollback_start::
      000008 00               [12]  115 	nop
      000009 00               [12]  116 	nop
      00000A                        117 sdcc_atomic_exchange_pdata_impl:
      00000A E2               [24]  118 	movx	a, @r0
      00000B FB               [12]  119 	mov	r3, a
      00000C EA               [12]  120 	mov	a, r2
      00000D F2               [24]  121 	movx	@r0, a
      00000E 80 2C            [24]  122 	sjmp	sdcc_atomic_exchange_exit
      000010 00               [12]  123 	nop
      000011 00               [12]  124 	nop
      000012                        125 sdcc_atomic_exchange_xdata_impl:
      000012 E0               [24]  126 	movx	a, @dptr
      000013 FB               [12]  127 	mov	r3, a
      000014 EA               [12]  128 	mov	a, r2
      000015 F0               [24]  129 	movx	@dptr, a
      000016 80 24            [24]  130 	sjmp	sdcc_atomic_exchange_exit
      000018                        131 sdcc_atomic_compare_exchange_idata_impl:
      000018 E6               [12]  132 	mov	a, @r0
      000019 B5 02 02         [24]  133 	cjne	a, ar2, .+#5
      00001C EB               [12]  134 	mov	a, r3
      00001D F6               [12]  135 	mov	@r0, a
      00001E 22               [24]  136 	ret
      00001F 00               [12]  137 	nop
      000020                        138 sdcc_atomic_compare_exchange_pdata_impl:
      000020 E2               [24]  139 	movx	a, @r0
      000021 B5 02 02         [24]  140 	cjne	a, ar2, .+#5
      000024 EB               [12]  141 	mov	a, r3
      000025 F2               [24]  142 	movx	@r0, a
      000026 22               [24]  143 	ret
      000027 00               [12]  144 	nop
      000028                        145 sdcc_atomic_compare_exchange_xdata_impl:
      000028 E0               [24]  146 	movx	a, @dptr
      000029 B5 02 02         [24]  147 	cjne	a, ar2, .+#5
      00002C EB               [12]  148 	mov	a, r3
      00002D F0               [24]  149 	movx	@dptr, a
      00002E 22               [24]  150 	ret
      00002F                        151 sdcc_atomic_exchange_rollback_end::
                                    152 
      00002F                        153 sdcc_atomic_exchange_gptr_impl::
      00002F 30 F6 E0         [24]  154 	jnb	b.6, sdcc_atomic_exchange_xdata_impl
      000032 A8 82            [24]  155 	mov	r0, dpl
      000034 20 F5 D3         [24]  156 	jb	b.5, sdcc_atomic_exchange_pdata_impl
      000037                        157 sdcc_atomic_exchange_idata_impl:
      000037 EA               [12]  158 	mov	a, r2
      000038 C6               [12]  159 	xch	a, @r0
      000039 F5 82            [12]  160 	mov	dpl, a
      00003B 22               [24]  161 	ret
      00003C                        162 sdcc_atomic_exchange_exit:
      00003C 8B 82            [24]  163 	mov	dpl, r3
      00003E 22               [24]  164 	ret
      00003F                        165 sdcc_atomic_compare_exchange_gptr_impl::
      00003F 30 F6 E6         [24]  166 	jnb	b.6, sdcc_atomic_compare_exchange_xdata_impl
      000042 A8 82            [24]  167 	mov	r0, dpl
      000044 20 F5 D9         [24]  168 	jb	b.5, sdcc_atomic_compare_exchange_pdata_impl
      000047 80 CF            [24]  169 	sjmp	sdcc_atomic_compare_exchange_idata_impl
                                    170 ;--------------------------------------------------------
                                    171 ; global & static initialisations
                                    172 ;--------------------------------------------------------
                                    173 	.area HOME    (CODE)
                                    174 	.area GSINIT  (CODE)
                                    175 	.area GSFINAL (CODE)
                                    176 	.area GSINIT  (CODE)
                                    177 	.globl __sdcc_gsinit_startup
                                    178 	.globl __sdcc_program_startup
                                    179 	.globl __start__stack
                                    180 	.globl __mcs51_genXINIT
                                    181 	.globl __mcs51_genXRAMCLEAR
                                    182 	.globl __mcs51_genRAMCLEAR
                                    183 	.area GSFINAL (CODE)
      0000A5 02 00 49         [24]  184 	ljmp	__sdcc_program_startup
                                    185 ;--------------------------------------------------------
                                    186 ; Home
                                    187 ;--------------------------------------------------------
                                    188 	.area HOME    (CODE)
                                    189 	.area HOME    (CODE)
      000049                        190 __sdcc_program_startup:
      000049 02 00 F2         [24]  191 	ljmp	_main
                                    192 ;	return from main will return to caller
                                    193 ;--------------------------------------------------------
                                    194 ; code
                                    195 ;--------------------------------------------------------
                                    196 	.area CSEG    (CODE)
                                    197 ;------------------------------------------------------------
                                    198 ;Allocation info for local variables in function 'pet_hw'
                                    199 ;------------------------------------------------------------
                                    200 ;	watchdog.c:18: static void pet_hw(void) {
                                    201 ;	-----------------------------------------
                                    202 ;	 function pet_hw
                                    203 ;	-----------------------------------------
      0000A8                        204 _pet_hw:
                           000007   205 	ar7 = 0x07
                           000006   206 	ar6 = 0x06
                           000005   207 	ar5 = 0x05
                           000004   208 	ar4 = 0x04
                           000003   209 	ar3 = 0x03
                           000002   210 	ar2 = 0x02
                           000001   211 	ar1 = 0x01
                           000000   212 	ar0 = 0x00
                                    213 ;	watchdog.c:19: watchdog_pet_count++;
      0000A8 90 00 3C         [24]  214 	mov	dptr,#_watchdog_pet_count
      0000AB E0               [24]  215 	movx	a,@dptr
      0000AC FC               [12]  216 	mov	r4,a
      0000AD A3               [24]  217 	inc	dptr
      0000AE E0               [24]  218 	movx	a,@dptr
      0000AF FD               [12]  219 	mov	r5,a
      0000B0 A3               [24]  220 	inc	dptr
      0000B1 E0               [24]  221 	movx	a,@dptr
      0000B2 FE               [12]  222 	mov	r6,a
      0000B3 A3               [24]  223 	inc	dptr
      0000B4 E0               [24]  224 	movx	a,@dptr
      0000B5 FF               [12]  225 	mov	r7,a
      0000B6 90 00 3C         [24]  226 	mov	dptr,#_watchdog_pet_count
      0000B9 74 01            [12]  227 	mov	a,#0x01
      0000BB 2C               [12]  228 	add	a, r4
      0000BC F0               [24]  229 	movx	@dptr,a
      0000BD E4               [12]  230 	clr	a
      0000BE 3D               [12]  231 	addc	a, r5
      0000BF A3               [24]  232 	inc	dptr
      0000C0 F0               [24]  233 	movx	@dptr,a
      0000C1 E4               [12]  234 	clr	a
      0000C2 3E               [12]  235 	addc	a, r6
      0000C3 A3               [24]  236 	inc	dptr
      0000C4 F0               [24]  237 	movx	@dptr,a
      0000C5 E4               [12]  238 	clr	a
      0000C6 3F               [12]  239 	addc	a, r7
      0000C7 A3               [24]  240 	inc	dptr
      0000C8 F0               [24]  241 	movx	@dptr,a
                                    242 ;	watchdog.c:20: }
      0000C9 22               [24]  243 	ret
                                    244 ;------------------------------------------------------------
                                    245 ;Allocation info for local variables in function 'do_reset'
                                    246 ;------------------------------------------------------------
                                    247 ;reason        Allocated to registers r4 r5 r6 r7 
                                    248 ;------------------------------------------------------------
                                    249 ;	watchdog.c:22: static void do_reset(uint32_t reason) {
                                    250 ;	-----------------------------------------
                                    251 ;	 function do_reset
                                    252 ;	-----------------------------------------
      0000CA                        253 _do_reset:
      0000CA AC 82            [24]  254 	mov	r4,dpl
      0000CC AD 83            [24]  255 	mov	r5,dph
      0000CE AE F0            [24]  256 	mov	r6,b
      0000D0 FF               [12]  257 	mov	r7,a
                                    258 ;	watchdog.c:23: reset_reason = reason;
      0000D1 90 00 10         [24]  259 	mov	dptr,#_reset_reason
      0000D4 EC               [12]  260 	mov	a,r4
      0000D5 F0               [24]  261 	movx	@dptr,a
      0000D6 ED               [12]  262 	mov	a,r5
      0000D7 A3               [24]  263 	inc	dptr
      0000D8 F0               [24]  264 	movx	@dptr,a
      0000D9 EE               [12]  265 	mov	a,r6
      0000DA A3               [24]  266 	inc	dptr
      0000DB F0               [24]  267 	movx	@dptr,a
      0000DC EF               [12]  268 	mov	a,r7
      0000DD A3               [24]  269 	inc	dptr
      0000DE F0               [24]  270 	movx	@dptr,a
                                    271 ;	watchdog.c:24: system_flags |= 0x80000000UL;
      0000DF 90 00 0C         [24]  272 	mov	dptr,#_system_flags
      0000E2 E0               [24]  273 	movx	a,@dptr
      0000E3 A3               [24]  274 	inc	dptr
      0000E4 E0               [24]  275 	movx	a,@dptr
      0000E5 A3               [24]  276 	inc	dptr
      0000E6 E0               [24]  277 	movx	a,@dptr
      0000E7 A3               [24]  278 	inc	dptr
      0000E8 E0               [24]  279 	movx	a,@dptr
      0000E9 43 E0 80         [24]  280 	orl	acc,#0x80
      0000EC F0               [24]  281 	movx	@dptr,a
      0000ED                        282 00102$:
                                    283 ;	watchdog.c:26: pet_hw();
      0000ED 12 00 A8         [24]  284 	lcall	_pet_hw
                                    285 ;	watchdog.c:28: }
      0000F0 80 FB            [24]  286 	sjmp	00102$
                                    287 ;------------------------------------------------------------
                                    288 ;Allocation info for local variables in function 'main'
                                    289 ;------------------------------------------------------------
                                    290 ;last_pet      Allocated to registers r4 r5 r6 r7 
                                    291 ;last_worker   Allocated with name '_main_last_worker_10000_8'
                                    292 ;kernel_timeout Allocated with name '_main_kernel_timeout_10000_8'
                                    293 ;worker_timeout Allocated with name '_main_worker_timeout_10000_8'
                                    294 ;------------------------------------------------------------
                                    295 ;	watchdog.c:30: void main(void) {
                                    296 ;	-----------------------------------------
                                    297 ;	 function main
                                    298 ;	-----------------------------------------
      0000F2                        299 _main:
                                    300 ;	watchdog.c:31: uint32_t last_pet = kernel_pet_seq;
      0000F2 90 00 30         [24]  301 	mov	dptr,#_kernel_pet_seq
      0000F5 E0               [24]  302 	movx	a,@dptr
      0000F6 FC               [12]  303 	mov	r4,a
      0000F7 A3               [24]  304 	inc	dptr
      0000F8 E0               [24]  305 	movx	a,@dptr
      0000F9 FD               [12]  306 	mov	r5,a
      0000FA A3               [24]  307 	inc	dptr
      0000FB E0               [24]  308 	movx	a,@dptr
      0000FC FE               [12]  309 	mov	r6,a
      0000FD A3               [24]  310 	inc	dptr
      0000FE E0               [24]  311 	movx	a,@dptr
      0000FF FF               [12]  312 	mov	r7,a
                                    313 ;	watchdog.c:32: uint32_t last_worker = worker_heartbeat;
      000100 90 00 24         [24]  314 	mov	dptr,#_worker_heartbeat
      000103 E0               [24]  315 	movx	a,@dptr
      000104 F5 08            [12]  316 	mov	_main_last_worker_10000_8,a
      000106 A3               [24]  317 	inc	dptr
      000107 E0               [24]  318 	movx	a,@dptr
      000108 F5 09            [12]  319 	mov	(_main_last_worker_10000_8 + 1),a
      00010A A3               [24]  320 	inc	dptr
      00010B E0               [24]  321 	movx	a,@dptr
      00010C F5 0A            [12]  322 	mov	(_main_last_worker_10000_8 + 2),a
      00010E A3               [24]  323 	inc	dptr
      00010F E0               [24]  324 	movx	a,@dptr
      000110 F5 0B            [12]  325 	mov	(_main_last_worker_10000_8 + 3),a
                                    326 ;	watchdog.c:33: uint16_t kernel_timeout = 0;
      000112 E4               [12]  327 	clr	a
      000113 F5 0C            [12]  328 	mov	_main_kernel_timeout_10000_8,a
      000115 F5 0D            [12]  329 	mov	(_main_kernel_timeout_10000_8 + 1),a
                                    330 ;	watchdog.c:34: uint16_t worker_timeout = 0;
      000117 F5 0E            [12]  331 	mov	_main_worker_timeout_10000_8,a
      000119 F5 0F            [12]  332 	mov	(_main_worker_timeout_10000_8 + 1),a
                                    333 ;	watchdog.c:36: watchdog_last_kernel_seq = last_pet;
      00011B 90 00 34         [24]  334 	mov	dptr,#_watchdog_last_kernel_seq
      00011E EC               [12]  335 	mov	a,r4
      00011F F0               [24]  336 	movx	@dptr,a
      000120 ED               [12]  337 	mov	a,r5
      000121 A3               [24]  338 	inc	dptr
      000122 F0               [24]  339 	movx	@dptr,a
      000123 EE               [12]  340 	mov	a,r6
      000124 A3               [24]  341 	inc	dptr
      000125 F0               [24]  342 	movx	@dptr,a
      000126 EF               [12]  343 	mov	a,r7
      000127 A3               [24]  344 	inc	dptr
      000128 F0               [24]  345 	movx	@dptr,a
                                    346 ;	watchdog.c:37: watchdog_last_worker_seq = last_worker;
      000129 90 00 38         [24]  347 	mov	dptr,#_watchdog_last_worker_seq
      00012C E5 08            [12]  348 	mov	a,_main_last_worker_10000_8
      00012E F0               [24]  349 	movx	@dptr,a
      00012F E5 09            [12]  350 	mov	a,(_main_last_worker_10000_8 + 1)
      000131 A3               [24]  351 	inc	dptr
      000132 F0               [24]  352 	movx	@dptr,a
      000133 E5 0A            [12]  353 	mov	a,(_main_last_worker_10000_8 + 2)
      000135 A3               [24]  354 	inc	dptr
      000136 F0               [24]  355 	movx	@dptr,a
      000137 E5 0B            [12]  356 	mov	a,(_main_last_worker_10000_8 + 3)
      000139 A3               [24]  357 	inc	dptr
      00013A F0               [24]  358 	movx	@dptr,a
                                    359 ;	watchdog.c:38: watchdog_pet_count = 0;
      00013B 90 00 3C         [24]  360 	mov	dptr,#_watchdog_pet_count
      00013E E4               [12]  361 	clr	a
      00013F F0               [24]  362 	movx	@dptr,a
      000140 A3               [24]  363 	inc	dptr
      000141 F0               [24]  364 	movx	@dptr,a
      000142 A3               [24]  365 	inc	dptr
      000143 F0               [24]  366 	movx	@dptr,a
      000144 A3               [24]  367 	inc	dptr
      000145 F0               [24]  368 	movx	@dptr,a
                                    369 ;	watchdog.c:40: while (1) {
      000146                        370 00113$:
                                    371 ;	watchdog.c:41: if (kernel_pet_seq != last_pet) {
      000146 90 00 30         [24]  372 	mov	dptr,#_kernel_pet_seq
      000149 E0               [24]  373 	movx	a,@dptr
      00014A F8               [12]  374 	mov	r0,a
      00014B A3               [24]  375 	inc	dptr
      00014C E0               [24]  376 	movx	a,@dptr
      00014D F9               [12]  377 	mov	r1,a
      00014E A3               [24]  378 	inc	dptr
      00014F E0               [24]  379 	movx	a,@dptr
      000150 FA               [12]  380 	mov	r2,a
      000151 A3               [24]  381 	inc	dptr
      000152 E0               [24]  382 	movx	a,@dptr
      000153 FB               [12]  383 	mov	r3,a
      000154 E8               [12]  384 	mov	a,r0
      000155 B5 04 0E         [24]  385 	cjne	a,ar4,00153$
      000158 E9               [12]  386 	mov	a,r1
      000159 B5 05 0A         [24]  387 	cjne	a,ar5,00153$
      00015C EA               [12]  388 	mov	a,r2
      00015D B5 06 06         [24]  389 	cjne	a,ar6,00153$
      000160 EB               [12]  390 	mov	a,r3
      000161 B5 07 02         [24]  391 	cjne	a,ar7,00153$
      000164 80 36            [24]  392 	sjmp	00104$
      000166                        393 00153$:
                                    394 ;	watchdog.c:42: last_pet = kernel_pet_seq;
      000166 90 00 30         [24]  395 	mov	dptr,#_kernel_pet_seq
      000169 E0               [24]  396 	movx	a,@dptr
      00016A FC               [12]  397 	mov	r4,a
      00016B A3               [24]  398 	inc	dptr
      00016C E0               [24]  399 	movx	a,@dptr
      00016D FD               [12]  400 	mov	r5,a
      00016E A3               [24]  401 	inc	dptr
      00016F E0               [24]  402 	movx	a,@dptr
      000170 FE               [12]  403 	mov	r6,a
      000171 A3               [24]  404 	inc	dptr
      000172 E0               [24]  405 	movx	a,@dptr
      000173 FF               [12]  406 	mov	r7,a
                                    407 ;	watchdog.c:43: watchdog_last_kernel_seq = last_pet;
      000174 90 00 34         [24]  408 	mov	dptr,#_watchdog_last_kernel_seq
      000177 EC               [12]  409 	mov	a,r4
      000178 F0               [24]  410 	movx	@dptr,a
      000179 ED               [12]  411 	mov	a,r5
      00017A A3               [24]  412 	inc	dptr
      00017B F0               [24]  413 	movx	@dptr,a
      00017C EE               [12]  414 	mov	a,r6
      00017D A3               [24]  415 	inc	dptr
      00017E F0               [24]  416 	movx	@dptr,a
      00017F EF               [12]  417 	mov	a,r7
      000180 A3               [24]  418 	inc	dptr
      000181 F0               [24]  419 	movx	@dptr,a
                                    420 ;	watchdog.c:44: kernel_timeout = 0;
      000182 E4               [12]  421 	clr	a
      000183 F5 0C            [12]  422 	mov	_main_kernel_timeout_10000_8,a
      000185 F5 0D            [12]  423 	mov	(_main_kernel_timeout_10000_8 + 1),a
                                    424 ;	watchdog.c:45: pet_hw();
      000187 C0 07            [24]  425 	push	ar7
      000189 C0 06            [24]  426 	push	ar6
      00018B C0 05            [24]  427 	push	ar5
      00018D C0 04            [24]  428 	push	ar4
      00018F 12 00 A8         [24]  429 	lcall	_pet_hw
      000192 D0 04            [24]  430 	pop	ar4
      000194 D0 05            [24]  431 	pop	ar5
      000196 D0 06            [24]  432 	pop	ar6
      000198 D0 07            [24]  433 	pop	ar7
      00019A 80 30            [24]  434 	sjmp	00105$
      00019C                        435 00104$:
                                    436 ;	watchdog.c:46: } else if (++kernel_timeout > 60000u) {
      00019C 05 0C            [12]  437 	inc	_main_kernel_timeout_10000_8
      00019E E4               [12]  438 	clr	a
      00019F B5 0C 02         [24]  439 	cjne	a,_main_kernel_timeout_10000_8,00154$
      0001A2 05 0D            [12]  440 	inc	(_main_kernel_timeout_10000_8 + 1)
      0001A4                        441 00154$:
      0001A4 AA 0C            [24]  442 	mov	r2,_main_kernel_timeout_10000_8
      0001A6 AB 0D            [24]  443 	mov	r3,(_main_kernel_timeout_10000_8 + 1)
      0001A8 C3               [12]  444 	clr	c
      0001A9 74 60            [12]  445 	mov	a,#0x60
      0001AB 9A               [12]  446 	subb	a,r2
      0001AC 74 EA            [12]  447 	mov	a,#0xea
      0001AE 9B               [12]  448 	subb	a,r3
      0001AF 50 1B            [24]  449 	jnc	00105$
                                    450 ;	watchdog.c:47: do_reset(0x80510001UL);
      0001B1 90 00 01         [24]  451 	mov	dptr,#0x0001
      0001B4 75 F0 51         [24]  452 	mov	b, #0x51
      0001B7 74 80            [12]  453 	mov	a, #0x80
      0001B9 C0 07            [24]  454 	push	ar7
      0001BB C0 06            [24]  455 	push	ar6
      0001BD C0 05            [24]  456 	push	ar5
      0001BF C0 04            [24]  457 	push	ar4
      0001C1 12 00 CA         [24]  458 	lcall	_do_reset
      0001C4 D0 04            [24]  459 	pop	ar4
      0001C6 D0 05            [24]  460 	pop	ar5
      0001C8 D0 06            [24]  461 	pop	ar6
      0001CA D0 07            [24]  462 	pop	ar7
      0001CC                        463 00105$:
                                    464 ;	watchdog.c:50: if (worker_heartbeat != last_worker) {
      0001CC 90 00 24         [24]  465 	mov	dptr,#_worker_heartbeat
      0001CF E0               [24]  466 	movx	a,@dptr
      0001D0 F8               [12]  467 	mov	r0,a
      0001D1 A3               [24]  468 	inc	dptr
      0001D2 E0               [24]  469 	movx	a,@dptr
      0001D3 F9               [12]  470 	mov	r1,a
      0001D4 A3               [24]  471 	inc	dptr
      0001D5 E0               [24]  472 	movx	a,@dptr
      0001D6 FA               [12]  473 	mov	r2,a
      0001D7 A3               [24]  474 	inc	dptr
      0001D8 E0               [24]  475 	movx	a,@dptr
      0001D9 FB               [12]  476 	mov	r3,a
      0001DA E8               [12]  477 	mov	a,r0
      0001DB B5 08 0E         [24]  478 	cjne	a,_main_last_worker_10000_8,00156$
      0001DE E9               [12]  479 	mov	a,r1
      0001DF B5 09 0A         [24]  480 	cjne	a,(_main_last_worker_10000_8 + 1),00156$
      0001E2 EA               [12]  481 	mov	a,r2
      0001E3 B5 0A 06         [24]  482 	cjne	a,(_main_last_worker_10000_8 + 2),00156$
      0001E6 EB               [12]  483 	mov	a,r3
      0001E7 B5 0B 02         [24]  484 	cjne	a,(_main_last_worker_10000_8 + 3),00156$
      0001EA 80 2C            [24]  485 	sjmp	00110$
      0001EC                        486 00156$:
                                    487 ;	watchdog.c:51: last_worker = worker_heartbeat;
      0001EC 90 00 24         [24]  488 	mov	dptr,#_worker_heartbeat
      0001EF E0               [24]  489 	movx	a,@dptr
      0001F0 F5 08            [12]  490 	mov	_main_last_worker_10000_8,a
      0001F2 A3               [24]  491 	inc	dptr
      0001F3 E0               [24]  492 	movx	a,@dptr
      0001F4 F5 09            [12]  493 	mov	(_main_last_worker_10000_8 + 1),a
      0001F6 A3               [24]  494 	inc	dptr
      0001F7 E0               [24]  495 	movx	a,@dptr
      0001F8 F5 0A            [12]  496 	mov	(_main_last_worker_10000_8 + 2),a
      0001FA A3               [24]  497 	inc	dptr
      0001FB E0               [24]  498 	movx	a,@dptr
      0001FC F5 0B            [12]  499 	mov	(_main_last_worker_10000_8 + 3),a
                                    500 ;	watchdog.c:52: watchdog_last_worker_seq = last_worker;
      0001FE 90 00 38         [24]  501 	mov	dptr,#_watchdog_last_worker_seq
      000201 E5 08            [12]  502 	mov	a,_main_last_worker_10000_8
      000203 F0               [24]  503 	movx	@dptr,a
      000204 E5 09            [12]  504 	mov	a,(_main_last_worker_10000_8 + 1)
      000206 A3               [24]  505 	inc	dptr
      000207 F0               [24]  506 	movx	@dptr,a
      000208 E5 0A            [12]  507 	mov	a,(_main_last_worker_10000_8 + 2)
      00020A A3               [24]  508 	inc	dptr
      00020B F0               [24]  509 	movx	@dptr,a
      00020C E5 0B            [12]  510 	mov	a,(_main_last_worker_10000_8 + 3)
      00020E A3               [24]  511 	inc	dptr
      00020F F0               [24]  512 	movx	@dptr,a
                                    513 ;	watchdog.c:53: worker_timeout = 0;
      000210 E4               [12]  514 	clr	a
      000211 F5 0E            [12]  515 	mov	_main_worker_timeout_10000_8,a
      000213 F5 0F            [12]  516 	mov	(_main_worker_timeout_10000_8 + 1),a
      000215 02 01 46         [24]  517 	ljmp	00113$
      000218                        518 00110$:
                                    519 ;	watchdog.c:54: } else if (worker_state == 3u && ++worker_timeout > 60000u) {
      000218 90 00 28         [24]  520 	mov	dptr,#_worker_state
      00021B E0               [24]  521 	movx	a,@dptr
      00021C F8               [12]  522 	mov	r0,a
      00021D A3               [24]  523 	inc	dptr
      00021E E0               [24]  524 	movx	a,@dptr
      00021F F9               [12]  525 	mov	r1,a
      000220 A3               [24]  526 	inc	dptr
      000221 E0               [24]  527 	movx	a,@dptr
      000222 FA               [12]  528 	mov	r2,a
      000223 A3               [24]  529 	inc	dptr
      000224 E0               [24]  530 	movx	a,@dptr
      000225 FB               [12]  531 	mov	r3,a
      000226 B8 03 0B         [24]  532 	cjne	r0,#0x03,00157$
      000229 B9 00 08         [24]  533 	cjne	r1,#0x00,00157$
      00022C BA 00 05         [24]  534 	cjne	r2,#0x00,00157$
      00022F BB 00 02         [24]  535 	cjne	r3,#0x00,00157$
      000232 80 03            [24]  536 	sjmp	00158$
      000234                        537 00157$:
      000234 02 01 46         [24]  538 	ljmp	00113$
      000237                        539 00158$:
      000237 05 0E            [12]  540 	inc	_main_worker_timeout_10000_8
      000239 E4               [12]  541 	clr	a
      00023A B5 0E 02         [24]  542 	cjne	a,_main_worker_timeout_10000_8,00159$
      00023D 05 0F            [12]  543 	inc	(_main_worker_timeout_10000_8 + 1)
      00023F                        544 00159$:
      00023F AA 0E            [24]  545 	mov	r2,_main_worker_timeout_10000_8
      000241 AB 0F            [24]  546 	mov	r3,(_main_worker_timeout_10000_8 + 1)
      000243 C3               [12]  547 	clr	c
      000244 74 60            [12]  548 	mov	a,#0x60
      000246 9A               [12]  549 	subb	a,r2
      000247 74 EA            [12]  550 	mov	a,#0xea
      000249 9B               [12]  551 	subb	a,r3
      00024A 40 03            [24]  552 	jc	00160$
      00024C 02 01 46         [24]  553 	ljmp	00113$
      00024F                        554 00160$:
                                    555 ;	watchdog.c:55: do_reset(0x80510002UL);
      00024F 90 00 02         [24]  556 	mov	dptr,#0x0002
      000252 75 F0 51         [24]  557 	mov	b, #0x51
      000255 74 80            [12]  558 	mov	a, #0x80
      000257 C0 07            [24]  559 	push	ar7
      000259 C0 06            [24]  560 	push	ar6
      00025B C0 05            [24]  561 	push	ar5
      00025D C0 04            [24]  562 	push	ar4
      00025F 12 00 CA         [24]  563 	lcall	_do_reset
      000262 D0 04            [24]  564 	pop	ar4
      000264 D0 05            [24]  565 	pop	ar5
      000266 D0 06            [24]  566 	pop	ar6
      000268 D0 07            [24]  567 	pop	ar7
                                    568 ;	watchdog.c:58: }
      00026A 02 01 46         [24]  569 	ljmp	00113$
                                    570 	.area CSEG    (CODE)
                                    571 	.area CONST   (CODE)
                                    572 	.area XINIT   (CODE)
                                    573 	.area CABS    (ABS,CODE)
