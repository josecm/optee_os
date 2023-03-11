/* SPDX-License-Identifier: BSL-1.0 */
/* Copyright (c) 2017 Arm Limited. All rights reserved.

  Boost Software License - Version 1.0 - August 17th, 2003

  Permission is hereby granted, free of charge, to any person or organization
  obtaining a copy of the software and accompanying documentation covered by
  this license (the "Software") to use, reproduce, display, distribute,
  execute, and transmit the Software, and to prepare derivative works of the
  Software, and to permit third-parties to whom the Software is furnished to do
  so, all subject to the following:

  The copyright notices in the Software and this entire statement, including
  the above license grant, this restriction and the following disclaimer, must
  be included in all copies of the Software, in whole or in part, and all
  derivative works of the Software, unless such copies or derivative works are
  solely in the form of machine-executable object code generated by a source
  language processor.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
  SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR
  ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.  */


#ifdef __HAVE_LOAD_NO_SPECULATE
#define load_no_speculate(__ptr, __low, __high) 			\
(__extension__ ({							\
  __typeof__ ((__ptr)) __ptr_once = (__ptr);				\
  __builtin_load_no_speculate (__ptr_once, __low, __high,		\
			       0, __ptr_once);				\
}))

#define load_no_speculate_fail(__ptr, __low, __high, __failval) 	\
(__extension__ ({							\
  __typeof__ ((__ptr)) __ptr_once = (__ptr);				\
  __builtin_load_no_speculate (__ptr_once, __low, __high,		\
			       __failval, __ptr_once);			\
}))

#define load_no_speculate_cmp(__ptr, __low, __high, __failval, __cmpptr) \
  (__builtin_load_no_speculate (__ptr, __low, __high, __failval, __cmpptr))

#else

#ifdef __GNUC__
#define __UNUSED __attribute__((unused))
#else
#define __UNUSED
#endif

#ifdef __aarch64__

#define __load_no_speculate1(__ptr, __low, __high, __failval,		\
			     __cmpptr, __w, __sz) 			\
(__extension__ ({							\
  __typeof__ (0 + (*(__ptr)))  __nln_val;				\
  /* This typecasting is required to ensure correct handling of upper   \
     bits of failval, to ensure a clean return from the CSEL below.  */	\
  __typeof__(*(__ptr)) __fv						\
    = (__typeof__(*(__ptr)))(unsigned long long) (__failval);		\
  /* If __high is explicitly NULL, we must not emit the			\
     upper-bound comparison.  We need to cast __high to an		\
     unsigned long long before handing it to __builtin_constant_p to	\
     ensure that clang/llvm correctly detects NULL as a constant if it	\
     is defined as (void*) 0.  */					\
  if (__builtin_constant_p ((unsigned long long)__high)			\
      && __high == ((void *)0))						\
    {									\
      __asm__ volatile (						\
      "cmp\t%[__c], %[__l]\n\t"						\
      "bcc\t.ns%=\n\t"							\
      "ldr" __sz "\t%" __w "[__v], %[__p]\n"				\
      ".ns%=:\n\t"							\
      "csel\t%" __w "[__v], %" __w "[__v], %" __w "[__f], cs\n\t"	\
      "hint\t#0x14 // CSDB"						\
      /* The value we have loaded, or failval if the condition check	\
	 fails.  */							\
      : [__v] "=&r" (__nln_val)						\
      /* The pointer we wish to use for comparisons, and the low and	\
	 high bounds to use in that comparison. Note that this need	\
	 not be the same as the pointer from which we will load.  */	\
      : [__c] "r" (__cmpptr), [__l] "r" (__low),			\
      /* The memory location from which we will load.  */		\
      [__p] "m" (*(__ptr)),						\
      /* The value to return if the condition check fails.  */		\
      [__f] "rZ" (__fv) 						\
      /* We always clobber the condition codes.  */			\
      : "cc");								\
    }									\
  else									\
    {									\
      __asm__ volatile (						\
      "cmp\t%[__c], %[__l]\n\t"						\
      "ccmp\t%[__c], %[__h], 2, cs\n\t"					\
      "bcs\t.ns%=\n\t"							\
      "ldr" __sz "\t%" __w "[__v], %[__p]\n"				\
      ".ns%=:\n\t"							\
      "csel\t%" __w "[__v], %" __w "[__v], %" __w "[__f], cc\n\t"	\
      "hint\t#0x14 // CSDB"						\
      /* The value we have loaded, or failval if the condition check	\
	 fails.  */							\
      : [__v] "=&r" (__nln_val)						\
      /* The pointer we wish to use for comparisons, and the low and	\
	 high bounds to use in that comparison. Note that this need	\
	 not be the same as the pointer from which we will load.  */	\
      : [__c] "r" (__cmpptr), [__l] "r" (__low),			\
      [__h] "r" (__high),						\
      /* The memory location from which we will load.  */		\
      [__p] "m" (*(__ptr)),						\
      /* The value to return if the condition check fails.  */		\
      [__f] "rZ" (__fv) 						\
      /* We always clobber the condition codes.  */			\
      : "cc");								\
    }									\
  (__typeof__ (*(__ptr))) __nln_val;					\
}))

