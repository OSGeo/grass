/*!
   \file lib/gis/percent.c

   \brief GIS Library - Progress reporting and telemetry support for
   GRASS operations

   This file implements the `G_progress_*` API used to report incremental work
   completion for long-running operations. It supports both percentage-based
   and time-based progress contexts, plus compatibility wrappers for the legacy
   global G_percent() and G_progress() entry points.

   The implementation is organized as a telemetry pipeline. Producer-side API
   calls update atomic progress state and enqueue ::event_type_t `EV_PROGRESS`
   or `EV_LOG` records into a bounded ring buffer. A single consumer thread
   drains that buffer, converts raw records into `GProgressEvent` values, and
   forwards them either to installed sink callbacks or to default renderers
   selected from the current G_info_format() mode.

   Concurrency is designed as multi-producer, single-consumer per telemetry
   stream. Producers reserve slots with an atomic `write_index`, publish events
   by setting a per-slot `ready` flag with release semantics, and use atomic
   compare-and-swap to ensure that only one producer emits a given percent
   threshold or time-gated update. The consumer advances a non-atomic
   `read_index`, waits for published slots, processes events in FIFO order, and
   then marks slots free again.

   Two lifecycle models are used. Isolated `GProgressContext` instances create
   a dedicated consumer thread that is joined during destruction. The legacy
   process-wide G_percent() path initializes one shared telemetry instance and
   a detached consumer thread on first use.

   The requirements of the new `G_progress_*` API is support of C11 atomic
   operations and presens of pthreads, which --if are met-- is indicated by
   the definition of G_USE_PROGRESS_NG.

   \copyright
   (C) 2001-2026 by the GRASS Development Team
   \copyright
   SPDX-License-Identifier: GPL-2.0-or-later

   \author GRASS Development Team
   \author Nicklas Larsson (concurrency support)
 */

#include <grass/gis.h>

#if defined(G_USE_PROGRESS_NG)

#include <assert.h>
#include <stdio.h>
#include <sched.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

static void G__percent_ng(long, long, int);
static void G__percent_reset_ng(void);
static void G__progress_ng(long n, int s);
static void G__set_percent_routine_ng(int (*)(int));
static void G__unset_percent_routine_ng(void);

#else

#include <stdio.h>

static struct state {
    int prev;
    int first;
} state = {-1, 1};

static struct state *st = &state;
static int (*ext_percent)(int);

#endif

/*!
   \brief Print percent complete messages.

   This routine prints a percentage complete message to stderr. The
   percentage complete is <i>(<b>n</b>/<b>d</b>)*100</i>, and these are
   printed only for each <b>s</b> percentage. This is perhaps best
   explained by example:
   \code
   #include <stdio.h>
   #include <grass/gis.h>
   int row;
   int nrows;
   nrows = 1352; // 1352 is not a special value - example only

   G_message(_("Percent complete..."));
   for (row = 0; row < nrows; row++)
   {
   G_percent(row, nrows, 10);
   do_calculation(row);
   }
   G_percent(1, 1, 1);
   \endcode

   This example code will print completion messages at 10% increments;
   i.e., 0%, 10%, 20%, 30%, etc., up to 100%. Each message does not appear
   on a new line, but rather erases the previous message.

   Note that to prevent the illusion of the module stalling, the G_percent()
   call is placed before the time consuming part of the for loop, and an
   additional call is generally needed after the loop to "finish it off"
   at 100%.

   \param n current element
   \param d total number of elements
   \param s increment size
 */
void G_percent(long n, long d, int s)
{
#if defined(G_USE_PROGRESS_NG)
    G__percent_ng(n, d, s);
#else
    int x, format;

    format = G_info_format();

    x = (d <= 0 || s <= 0) ? 100 : (int)(100 * n / d);

    /* be verbose only 1> */
    if (format == G_INFO_FORMAT_SILENT || G_verbose() < 1)
        return;

    if (n <= 0 || n >= d || x > st->prev + s) {
        st->prev = x;

        if (ext_percent) {
            ext_percent(x);
        }
        else {
            if (format == G_INFO_FORMAT_STANDARD) {
                fprintf(stderr, "%4d%%\b\b\b\b\b", x);
            }
            else {
                if (format == G_INFO_FORMAT_PLAIN) {
                    if (x == 100)
                        fprintf(stderr, "%d\n", x);
                    else
                        fprintf(stderr, "%d..", x);
                }
                else { /* GUI */
                    if (st->first) {
                        fprintf(stderr, "\n");
                    }
                    fprintf(stderr, "GRASS_INFO_PERCENT: %d\n", x);
                    fflush(stderr);
                    st->first = 0;
                }
            }
        }
    }

    if (x >= 100) {
        if (ext_percent) {
            ext_percent(100);
        }
        else if (format == G_INFO_FORMAT_STANDARD) {
            fprintf(stderr, "\n");
        }
        st->prev = -1;
        st->first = 1;
    }
#endif
}

/*!
   \brief Reset G_percent() to 0%; do not add newline.
 */
void G_percent_reset(void)
{
#if defined(G_USE_PROGRESS_NG)
    G__percent_reset_ng();
#else
    st->prev = -1;
    st->first = 1;
#endif
}

