/* **********************************************************
 * Copyright (C) 2002 VMware, Inc.  All Rights Reserved. 
 * **********************************************************
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __COMPAT_SCHED_H__
#   define __COMPAT_SCHED_H__


#include <linux/sched.h>

/* CLONE_KERNEL available in 2.5.35 and higher. */
#ifndef CLONE_KERNEL
#define CLONE_KERNEL CLONE_FILES | CLONE_FS | CLONE_SIGHAND
#endif

/* The capable() API appeared in 2.1.92 --hpreg */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 1, 92)
#   define capable(_capability) suser()
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 0)
#   define need_resched() need_resched
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 3)
#   define need_resched() (current->need_resched)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 3)
#   define cond_resched() (need_resched() ? schedule() : (void) 0)
#endif


/*
 * Since 2.5.34 there are two methods to enumerate tasks:
 * for_each_process(p) { ... } which enumerates only tasks and
 * do_each_thread(g,t) { ... } while_each_thread(g,t) which enumerates
 *     also threads even if they share same pid.
 */
#ifndef for_each_process
#   define for_each_process(p) for_each_task(p)
#endif

#ifndef do_each_thread
#   define do_each_thread(g, t) for_each_task(g) { t = g; do
#   define while_each_thread(g, t) while (0) }
#endif


/*
 * Lock for signal mask is moving target...
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 40) && defined(CLONE_PID)
/* 2.4.x without NPTL patches or early 2.5.x */
#define compat_sigmask_lock sigmask_lock
#define compat_dequeue_signal_current(siginfo_ptr) \
   dequeue_signal(&current->blocked, (siginfo_ptr))
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 60) && !defined(INIT_SIGHAND)
/* RedHat's 2.4.x with first version of NPTL support, or 2.5.40 to 2.5.59 */
#define compat_sigmask_lock sig->siglock
#define compat_dequeue_signal_current(siginfo_ptr) \
   dequeue_signal(&current->blocked, (siginfo_ptr))
#else
/* RedHat's 2.4.x with second version of NPTL support, or 2.5.60+. */
#define compat_sigmask_lock sighand->siglock
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
#define compat_dequeue_signal_current(siginfo_ptr) \
   dequeue_signal(&current->blocked, (siginfo_ptr))
#else
#define compat_dequeue_signal_current(siginfo_ptr) \
   dequeue_signal(current, &current->blocked, (siginfo_ptr))
#endif
#endif

/*
 * recalc_sigpending() had task argument in the past
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 29) && defined(CLONE_PID)
/* 2.4.x without NPTL patches or early 2.5.x */
#define compat_recalc_sigpending() recalc_sigpending(current)
#else
/* RedHat's 2.4.x with NPTL support, or 2.5.29+ */
#define compat_recalc_sigpending() recalc_sigpending()
#endif


/*
 * reparent_to_init() was introduced in 2.4.8.  In 2.5.38 (or possibly
 * earlier, but later than 2.5.31) a call to it was added into
 * daemonize(), so compat_daemonize no longer needs to call it.
 *
 * In 2.4.x kernels reparent_to_init() forgets to do correct refcounting
 * on current->user. It is better to count one too many than one too few...
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 8) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 38)
#define compat_reparent_to_init() do { \
					reparent_to_init(); \
					atomic_inc(&current->user->__count); \
				  } while (0)
#else
#define compat_reparent_to_init() do {} while (0)
#endif


/*
 * daemonize appeared in 2.2.18. Except 2.2.17-4-RH7.0, which has it too.
 * Fortunately 2.2.17-4-RH7.0 uses versioned symbols, so we can check
 * its existence with defined().
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 18)) && !defined(daemonize)
static inline void daemonize(void) {
   struct fs_struct *fs;

   exit_mm(current);
   current->session = 1;
   current->pgrp = 1;
   exit_fs(current);
   fs = init_task.fs;
   current->fs = fs;
   atomic_inc(&fs->count);
}
#endif


/*
 * flush_signals acquires sighand->siglock since 2.5.61... Verify RH's kernels!
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 61)
#define compat_flush_signals(task) do { \
				      spin_lock_irq(&task->compat_sigmask_lock); \
				      flush_signals(task); \
				      spin_unlock_irq(&task->compat_sigmask_lock); \
				   } while (0)
#else
#define compat_flush_signals(task) flush_signals(task)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 61)
#define compat_allow_signal(signr) do { \
                                      spin_lock_irq(&current->compat_sigmask_lock); \
                                      sigdelset(&current->blocked, signr); \
                                      compat_recalc_sigpending(); \
                                      spin_unlock_irq(&current->compat_sigmask_lock); \
                                   } while (0)
#else
#define compat_allow_signal(signr) allow_signal(signr)
#endif

/*
 * daemonize can set process name since 2.5.61. Prior to 2.5.61, daemonize
 * didn't block signals on our behalf.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 61)
#define compat_daemonize(x...)                                                \
({                                                                            \
   /* Beware! No snprintf here, so verify arguments! */                       \
   sprintf(current->comm, x);                                                 \
                                                                              \
   /* Block all signals. */                                                   \
   spin_lock_irq(&current->compat_sigmask_lock);                              \
   sigfillset(&current->blocked);                                             \
   compat_recalc_sigpending();                                                \
   spin_unlock_irq(&current->compat_sigmask_lock);                            \
   compat_flush_signals(current);                                             \
                                                                              \
   daemonize();                                                               \
   compat_reparent_to_init();                                                 \
})
#else
#define compat_daemonize(x...) daemonize(x)
#endif


/*
 * set priority for specified thread. Exists on 2.6.x kernels and some
 * 2.4.x vendor's kernels.
 */
#if defined(VMW_HAVE_SET_USER_NICE)
#define compat_set_user_nice(task, n) set_user_nice((task), (n))
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
#define compat_set_user_nice(task, n) do { (task)->priority = 20 - (n); } while (0)
#elif !defined(VMW_HAVE_SET_USER_NICE)
#define compat_set_user_nice(task, n) do { (task)->nice = (n); } while (0)
#endif

/*
 * try to freeze a process. For kernels 2.6.11 or newer, we know how to choose
 * the interface. The problem is that the oldest interface, introduced in
 * 2.5.18, was backported to 2.4.x kernels. So if we're older than 2.6.11,
 * we'll decide what to do based on whether or not swsusp was configured
 * for the kernel.  For kernels 2.6.20 and newer, we'll also need to include
 * freezer.h since the try_to_freeze definition was pulled out of sched.h.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
#include <linux/freezer.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13) || defined(VMW_TL10S64_WORKAROUND)
#define compat_try_to_freeze() try_to_freeze()
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 11)
#define compat_try_to_freeze() try_to_freeze(PF_FREEZE)
#elif defined(CONFIG_SOFTWARE_SUSPEND) || defined(CONFIG_SOFTWARE_SUSPEND2)
#include "compat_mm.h"
#include <linux/errno.h>
#include <linux/suspend.h>
static inline int compat_try_to_freeze(void)  { 
   if (current->flags & PF_FREEZE) {
      refrigerator(PF_FREEZE); 
      return 1;
   } else {
      return 0;
   }
}
#else
static inline int compat_try_to_freeze(void) { return 0; }
#endif

/*
 * As of 2.6.23-rc1, kernel threads are no longer freezable by
 * default. Instead, kernel threads that need to be frozen must opt-in
 * by calling set_freezable() as soon as the thread is created.
 */

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 22)
#define compat_set_freezable() do { set_freezable(); } while (0)
#else
#define compat_set_freezable() do {} while (0)
#endif

#endif /* __COMPAT_SCHED_H__ */