#define __load_no_speculate(__ptr, __low, __high, __failval, __cmpptr)	\
(__extension__ ({							\
  __typeof__ (0 + *(__ptr)) __nl_val;					\
									\
  switch (sizeof(*(__ptr))) {						\
    case 1:								\
      __nl_val = __load_no_speculate1 (__ptr, __low, __high,		\
				       __failval, __cmpptr, "w", "b");	\
      break;								\
    case 2:								\
      __nl_val = __load_no_speculate1 (__ptr, __low, __high,		\
				       __failval, __cmpptr, "w", "h");	\
      break;								\
    case 4:								\
      __nl_val = __load_no_speculate1 (__ptr, __low, __high,		\
				       __failval, __cmpptr, "w", "");	\
      break;								\
    case 8:								\
      __nl_val = __load_no_speculate1 (__ptr, __low, __high,		\
				       __failval, __cmpptr, "x", "");	\
      break;								\
    default:								\
      {									\
        char __static_assert_no_speculate_load_size_too_big 		\
		[sizeof (__nl_val) > 8 ? -1 : 1] __UNUSED;		\
        break;								\
      }									\
 }									\
									\
  (__typeof__ (*(__ptr))) __nl_val;					\
}))

#define load_no_speculate(__ptr, __low, __high) 			\
(__extension__ ({							\
  __typeof__ ((__ptr)) __ptr_once = (__ptr);				\
  __load_no_speculate (__ptr_once, __low, __high, 0, __ptr_once);	\
}))

#define load_no_speculate_fail(__ptr, __low, __high, __failval) 	\
(__extension__ ({							\
  __typeof__ ((__ptr)) __ptr_once = (__ptr);				\
  __load_no_speculate (__ptr_once, __low, __high,			\
		       __failval, __ptr_once);				\
}))

#define load_no_speculate_cmp(__ptr, __low, __high, __failval, __cmpptr) \
  (__load_no_speculate (__ptr, __low, __high, __failval, __cmpptr))

/* AArch32 support for ARM and Thumb-2.  Thumb-1 is not supported.  */
#elif defined (__ARM_32BIT_STATE) && (defined (__thumb2__) || !defined (__thumb__))
#ifdef __thumb2__
/* Thumb2 case.  */

#define __load_no_speculate1(__ptr, __low, __high, __failval,		\
			     __cmpptr, __sz) 				\