/*!
   \brief Print progress info messages

   Use G_percent() when number of elements is defined.

   This routine prints a progress info message to stderr. The value
   <b>n</b> is printed only for each <b>s</b>. This is perhaps best
   explained by example:
   \code
   #include <grass/vector.h>

   int line;

   G_message(_("Reading features..."));
   line = 0;
   while(TRUE)
   {
   if (Vect_read_next_line(Map, Points, Cats) < 0)
   break;
   line++;
   G_progress(line, 1e3);
   }
   G_progress(1, 1);
   \endcode

   This example code will print progress in messages at 1000
   increments; i.e., 1000, 2000, 3000, 4000, etc., up to number of
   features for given vector map. Each message does not appear on a new
   line, but rather erases the previous message.

   \param n current element
   \param s increment size
 */
void G_progress(long n, int s)
{
#if defined(G_USE_PROGRESS_NG)
    G__progress_ng(n, s);
#else
    int format;

    format = G_info_format();

    /* be verbose only 1> */
    if (format == G_INFO_FORMAT_SILENT || G_verbose() < 1)
        return;

    if (n == s && n == 1) {
        if (format == G_INFO_FORMAT_PLAIN)
            fprintf(stderr, "\n");
        else if (format != G_INFO_FORMAT_GUI)
            fprintf(stderr, "\r");
        return;
    }

    if (n % s == 0) {
        if (format == G_INFO_FORMAT_PLAIN)
            fprintf(stderr, "%ld..", n);
        else if (format == G_INFO_FORMAT_GUI)
            fprintf(stderr, "GRASS_INFO_PROGRESS: %ld\n", n);
        else
            fprintf(stderr, "%10ld\b\b\b\b\b\b\b\b\b\b", n);
    }
#endif
}

/*!
   \brief Establishes percent_routine as the routine that will handle
   the printing of percentage progress messages.

   \param percent_routine routine will be called like this: percent_routine(x)
 */
void G_set_percent_routine(int (*percent_routine)(int))
{
#if defined(G_USE_PROGRESS_NG)
    G__set_percent_routine_ng(percent_routine);
#else
    ext_percent = percent_routine;
#endif
}

/*!
   \brief After this call subsequent percentage progress messages will
   be handled in the default method.

   Percentage progress messages are printed directly to stderr.
 */
void G_unset_percent_routine(void)
{
#if defined(G_USE_PROGRESS_NG)
    G__unset_percent_routine_ng();
#else
    ext_percent = NULL;
#endif
}

#if defined(G_USE_PROGRESS_NG)

#define LOG_CAPACITY       1024
#define LOG_MSG_SIZE       128
#define TIME_RATE_LIMIT_MS 100

typedef enum { EV_LOG, EV_PROGRESS } event_type_t;

typedef struct {
    event_type_t type;
    size_t completed;
    size_t total;
    bool is_terminal;
    char message[LOG_MSG_SIZE];
    atomic_bool ready;
} event_t;

/*!
   \brief Internal telemetry state shared by the progress producer and consumer.

   `telemetry_t` owns the ring buffer of queued log/progress events, the
   counters and thresholds used for time- or percent-based emission, and the
   sink configuration that determines how flushed events are rendered.
*/
typedef struct {
    event_t buffer[LOG_CAPACITY];
    atomic_size_t write_index;
    size_t read_index;
    atomic_size_t completed;
    size_t total;
    int info_format;
    atomic_long last_progress_ns;
    long interval_ns;
    size_t percent_step;
    atomic_size_t next_percent_threshold;
    atomic_bool stop;
    GProgressSink sink; // optional sink
} telemetry_t;

typedef void (*context_progress_fn)(telemetry_t *, size_t);

struct GProgressContext {
    telemetry_t telemetry;
    context_progress_fn report_progress;
    GProgressSink sink; // per-context override (optional)
    atomic_bool initialized;
    pthread_t consumer_thread;
    atomic_bool consumer_started;
};

static telemetry_t g_percent_telemetry;
static atomic_bool g_percent_initialized = false;
static atomic_bool g_percent_consumer_started = false;
static GProgressSink g_percent_sink = {0};
static int (*g_legacy_percent_routine)(int) = NULL;

static GProgressContext *context_create(size_t, size_t, long);
static bool telemetry_has_pending_events(telemetry_t *);
static void telemetry_init_time(telemetry_t *, size_t, long);
static void telemetry_init_percent(telemetry_t *, size_t, size_t);
static void enqueue_event(telemetry_t *, event_t *);
static void telemetry_enqueue_final_progress(telemetry_t *);
static void telemetry_enqueue_final_counter(telemetry_t *);
static void telemetry_log(telemetry_t *, const char *);
static void telemetry_set_info_format(telemetry_t *);
static void telemetry_install_default_sink(telemetry_t *t);
static void telemetry_progress(telemetry_t *, size_t);
static void context_progress_percent(telemetry_t *, size_t);
static void context_progress_time(telemetry_t *, size_t);
static void *telemetry_consumer(void *);
static void start_global_percent(size_t, size_t);
static void set_global_sink(const GProgressSink *);
static bool output_is_silenced(void);
static long now_ns(void);
static void legacy_percent_adapter(const GProgressEvent *, void *);
static void sink_progress_standard(const GProgressEvent *, void *);
static void sink_progress_plain(const GProgressEvent *, void *);
static void sink_progress_gui(const GProgressEvent *, void *);
static void sink_log_default(const char *, void *);

/*!
   \brief Creates an isolated progress-reporting context for concurrent work.

   The returned context tracks progress for `total_num_elements` items and
   emits progress updates whenever completion advances by at least
   `percent_step` percentage points. `total_num_elements` must match the
   actual number of work units that will be reported through
   `G_progress_update()`. In particular, callers should pass a completed-work
   count, not a raw loop index or a larger container size, otherwise the
   terminal `100%` update may never be reached. If output is enabled by the
   current runtime configuration, this function also starts the background
   consumer thread used to flush queued telemetry events.

   \param total_num_elements Total number of elements to process.
   \param step Minimum percentage increment that triggers a
     progress event.
   \return A newly allocated `GPercentContext`, or `NULL` if output
     is silenced by environment variable `GRASS_MESSAGE_FORMAT` or
     verbosity level is below `1`.
 */
