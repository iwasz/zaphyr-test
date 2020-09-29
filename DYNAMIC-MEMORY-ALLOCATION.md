# Dynamic memory allocation in C++ and C++ support in general
To be researched.

## TODO to check : 
- Why [they write, that](https://docs.zephyrproject.org/latest/reference/kernel/other/cxx_support.html) dynamic allocation is not supported? I think this is outdated. Check.
- Zephyr with C++. Apparently there ale plenty of Kconfig options which affect C++ usage.
  - ZEPHYR_CPLUSPLUS
  - MINIMAL_LIBC
  - LIB_CPLUSPLUS
- What dynamic memory allocation algorithms are used (in malloc a free since new and delete use them).
- If those algorithms are dumb and lead to memory fragmentation, how can we replace them (with say umm_malloc)?
- What are the differences between various libc implementations in this regard? [Here we can read](https://github.com/zephyrproject-rtos/zephyr/issues/18990#issuecomment-553539514) : *minimal libc currently does it with a sys_mem_pool(), newlib has its own logic which derives from a memory region implemented in lib/libc/newlib/libc-hooks.c*. [Here](https://github.com/zephyrproject-rtos/zephyr/pull/20678) they moved from using k_malloc to malloc.
- How much memory can be allocated?
- Compare Zephyr to FreeRTOS in this regard.
- Do replacing week symbols malloc, calloc, free etc. is indeed so simple as I anticipated? My doubts came [from this page](https://tracker.mender.io/browse/MEN-1856) where they write : *FreeRTOS dynamic memory allocation : Probably yes, but we need to cross compile stdlib with malloc function defined in FreeRTOS.*. So why could I simply redefine malloc and free. This is what I did in the past with FreeRTOS:

```c
#include <FreeRTOS.h>
#include <errno.h>
#include <stdio.h>

/**
 * Compatibility with libc heap allocator
 */
extern int errno;

caddr_t _sbrk (int incr)
{
        errno = ENOMEM;
        return (caddr_t)-1;
}

void *malloc (size_t size) { return pvPortMalloc (size); }

void *calloc (size_t num, size_t size)
{
        (void)num;
        (void)size;
        return NULL;
}

void *realloc (void *ptr, size_t size)
{
        (void)ptr;
        (void)size;
        return NULL;
}

void free (void *ptr) { vPortFree (ptr); }
```

- How to use etl and gsl and other C++ libraries (I assume this is a no-brainer, you simply compile them in).
