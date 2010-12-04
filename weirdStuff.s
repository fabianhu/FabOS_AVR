#include "avr/io.h"

.global clutterRegs
.func clutterRegs
clutterRegs:

	ldi r16,55;
	ldi r17,55;
	ldi r18,55;
	ldi r19,55;
	ldi r19,55;
	ldi r20,55;
	ldi r21,55;
	ldi r22,55;
	ldi r23,55;
	ldi r24,55;
	ldi r25,55;
	ldi r26,55;
	ldi r27,55;
	ldi r28,55;
	ldi r29,55;
	ldi r30,55;
	ldi r31,55;
	mov r0,r16;
	mov r1,r16;
	mov r2,r16;
	mov r3,r16;
	mov r4,r16;
	mov r5,r16;
	mov r6,r16;
	mov r7,r16;
	mov r8,r16;
	mov r9,r16;
	mov r10,r16;
	mov r11,r16;
	mov r12,r16;
	mov r13,r16;
	mov r14,r16;
	mov r15,r16;

	rjmp clutterRegs;


.endfunc