GProgressContext *G_progress_context_create(size_t total_num_elements,
                                            size_t step)
{
    return context_create(total_num_elements, step,
                          (step == 0 ? TIME_RATE_LIMIT_MS : 0));
}

/*!
   \brief Creates an isolated progress-reporting context with time-based
   updates.

   Unlike `G_progress_context_create()`, which emits progress events when
   completion crosses percentage thresholds, this variant rate-limits progress
   emission by elapsed time. The returned context tracks
   `total_num_elements` work units and reports updates no more frequently than
   once every `interval_ms` milliseconds while work is in progress.

   Callers should report monotonically increasing completed-work counts through
   `G_progress_update()` and destroy the context with
   `G_progress_context_destroy()` when processing finishes.

   \param total_num_elements Total number of elements to process.
   \param interval_ms Minimum time interval, in milliseconds, between emitted
     progress updates.
   \return A newly allocated `GProgressContext`, or `NULL` if output is
     silenced by the current runtime configuration.
*/
GProgressContext *G_progress_context_create_time(size_t total_num_elements,
                                                 long interval_ms)
{
    return context_create(total_num_elements, 0, interval_ms);
}

/*!
   \brief Creates an isolated progress-reporting context for open-ended
   increments.

   This convenience constructor creates a context intended for
   `G_progress_increment()` when the total number of work units is not known up
   front. The returned context behaves as a simple counter and emits updates
   whenever the completed count advances by at least `increment_step` units.

   \param increment_step Minimum completed-count increment that triggers a
     progress event.

   \return A newly allocated `GProgressContext`, or `NULL` if output is
     silenced by the current runtime configuration.
*/
GProgressContext *G_progress_context_create_increment(size_t increment_step)
{
    assert(increment_step > 0);
    return context_create(0, increment_step, 0);
}

/*!
   \brief Destroys a `GPercentContext` and releases any resources it owns.

   This function stops the context's background telemetry consumer, waits for
   the consumer thread to finish when it was started, marks the context as no
   longer initialized, and frees the context memory. Passing `NULL` is safe and
   has no effect.

   \param ctx The progress-reporting context previously created by
     `G_percent_context_create()`, or `NULL`.
*/
void G_progress_context_destroy(GProgressContext *ctx)
{
    if (!ctx) {
        return;
    }

    if (!atomic_load_explicit(&ctx->initialized, memory_order_acquire)) {
        G_free(ctx);
        return;
    }

    if (ctx->telemetry.total == 0) {
        if (atomic_load_explicit(&ctx->telemetry.completed,
                                 memory_order_acquire) > 0) {
            telemetry_enqueue_final_counter(&ctx->telemetry);
        }
    }
    else if (atomic_load_explicit(&ctx->telemetry.completed,
                                  memory_order_acquire) >=
                 ctx->telemetry.total &&
             atomic_load_explicit(&ctx->telemetry.next_percent_threshold,
                                  memory_order_acquire) <= 100) {
        telemetry_enqueue_final_progress(&ctx->telemetry);
    }

    atomic_store_explicit(&ctx->telemetry.stop, true, memory_order_release);

    if (atomic_exchange_explicit(&ctx->consumer_started, false,
                                 memory_order_acq_rel)) {
        pthread_join(ctx->consumer_thread, NULL);
    }

    atomic_store_explicit(&ctx->initialized, false, memory_order_release);
    G_free(ctx);
}

/*!
   \brief Sets or clears the output sink used by a progress context.

   Installs a per-context `GProgressSink` override for progress and log events
   emitted by `ctx`. When `sink` is non-`NULL`, its callbacks and `user_data`
   are copied into the context and used by the telemetry consumer. Passing
   `NULL` clears any custom sink so the context falls back to its default
   output behavior.

   Example:
   ```c
   GProgressSink sink = {
       .on_progress = my_progress_handler,
       .on_log = my_log_handler,
       .user_data = my_context,
   };
   G_progress_context_set_sink(progress_ctx, &sink);
 ```
 \param ctx The progress context to update. If `NULL`, the function has
   no effect.
 \param sink The sink configuration to copy into the context, or `NULL`
   to remove the custom sink.

*/
void G_progress_context_set_sink(GProgressContext *ctx,
                                 const GProgressSink *sink)
{
    if (!ctx)
        return;
    if (sink) {
        ctx->sink = *sink;
    }
    else {
        ctx->sink.on_progress = NULL;
        ctx->sink.on_log = NULL;
        ctx->sink.user_data = NULL;
    }
    // update telemetry copy; safe because sink is read-only
    // by consumer after set
    ctx->telemetry.sink = ctx->sink;
}

