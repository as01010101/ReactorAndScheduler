
#ifndef MY_UTIL
#define MY_UTIL

//#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <unistd.h>
#include <sys/syscall.h>

static int  GetCurrssThreadId() {    ///notice here static 
	return syscall(SYS_gettid);
}

#endif