(__extension__ ({							\
  __typeof__ (0 + *(__ptr))  __nln_val;					\
  __typeof__(*(__ptr)) __fv						\
    = (__typeof__(*(__ptr)))(unsigned long) (__failval);		\
  /* If __high is explicitly NULL, we must not emit the			\
     upper-bound comparison.  We need to cast __high to an		\
     unsigned long before handing it to __builtin_constant_p to		\
     ensure that clang/llvm correctly detects NULL as a constant if it	\
     is defined as (void*) 0.  */					\
  if (__builtin_constant_p ((unsigned long)__high)			\
      && __high == ((void *)0))						\
    {									\
      __asm__ volatile (						\
      ".syntax unified\n\t"						\
      "cmp\t%[__c], %[__l]\n\t"						\
      "bcc\t.ns%=\n\t"							\
      "ldr" __sz "\t%[__v], %[__p]\n"					\
      ".ns%=:\n\t"							\
      "it\tcc\n\t"							\
      "movcc\t%[__v], %[__f]\n\t"					\
      ".inst.n 0xf3af\t@ CSDB\n\t"					\
      ".inst.n 0x8014\t@ CSDB"						\
      /* The value we have loaded, or failval if the condition check	\
	 fails.  */							\
      : [__v] "=&l" (__nln_val)						\
      /* The pointer we wish to use for comparisons, and the low and	\
	 high bounds to use in that comparison. Note that this need	\
	 not be the same as the pointer from which we will load.  */	\
      : [__c] "r" (__cmpptr), [__l] "r" (__low), 			\
      /* The memory location from which we will load.  */		\
      [__p] "m" (*(__ptr)),						\
      /* The value to return if the condition check fails.  */		\
      [__f] "r" (__fv) 							\
      /* We always clobber the condition codes.  */			\
      : "cc");								\
    }									\
  else									\
    {									\
      __asm__ volatile (						\
      ".syntax unified\n\t"						\
      "cmp\t%[__c], %[__l]\n\t"						\
      "it\tcs\n\t"							\
      "cmpcs\t%[__h], %[__c]\n\t"					\
      "bls\t.ns%=\n\t"							\
      "ldr" __sz "\t%[__v], %[__p]\n"					\
      ".ns%=:\n\t"							\
      "it\tls\n\t"							\
      "movls\t%[__v], %[__f]\n\t"					\
      ".inst.n 0xf3af\t@ CSDB\n\t"					\
      ".inst.n 0x8014\t@ CSDB"					\
      /* The value we have loaded, or failval if the condition check	\
	 fails.  */							\
      : [__v] "=&l" (__nln_val)						\
      /* The pointer we wish to use for comparisons, and the low and	\
	 high bounds to use in that comparison. Note that this need	\
	 not be the same as the pointer from which we will load.  */	\
      : [__c] "r" (__cmpptr), [__l] "r" (__low),			\
      [__h] "r" (__high),						\
      /* The memory location from which we will load.  */		\
      [__p] "m" (*(__ptr)),						\
      /* The value to return if the condition check fails.  */		\
      [__f] "r" (__fv) 							\
      /* We always clobber the condition codes.  */			\
      : "cc");								\
    }									\
  (__typeof__ (*(__ptr))) __nln_val;					\
}))									\
									\
/* Double-word version.  */
#define __load_no_speculate2(__ptr, __low, __high, __failval,		\
			     __cmpptr) 					\