/*!
   \brief Reports progress for an isolated `GProgressContext` instance.

   This re-entrant variant of `G_percent` is intended for concurrent or
   context-specific work. It validates that `ctx` is initialized, clamps
   `current_element` to the valid `0...total` range, and enqueues a progress
   event only when the computed percentage reaches the next configured
   threshold for the context.

   Callers typically create the context with `G_progress_context_create()`,
   call this function as work advances, and later release resources with
   `G_progress_context_destroy()`.

   Example:
   ```c
   size_t n_rows = window.rows;  // total number of rows
   size_t step = 10;  // output step, every 10%
   GProgressContext *ctx = G_progress_context_create(n_rows, step);
   for (row = 0; row < window.rows; row++) {
       // costly calculation ...

       // note: not counting from zero, as for loop never reaches n_rows
       //       and we want to reach 100%
       size_t completed_row = row + 1;

       G_progress_update(ctx, completed_row);
   }
   G_progress_context_destroy(ctx);
   ```

   \param ctx The progress-reporting context created by
     `G_percent_context_create()`.
   \param completed The current completed element index or count.
*/
void G_progress_update(GProgressContext *ctx, size_t completed)
{
    if (!ctx)
        return;
    if (!atomic_load_explicit(&ctx->initialized, memory_order_acquire))
        return;

    telemetry_t *t = &ctx->telemetry;
    if (t->total == 0)
        return;

    size_t total = t->total;
    if (completed > total)
        completed = total;

    atomic_store_explicit(&t->completed, completed, memory_order_release);
    ctx->report_progress(t, completed);
}

/**
   \brief Reports progress for open-ended loops.

   Use this function with contexts created by
   `G_progress_context_create_increment()` when work is tracked as an open-ended
   completed count instead of progress toward a known total. The function
   ignores `NULL` or uninitialized contexts and forwards the completed-count
   update to the context telemetry.

   Example:
   ```c
   GProgressContext *ctx = G_progress_context_create_increment(100);
   while (TRUE) {
       G_progress_increment(ctx, rows_processed);
   }
   ```

   \param ctx The increment-based progress context to update.
   \param completed The current completed count used to determine whether a new
     progress event should be emitted.
*/
void G_progress_increment(GProgressContext *ctx, size_t completed)
{
    if (!ctx)
        return;
    if (!atomic_load_explicit(&ctx->initialized, memory_order_acquire))
        return;

    telemetry_progress(&ctx->telemetry, completed);
}

/**
   \brief Enqueues a log message for a progress reporting context.

   Use this function to attach informational output to an active
   `GProgressContext`. The message is forwarded to the context telemetry and
   emitted by the configured progress sink in the order it is queued. The
   function ignores `NULL` messages and contexts that are `NULL` or not yet
   initialized.

   Example:
   ```c
   GProgressContext *ctx = G_progress_context_create_time(total_rows, 100);
   G_progress_log(ctx, _("Starting import"));
   ```

   \param ctx The progress context that should receive the log message.
   \param message A null-terminated message string to enqueue for output.
*/
void G_progress_log(GProgressContext *ctx, const char *message)
{
    if (!ctx || !message)
        return;
    if (!atomic_load_explicit(&ctx->initialized, memory_order_acquire))
        return;
    telemetry_log(&ctx->telemetry, message);
}

/*!
   \brief Creates and initializes a progress reporting context.

   The created context configures its reporting mode based on `step`. When
   `step` is `0`, progress updates are emitted using a time-based interval
   controlled by `interval_ms`. Otherwise, progress updates are emitted at
   percentage increments defined by `step`.

   \param total_num_elements Total number of elements expected for the
     operation being tracked.
   \param step Percentage increment for reporting progress. A value of `0`
     selects time-based reporting instead.
   \param interval_ms Time interval in milliseconds between progress updates
     when `step` is `0`.
   \return A newly allocated and initialized `GProgressContext`, or `NULL`
     if output is currently silenced.
*/
static GProgressContext *context_create(size_t total_num_elements, size_t step,
                                        long interval_ms)
{
    if (output_is_silenced())
        return NULL;

    GProgressContext *ctx = G_calloc(1, sizeof(*ctx));

    atomic_init(&ctx->initialized, true);

    assert(step == 0 || total_num_elements == 0 || step <= 100);

    if (step == 0) {
        assert(interval_ms > 0);
        telemetry_init_time(&ctx->telemetry, total_num_elements, interval_ms);
        ctx->report_progress = context_progress_time;
    }
    else {
        telemetry_init_percent(&ctx->telemetry, total_num_elements, step);
        ctx->report_progress = context_progress_percent;
    }

    ctx->sink.on_progress = NULL;
    ctx->sink.on_log = NULL;
    ctx->sink.user_data = NULL;

    // propagate context sink to telemetry by default
    ctx->telemetry.sink = ctx->sink;

    atomic_init(&ctx->consumer_started, false);

    bool expected_started = false;
    if (atomic_compare_exchange_strong_explicit(
            &ctx->consumer_started, &expected_started, true,
            memory_order_acq_rel, memory_order_relaxed)) {
        pthread_create(&ctx->consumer_thread, NULL, telemetry_consumer,
                       &ctx->telemetry);
    }

    return ctx;
}

static void context_progress_percent(telemetry_t *t, size_t completed)
{
    size_t total = t->total;

    if (completed == total) {
        telemetry_enqueue_final_progress(t);
        return;
    }

    size_t current_pct = (size_t)((completed * 100) / total);
    size_t expected =
        atomic_load_explicit(&t->next_percent_threshold, memory_order_relaxed);
    while (current_pct >= expected && expected <= 100) {
        size_t next = expected + t->percent_step;
        if (expected < 100 && next > 100)
            next = 100;
        else if (next > 100)
            next = 101;
        if (atomic_compare_exchange_strong_explicit(
                &t->next_percent_threshold, &expected, next,
                memory_order_acq_rel, memory_order_relaxed)) {
            event_t ev = {0};
            ev.type = EV_PROGRESS;
            ev.completed = completed;
            ev.total = total;
            enqueue_event(t, &ev);
            return;
        }
    }
}

