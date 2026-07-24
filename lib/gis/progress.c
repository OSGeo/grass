/*!
   \file lib/gis/progress.c

   \brief GIS Library - Progress reporting and telemetry support for
   GRASS operations

   This file implements the `G_progress_*` API used to report incremental work
   completion for long-running operations. It supports percentage-, count-
   and time-based progress contexts.

   The implementation is organized as a telemetry pipeline. Producer-side API
   calls update atomic progress state and enqueue ::event_type_t `EV_PROGRESS`
   or `EV_LOG` records into a bounded ring buffer. A single consumer thread
   drains that buffer, converts raw records into `event_t` values, and
   forwards them either to installed sink callbacks or to default renderers
   selected from the current G_info_format() mode.

   Concurrency is designed as multi-producer, single-consumer per telemetry
   stream. Producers reserve slots with an atomic `write_index`, publish events
   by setting a per-slot `ready` flag with release semantics, and use atomic
   compare-and-swap to ensure that only one producer emits a given percent
   threshold or time-gated update. The consumer advances a non-atomic
   `read_index`, waits for published slots, processes events in FIFO order, and
   then marks slots free again.

   The `G_progress_*` API requires C11 atomic operations and POSIX threads.
   When those prerequisites are available, `G_USE_PROGRESS_NG` is defined and
   this implementation is enabled.

   \copyright
   (C) 2026 by the GRASS Development Team
   \copyright
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Nicklas Larsson
 */

#include <grass/gis.h>

#if G_USE_PROGRESS_NG

#include <assert.h>
#include <sched.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define LOG_CAPACITY       1024
#define LOG_MSG_SIZE       128
#define TIME_RATE_LIMIT_MS 100

typedef enum { EV_LOG, EV_PROGRESS } event_type_t;

typedef struct {
    event_type_t type;
    size_t completed;
    size_t total;
    bool is_terminal;
    double percent; // 0.0 ... 100.0
    char message[LOG_MSG_SIZE];
    atomic_bool ready;
} event_t;

typedef void (*GProgressCallback)(const event_t *event);
typedef void (*GLogCallback)(const char *message);

/*!
   \brief Callback bundle used to receive progress and log output.

   `sink_t` lets callers redirect telemetry emitted by the progress APIs.
   Each callback is optional.
*/
typedef struct {
    GProgressCallback on_progress; // optional
    GLogCallback on_log;           // optional
} sink_t;

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
    sink_t sink; // optional sink
} telemetry_t;

struct GProgressContext {
    telemetry_t telemetry;
    atomic_bool initialized;
    pthread_t consumer_thread;
    atomic_bool consumer_started;
};

static GProgressContext *context_create(size_t, size_t, long);
static bool telemetry_has_pending_events(telemetry_t *);
static void telemetry_init_time(telemetry_t *, size_t, long);
static void telemetry_init_percent(telemetry_t *, size_t, size_t);
static void enqueue_event(telemetry_t *, event_t *);
static void telemetry_enqueue_final_progress(telemetry_t *);
static void telemetry_enqueue_final_counter(telemetry_t *);
static void telemetry_log(telemetry_t *, const char *);
static void telemetry_install_default_sink(telemetry_t *t);
static void telemetry_progress(telemetry_t *);
static void *telemetry_consumer(void *);
static bool output_is_silenced(void);
static long now_ns(void);
static void sink_progress_standard(const event_t *);
static void sink_progress_plain(const event_t *);
static void sink_progress_gui(const event_t *);
static void sink_log_default(const char *);

/*!
   \brief Creates an isolated progress-reporting context for concurrent work.

   The returned context tracks progress for `total_num_elements` items and
   emits progress updates whenever completion advances by at least
   `percent_step` percentage points. `total_num_elements` must match the
   actual number of work units that will be reported through
   `G_progress_update()`. Each call to `G_progress_update()` records exactly
   one newly completed work unit. If output is enabled by the current runtime
   configuration, this function also starts the background consumer thread
   used to flush queued telemetry events.

   \param total_num_elements Total number of elements to process.
   \param step Minimum percentage increment that triggers a
     progress event.
   \return A newly allocated `GProgressContext`, or `NULL` if output
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

   Callers should invoke `G_progress_update()` once per completed work unit and
   destroy the context with `G_progress_context_destroy()` when processing
   finishes.

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
   `G_progress_update()` when the total number of work units is not known up
   front. The returned context behaves as a simple counter and emits updates
   whenever the completed count advances by at least `increment_step` units.

   \param increment_step Minimum completed-count increment that triggers a
     progress event.

   \return A newly allocated `GProgressContext`, or `NULL` if output is
     silenced by the current runtime configuration.
*/
GProgressContext *G_progress_context_create_counter(size_t increment_step)
{
    assert(increment_step > 0);
    return context_create(0, increment_step, 0);
}