(__extension__ ({							\
  __typeof__ (0 + *(__ptr))  __nln_val;					\
  __typeof__(*(__ptr)) __fv						\
    = (__typeof__(*(__ptr)))(unsigned long) (__failval);		\
  /* If __high is explicitly NULL, we must not emit the			\
     upper-bound comparison.  We need to cast __high to an		\
     unsigned long before handing it to __builtin_constant_p to	\
     ensure that clang/llvm correctly detects NULL as a constant if it	\
     is defined as (void*) 0.  */					\
  if (__builtin_constant_p ((unsigned long)__high)			\
      && __high == ((void *)0))						\
    {									\
      __asm__ volatile (						\
      ".syntax unified\n\t"						\
      "cmp\t%[__c], %[__l]\n\t"						\
      "bcc\t.ns%=\n\t"							\
      "ldr\t%Q[__v], [%[__p]]\n\t"					\
      "ldr\t%R[__v], [%[__p], #4]\n"					\
      ".ns%=:\n\t"							\
      "it\tcc\n\t"							\
      "movcc\t%Q[__v], %Q[__f]\n\t"					\
      "it\tcc\n\t"							\
      "movcc\t%R[__v], %R[__f]\n\t"					\
      ".inst.n 0xf3af\t@ CSDB\n\t"					\
      ".inst.n 0x8014\t@ CSDB"					\
      /* The value we have loaded, or failval if the condition check	\
	 fails.  */							\
      : [__v] "=&l" (__nln_val)						\
      /* The pointer we wish to use for comparisons, and the low and	\
	 high bounds to use in that comparison. Note that this need	\
	 not be the same as the pointer from which we will load.  */	\
      : [__c] "r" (__cmpptr), [__l] "r" (__low), 			\
      /* The memory location from which we will load.  */		\
      [__p] "r" (__ptr),						\
      /* The value to return if the condition check fails.  */		\
      [__f] "r" (__fv) 							\
      /* We always clobber the condition codes.  */			\
      : "cc");								\
    }									\
  else									\
    {									\
      __asm__ volatile (						\
      ".syntax unified\n\t"						\
      "cmp\t%[__c], %[__l]\n\t"						\
      "it\tcs\n\t"							\
      "cmpcs\t%[__h], %[__c]\n\t"					\
      "bls\t.ns%=\n\t"							\
      "ldr\t%Q[__v], [%[__p]]\n\t"					\
      "ldr\t%R[__v], [%[__p], #4]\n"					\
      ".ns%=:\n\t"							\
      "it\tls\n\t"							\
      "movls\t%Q[__v], %Q[__f]\n\t"					\
      "it\tls\n\t"							\
      "movls\t%R[__v], %R[__f]\n\t"					\
      ".inst.n 0xf3af\t@ CSDB\n\t"					\
      ".inst.n 0x8014\t@ CSDB"						\
      /* The value we have loaded, or failval if the condition check	\
	 fails.  */							\
      : [__v] "=&l" (__nln_val)						\
      /* The pointer we wish to use for comparisons, and the low and	\
	 high bounds to use in that comparison. Note that this need	\
	 not be the same as the pointer from which we will load.  */	\
      : [__c] "r" (__cmpptr), [__l] "r" (__low),			\
      [__h] "r" (__high),						\
      /* The memory location from which we will load.  */		\
      [__p] "r" (__ptr),						\
      /* The value to return if the condition check fails.  */		\
      [__f] "r" (__fv) 							\
      /* We always clobber the condition codes.  */			\
      : "cc");								\
    }									\
  (__typeof__ (*(__ptr))) __nln_val;					\
}))

#else
/* ARM case.  */

#define __load_no_speculate1(__ptr, __low, __high, __failval,		\
			     __cmpptr, __sz) 				\