static void context_progress_time(telemetry_t *t, size_t completed)
{
    if (completed == t->total) {
        telemetry_enqueue_final_progress(t);
        return;
    }

    long now = now_ns();
    long last =
        atomic_load_explicit(&t->last_progress_ns, memory_order_relaxed);

    if (now - last < t->interval_ns) {
        return;
    }
    if (!atomic_compare_exchange_strong_explicit(&t->last_progress_ns, &last,
                                                 now, memory_order_acq_rel,
                                                 memory_order_relaxed)) {
        return;
    }

    event_t ev = {0};
    ev.type = EV_PROGRESS;
    ev.completed = completed;
    ev.total = t->total;
    enqueue_event(t, &ev);
}

/*!
   \brief Consumes queued telemetry events and emits log or progress output
   until shutdown is requested and the event buffer has been drained.

   \param arg Pointer to the `telemetry_t` instance whose ring buffer and
     formatting settings should be consumed.
   \return `NULL` after the consumer loop exits and any global consumer state
     has been reset.
*/
static void *telemetry_consumer(void *arg)
{
    telemetry_t *t = arg;

    while (true) {
        if (atomic_load_explicit(&t->stop, memory_order_acquire) &&
            !telemetry_has_pending_events(t)) {
            break;
        }

        event_t *ev = &t->buffer[t->read_index % LOG_CAPACITY];

        if (!atomic_load_explicit(&ev->ready, memory_order_acquire)) {
            sched_yield();
            continue;
        }

        // handle event
        if (ev->type == EV_LOG) {
            if (t->sink.on_log) {
                t->sink.on_log(ev->message, t->sink.user_data);
            }
            else {
                sink_log_default(ev->message, NULL);
            }
        }
        else if (ev->type == EV_PROGRESS) {
            double pct = (ev->total > 0)
                             ? (double)ev->completed * 100.0 / (double)ev->total
                             : 0.0;
            bool is_terminal = ev->is_terminal ||
                               (ev->total > 0 && ev->completed >= ev->total);

            if (t->sink.on_progress) {
                GProgressEvent pe = {
                    .completed = ev->completed,
                    .total = ev->total,
                    .percent = pct,
                    .is_terminal = is_terminal,
                };
                t->sink.on_progress(&pe, t->sink.user_data);
            }
            else {
                // Ensure defaults exist (defensive, should already be set at
                // init)
                telemetry_install_default_sink(t);
                if (t->sink.on_progress) {
                    GProgressEvent pe = {
                        .completed = ev->completed,
                        .total = ev->total,
                        .percent = pct,
                        .is_terminal = is_terminal,
                    };
                    t->sink.on_progress(&pe, t->sink.user_data);
                }
            }
        }

        // mark slot free
        atomic_store_explicit(&ev->ready, false, memory_order_release);
        t->read_index++;
    }

    if (t == &g_percent_telemetry) {
        atomic_store_explicit(&g_percent_consumer_started, false,
                              memory_order_release);
        atomic_store_explicit(&g_percent_initialized, false,
                              memory_order_release);
        // keep g_percent_sink as-is; no change needed on shutdown
    }

    return NULL;
}

static void telemetry_init_time(telemetry_t *t, size_t total, long interval_ms)
{
    atomic_init(&t->write_index, 0);
    t->read_index = 0;

    for (size_t i = 0; i < LOG_CAPACITY; ++i) {
        atomic_init(&t->buffer[i].ready, false);
    }

    atomic_init(&t->completed, 0);
    t->total = total;
    telemetry_set_info_format(t);

    atomic_init(&t->last_progress_ns, 0);
    t->interval_ns = interval_ms * 1000000L;

    t->percent_step = 0; // 0 => disabled, use time-based if interval_ns > 0
    atomic_init(&t->next_percent_threshold, 0);

    atomic_init(&t->stop, false);

    // default: no custom sink; callbacks NULL imply fallback to info_format
    t->sink.on_progress = NULL;
    t->sink.on_log = NULL;
    t->sink.user_data = NULL;
    telemetry_install_default_sink(t);
}

/*!
   \brief Initializes telemetry state for percentage-based progress reporting.

   Resets the telemetry ring buffer and counters, disables time-based
   throttling, and configures the next progress event to be emitted when the
   completed work reaches the first `percent_step` threshold.

   \param t The telemetry instance to reset and configure.
   \param total The total number of work units expected for the tracked
     operation.
   \param percent_step The percentage increment that controls when
     progress updates are emitted. A value of `0` disables percentage-based
     thresholds.
*/
static void telemetry_init_percent(telemetry_t *t, size_t total,
                                   size_t percent_step)
{
    atomic_init(&t->write_index, 0);
    t->read_index = 0;
    for (size_t i = 0; i < LOG_CAPACITY; ++i) {
        atomic_init(&t->buffer[i].ready, false);
    }
    atomic_init(&t->completed, 0);
    t->total = total;
    telemetry_set_info_format(t);

    // disable time-based gating
    atomic_init(&t->last_progress_ns, 0);
    t->interval_ns = 0;

    // enable percentage-based gating
    t->percent_step = percent_step;
    size_t first = percent_step > 0 ? percent_step : 0;
    atomic_init(&t->next_percent_threshold, first);

    atomic_init(&t->stop, false);

    // default: no custom sink; callbacks NULL imply fallback to info_format
    t->sink.on_progress = NULL;
    t->sink.on_log = NULL;
    t->sink.user_data = NULL;
    telemetry_install_default_sink(t);
}