/*!
   \brief Destroys a `GProgressContext` and releases any resources it owns.

   This function stops the context's background telemetry consumer, waits for
   the consumer thread to finish when it was started, marks the context as no
   longer initialized, and frees the context memory. Passing `NULL` is safe and
   has no effect.

   \param ctx The progress-reporting context previously created by
     `G_progress_context_create*()`, or `NULL`.
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
   \brief Reports one completed work unit for a `GProgressContext`.

   This re-entrant function advances the context-owned progress counter by one
   and enqueues a progress event when the configured threshold for the context
   is reached. The reporting mode is determined by the context created with
   `G_progress_context_create()`, `G_progress_context_create_time()`, or
   `G_progress_context_create_increment()`.

   Callers typically create a context, invoke this function once per completed
   work unit, and then release resources with `G_progress_context_destroy()`.

   Example:
   ```c
   size_t n_rows = window.rows;  // total number of rows
   size_t step = 10;  // output step, every 10%
   GProgressContext *ctx = G_progress_context_create(n_rows, step);
   for (row = 0; row < window.rows; row++) {
       // costly calculation ...

       G_progress_update(ctx);
   }
   G_progress_context_destroy(ctx);
   ```

   \param ctx The progress-reporting context created by the
     `G_progress_context_create*()` initializers.
*/
void G_progress_update(GProgressContext *ctx)
{
    if (!ctx)
        return;
    if (!atomic_load_explicit(&ctx->initialized, memory_order_acquire))
        return;

    telemetry_progress(&ctx->telemetry);
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

   The created context configures its reporting mode from
   `total_num_elements`, `step`, and `interval_ms`:
   when `step` is `0`, progress updates are emitted using a time-based
   interval controlled by `interval_ms`; when `step` is greater than `0`
   and `total_num_elements` is greater than `0`, updates are emitted at
   percentage thresholds derived from `step`; when `step` is greater than
   `0` and `total_num_elements` is `0`, updates are emitted as an open-ended
   counter every `step` completed work units.

   \param total_num_elements Total number of elements expected for the
     operation being tracked. A value of `0` selects open-ended counter mode
     when `step` is greater than `0`.
   \param step Reporting increment. In fixed-total mode it is interpreted as
     a percentage step; in open-ended mode it is interpreted as a completed-
     count step. A value of `0` selects time-based reporting instead.
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
    }
    else {
        telemetry_init_percent(&ctx->telemetry, total_num_elements, step);
    }

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