(__extension__ ({							\
  __typeof__ (0 + *(__ptr))  __nln_val;					\
  __typeof__(*(__ptr)) __fv						\
    = (__typeof__(*(__ptr)))(unsigned long) (__failval);		\
  /* If __high is explicitly NULL, we must not emit the			\
     upper-bound comparison.  We need to cast __high to an		\
     unsigned long before handing it to __builtin_constant_p to		\
     ensure that clang/llvm correctly detects NULL as a constant if it	\
     is defined as (void*) 0.  */					\
  if (__builtin_constant_p ((unsigned long)__high)			\
      && __high == ((void *)0))						\
    {									\
      __asm__ volatile (						\
      ".syntax unified\n\t"						\
      "cmp\t%[__c], %[__l]\n\t"						\
      "ldr" __sz "cs\t%[__v], %[__p]\n\t"				\
      "movcc\t%[__v], %[__f]\n\t"					\
      ".inst 0xe320f014\t@ CSDB"					\
      /* The value we have loaded, or failval if the condition check	\
	 fails.  */							\
      : [__v] "=&r" (__nln_val)						\
      /* The pointer we wish to use for comparisons, and the low and	\
	 high bounds to use in that comparison. Note that this need	\
	 not be the same as the pointer from which we will load.  */	\
      : [__c] "r" (__cmpptr), [__l] "r" (__low),			\
      /* The memory location from which we will load.  */		\
      [__p] "m" (*(__ptr)),						\
      /* The value to return if the condition check fails.  */		\
      [__f] "rKI" (__fv) 						\
      /* We always clobber the condition codes.  */			\
      : "cc");								\
    }									\
  else									\
    {									\
      __asm__ volatile (						\
      ".syntax unified\n\t"						\
      "cmp\t%[__c], %[__l]\n\t"						\
      "cmpcs\t%[__h], %[__c]\n\t"					\
      "ldr" __sz "hi\t%[__v], %[__p]\n\t"				\
      "movls\t%[__v], %[__f]\n\t"					\
      ".inst 0xe320f014\t@ CSDB"					\
      /* The value we have loaded, or failval if the condition check	\
	 fails.  */							\
      : [__v] "=&r" (__nln_val)						\
      /* The pointer we wish to use for comparisons, and the low and	\
	 high bounds to use in that comparison. Note that this need	\
	 not be the same as the pointer from which we will load.  */	\
      : [__c] "r" (__cmpptr), [__l] "r" (__low),			\
      [__h] "r" (__high),						\
      /* The memory location from which we will load.  */		\
      [__p] "m" (*(__ptr)),						\
      /* The value to return if the condition check fails.  */		\
      [__f] "rKI" (__fv) 						\
      /* We always clobber the condition codes.  */			\
      : "cc");								\
    }									\
  (__typeof__ (*(__ptr))) __nln_val;					\
}))

/* Double-word version.  */
#define __load_no_speculate2(__ptr, __low, __high, __failval,		\
			     __cmpptr) 					\
(__extension__ ({							\
  __typeof__ (0 + *(__ptr))  __nln_val;					\
  __typeof__(*(__ptr)) __fv						\
    = (__typeof__(*(__ptr)))(unsigned long) (__failval);		\
  /* If __high is explicitly NULL, we must not emit the			\
     upper-bound comparison.  We need to cast __high to an		\
     unsigned long before handing it to __builtin_constant_p to		\
     ensure that clang/llvm correctly detects NULL as a constant if it	\
     is defined as (void*) 0.  */					\
  if (__builtin_constant_p ((unsigned long)__high)			\
      && __high == ((void *)0))						\
    {									\
      __asm__ volatile (						\
      ".syntax unified\n\t"						\
      "cmp\t%[__c], %[__l]\n\t"						\
      "ldrcs\t%Q[__v], [%[__p]]\n\t"					\
      "ldrcs\t%R[__v], [%[__p], #4]\n\t"				\
      "movcc\t%Q[__v], %Q[__f]\n\t"					\
      "movcc\t%R[__v], %R[__f]\n\t"					\
      ".inst 0xe320f014\t@ CSDB"					\
      /* The value we have loaded, or failval if the condition check	\
	 fails.  */							\
      : [__v] "=&r" (__nln_val)						\
      /* The pointer we wish to use for comparisons, and the low and	\
	 high bounds to use in that comparison. Note that this need	\
	 not be the same as the pointer from which we will load.  */	\
      : [__c] "r" (__cmpptr), [__l] "r" (__low),			\
      /* The memory location from which we will load.  */		\
      [__p] "r" (__ptr),						\
      /* The value to return if the condition check fails.  */		\
      [__f] "r" (__fv) 							\
      /* We always clobber the condition codes.  */			\
      : "cc");								\
    }									\
  else									\
    {									\
      __asm__ volatile (						\
      ".syntax unified\n\t"						\
      "cmp\t%[__c], %[__l]\n\t"						\
      "cmpcs\t%[__h], %[__c]\n\t"					\
      "ldrhi\t%Q[__v], [%[__p]]\n\t"					\
      "ldrhi\t%R[__v], [%[__p], #4]\n\t"				\
      "movls\t%Q[__v], %Q[__f]\n\t"					\
      "movls\t%R[__v], %R[__f]\n\t"					\
      ".inst 0xe320f014\t@ CSDB"					\
      /* The value we have loaded, or failval if the condition check	\
	 fails.  */							\
      : [__v] "=&r" (__nln_val)						\
      /* The pointer we wish to use for comparisons, and the low and	\
	 high bounds to use in that comparison. Note that this need	\
	 not be the same as the pointer from which we will load.  */	\
      : [__c] "r" (__cmpptr), [__l] "r" (__low),			\
      [__h] "r" (__high),						\
      /* The memory location from which we will load.  */		\
      [__p] "r" (__ptr),						\
      /* The value to return if the condition check fails.  */		\
      [__f] "r" (__fv) 							\
      /* We always clobber the condition codes.  */			\
      : "cc");								\
    }									\
  (__typeof__ (*(__ptr))) __nln_val;					\
}))

