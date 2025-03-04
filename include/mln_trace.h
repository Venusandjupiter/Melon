
/*
 * Copyright (C) Niklaus F.Schen.
 */
#ifndef __MLN_TRACE_H
#define __MLN_TRACE_H

#include "mln_lang.h"

typedef int (*mln_trace_init_cb_t)(mln_lang_ctx_t *ctx);

#define mln_trace(fmt, ...) \
    ({\
        int ret = 0;\
        mln_lang_ctx_t *__trace_ctx = mln_trace_task_get();\
        if (__trace_ctx != NULL) ret = mln_lang_ctx_pipe_send(__trace_ctx, fmt, ## __VA_ARGS__);\
        ret;\
    })

extern mln_string_t *mln_trace_path(void);
extern int mln_trace_init(mln_event_t *ev, mln_string_t *path);
extern mln_lang_ctx_t *mln_trace_task_get(void);
extern void mln_trace_finalize(void);
extern void mln_trace_init_callback_set(mln_trace_init_cb_t cb);

#endif