/*!
   \brief Queues a telemetry event into the ring buffer for later consumption.

   Waits until the destination slot becomes available, copies the event payload
   into that slot, and then marks the slot as ready using release semantics so
   readers can safely observe the published event.

   \param t The telemetry instance that owns the event buffer.
   \param src The event payload to enqueue.
*/
static void enqueue_event(telemetry_t *t, event_t *src)
{
    size_t idx =
        atomic_fetch_add_explicit(&t->write_index, 1, memory_order_relaxed);

    event_t *dst = &t->buffer[idx % LOG_CAPACITY];

    // wait until slot is free (bounded spin)
    while (atomic_load_explicit(&dst->ready, memory_order_acquire)) {
        sched_yield();
    }

    // copy payload
    *dst = *src;

    // publish
    atomic_store_explicit(&dst->ready, true, memory_order_release);
}

/*!
   \brief Queues a terminal `100%` progress event for a telemetry stream.

   This helper records the stream as fully completed, disables further
   percentage-threshold reporting, and enqueues one last progress event with
   `completed == total` so the consumer can emit the final `100%` update.

   \param t The telemetry instance to finalize.
*/
static void telemetry_enqueue_final_progress(telemetry_t *t)
{
    event_t ev = {0};

    atomic_store_explicit(&t->completed, t->total, memory_order_release);
    atomic_store_explicit(&t->next_percent_threshold, 101,
                          memory_order_release);

    ev.type = EV_PROGRESS;
    ev.completed = t->total;
    ev.total = t->total;
    ev.is_terminal = true;
    enqueue_event(t, &ev);
}

static void telemetry_enqueue_final_counter(telemetry_t *t)
{
    event_t ev = {0};

    ev.type = EV_PROGRESS;
    ev.completed = atomic_load_explicit(&t->completed, memory_order_acquire);
    ev.total = 0;
    ev.is_terminal = true;
    enqueue_event(t, &ev);
}

static bool telemetry_has_pending_events(telemetry_t *t)
{
    if (t->read_index !=
        atomic_load_explicit(&t->write_index, memory_order_acquire)) {
        return true;
    }

    event_t *ev = &t->buffer[t->read_index % LOG_CAPACITY];
    return atomic_load_explicit(&ev->ready, memory_order_acquire);
}

static void telemetry_log(telemetry_t *t, const char *msg)
{
    event_t ev = {0};
    ev.type = EV_LOG;
    snprintf(ev.message, LOG_MSG_SIZE, "%s", msg);

    enqueue_event(t, &ev);
}

/*!
   \brief Captures the current GRASS info output format for subsequent
   telemetry.

   Reads the process-wide info formatting mode and stores it on the telemetry
   instance so later progress and log events can format output consistently.

   \param t The telemetry state that caches the active info format.
*/
static void telemetry_set_info_format(telemetry_t *t)
{
    t->info_format = G_info_format();
}

/*!
   \brief Records completed work and enqueues a progress event when the next
   reportable threshold is reached.

   The function stores the current completed count in the telemetry state, then
   decides whether to emit a progress event using one of two modes:
   percent-based reporting when `percent_step` and `total` are configured, or
   count-based/time-based throttling when they are not. Atomic compare-and-swap
   operations ensure that only one caller emits an event for a given threshold
   or interval.

   \param t The telemetry state to update and publish through.
   \param completed The current completed count.
*/
static void telemetry_progress(telemetry_t *t, size_t completed)
{
    size_t previous = atomic_load_explicit(&t->completed, memory_order_acquire);
    if (completed <= previous) {
        return;
    }

    atomic_store_explicit(&t->completed, completed, memory_order_release);

    if (t->percent_step > 0 && t->total > 0) {
        if (completed >= t->total) {
            telemetry_enqueue_final_progress(t);
            return;
        }

        size_t current_pct = (size_t)((completed * 100) / t->total);
        size_t expected = atomic_load_explicit(&t->next_percent_threshold,
                                               memory_order_relaxed);
        while (current_pct >= expected && expected <= 100) {
            size_t next = expected + t->percent_step;
            if (expected < 100 && next > 100)
                next = 100;
            else if (next > 100)
                next = 101; // sentinel beyond 100 to stop further emits
            if (atomic_compare_exchange_strong_explicit(
                    &t->next_percent_threshold, &expected, next,
                    memory_order_acq_rel, memory_order_relaxed)) {
                // we won the right to emit at this threshold
                break;
            }
            // CAS failed, expected now contains the latest value; loop to
            // re-check
        }
        // If we didn't advance, nothing to emit
        if (current_pct < expected || expected > 100) {
            return;
        }
    }
    else if (t->percent_step > 0) {
        size_t expected = atomic_load_explicit(&t->next_percent_threshold,
                                               memory_order_relaxed);
        while (completed >= expected) {
            size_t next = expected + t->percent_step;
            if (atomic_compare_exchange_strong_explicit(
                    &t->next_percent_threshold, &expected, next,
                    memory_order_acq_rel, memory_order_relaxed)) {
                break;
            }
        }
        if (completed < expected) {
            return;
        }
    }
    else {
        long now = now_ns();
        long last =
            atomic_load_explicit(&t->last_progress_ns, memory_order_relaxed);
        if (now - last < t->interval_ns) {
            return;
        }
        if (!atomic_compare_exchange_strong_explicit(
                &t->last_progress_ns, &last, now, memory_order_acq_rel,
                memory_order_relaxed)) {
            return;
        }
    }

    event_t ev = {0};
    ev.type = EV_PROGRESS;
    ev.completed = completed;
    ev.total = t->total;

    enqueue_event(t, &ev);
}

