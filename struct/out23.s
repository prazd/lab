
a.out:	file format mach-o 64-bit x86-64

Disassembly of section __TEXT,__text:

00000001000031f4 <__ZNSt11char_traitsIcE6lengthEPKc>:
1000031f4: 55                          	push	rbp
1000031f5: 48 89 e5                    	mov	rbp, rsp
1000031f8: 48 83 ec 10                 	sub	rsp, 16
1000031fc: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
100003200: b8 00 00 00 00              	mov	eax, 0
100003205: 84 c0                       	test	al, al
100003207: 74 0e                       	je	0x100003217 <__ZNSt11char_traitsIcE6lengthEPKc+0x23>
100003209: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000320d: 48 89 c7                    	mov	rdi, rax
100003210: e8 43 07 00 00              	call	0x100003958 <_strlen+0x100003958>
100003215: eb 0d                       	jmp	0x100003224 <__ZNSt11char_traitsIcE6lengthEPKc+0x30>
100003217: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000321b: 48 89 c7                    	mov	rdi, rax
10000321e: e8 a7 07 00 00              	call	0x1000039ca <_strlen+0x1000039ca>
100003223: 90                          	nop
100003224: c9                          	leave
100003225: c3                          	ret

0000000100003226 <__ZStanSt13_Ios_FmtflagsS_>:
100003226: 55                          	push	rbp
100003227: 48 89 e5                    	mov	rbp, rsp
10000322a: 89 7d fc                    	mov	dword ptr [rbp - 4], edi
10000322d: 89 75 f8                    	mov	dword ptr [rbp - 8], esi
100003230: 8b 45 fc                    	mov	eax, dword ptr [rbp - 4]
100003233: 23 45 f8                    	and	eax, dword ptr [rbp - 8]
100003236: 5d                          	pop	rbp
100003237: c3                          	ret

0000000100003238 <__ZStorSt13_Ios_FmtflagsS_>:
100003238: 55                          	push	rbp
100003239: 48 89 e5                    	mov	rbp, rsp
10000323c: 89 7d fc                    	mov	dword ptr [rbp - 4], edi
10000323f: 89 75 f8                    	mov	dword ptr [rbp - 8], esi
100003242: 8b 45 fc                    	mov	eax, dword ptr [rbp - 4]
100003245: 0b 45 f8                    	or	eax, dword ptr [rbp - 8]
100003248: 5d                          	pop	rbp
100003249: c3                          	ret

000000010000324a <__ZStcoSt13_Ios_Fmtflags>:
10000324a: 55                          	push	rbp
10000324b: 48 89 e5                    	mov	rbp, rsp
10000324e: 89 7d fc                    	mov	dword ptr [rbp - 4], edi
100003251: 8b 45 fc                    	mov	eax, dword ptr [rbp - 4]
100003254: f7 d0                       	not	eax
100003256: 5d                          	pop	rbp
100003257: c3                          	ret

0000000100003258 <__ZStoRRSt13_Ios_FmtflagsS_>:
100003258: 55                          	push	rbp
100003259: 48 89 e5                    	mov	rbp, rsp
10000325c: 48 83 ec 10                 	sub	rsp, 16
100003260: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
100003264: 89 75 f4                    	mov	dword ptr [rbp - 12], esi
100003267: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000326b: 8b 00                       	mov	eax, dword ptr [rax]
10000326d: 8b 55 f4                    	mov	edx, dword ptr [rbp - 12]
100003270: 89 d6                       	mov	esi, edx
100003272: 89 c7                       	mov	edi, eax
100003274: e8 45 07 00 00              	call	0x1000039be <_strlen+0x1000039be>
100003279: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
10000327d: 89 02                       	mov	dword ptr [rdx], eax
10000327f: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003283: c9                          	leave
100003284: c3                          	ret

0000000100003285 <__ZStaNRSt13_Ios_FmtflagsS_>:
100003285: 55                          	push	rbp
100003286: 48 89 e5                    	mov	rbp, rsp
100003289: 48 83 ec 10                 	sub	rsp, 16
10000328d: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
100003291: 89 75 f4                    	mov	dword ptr [rbp - 12], esi
100003294: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003298: 8b 00                       	mov	eax, dword ptr [rax]
10000329a: 8b 55 f4                    	mov	edx, dword ptr [rbp - 12]
10000329d: 89 d6                       	mov	esi, edx
10000329f: 89 c7                       	mov	edi, eax
1000032a1: e8 f4 06 00 00              	call	0x10000399a <_strlen+0x10000399a>
1000032a6: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
1000032aa: 89 02                       	mov	dword ptr [rdx], eax
1000032ac: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
1000032b0: c9                          	leave
1000032b1: c3                          	ret