/*!
   \brief Consumes queued telemetry events and dispatches log or progress output
   until shutdown is requested and the event buffer has been drained.

   The consumer waits for published events in the telemetry ring buffer,
   dispatches `EV_LOG` messages through the configured log sink, derives
   terminal and percentage state for `EV_PROGRESS` events, forwards them to
   the configured progress sink, and then releases each buffer slot for reuse.

   \param arg Pointer to the `telemetry_t` instance whose ring buffer and
     sink configuration should be consumed.
   \return `NULL` after the consumer loop exits.
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
                t->sink.on_log(ev->message);
            }
            else {
                sink_log_default(ev->message);
            }
        }
        else if (ev->type == EV_PROGRESS) {
            ev->is_terminal = ev->is_terminal ||
                              (ev->total > 0 && ev->completed >= ev->total);
            ev->percent = (ev->total > 0) ? (double)ev->completed * 100.0 /
                                                (double)ev->total
                                          : 0.0;

            if (t->sink.on_progress)
                t->sink.on_progress(ev);
        }

        // mark slot free
        atomic_store_explicit(&ev->ready, false, memory_order_release);
        t->read_index++;
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
    t->info_format = G_info_format();

    atomic_init(&t->last_progress_ns, 0);
    t->interval_ns = interval_ms * 1000000L;

    t->percent_step = 0; // 0 => disabled, use time-based if interval_ns > 0
    atomic_init(&t->next_percent_threshold, 0);

    atomic_init(&t->stop, false);

    telemetry_install_default_sink(t);
}

/*!
   \brief Initializes telemetry state for threshold-based progress reporting.

   Resets the telemetry ring buffer and counters, disables time-based
   throttling, and configures the next progress event to be emitted when the
   completed work reaches the first `percent_step` threshold. When `total` is
   greater than `0`, the threshold is interpreted as a percentage step.
   When `total` is `0`, the threshold is interpreted as an open-ended
   completed-count step.

   \param t The telemetry instance to reset and configure.
   \param total The total number of work units expected for the tracked
     operation. A value of `0` selects open-ended counter mode.
   \param percent_step Reporting threshold. In fixed-total mode it is used as
     a percentage step; in open-ended mode it is used as a completed-count
     step. A value of `0` disables threshold-based reporting.
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
    t->info_format = G_info_format();

    // disable time-based gating
    atomic_init(&t->last_progress_ns, 0);
    t->interval_ns = 0;

    // enable percentage-based gating
    t->percent_step = percent_step;
    size_t first = percent_step > 0 ? percent_step : 0;
    atomic_init(&t->next_percent_threshold, first);

    atomic_init(&t->stop, false);

    telemetry_install_default_sink(t);
}

/*!
   \brief Queues a telemetry event into the ring buffer for later
   consumption.

   Waits until the destination slot becomes available, copies the event
   payload into that slot, and then marks the slot as ready using release
   semantics so readers can safely observe the published event.

   \param t The telemetry instance that owns the event buffer.
   \param src The event payload to enqueue.
*/
static void enqueue_event(telemetry_t *t, event_t *src)
{
    size_t idx =
        atomic_fetch_add_explicit(&t->write_index, 1, memory_order_relaxed);

    event_t *dst = &t->buffer[idx % LOG_CAPACITY];

    // wait until the slot is free
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
   \brief Records one completed work unit and enqueues a progress event when
   the next reportable threshold is reached.

   The function atomically increments the telemetry completed count by one.
   On the first update, threshold-based modes emit an initial zero-progress
   event before reporting further progress. Subsequent progress events are
   emitted in one of three modes: percentage-based reporting when
   `percent_step` and `total` are configured, count-based reporting when only
   `percent_step` is configured, or time-based throttling when only
   `interval_ns` is configured. Atomic compare-and-swap operations ensure that
   only one caller emits an event for a given threshold or interval.

   \param t The telemetry state to update and publish through.
*/
static void telemetry_progress(telemetry_t *t)
{
    size_t previous =
        atomic_fetch_add_explicit(&t->completed, 1, memory_order_relaxed);
    size_t completed = previous + 1;

    if (previous == 0 && t->percent_step > 0) {
        event_t ev = {0};
        ev.type = EV_PROGRESS;
        ev.completed = 0;
        ev.total = t->total;
        enqueue_event(t, &ev);
    }

    if (t->percent_step > 0 && t->total > 0) {
        // percentage-based reporting
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
            // Compare-and-swap failed, expected now contains the latest
            // value; loop to re-check
        }
        // If we didn't advance, nothing to emit
        if (current_pct < expected || expected > 100) {
            return;
        }
    }
    else if (t->percent_step > 0) {
        // count-based reporting
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
        // time-based reporting
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

// Internal default sinks for different G_info_format modes,
// `e->total == 0` indicates counter-mode.
static void sink_progress_standard(const event_t *e)
{
    if (e->total == 0) {
        fprintf(stderr, "%10zu\b\b\b\b\b\b\b\b\b\b", e->completed);
        if (e->is_terminal)
            fprintf(stderr, "\n");
        return;
    }
    int pct = (int)(e->percent);
    fprintf(stderr, "%4d%%\b\b\b\b\b", pct);
    if (pct == 100)
        fprintf(stderr, "\n");
}

static void sink_progress_plain(const event_t *e)
{
    if (e->total == 0) {
        if (e->is_terminal)
            fprintf(stderr, "\b\b  \b\b\n");
        else
            fprintf(stderr, "%zu..", e->completed);
        return;
    }
    int pct = (int)(e->percent);
    fprintf(stderr, "%d%s", pct, (pct == 100 ? "\n" : ".."));
}

static void sink_progress_gui(const event_t *e)
{
    if (e->total == 0) {
        if (!e->is_terminal) {
            fprintf(stderr, "GRASS_INFO_PROGRESS: %zu\n", e->completed);
        }
        fflush(stderr);
        return;
    }
    int pct = (int)(e->percent);

    int comp = (int)e->completed;
    int tot = (int)e->total;

    fprintf(stderr, "GRASS_INFO_PERCENT: %d (%d/%d)\n", pct, comp, tot);
    fflush(stderr);
}

static void sink_log_default(const char *message)
{
    // default logging to stdout
    printf("[LOG] %s\n", message);
}

#else

#pragma message("New PROGRESS API is not enabled!")

struct GProgressContext {
    int dummy;
};

GProgressContext *G_progress_context_create(size_t total_num_elements,
                                            size_t step)
{
    (void)total_num_elements;
    (void)step;
    return NULL;
}

GProgressContext *G_progress_context_create_time(size_t total_num_elements,
                                                 long interval_ms)
{
    (void)total_num_elements;
    (void)interval_ms;
    return NULL;
}

GProgressContext *G_progress_context_create_increment(size_t increment_step)
{
    (void)increment_step;
    return NULL;
}

void G_progress_context_destroy(GProgressContext *ctx)
{
    (void)ctx;
}

void G_progress_update(GProgressContext *ctx)
{
    (void)ctx;
}

void G_progress_log(GProgressContext *ctx, const char *message)
{
    (void)ctx;
    (void)message;
}

#endif // G_USE_PROGRESS_NG