static void telemetry_install_default_sink(telemetry_t *t)
{
    // Only set defaults if no custom sink is present
    if (t->sink.on_progress || t->sink.on_log)
        return;

    switch (t->info_format) {
    case G_INFO_FORMAT_STANDARD:
        t->sink.on_progress = sink_progress_standard;
        break;
    case G_INFO_FORMAT_GUI:
        t->sink.on_progress = sink_progress_gui;
        break;
    case G_INFO_FORMAT_PLAIN:
        t->sink.on_progress = sink_progress_plain;
        break;
    default:
        t->sink.on_progress = NULL; // silent/no output
        break;
    }
    t->sink.on_log = sink_log_default;
    t->sink.user_data = NULL;
}

/*!
   \brief Initializes shared percent-based telemetry and starts the detached
   consumer thread once.

   This function performs one-time global setup for percent progress reporting.
   Repeated calls return immediately after the initialization state has been
   set. If output is disabled or the consumer thread cannot be created, no
   further progress consumer setup is performed.

   \param total_num_elements The total number of elements used to compute
     progress percentages.
   \param percent_step The percentage increment that controls when
     progress updates are emitted.
*/
static void start_global_percent(size_t total_num_elements, size_t percent_step)
{
    bool expected_init = false;
    if (!atomic_compare_exchange_strong_explicit(
            &g_percent_initialized, &expected_init, true, memory_order_acq_rel,
            memory_order_relaxed)) {
        return;
    }

    telemetry_init_percent(&g_percent_telemetry,
                           ((total_num_elements > 0) ? total_num_elements : 0),
                           ((percent_step > 0) ? percent_step : 0));

    // attach current global sink (may be empty for default behavior)
    if (g_percent_sink.on_progress || g_percent_sink.on_log) {
        g_percent_telemetry.sink = g_percent_sink;
    } // else keep defaults installed by telemetry_init_percent

    bool expected_started = false;
    if (atomic_compare_exchange_strong_explicit(
            &g_percent_consumer_started, &expected_started, true,
            memory_order_acq_rel, memory_order_relaxed)) {
        pthread_t consumer_thread;
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&consumer_thread, &attr, telemetry_consumer,
                           &g_percent_telemetry) != 0) {
            atomic_store_explicit(&g_percent_consumer_started, false,
                                  memory_order_release);
            atomic_store_explicit(&g_percent_initialized, false,
                                  memory_order_release);
        }
        pthread_attr_destroy(&attr);
    }
}

/*!
   \brief Sets or clears the global sink used by `G_percent` progress
   reporting.

   Copies `sink` into the shared global progress configuration used by the
   legacy `G_percent` API. When `sink` is non-`NULL`, its callbacks and
   `user_data` are used for subsequent progress and log events. Passing `NULL`
   clears the custom sink and restores the default output behavior derived from
   the current runtime info format.

   If global progress telemetry has already been initialized, the active
   telemetry sink is updated immediately so later events follow the new
   configuration.

   \param sink The sink configuration to install globally, or `NULL` to remove
     the custom sink and fall back to the default renderer.
*/
static void set_global_sink(const GProgressSink *sink)
{
    if (sink) {
        g_percent_sink = *sink;
    }
    else {
        g_percent_sink.on_progress = NULL;
        g_percent_sink.on_log = NULL;
        g_percent_sink.user_data = NULL;
    }

    // apply to global telemetry if initialized
    if (atomic_load_explicit(&g_percent_initialized, memory_order_acquire)) {
        if (g_percent_sink.on_progress || g_percent_sink.on_log) {
            g_percent_telemetry.sink = g_percent_sink;
        }
        else {
            // reinstall defaults based on current info_format
            g_percent_telemetry.sink.on_progress = NULL;
            g_percent_telemetry.sink.on_log = NULL;
            g_percent_telemetry.sink.user_data = NULL;
            telemetry_install_default_sink(&g_percent_telemetry);
        }
    }
}

static bool output_is_silenced(void)
{
    return (G_info_format() == G_INFO_FORMAT_SILENT || G_verbose() < 1);
}

/*! \brief Returns the current UTC time in nanoseconds. */
static long now_ns(void)
{
#if defined(TIME_UTC)
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (long)ts.tv_sec * 1000000000L + ts.tv_nsec;
#elif defined(CLOCK_REALTIME)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long)ts.tv_sec * 1000000000L + ts.tv_nsec;
#elif defined(HAVE_GETTIMEOFDAY)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long)tv.tv_sec * 1000000000L + (long)tv.tv_usec * 1000L;
#else
    return (long)time(NULL) * 1000000000L;
#endif
}

// Legacy compatibility: adapter for void (*fn)(int)
static void legacy_percent_adapter(const GProgressEvent *e, void *ud)
{
    int (*fn)(int) = g_legacy_percent_routine;
    (void)ud;
    if (fn) {
        int pct = (int)(e->percent);
        (void)fn(pct);
    }
}

// Internal default sinks for different G_info_format modes
static void sink_progress_standard(const GProgressEvent *e, void *ud)
{
    (void)ud;
    if (e->total == 0) {
        if (!e->is_terminal)
            fprintf(stderr, "%10zu\b\b\b\b\b\b\b\b\b\b", e->completed);
        else
            fprintf(stderr, "\n");
        return;
    }
    int pct = (int)(e->percent);
    fprintf(stderr, "%4d%%\b\b\b\b\b", pct);
    if (pct == 100)
        fprintf(stderr, "\n");
}

static void sink_progress_plain(const GProgressEvent *e, void *ud)
{
    (void)ud;
    if (e->total == 0) {
        if (e->is_terminal)
            fprintf(stderr, "%s", "\n");
        else
            fprintf(stderr, "%zu..", e->completed);
        return;
    }
    int pct = (int)(e->percent);
    fprintf(stderr, "%d%s", pct, (pct == 100 ? "\n" : ".."));
}