00000001000032b2 <__ZNSt8ios_base4setfESt13_Ios_FmtflagsS0_>:
1000032b2: 55                          	push	rbp
1000032b3: 48 89 e5                    	mov	rbp, rsp
1000032b6: 48 83 ec 20                 	sub	rsp, 32
1000032ba: 48 89 7d e8                 	mov	qword ptr [rbp - 24], rdi
1000032be: 89 75 e4                    	mov	dword ptr [rbp - 28], esi
1000032c1: 89 55 e0                    	mov	dword ptr [rbp - 32], edx
1000032c4: 48 8b 45 e8                 	mov	rax, qword ptr [rbp - 24]
1000032c8: 8b 40 18                    	mov	eax, dword ptr [rax + 24]
1000032cb: 89 45 fc                    	mov	dword ptr [rbp - 4], eax
1000032ce: 8b 45 e0                    	mov	eax, dword ptr [rbp - 32]
1000032d1: 89 c7                       	mov	edi, eax
1000032d3: e8 c8 06 00 00              	call	0x1000039a0 <_strlen+0x1000039a0>
1000032d8: 89 c2                       	mov	edx, eax
1000032da: 48 8b 45 e8                 	mov	rax, qword ptr [rbp - 24]
1000032de: 48 83 c0 18                 	add	rax, 24
1000032e2: 89 d6                       	mov	esi, edx
1000032e4: 48 89 c7                    	mov	rdi, rax
1000032e7: e8 a8 06 00 00              	call	0x100003994 <_strlen+0x100003994>
1000032ec: 8b 55 e0                    	mov	edx, dword ptr [rbp - 32]
1000032ef: 8b 45 e4                    	mov	eax, dword ptr [rbp - 28]
1000032f2: 89 d6                       	mov	esi, edx
1000032f4: 89 c7                       	mov	edi, eax
1000032f6: e8 9f 06 00 00              	call	0x10000399a <_strlen+0x10000399a>
1000032fb: 89 c2                       	mov	edx, eax
1000032fd: 48 8b 45 e8                 	mov	rax, qword ptr [rbp - 24]
100003301: 48 83 c0 18                 	add	rax, 24
100003305: 89 d6                       	mov	esi, edx
100003307: 48 89 c7                    	mov	rdi, rax
10000330a: e8 a9 06 00 00              	call	0x1000039b8 <_strlen+0x1000039b8>
10000330f: 8b 45 fc                    	mov	eax, dword ptr [rbp - 4]
100003312: c9                          	leave
100003313: c3                          	ret

0000000100003314 <__ZSt3decRSt8ios_base>:
100003314: 55                          	push	rbp
100003315: 48 89 e5                    	mov	rbp, rsp
100003318: 48 83 ec 10                 	sub	rsp, 16
10000331c: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
100003320: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003324: ba 4a 00 00 00              	mov	edx, 74
100003329: be 02 00 00 00              	mov	esi, 2
10000332e: 48 89 c7                    	mov	rdi, rax
100003331: e8 52 06 00 00              	call	0x100003988 <_strlen+0x100003988>
100003336: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000333a: c9                          	leave
10000333b: c3                          	ret

000000010000333c <__ZSt3hexRSt8ios_base>:
10000333c: 55                          	push	rbp
10000333d: 48 89 e5                    	mov	rbp, rsp
100003340: 48 83 ec 10                 	sub	rsp, 16
100003344: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
100003348: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000334c: ba 4a 00 00 00              	mov	edx, 74
100003351: be 08 00 00 00              	mov	esi, 8
100003356: 48 89 c7                    	mov	rdi, rax
100003359: e8 2a 06 00 00              	call	0x100003988 <_strlen+0x100003988>
10000335e: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003362: c9                          	leave
100003363: c3                          	ret

0000000100003364 <__ZNKSt17basic_string_viewIcSt11char_traitsIcEE4sizeEv>:
100003364: 55                          	push	rbp
100003365: 48 89 e5                    	mov	rbp, rsp
100003368: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
10000336c: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003370: 48 8b 00                    	mov	rax, qword ptr [rax]
100003373: 5d                          	pop	rbp
100003374: c3                          	ret
100003375: 90                          	nop

0000000100003376 <__ZNSt17basic_string_viewIcSt11char_traitsIcEEC1EPKc>:
100003376: 55                          	push	rbp
100003377: 48 89 e5                    	mov	rbp, rsp
10000337a: 48 83 ec 10                 	sub	rsp, 16
10000337e: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
100003382: 48 89 75 f0                 	mov	qword ptr [rbp - 16], rsi
100003386: 48 8b 45 f0                 	mov	rax, qword ptr [rbp - 16]
10000338a: 48 89 c7                    	mov	rdi, rax
10000338d: e8 e4 05 00 00              	call	0x100003976 <_strlen+0x100003976>
100003392: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
100003396: 48 89 02                    	mov	qword ptr [rdx], rax
100003399: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000339d: 48 8b 55 f0                 	mov	rdx, qword ptr [rbp - 16]
1000033a1: 48 89 50 08                 	mov	qword ptr [rax + 8], rdx
1000033a5: 90                          	nop
1000033a6: c9                          	leave
1000033a7: c3                          	ret

