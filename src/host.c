#include "host.h"
#include <stdarg.h>

typedef union param_t{
	int pdInt;
	void *pdPtr;
	char *pdChrPtr;
} param;

typedef int hostfunc(va_list);

typedef struct {
        enum HOST_SYSCALL action;
	hostfunc *fptr;
} hostcmdlist;

#define MKHCL(a, n) {.action=a, .fptr=host_ ## n}

const hostcmdlist hcl[23]={
    [SYS_OPEN] = MKHCL(SYS_OPEN, open),
    [SYS_CLOSE] = MKHCL(SYS_CLOSE, close),
    [SYS_WRITE] = MKHCL(SYS_WRITE, write),
    [SYS_SYSTEM] = MKHCL(SYS_SYSTEM, system),
    [SYS_READ] = MKHCL(SYS_READ, read),
    [SYS_FLEN]= MKHCL(SYS_FLEN, flen),
};

/*action will be in r0, and argv in r1*/
int host_call(enum HOST_SYSCALL action, void *argv)
{
    /* For Thumb-2 code use the BKPT instruction instead of SWI.
* Refer to:
* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0471c/Bgbjhiea.html
* http://en.wikipedia.org/wiki/ARM_Cortex-M#Cortex-M4 */
    int result;
    __asm__( \
      "bkpt 0xAB\n"\
      "nop\n" \
      "bx lr\n"\
        :"=r" (result) ::\
    );
    return result;
}

int host_system(va_list v1){
    char *tmpChrPtr;

    tmpChrPtr = va_arg(v1, char *);
    return host_call(SYS_SYSTEM, (param []){{.pdChrPtr=tmpChrPtr}, {.pdInt=strlen(tmpChrPtr)}});
}

int host_open(va_list v1) {
    char *tmpChrPtr;

    tmpChrPtr = va_arg(v1, char *);
    return host_call(SYS_OPEN, (param []){{.pdChrPtr=tmpChrPtr}, {.pdInt=va_arg(v1, int)}, {.pdInt=strlen(tmpChrPtr)}});
}

int host_close(va_list v1) {
    return host_call(SYS_CLOSE, (param []){{.pdInt=va_arg(v1, int)}});
}
int host_flen(va_list v1) {
    return host_call(SYS_FLEN, (param []){{.pdInt=va_arg(v1, int)}});
}

int host_write(va_list v1) {
    return host_call(SYS_WRITE, (param []){{.pdInt=va_arg(v1, int)}, {.pdPtr=va_arg(v1, void *)}, {.pdInt=va_arg(v1, int)}});
}
int host_read(va_list v1) {
    return host_call(SYS_READ, (param []){{.pdInt=va_arg(v1, int)}, {.pdPtr=va_arg(v1, void *)}, {.pdInt=va_arg(v1, int)}});
}

int host_action(enum HOST_SYSCALL action, ...)
{
    int result;

    va_list v1;
    va_start(v1, action);

    result = hcl[action].fptr(v1);

    va_end(v1);

    return result;
}