static void sink_progress_gui(const GProgressEvent *e, void *ud)
{
    (void)ud;
    if (e->total == 0) {
        if (!e->is_terminal) {
            fprintf(stderr, "GRASS_INFO_PROGRESS: %zu\n", e->completed);
            fflush(stderr);
        }
        return;
    }
    int pct = (int)(e->percent);

    int comp = (int)e->completed;
    int tot = (int)e->total;

    fprintf(stderr, "GRASS_INFO_PERCENT: %d (%d/%d)\n", pct, comp, tot);
    fflush(stderr);
}

static void sink_log_default(const char *message, void *ud)
{
    (void)ud;
    // default logging to stdout
    printf("[LOG] %s\n", message);
}

// Legacy API

/*!
   \brief Reports global progress when completion crosses the next percentage
   step.

   This function initializes the shared global telemetry stream on first use,
   clamps `current_element` into the valid `0...total_num_elements` range, and
   enqueues a progress update only when the computed percentage reaches the
   next configured threshold. When progress reaches the total, a terminal
   `100%` event is always queued and the background consumer is asked to stop
   after pending events have been flushed.

   \param current_element The current completed element index or count.
   \param total_num_elements The total number of elements to process. Values
     less than or equal to `0` disable reporting.
   \param percent_step The minimum percentage increment required before a new
     progress event is emitted.
*/
static void G__percent_ng(long current_element, long total_num_elements,
                          int percent_step)
{
    if (total_num_elements <= 0 || output_is_silenced())
        return;

    start_global_percent((size_t)total_num_elements, (size_t)percent_step);

    // If someone initialized with different totals/steps, we keep the first
    // ones for simplicity.

    size_t total = (size_t)total_num_elements;
    size_t completed = (current_element < 0) ? 0 : (size_t)current_element;
    if (completed > total)
        completed = total;

    if (g_percent_telemetry.percent_step == 0)
        return; // not configured

    atomic_store_explicit(&g_percent_telemetry.completed, completed,
                          memory_order_release);

    if (completed == total) {
        telemetry_enqueue_final_progress(&g_percent_telemetry);
        atomic_store_explicit(&g_percent_telemetry.stop, true,
                              memory_order_release);
        return;
    }

    size_t current_pct = (size_t)((completed * 100) / total);
    size_t expected = atomic_load_explicit(
        &g_percent_telemetry.next_percent_threshold, memory_order_relaxed);
    while (current_pct >= expected && expected <= 100) {
        size_t next = expected + g_percent_telemetry.percent_step;
        if (expected < 100 && next > 100)
            next = 100;
        else if (next > 100)
            next = 101;
        if (atomic_compare_exchange_strong_explicit(
                &g_percent_telemetry.next_percent_threshold, &expected, next,
                memory_order_acq_rel, memory_order_relaxed)) {
            event_t ev = {0};
            ev.type = EV_PROGRESS;
            ev.completed = completed;
            ev.total = total;
            enqueue_event(&g_percent_telemetry, &ev);
            if (completed == total) {
                atomic_store_explicit(&g_percent_telemetry.stop, true,
                                      memory_order_release);
            }
            return;
        }
        // CAS failed; expected updated, loop continues
    }
}

// no-op: retained for compatibility with legacy callers.
static void G__percent_reset_ng(void)
{
    // No global state to reset in the concurrent API.
    // Kept to avoid breaking legacy code paths that call G_percent_reset().
}

// Print progress info messages
static void G__progress_ng(long n, int s)
{
    // Mirror legacy behavior: emit on multiples of s. Routing through the
    // global telemetry.

    if (s <= 0 || output_is_silenced())
        return;

    // Initialize global telemetry if needed with percent_step=0 to disable
    // percent thresholds
    start_global_percent(0, 0);

    if (n == s && n == 1) {
        event_t ev = {0};

        ev.type = EV_PROGRESS;
        ev.completed = atomic_load_explicit(&g_percent_telemetry.completed,
                                            memory_order_acquire);
        ev.total = 0;
        ev.is_terminal = true;
        enqueue_event(&g_percent_telemetry, &ev);
        return;
    }

    if (n % s != 0)
        return;

    // For counter mode, we do not know total; publish completed=n, total=0
    event_t ev = {0};
    ev.type = EV_PROGRESS;
    ev.completed = (n < 0 ? 0 : (size_t)n);
    ev.total = 0; // unknown total; consumer/sink can render raw counts
    atomic_store_explicit(&g_percent_telemetry.completed, ev.completed,
                          memory_order_release);
    enqueue_event(&g_percent_telemetry, &ev);
}

// Compatibility layer for legacy percent routine API
static void G__set_percent_routine_ng(int (*fn)(int))
{
    // The historical signature in gis.h declares int (*)(int), but actual
    // implementers often used void(*)(int). We accept int-returning and
    // ignore the return value.
    if (!fn) {
        // Reset to default behavior
        g_legacy_percent_routine = NULL;
        set_global_sink(NULL);
        return;
    }
    // Route legacy callbacks through a dedicated function-pointer slot.
    GProgressSink s = {0};
    s.on_progress = legacy_percent_adapter;
    s.user_data = NULL;
    g_legacy_percent_routine = fn;
    set_global_sink(&s);
}

static void G__unset_percent_routine_ng(void)
{
    // Reset to default (env-driven G_info_format output)
    set_global_sink(NULL);
}

#endif // defined(G_USE_PROGRESS_NG)