00000001000033a8 <__ZN17RegistersStateX865printEv>:
1000033a8: 55                          	push	rbp
1000033a9: 48 89 e5                    	mov	rbp, rsp
1000033ac: 48 83 ec 40                 	sub	rsp, 64
1000033b0: 48 89 7d c8                 	mov	qword ptr [rbp - 56], rdi
1000033b4: 48 8b 05 cd 0c 00 00        	mov	rax, qword ptr [rip + 3277] ## 0x100004088 <_strlen+0x100004088>
1000033bb: 48 89 c2                    	mov	rdx, rax
1000033be: 48 8b 05 cb 0c 00 00        	mov	rax, qword ptr [rip + 3275] ## 0x100004090 <_strlen+0x100004090>
1000033c5: 48 89 d6                    	mov	rsi, rdx
1000033c8: 48 89 c7                    	mov	rdi, rax
1000033cb: e8 9a 05 00 00              	call	0x10000396a <_strlen+0x10000396a>
1000033d0: c7 45 fc 00 00 00 00        	mov	dword ptr [rbp - 4], 0
1000033d7: 48 8b 45 c8                 	mov	rax, qword ptr [rbp - 56]
1000033db: 48 89 45 e8                 	mov	qword ptr [rbp - 24], rax
1000033df: 48 8b 45 e8                 	mov	rax, qword ptr [rbp - 24]
1000033e3: 48 89 45 f0                 	mov	qword ptr [rbp - 16], rax
1000033e7: 48 8b 45 e8                 	mov	rax, qword ptr [rbp - 24]
1000033eb: 48 05 88 00 00 00           	add	rax, 136
1000033f1: 48 89 45 e0                 	mov	qword ptr [rbp - 32], rax
1000033f5: e9 82 00 00 00              	jmp	0x10000347c <__ZN17RegistersStateX865printEv+0xd4>
1000033fa: 48 8b 45 f0                 	mov	rax, qword ptr [rbp - 16]
1000033fe: 48 8b 00                    	mov	rax, qword ptr [rax]
100003401: 48 89 45 d8                 	mov	qword ptr [rbp - 40], rax
100003405: 48 8b 55 c8                 	mov	rdx, qword ptr [rbp - 56]
100003409: 8b 45 fc                    	mov	eax, dword ptr [rbp - 4]
10000340c: 48 98                       	cdqe
10000340e: 48 83 c0 08                 	add	rax, 8
100003412: 48 c1 e0 04                 	shl	rax, 4
100003416: 48 01 d0                    	add	rax, rdx
100003419: 48 8b 50 08                 	mov	rdx, qword ptr [rax + 8]
10000341d: 48 8b 40 10                 	mov	rax, qword ptr [rax + 16]
100003421: 48 8b 0d 68 0c 00 00        	mov	rcx, qword ptr [rip + 3176] ## 0x100004090 <_strlen+0x100004090>
100003428: 48 89 d6                    	mov	rsi, rdx
10000342b: 48 89 c2                    	mov	rdx, rax
10000342e: 48 89 cf                    	mov	rdi, rcx
100003431: e8 7c 05 00 00              	call	0x1000039b2 <_strlen+0x1000039b2>
100003436: 48 89 c2                    	mov	rdx, rax
100003439: 48 8d 05 98 05 00 00        	lea	rax, [rip + 1432]       ## 0x1000039d8 <_strlen+0x1000039d8>
100003440: 48 89 c6                    	mov	rsi, rax
100003443: 48 89 d7                    	mov	rdi, rdx
100003446: e8 5b 05 00 00              	call	0x1000039a6 <_strlen+0x1000039a6>
10000344b: 48 89 c2                    	mov	rdx, rax
10000344e: 48 8b 4d c8                 	mov	rcx, qword ptr [rbp - 56]
100003452: 8b 45 fc                    	mov	eax, dword ptr [rbp - 4]
100003455: 48 98                       	cdqe
100003457: 48 8b 04 c1                 	mov	rax, qword ptr [rcx + 8*rax]
10000345b: 48 89 c6                    	mov	rsi, rax
10000345e: 48 89 d7                    	mov	rdi, rdx
100003461: e8 0a 05 00 00              	call	0x100003970 <_strlen+0x100003970>
100003466: be 0a 00 00 00              	mov	esi, 10
10000346b: 48 89 c7                    	mov	rdi, rax
10000346e: e8 39 05 00 00              	call	0x1000039ac <_strlen+0x1000039ac>
100003473: 83 45 fc 01                 	add	dword ptr [rbp - 4], 1
100003477: 48 83 45 f0 08              	add	qword ptr [rbp - 16], 8
10000347c: 48 8b 45 f0                 	mov	rax, qword ptr [rbp - 16]
100003480: 48 3b 45 e0                 	cmp	rax, qword ptr [rbp - 32]
100003484: 0f 85 70 ff ff ff           	jne	0x1000033fa <__ZN17RegistersStateX865printEv+0x52>
10000348a: 48 8b 05 ef 0b 00 00        	mov	rax, qword ptr [rip + 3055] ## 0x100004080 <_strlen+0x100004080>
100003491: 48 89 c2                    	mov	rdx, rax
100003494: 48 8b 05 f5 0b 00 00        	mov	rax, qword ptr [rip + 3061] ## 0x100004090 <_strlen+0x100004090>
10000349b: 48 89 d6                    	mov	rsi, rdx
10000349e: 48 89 c7                    	mov	rdi, rax
1000034a1: e8 c4 04 00 00              	call	0x10000396a <_strlen+0x10000396a>
1000034a6: 90                          	nop
1000034a7: c9                          	leave
1000034a8: c3                          	ret
1000034a9: 90                          	nop

