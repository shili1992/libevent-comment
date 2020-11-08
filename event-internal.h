/*
 * Copyright (c) 2000-2004 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _EVENT_INTERNAL_H_
#define _EVENT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "min_heap.h"
#include "evsignal.h"

struct eventop {
    const char* name;
    void* (*init)(struct event_base*); // 初始化
    int (*add)(void*, struct event*);  // 注册事件
    int (*del)(void*, struct event*);  // 删除事件
    int (*dispatch)(struct event_base*, void*, struct timeval*); // 事件分发
    void (*dealloc)(struct event_base*, void*); // 注销，释放资源
    /* set if we need to reinitialize the event base */
    int need_reinit;
};

struct event_base {
    // 与操作系统相关的io多路复用模型（比如epoll）, 每种I/O demultiplex机制的实现都必须提供这五个函数接口，来完成自身的初始化、销毁释放；对事件的注册、注销和分发。
    const struct eventop* evsel;
    //io多路复用模型上线文（比如epollop），
    // 调用i/o模型evsel->init返回的变量，之后调用与evsel相关的io模型函数都会将该变量传入
    void* evbase;
    //当前注册的事件event总数
    int event_count;        /* counts number of total events */
    //处于活动队列的事件event总数，这部分事件已经触发即将被回调
    int event_count_active; /* counts number of active events */

    int event_gotterm;      /* Set to terminate loop 正常退出dispatch*/
    int event_break;        /* Set to terminate loop immediately 马上退出dispatch*/

    // activequeues是一个二级指针，前面讲过libevent支持事件优先级，因此你可以把它看作是数组，
    // 其中的元素activequeues[priority]是一个链表，链表的每个节点指向一个优先级为priority的就绪事件event
    //1. active list active队里，事件已经触发等待回调通知
    // - 注册一个2s计时器，2s过后该event会被放到active队列等待回调
    // - 注册一个socket读事件,当socket可读会将socket读事件放到active队列等待回调
    //2. 指针数组的原因是要实现优先级功能
    //3. 越靠前优先级越大
    struct event_list** activequeues;
    // 优先级队列数
    int nactivequeues;

    /* signal handling info */
    struct evsignal_info sig; //信号相关

    //eventqueue，链表，保存了所有的注册事件event的指针
    struct event_list eventqueue; //添加到事件循环中的所有event
    struct timeval event_tv;

    struct min_heap timeheap; //管理定时事件的小根堆, 最小二叉堆用于处理计时器

    struct timeval tv_cache;
};

/* Internal use only: Functions that might be missing from <sys/queue.h> */
#ifndef HAVE_TAILQFOREACH
#define TAILQ_FIRST(head)       ((head)->tqh_first)
#define TAILQ_END(head)         NULL
#define TAILQ_NEXT(elm, field)      ((elm)->field.tqe_next)
#define TAILQ_FOREACH(var, head, field)                 \
    for((var) = TAILQ_FIRST(head);                  \
        (var) != TAILQ_END(head);                   \
        (var) = TAILQ_NEXT(var, field))
#define TAILQ_INSERT_BEFORE(listelm, elm, field) do {           \
    (elm)->field.tqe_prev = (listelm)->field.tqe_prev;      \
    (elm)->field.tqe_next = (listelm);              \
    *(listelm)->field.tqe_prev = (elm);             \
    (listelm)->field.tqe_prev = &(elm)->field.tqe_next;     \
} while (0)
#endif /* TAILQ_FOREACH */

int _evsignal_set_handler(struct event_base* base, int evsignal,
                          void (*fn)(int));
int _evsignal_restore_handler(struct event_base* base, int evsignal);

/* defined in evutil.c */
const char* evutil_getenv(const char* varname);

#ifdef __cplusplus
}
#endif

#endif /* _EVENT_INTERNAL_H_ */
