/*
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Copyright (c) 1996-1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */
// talon: SGI STL 3.2's stl_threads.h, its Win32 spin lock (the node
// allocator's, _M_acquire_lock at 0x44a790: InterlockedExchange, the
// spin of 30 or 1000 rounds, then Sleep).

#ifndef __SGI_STL_INTERNAL_THREADS_H
#define __SGI_STL_INTERNAL_THREADS_H

#ifndef __STL_CONFIG_H
#include "stl_config.h"
#endif

#if defined(__STL_WIN32THREADS)
#  include <windows.h>
#endif

__STL_BEGIN_NAMESPACE

#if defined(__STL_WIN32THREADS)
inline unsigned long _Atomic_swap(unsigned long * __p, unsigned long __q) {
    return (unsigned long) InterlockedExchange((LPLONG)__p, (LONG)__q);
}
#endif

template <int __inst>
struct _STL_mutex_spin {
  enum { __low_max = 30, __high_max = 1000 };
  // Low if we suspect uniprocessor, high for multiprocessor.

  static unsigned __max;
  static unsigned __last;
};

template <int __inst>
unsigned _STL_mutex_spin<__inst>::__max = _STL_mutex_spin<__inst>::__low_max;

template <int __inst>
unsigned _STL_mutex_spin<__inst>::__last = 0;

struct _STL_mutex_lock
{
#if defined(__STL_WIN32THREADS)
  // It should be relatively easy to get this to work on any modern Unix.
  volatile unsigned long _M_lock;
  void _M_initialize() { _M_lock = 0; }
  static void _S_nsec_sleep(int __log_nsec) {
      if (__log_nsec <= 20) {
          Sleep(0);
      } else {
          Sleep(1 << (__log_nsec - 20));
      }
  }
  void _M_acquire_lock() {
    volatile unsigned long* __lock = &this->_M_lock;

    if (!_Atomic_swap((unsigned long*)__lock, 1)) {
      return;
    }
    unsigned __my_spin_max = _STL_mutex_spin<0>::__max;
    unsigned __my_last_spins = _STL_mutex_spin<0>::__last;
    volatile unsigned __junk = 17;      // Value doesn't matter.
    unsigned __i;
    for (__i = 0; __i < __my_spin_max; __i++) {
      if (__i < __my_last_spins/2 || *__lock) {
        __junk *= __junk; __junk *= __junk;
        __junk *= __junk; __junk *= __junk;
        continue;
      }
      if (!_Atomic_swap((unsigned long*)__lock, 1)) {
        // got it!
        // Spinning worked.  Thus we're probably not being scheduled
        // against the other process with which we were contending.
        // Thus it makes sense to spin longer the next time.
        _STL_mutex_spin<0>::__last = __i;
        _STL_mutex_spin<0>::__max = _STL_mutex_spin<0>::__high_max;
        return;
      }
    }
    // We are probably being scheduled against the other process.  Sleep.
    _STL_mutex_spin<0>::__max = _STL_mutex_spin<0>::__low_max;
    for (__i = 0 ;; ++__i) {
      int __log_nsec = __i + 6;

      if (__log_nsec > 27) __log_nsec = 27;
      if (!_Atomic_swap((unsigned long *)__lock, 1)) {
        return;
      }
      _S_nsec_sleep(__log_nsec);
    }
  }
  void _M_release_lock() {
    volatile unsigned long* __lock = &_M_lock;
    *__lock = 0;
    // This is not sufficient on many multiprocessors, since
    // writes to protected variables and the lock may be reordered.
  }
#else
  void _M_initialize()   {}
  void _M_acquire_lock() {}
  void _M_release_lock() {}
#endif
};

#ifdef __STL_WIN32THREADS
#  define __STL_MUTEX_INITIALIZER = { 0 }
#else
#  define __STL_MUTEX_INITIALIZER
#endif

__STL_END_NAMESPACE

#endif /* __SGI_STL_INTERNAL_THREADS_H */