00000001000034aa <__ZN17RegistersStateX866updateEv>:
1000034aa: 55                          	push	rbp
1000034ab: 48 89 e5                    	mov	rbp, rsp
1000034ae: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
1000034b2: 48 89 c0                    	mov	rax, rax
1000034b5: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
1000034b9: 48 89 02                    	mov	qword ptr [rdx], rax
1000034bc: 48 89 d8                    	mov	rax, rbx
1000034bf: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
1000034c3: 48 89 42 08                 	mov	qword ptr [rdx + 8], rax
1000034c7: 48 89 c8                    	mov	rax, rcx
1000034ca: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
1000034ce: 48 89 42 10                 	mov	qword ptr [rdx + 16], rax
1000034d2: 48 89 d0                    	mov	rax, rdx
1000034d5: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
1000034d9: 48 89 42 18                 	mov	qword ptr [rdx + 24], rax
1000034dd: 48 89 f8                    	mov	rax, rdi
1000034e0: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
1000034e4: 48 89 42 20                 	mov	qword ptr [rdx + 32], rax
1000034e8: 48 89 f0                    	mov	rax, rsi
1000034eb: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
1000034ef: 48 89 42 28                 	mov	qword ptr [rdx + 40], rax
1000034f3: 48 89 e8                    	mov	rax, rbp
1000034f6: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
1000034fa: 48 89 42 30                 	mov	qword ptr [rdx + 48], rax
1000034fe: 48 89 e0                    	mov	rax, rsp
100003501: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
100003505: 48 89 42 38                 	mov	qword ptr [rdx + 56], rax
100003509: 4c 89 c0                    	mov	rax, r8
10000350c: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
100003510: 48 89 42 40                 	mov	qword ptr [rdx + 64], rax
100003514: 4c 89 c8                    	mov	rax, r9
100003517: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
10000351b: 48 89 42 48                 	mov	qword ptr [rdx + 72], rax
10000351f: 4c 89 d0                    	mov	rax, r10
100003522: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
100003526: 48 89 42 50                 	mov	qword ptr [rdx + 80], rax
10000352a: 4c 89 d8                    	mov	rax, r11
10000352d: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
100003531: 48 89 42 58                 	mov	qword ptr [rdx + 88], rax
100003535: 4c 89 e0                    	mov	rax, r12
100003538: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
10000353c: 48 89 42 60                 	mov	qword ptr [rdx + 96], rax
100003540: 4c 89 e8                    	mov	rax, r13
100003543: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
100003547: 48 89 42 68                 	mov	qword ptr [rdx + 104], rax
10000354b: 4c 89 f0                    	mov	rax, r14
10000354e: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
100003552: 48 89 42 70                 	mov	qword ptr [rdx + 112], rax
100003556: 4c 89 f8                    	mov	rax, r15
100003559: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
10000355d: 48 89 42 78                 	mov	qword ptr [rdx + 120], rax
100003561: 48 8d 05 00 00 00 00        	lea	rax, [rip]              ## 0x100003568 <__ZN17RegistersStateX866updateEv+0xbe>
100003568: 48 8b 55 f8                 	mov	rdx, qword ptr [rbp - 8]
10000356c: 48 89 82 80 00 00 00        	mov	qword ptr [rdx + 128], rax
100003573: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003577: 48 8b 80 80 00 00 00        	mov	rax, qword ptr [rax + 128]
10000357e: 48 8d 50 0e                 	lea	rdx, [rax + 14]
100003582: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003586: 48 89 90 80 00 00 00        	mov	qword ptr [rax + 128], rdx
10000358d: 90                          	nop
10000358e: 5d                          	pop	rbp
10000358f: c3                          	ret