#endif // __thumb2__

/* Common to ARM and Thumb2.  */

#define __load_no_speculate(__ptr, __low, __high, __failval, __cmpptr)	\
(__extension__ ({							\
  __typeof__ (0 + *(__ptr)) __nl_val;					\
									\
  switch (sizeof(*(__ptr))) {						\
    case 1:								\
      __nl_val = __load_no_speculate1 (__ptr, __low, __high,		\
				       __failval, __cmpptr, "b");	\
      break;								\
    case 2:								\
      __nl_val = __load_no_speculate1 (__ptr, __low, __high,		\
				       __failval, __cmpptr, "h");	\
      break;								\
    case 4:								\
      __nl_val = __load_no_speculate1 (__ptr, __low, __high,		\
				       __failval, __cmpptr, "");	\
      break;								\
    case 8:								\
      __nl_val = __load_no_speculate2 (__ptr, __low, __high,		\
				       __failval, __cmpptr);		\
      break;								\
    default:								\
      {									\
        char __static_assert_no_speculate_load_size_too_big 		\
		[sizeof (__nl_val) > 8 ? -1 : 1] __UNUSED;		\
        break;								\
      }									\
 }									\
									\
  (__typeof__ (*(__ptr))) __nl_val;					\
}))

#define load_no_speculate(__ptr, __low, __high) 			\
(__extension__ ({							\
  __typeof__ ((__ptr)) __ptr_once = (__ptr);				\
  __load_no_speculate (__ptr_once, __low, __high, 0, __ptr_once);	\
}))

#define load_no_speculate_fail(__ptr, __low, __high, __failval) 	\
(__extension__ ({							\
  __typeof__ ((__ptr)) __ptr_once = (__ptr);				\
  __load_no_speculate (__ptr_once, __low, __high,			\
		       __failval, __ptr_once);				\
}))

#define load_no_speculate_cmp(__ptr, __low, __high, __failval, __cmpptr) \
  (__load_no_speculate (__ptr, __low, __high, __failval, __cmpptr))

#else
#warning "No load_no_speculate available. Using plain load."
#define load_no_speculate_fail(__ptr, __low, __high, __failval) (*(__ptr))
#define load_no_speculate(__ptr, __low, __high) (*(__ptr))
#define load_no_speculate_cmp(__ptr, __low, __high, __failval, __cmpptr) (*(__ptr))
#endif

#endif
