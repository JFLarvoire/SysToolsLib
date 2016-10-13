#ifndef GNU_EFI_X86_SETJMP_H
#define GNU_EFI_X86_SETJMP_H

#define JMPBUF_ALIGN 4

#ifdef _MSDOS

typedef struct {
	UINT16	bx;
	UINT16	si;
	UINT16	di;
	UINT16	bp;
	UINT16	sp;
	UINT16	ip;
} jmp_buf;

#endif

#ifdef _WIN32

typedef struct {
	UINT32	Ebx;
	UINT32	Esi;
	UINT32	Edi;
	UINT32	Ebp;
	UINT32	Esp;
	UINT32	Eip;
} ALIGN(JMPBUF_ALIGN) jmp_buf;

#endif

#endif /* GNU_EFI_X86_SETJMP_H */