0000000100003590 <__ZN17RegistersStateX86C1Ev>:
100003590: 55                          	push	rbp
100003591: 48 89 e5                    	mov	rbp, rsp
100003594: 48 83 ec 10                 	sub	rsp, 16
100003598: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
10000359c: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
1000035a0: 48 89 c6                    	mov	rsi, rax
1000035a3: b8 00 00 00 00              	mov	eax, 0
1000035a8: ba 11 00 00 00              	mov	edx, 17
1000035ad: 48 89 f7                    	mov	rdi, rsi
1000035b0: 48 89 d1                    	mov	rcx, rdx
1000035b3: f3 48 ab                    	rep		stosq	qword ptr es:[rdi], rax
1000035b6: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
1000035ba: 48 05 88 00 00 00           	add	rax, 136
1000035c0: 48 8d 15 16 04 00 00        	lea	rdx, [rip + 1046]       ## 0x1000039dd <_strlen+0x1000039dd>
1000035c7: 48 89 d6                    	mov	rsi, rdx
1000035ca: 48 89 c7                    	mov	rdi, rax
1000035cd: e8 aa 03 00 00              	call	0x10000397c <_strlen+0x10000397c>
1000035d2: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
1000035d6: 48 05 98 00 00 00           	add	rax, 152
1000035dc: 48 8d 15 fe 03 00 00        	lea	rdx, [rip + 1022]       ## 0x1000039e1 <_strlen+0x1000039e1>
1000035e3: 48 89 d6                    	mov	rsi, rdx
1000035e6: 48 89 c7                    	mov	rdi, rax
1000035e9: e8 8e 03 00 00              	call	0x10000397c <_strlen+0x10000397c>
1000035ee: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
1000035f2: 48 05 a8 00 00 00           	add	rax, 168
1000035f8: 48 8d 15 e6 03 00 00        	lea	rdx, [rip + 998]        ## 0x1000039e5 <_strlen+0x1000039e5>
1000035ff: 48 89 d6                    	mov	rsi, rdx
100003602: 48 89 c7                    	mov	rdi, rax
100003605: e8 72 03 00 00              	call	0x10000397c <_strlen+0x10000397c>
10000360a: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000360e: 48 05 b8 00 00 00           	add	rax, 184
100003614: 48 8d 15 ce 03 00 00        	lea	rdx, [rip + 974]        ## 0x1000039e9 <_strlen+0x1000039e9>
10000361b: 48 89 d6                    	mov	rsi, rdx
10000361e: 48 89 c7                    	mov	rdi, rax
100003621: e8 56 03 00 00              	call	0x10000397c <_strlen+0x10000397c>
100003626: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000362a: 48 05 c8 00 00 00           	add	rax, 200
100003630: 48 8d 15 b6 03 00 00        	lea	rdx, [rip + 950]        ## 0x1000039ed <_strlen+0x1000039ed>
100003637: 48 89 d6                    	mov	rsi, rdx
10000363a: 48 89 c7                    	mov	rdi, rax
10000363d: e8 3a 03 00 00              	call	0x10000397c <_strlen+0x10000397c>
100003642: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003646: 48 05 d8 00 00 00           	add	rax, 216
10000364c: 48 8d 15 9e 03 00 00        	lea	rdx, [rip + 926]        ## 0x1000039f1 <_strlen+0x1000039f1>
100003653: 48 89 d6                    	mov	rsi, rdx
100003656: 48 89 c7                    	mov	rdi, rax
100003659: e8 1e 03 00 00              	call	0x10000397c <_strlen+0x10000397c>
10000365e: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003662: 48 05 e8 00 00 00           	add	rax, 232
100003668: 48 8d 15 86 03 00 00        	lea	rdx, [rip + 902]        ## 0x1000039f5 <_strlen+0x1000039f5>
10000366f: 48 89 d6                    	mov	rsi, rdx
100003672: 48 89 c7                    	mov	rdi, rax
100003675: e8 02 03 00 00              	call	0x10000397c <_strlen+0x10000397c>
10000367a: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000367e: 48 05 f8 00 00 00           	add	rax, 248
100003684: 48 8d 15 6e 03 00 00        	lea	rdx, [rip + 878]        ## 0x1000039f9 <_strlen+0x1000039f9>
10000368b: 48 89 d6                    	mov	rsi, rdx
10000368e: 48 89 c7                    	mov	rdi, rax
100003691: e8 e6 02 00 00              	call	0x10000397c <_strlen+0x10000397c>
100003696: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000369a: 48 05 08 01 00 00           	add	rax, 264
1000036a0: 48 8d 15 56 03 00 00        	lea	rdx, [rip + 854]        ## 0x1000039fd <_strlen+0x1000039fd>
1000036a7: 48 89 d6                    	mov	rsi, rdx
1000036aa: 48 89 c7                    	mov	rdi, rax
1000036ad: e8 ca 02 00 00              	call	0x10000397c <_strlen+0x10000397c>
1000036b2: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
1000036b6: 48 05 18 01 00 00           	add	rax, 280
1000036bc: 48 8d 15 3d 03 00 00        	lea	rdx, [rip + 829]        ## 0x100003a00 <_strlen+0x100003a00>
1000036c3: 48 89 d6                    	mov	rsi, rdx
1000036c6: 48 89 c7                    	mov	rdi, rax
1000036c9: e8 ae 02 00 00              	call	0x10000397c <_strlen+0x10000397c>
1000036ce: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
1000036d2: 48 05 28 01 00 00           	add	rax, 296
1000036d8: 48 8d 15 24 03 00 00        	lea	rdx, [rip + 804]        ## 0x100003a03 <_strlen+0x100003a03>
1000036df: 48 89 d6                    	mov	rsi, rdx
1000036e2: 48 89 c7                    	mov	rdi, rax
1000036e5: e8 92 02 00 00              	call	0x10000397c <_strlen+0x10000397c>
1000036ea: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
1000036ee: 48 05 38 01 00 00           	add	rax, 312
1000036f4: 48 8d 15 0c 03 00 00        	lea	rdx, [rip + 780]        ## 0x100003a07 <_strlen+0x100003a07>
1000036fb: 48 89 d6                    	mov	rsi, rdx
1000036fe: 48 89 c7                    	mov	rdi, rax
100003701: e8 76 02 00 00              	call	0x10000397c <_strlen+0x10000397c>
100003706: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000370a: 48 05 48 01 00 00           	add	rax, 328
100003710: 48 8d 15 f4 02 00 00        	lea	rdx, [rip + 756]        ## 0x100003a0b <_strlen+0x100003a0b>
100003717: 48 89 d6                    	mov	rsi, rdx
10000371a: 48 89 c7                    	mov	rdi, rax
10000371d: e8 5a 02 00 00              	call	0x10000397c <_strlen+0x10000397c>
100003722: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003726: 48 05 58 01 00 00           	add	rax, 344
10000372c: 48 8d 15 dc 02 00 00        	lea	rdx, [rip + 732]        ## 0x100003a0f <_strlen+0x100003a0f>
100003733: 48 89 d6                    	mov	rsi, rdx
100003736: 48 89 c7                    	mov	rdi, rax
100003739: e8 3e 02 00 00              	call	0x10000397c <_strlen+0x10000397c>
10000373e: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003742: 48 05 68 01 00 00           	add	rax, 360
100003748: 48 8d 15 c4 02 00 00        	lea	rdx, [rip + 708]        ## 0x100003a13 <_strlen+0x100003a13>
10000374f: 48 89 d6                    	mov	rsi, rdx
100003752: 48 89 c7                    	mov	rdi, rax
100003755: e8 22 02 00 00              	call	0x10000397c <_strlen+0x10000397c>
10000375a: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000375e: 48 05 78 01 00 00           	add	rax, 376
100003764: 48 8d 15 ac 02 00 00        	lea	rdx, [rip + 684]        ## 0x100003a17 <_strlen+0x100003a17>
10000376b: 48 89 d6                    	mov	rsi, rdx
10000376e: 48 89 c7                    	mov	rdi, rax
100003771: e8 06 02 00 00              	call	0x10000397c <_strlen+0x10000397c>
100003776: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000377a: 48 05 88 01 00 00           	add	rax, 392
100003780: 48 8d 15 94 02 00 00        	lea	rdx, [rip + 660]        ## 0x100003a1b <_strlen+0x100003a1b>
100003787: 48 89 d6                    	mov	rsi, rdx
10000378a: 48 89 c7                    	mov	rdi, rax
10000378d: e8 ea 01 00 00              	call	0x10000397c <_strlen+0x10000397c>
100003792: 90                          	nop
100003793: c9                          	leave
100003794: c3                          	ret

