/****************************************************************************
 *
 * MODULE:       simwe library
 * AUTHOR(S):    Corey White
 * PURPOSE:      Run summary of the SIMWE simulation printed with the -p flag
 * SPDX-FileCopyrightText: 2026 GRASS Development Team
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#include <math.h>
#include <stdio.h>

#include <grass/gis.h>
#include <grass/gjson.h>
#include <grass/glocale.h>

#include <grass/simlib.h>

void add_output_step(Summary *summary, const OutputStep *step)
{
    if (summary->nsteps == summary->nsteps_alloc) {
        summary->nsteps_alloc =
            summary->nsteps_alloc ? 2 * summary->nsteps_alloc : 8;
        summary->steps = G_realloc(summary->steps,
                                   summary->nsteps_alloc * sizeof(OutputStep));
    }
    summary->steps[summary->nsteps] = *step;
    summary->nsteps++;
}

double time_step_seconds(const Setup *setup)
{
    return setup->deltap * setup->timec;
}

int simulated_seconds(const Setup *setup, int iterations)
{
    return (int)(iterations * time_step_seconds(setup));
}

static void set_string_or_null(G_JSON_Object *object, const char *key,
                               const char *value)
{
    if (value)
        G_json_object_set_string(object, key, value);
    else
        G_json_object_set_null(object, key);
}

static void set_number_or_null(G_JSON_Object *object, const char *key,
                               double value)
{
    if (isfinite(value))
        G_json_object_set_number(object, key, value);
    else
        G_json_object_set_null(object, key);
}

static void print_json(const Setup *setup, const Settings *settings,
                       const Simulation *sim, const Inputs *inputs,
                       const Outputs *outputs, const Summary *summary)
{
    G_JSON_Value *root_value = G_json_value_init_object();
    G_JSON_Object *root = G_json_object(root_value);
    G_JSON_Value *steps_value = G_json_value_init_array();
    G_JSON_Array *steps = G_json_array(steps_value);
    bool sediment = inputs->wdepth != NULL;
    char *serialized;
    int i;

    if (root_value == NULL || steps_value == NULL)
        G_fatal_error(_("Failed to initialize JSON object. Out of memory?"));

    G_json_object_set_number(root, "walkers_requested", sim->maxwa);
    G_json_object_set_number(root, "walkers_generated", sim->nwalk);
    G_json_object_set_number(root, "walkers_remaining", sim->nwalka);
    G_json_object_set_number(root, "duration", settings->timesec);
    G_json_object_set_number(
        root, "simulated_time",
        simulated_seconds(setup, summary->iterations_completed));
    G_json_object_set_number(root, "time_step", time_step_seconds(setup));
    if (sediment)
        set_number_or_null(root, "time_step_sediment", setup->deltaw);
    G_json_object_set_number(root, "iterations_planned", setup->miter);
    G_json_object_set_number(root, "iterations_completed",
                             summary->iterations_completed);
    G_json_object_set_boolean(root, "stopped_early", summary->stopped_early);
    G_json_object_set_number(root, "mean_velocity", setup->vmean);
    if (sediment) {
        G_json_object_set_number(root, "velocity_max", setup->vmax);
        G_json_object_set_number(root, "sigma_max", setup->sigmax);
    }
    if (setup->chmean != 0.0)
        G_json_object_set_number(root, "mean_mannings_n", 1.0 / setup->chmean);
    else
        G_json_object_set_null(root, "mean_mannings_n");
    G_json_object_set_number(root, "mean_source_rate", setup->si0);
    if (!sediment)
        G_json_object_set_number(root, "mean_infiltration", setup->infmean);
    G_json_object_set_number(root, "threads", summary->threads);
    if (sediment) {
        set_string_or_null(root, "transport_capacity", outputs->tc);
        set_string_or_null(root, "tlimit_erosion_deposition", outputs->et);
    }

    for (i = 0; i < summary->nsteps; i++) {
        const OutputStep *step = &summary->steps[i];
        G_JSON_Value *step_value = G_json_value_init_object();
        G_JSON_Object *object = G_json_object(step_value);

        G_json_object_set_number(object, "simulated_time",
                                 step->simulated_time);
        G_json_object_set_string(object, "timestamp", step->timestamp);
        G_json_object_set_number(object, "walkers_remaining",
                                 step->walkers_remaining);
        if (sediment) {
            set_string_or_null(object, "sediment_concentration", step->conc);
            set_string_or_null(object, "sediment_flux", step->flux);
            set_string_or_null(object, "erosion_deposition", step->erdep);
        }
        else {
            set_string_or_null(object, "depth", step->depth);
            set_string_or_null(object, "discharge", step->disch);
            set_string_or_null(object, "error", step->err);
        }
        set_string_or_null(object, "walkers", step->outwalk);
        G_json_array_append_value(steps, step_value);
    }
    G_json_object_set_value(root, "outputs", steps_value);

    serialized = G_json_serialize_to_string_pretty(root_value);
    if (!serialized) {
        G_json_value_free(root_value);
        G_fatal_error(_("Failed to serialize JSON to pretty format."));
    }
    puts(serialized);
    G_json_free_serialized_string(serialized);
    G_json_value_free(root_value);
}

static void print_map_name(const char *indent, const char *key,
                           const char *name)
{
    if (name)
        printf("%s%s: %s\n", indent, key, name);
}

static void print_number(const char *key, double value)
{
    if (isfinite(value))
        printf("%s: %g\n", key, value);
    else
        printf("%s: undefined\n", key);
}

static void print_plain(const Setup *setup, const Settings *settings,
                        const Simulation *sim, const Inputs *inputs,
                        const Outputs *outputs, const Summary *summary)
{
    bool sediment = inputs->wdepth != NULL;
    int i;

    printf("walkers_requested: %d\n", sim->maxwa);
    printf("walkers_generated: %d\n", sim->nwalk);
    printf("walkers_remaining: %d\n", sim->nwalka);
    printf("duration: %d\n", settings->timesec);
    printf("simulated_time: %d\n",
           simulated_seconds(setup, summary->iterations_completed));
    print_number("time_step", time_step_seconds(setup));
    if (sediment)
        print_number("time_step_sediment", setup->deltaw);
    printf("iterations_planned: %d\n", setup->miter);
    printf("iterations_completed: %d\n", summary->iterations_completed);
    printf("stopped_early: %s\n", summary->stopped_early ? "true" : "false");
    print_number("mean_velocity", setup->vmean);
    if (sediment) {
        print_number("velocity_max", setup->vmax);
        print_number("sigma_max", setup->sigmax);
    }
    if (setup->chmean != 0.0)
        print_number("mean_mannings_n", 1.0 / setup->chmean);
    else
        printf("mean_mannings_n: undefined\n");
    print_number("mean_source_rate", setup->si0);
    if (!sediment)
        print_number("mean_infiltration", setup->infmean);
    printf("threads: %d\n", summary->threads);
    if (sediment) {
        print_map_name("", "transport_capacity", outputs->tc);
        print_map_name("", "tlimit_erosion_deposition", outputs->et);
    }

    for (i = 0; i < summary->nsteps; i++) {
        const OutputStep *step = &summary->steps[i];

        printf("output:\n");
        printf("  simulated_time: %d\n", step->simulated_time);
        printf("  timestamp: %s\n", step->timestamp);
        printf("  walkers_remaining: %d\n", step->walkers_remaining);
        if (sediment) {
            print_map_name("  ", "sediment_concentration", step->conc);
            print_map_name("  ", "sediment_flux", step->flux);
            print_map_name("  ", "erosion_deposition", step->erdep);
        }
        else {
            print_map_name("  ", "depth", step->depth);
            print_map_name("  ", "discharge", step->disch);
            print_map_name("  ", "error", step->err);
        }
        print_map_name("  ", "walkers", step->outwalk);
    }
}

void print_summary(SummaryFormat format, const Setup *setup,
                   const Settings *settings, const Simulation *sim,
                   const Inputs *inputs, const Outputs *outputs,
                   const Summary *summary)
{
    switch (format) {
    case SUMMARY_JSON:
        print_json(setup, settings, sim, inputs, outputs, summary);
        break;
    case SUMMARY_PLAIN:
        print_plain(setup, settings, sim, inputs, outputs, summary);
        break;
    case SUMMARY_NONE:
        break;
    }
}

void free_summary(Summary *summary)
{
    int i;

    for (i = 0; i < summary->nsteps; i++) {
        OutputStep *step = &summary->steps[i];

        G_free(step->timestamp);
        G_free(step->depth);
        G_free(step->disch);
        G_free(step->err);
        G_free(step->outwalk);
        G_free(step->conc);
        G_free(step->flux);
        G_free(step->erdep);
    }
    G_free(summary->steps);
    summary->steps = NULL;
    summary->nsteps = 0;
    summary->nsteps_alloc = 0;
}
