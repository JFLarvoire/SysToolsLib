#ifndef GNU_EFI_X86_SETJMP_H
#define GNU_EFI_X86_SETJMP_H

#if defined(_MSDOS)

#pragma message("_MSDOS")

typedef struct {
	UINT16	bx;
	UINT16	si;
	UINT16	di;
	UINT16	bp;
	UINT16	sp;
	UINT16	ip;
} jmp_buf;

#elif defined(_M_IX86)

#pragma message("_M_IX86")

typedef struct {
	UINT32	ebx;
	UINT32	esi;
	UINT32	edi;
	UINT32	ebp;
	UINT32	esp;
	UINT32	eip;
} ALIGN(4) jmp_buf;

#elif defined(_M_X64) || defined(_M_AMD64)

#pragma message("_M_X64")

typedef struct {
	UINT64	rbx;
	UINT64	rsp;
	UINT64	rbp;
	UINT64	rdi;
	UINT64	rsi;
	UINT64	r12;
	UINT64	r13;
	UINT64	r14;
	UINT64	r15;
	UINT64	rip;
	UINT64	mxcsr;
	UINT8	XmmBuffer[160]; // XMM6 - XMM15
} ALIGN(8) jmp_buf;

#elif defined(_M_IA64)

typedef struct {
	UINT64	F2[2];
	UINT64	F3[2];
	UINT64	F4[2];
	UINT64	F5[2];
	UINT64	F16[2];
	UINT64	F17[2];
	UINT64	F18[2];
	UINT64	F19[2];
	UINT64	F20[2];
	UINT64	F21[2];
	UINT64	F22[2];
	UINT64	F23[2];
	UINT64	F24[2];
	UINT64	F25[2];
	UINT64	F26[2];
	UINT64	F27[2];
	UINT64	F28[2];
	UINT64	F29[2];
	UINT64	F30[2];
	UINT64	F31[2];
	UINT64	R4;
	UINT64	R5;
	UINT64	R6;
	UINT64	R7;
	UINT64	SP;
	UINT64	BR0;
	UINT64	BR1;
	UINT64	BR2;
	UINT64	BR3;
	UINT64	BR4;
	UINT64	BR5;
	UINT64	InitialUNAT;
	UINT64	AfterSpillUNAT;
	UINT64	PFS;
	UINT64	BSP;
	UINT64	Predicates;
	UINT64	LoopCount;
	UINT64	FPSR;
} ALIGN(16) jmp_buf;

#elif defined(_M_ARM)

#pragma message("_M_ARM")

/* TO DO: The correct EFI jmp_buf definitions for ARM and ARM64 are commented out.
   There seems to be a conflict with another jmp_buf definition in Windows.
   As a short term workaround, using what Windows expects to avoid a redefinition error. */

/* 
typedef struct {
	UINT32 R3;
	UINT32 R4;
	UINT32 R5;
	UINT32 R6;
	UINT32 R7;
	UINT32 R8;
	UINT32 R9;
	UINT32 R10;
	UINT32 R11;
	UINT32 R12;
	UINT32 R13;
	UINT32 R14;
} ALIGN(4) jmp_buf;
*/
typedef int ALIGN(4) jmp_buf[28];

#elif defined(_M_ARM64)

#pragma message("_M_ARM64")

/*
typedef struct {
	UINT64	X19;
	UINT64	X20;
	UINT64	X21;
	UINT64	X22;
	UINT64	X23;
	UINT64	X24;
	UINT64	X25;
	UINT64	X26;
	UINT64	X27;
	UINT64	X28;
	UINT64	FP;
	UINT64	LR;
	UINT64	IP0;
	UINT64	D8;
	UINT64	D9;
	UINT64	D10;
	UINT64	D11;
	UINT64	D12;
	UINT64	D13;
	UINT64	D14;
	UINT64	D15;
} ALIGN(8) jmp_buf;
*/
typedef UINT64 ALIGN(8) jmp_buf[24];

#endif

#endif /* GNU_EFI_X86_SETJMP_H */