0000000100003795 <__Z3foov>:
100003795: 55                          	push	rbp
100003796: 48 89 e5                    	mov	rbp, rsp
100003799: 48 81 ec a0 01 00 00        	sub	rsp, 416
1000037a0: 48 8d 85 60 fe ff ff        	lea	rax, [rbp - 416]
1000037a7: 48 89 c7                    	mov	rdi, rax
1000037aa: e8 9d 01 00 00              	call	0x10000394c <_strlen+0x10000394c>
1000037af: 48 8d 85 60 fe ff ff        	lea	rax, [rbp - 416]
1000037b6: 48 89 c7                    	mov	rdi, rax
1000037b9: e8 88 01 00 00              	call	0x100003946 <_strlen+0x100003946>
1000037be: 90                          	nop
1000037bf: c9                          	leave
1000037c0: c3                          	ret

00000001000037c1 <__Z3sumIJiiEEDaDpT_>:
1000037c1: 55                          	push	rbp
1000037c2: 48 89 e5                    	mov	rbp, rsp
1000037c5: 89 7d fc                    	mov	dword ptr [rbp - 4], edi
1000037c8: 89 75 f8                    	mov	dword ptr [rbp - 8], esi
1000037cb: 8b 55 fc                    	mov	edx, dword ptr [rbp - 4]
1000037ce: 8b 45 f8                    	mov	eax, dword ptr [rbp - 8]
1000037d1: 01 d0                       	add	eax, edx
1000037d3: 5d                          	pop	rbp
1000037d4: c3                          	ret

00000001000037d5 <_main>:
1000037d5: 55                          	push	rbp
1000037d6: 48 89 e5                    	mov	rbp, rsp
1000037d9: 48 81 ec a0 01 00 00        	sub	rsp, 416
1000037e0: be 14 00 00 00              	mov	esi, 20
1000037e5: bf 0a 00 00 00              	mov	edi, 10
1000037ea: e8 4b 01 00 00              	call	0x10000393a <_strlen+0x10000393a>
1000037ef: 48 8d 85 60 fe ff ff        	lea	rax, [rbp - 416]
1000037f6: 48 89 c7                    	mov	rdi, rax
1000037f9: e8 4e 01 00 00              	call	0x10000394c <_strlen+0x10000394c>
1000037fe: 48 8d 85 60 fe ff ff        	lea	rax, [rbp - 416]
100003805: 48 89 c7                    	mov	rdi, rax
100003808: e8 39 01 00 00              	call	0x100003946 <_strlen+0x100003946>
10000380d: 48 8d 85 60 fe ff ff        	lea	rax, [rbp - 416]
100003814: 48 89 c7                    	mov	rdi, rax
100003817: e8 24 01 00 00              	call	0x100003940 <_strlen+0x100003940>
10000381c: b8 00 00 00 00              	mov	eax, 0
100003821: c9                          	leave
100003822: c3                          	ret
100003823: 90                          	nop

0000000100003824 <__ZN9__gnu_cxx11char_traitsIcE6lengthEPKc>:
100003824: 55                          	push	rbp
100003825: 48 89 e5                    	mov	rbp, rsp
100003828: 48 83 ec 20                 	sub	rsp, 32
10000382c: 48 89 7d e8                 	mov	qword ptr [rbp - 24], rdi
100003830: 48 c7 45 f8 00 00 00 00     	mov	qword ptr [rbp - 8], 0
100003838: eb 05                       	jmp	0x10000383f <__ZN9__gnu_cxx11char_traitsIcE6lengthEPKc+0x1b>
10000383a: 48 83 45 f8 01              	add	qword ptr [rbp - 8], 1
10000383f: c6 45 f7 00                 	mov	byte ptr [rbp - 9], 0
100003843: 48 8b 55 e8                 	mov	rdx, qword ptr [rbp - 24]
100003847: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
10000384b: 48 01 c2                    	add	rdx, rax
10000384e: 48 8d 45 f7                 	lea	rax, [rbp - 9]
100003852: 48 89 c6                    	mov	rsi, rax
100003855: 48 89 d7                    	mov	rdi, rdx
100003858: e8 f5 00 00 00              	call	0x100003952 <_strlen+0x100003952>
10000385d: 83 f0 01                    	xor	eax, 1
100003860: 84 c0                       	test	al, al
100003862: 75 d6                       	jne	0x10000383a <__ZN9__gnu_cxx11char_traitsIcE6lengthEPKc+0x16>
100003864: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003868: c9                          	leave
100003869: c3                          	ret

000000010000386a <__ZNKSt17basic_string_viewIcSt11char_traitsIcEE4dataEv>:
10000386a: 55                          	push	rbp
10000386b: 48 89 e5                    	mov	rbp, rsp
10000386e: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
100003872: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
100003876: 48 8b 40 08                 	mov	rax, qword ptr [rax + 8]
10000387a: 5d                          	pop	rbp
10000387b: c3                          	ret

000000010000387c <__ZStlsIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_St17basic_string_viewIS3_S4_E>:
10000387c: 55                          	push	rbp
10000387d: 48 89 e5                    	mov	rbp, rsp
100003880: 53                          	push	rbx
100003881: 48 83 ec 28                 	sub	rsp, 40
100003885: 48 89 7d e8                 	mov	qword ptr [rbp - 24], rdi
100003889: 48 89 f0                    	mov	rax, rsi
10000388c: 48 89 d1                    	mov	rcx, rdx
10000388f: 48 89 c0                    	mov	rax, rax
100003892: ba 00 00 00 00              	mov	edx, 0
100003897: 48 89 ca                    	mov	rdx, rcx
10000389a: 48 89 45 d0                 	mov	qword ptr [rbp - 48], rax
10000389e: 48 89 55 d8                 	mov	qword ptr [rbp - 40], rdx
1000038a2: 48 8d 45 d0                 	lea	rax, [rbp - 48]
1000038a6: 48 89 c7                    	mov	rdi, rax
1000038a9: e8 b6 00 00 00              	call	0x100003964 <_strlen+0x100003964>
1000038ae: 48 89 c3                    	mov	rbx, rax
1000038b1: 48 8d 45 d0                 	lea	rax, [rbp - 48]
1000038b5: 48 89 c7                    	mov	rdi, rax
1000038b8: e8 a1 00 00 00              	call	0x10000395e <_strlen+0x10000395e>
1000038bd: 48 89 c1                    	mov	rcx, rax
1000038c0: 48 8b 45 e8                 	mov	rax, qword ptr [rbp - 24]
1000038c4: 48 89 da                    	mov	rdx, rbx
1000038c7: 48 89 ce                    	mov	rsi, rcx
1000038ca: 48 89 c7                    	mov	rdi, rax
1000038cd: e8 bc 00 00 00              	call	0x10000398e <_strlen+0x10000398e>
1000038d2: 48 8b 5d f8                 	mov	rbx, qword ptr [rbp - 8]
1000038d6: c9                          	leave
1000038d7: c3                          	ret

00000001000038d8 <__ZN9__gnu_cxx11char_traitsIcE2eqERKcS3_>:
1000038d8: 55                          	push	rbp
1000038d9: 48 89 e5                    	mov	rbp, rsp
1000038dc: 48 89 7d f8                 	mov	qword ptr [rbp - 8], rdi
1000038e0: 48 89 75 f0                 	mov	qword ptr [rbp - 16], rsi
1000038e4: 48 8b 45 f8                 	mov	rax, qword ptr [rbp - 8]
1000038e8: 0f b6 10                    	movzx	edx, byte ptr [rax]
1000038eb: 48 8b 45 f0                 	mov	rax, qword ptr [rbp - 16]
1000038ef: 0f b6 00                    	movzx	eax, byte ptr [rax]
1000038f2: 38 c2                       	cmp	dl, al
1000038f4: 0f 94 c0                    	sete	al
1000038f7: 5d                          	pop	rbp
1000038f8: c3                          	ret

00000001000038f9 <__Z41__static_initialization_and_destruction_0v>:
1000038f9: 55                          	push	rbp
1000038fa: 48 89 e5                    	mov	rbp, rsp
1000038fd: 48 8d 05 fc 46 00 00        	lea	rax, [rip + 18172]      ## 0x100008000 <__ZStL8__ioinit>
100003904: 48 89 c7                    	mov	rdi, rax
100003907: e8 76 00 00 00              	call	0x100003982 <_strlen+0x100003982>
10000390c: 48 8d 15 ed c6 ff ff        	lea	rdx, [rip - 14611]      ## 0x100000000 <_strlen+0x100000000>
100003913: 48 8d 05 e6 46 00 00        	lea	rax, [rip + 18150]      ## 0x100008000 <__ZStL8__ioinit>
10000391a: 48 8b 0d 47 07 00 00        	mov	rcx, qword ptr [rip + 1863] ## 0x100004068 <_strlen+0x100004068>
100003921: 48 89 c6                    	mov	rsi, rax
100003924: 48 89 cf                    	mov	rdi, rcx
100003927: e8 98 00 00 00              	call	0x1000039c4 <_strlen+0x1000039c4>
10000392c: 90                          	nop
10000392d: 5d                          	pop	rbp
10000392e: c3                          	ret

000000010000392f <__GLOBAL__sub_I_registers_state_x86.cpp>:
10000392f: 55                          	push	rbp
100003930: 48 89 e5                    	mov	rbp, rsp
100003933: e8 c1 ff ff ff              	call	0x1000038f9 <__Z41__static_initialization_and_destruction_0v>
100003938: 5d                          	pop	rbp
100003939: c3                          	ret

Disassembly of section __TEXT,__stubs:

000000010000393a <__stubs>:
10000393a: ff 25 c0 06 00 00           	jmp	qword ptr [rip + 1728]  ## 0x100004000 <_strlen+0x100004000>
100003940: ff 25 c2 06 00 00           	jmp	qword ptr [rip + 1730]  ## 0x100004008 <_strlen+0x100004008>
100003946: ff 25 c4 06 00 00           	jmp	qword ptr [rip + 1732]  ## 0x100004010 <_strlen+0x100004010>
10000394c: ff 25 c6 06 00 00           	jmp	qword ptr [rip + 1734]  ## 0x100004018 <_strlen+0x100004018>
100003952: ff 25 c8 06 00 00           	jmp	qword ptr [rip + 1736]  ## 0x100004020 <_strlen+0x100004020>
100003958: ff 25 ca 06 00 00           	jmp	qword ptr [rip + 1738]  ## 0x100004028 <_strlen+0x100004028>
10000395e: ff 25 cc 06 00 00           	jmp	qword ptr [rip + 1740]  ## 0x100004030 <_strlen+0x100004030>
100003964: ff 25 ce 06 00 00           	jmp	qword ptr [rip + 1742]  ## 0x100004038 <_strlen+0x100004038>
10000396a: ff 25 d0 06 00 00           	jmp	qword ptr [rip + 1744]  ## 0x100004040 <_strlen+0x100004040>
100003970: ff 25 d2 06 00 00           	jmp	qword ptr [rip + 1746]  ## 0x100004048 <_strlen+0x100004048>
100003976: ff 25 d4 06 00 00           	jmp	qword ptr [rip + 1748]  ## 0x100004050 <_strlen+0x100004050>
10000397c: ff 25 d6 06 00 00           	jmp	qword ptr [rip + 1750]  ## 0x100004058 <_strlen+0x100004058>
100003982: ff 25 d8 06 00 00           	jmp	qword ptr [rip + 1752]  ## 0x100004060 <_strlen+0x100004060>
100003988: ff 25 e2 06 00 00           	jmp	qword ptr [rip + 1762]  ## 0x100004070 <_strlen+0x100004070>
10000398e: ff 25 e4 06 00 00           	jmp	qword ptr [rip + 1764]  ## 0x100004078 <_strlen+0x100004078>
100003994: ff 25 fe 06 00 00           	jmp	qword ptr [rip + 1790]  ## 0x100004098 <_strlen+0x100004098>
10000399a: ff 25 00 07 00 00           	jmp	qword ptr [rip + 1792]  ## 0x1000040a0 <_strlen+0x1000040a0>
1000039a0: ff 25 02 07 00 00           	jmp	qword ptr [rip + 1794]  ## 0x1000040a8 <_strlen+0x1000040a8>
1000039a6: ff 25 04 07 00 00           	jmp	qword ptr [rip + 1796]  ## 0x1000040b0 <_strlen+0x1000040b0>
1000039ac: ff 25 06 07 00 00           	jmp	qword ptr [rip + 1798]  ## 0x1000040b8 <_strlen+0x1000040b8>
1000039b2: ff 25 08 07 00 00           	jmp	qword ptr [rip + 1800]  ## 0x1000040c0 <_strlen+0x1000040c0>
1000039b8: ff 25 0a 07 00 00           	jmp	qword ptr [rip + 1802]  ## 0x1000040c8 <_strlen+0x1000040c8>
1000039be: ff 25 0c 07 00 00           	jmp	qword ptr [rip + 1804]  ## 0x1000040d0 <_strlen+0x1000040d0>
1000039c4: ff 25 0e 07 00 00           	jmp	qword ptr [rip + 1806]  ## 0x1000040d8 <_strlen+0x1000040d8>
1000039ca: ff 25 18 07 00 00           	jmp	qword ptr [rip + 1816]  ## 0x1000040e8 <_strlen+0x1000040e8>
