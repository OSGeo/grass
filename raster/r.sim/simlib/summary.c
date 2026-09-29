/****************************************************************************
 *
 * MODULE:       simwe library
 * AUTHOR(S):    Corey White
 * PURPOSE:      Run summary of the SIMWE simulation printed with the -p flag
 *
 * COPYRIGHT:    (C) 2026 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

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

/* Simulated time reached when the main loop ended [seconds] */
static int simulated_time(const Setup *setup, const Summary *summary)
{
    return (int)(summary->iterations_completed * setup->deltap * setup->timec);
}

static void set_string_or_null(G_JSON_Object *object, const char *key,
                               const char *value)
{
    if (value)
        G_json_object_set_string(object, key, value);
    else
        G_json_object_set_null(object, key);
}

static void print_json(const Setup *setup, const Settings *settings,
                       const Simulation *sim, const Inputs *inputs,
                       const Summary *summary)
{
    G_JSON_Value *root_value = G_json_value_init_object();
    G_JSON_Object *root = G_json_object(root_value);
    G_JSON_Value *outputs_value = G_json_value_init_array();
    G_JSON_Array *outputs = G_json_array(outputs_value);
    bool sediment = inputs->wdepth != NULL;
    char *serialized;
    int i;

    if (root_value == NULL || outputs_value == NULL)
        G_fatal_error(_("Failed to initialize JSON object. Out of memory?"));

    G_json_object_set_number(root, "walkers_requested", sim->maxwa);
    G_json_object_set_number(root, "walkers_generated", sim->nwalk);
    G_json_object_set_number(root, "walkers_active", sim->nwalka);
    G_json_object_set_number(root, "duration", settings->timesec);
    G_json_object_set_number(root, "simulated_time",
                             simulated_time(setup, summary));
    G_json_object_set_number(root, "time_step", setup->deltap);
    if (sediment)
        G_json_object_set_number(root, "time_step_sediment", setup->deltaw);
    G_json_object_set_number(root, "time_coefficient", setup->timec);
    G_json_object_set_number(root, "iterations_planned", setup->miter);
    G_json_object_set_number(root, "iterations_completed",
                             summary->iterations_completed);
    G_json_object_set_number(root, "iterations_per_output", setup->iterout);
    G_json_object_set_boolean(root, "stopped_early", summary->stopped_early);
    G_json_object_set_number(root, "elevation_min", setup->zmin);
    G_json_object_set_number(root, "elevation_max", setup->zmax);
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

    for (i = 0; i < summary->nsteps; i++) {
        const OutputStep *step = &summary->steps[i];
        G_JSON_Value *step_value = G_json_value_init_object();
        G_JSON_Object *object = G_json_object(step_value);

        G_json_object_set_number(object, "simulated_time",
                                 step->simulated_time);
        G_json_object_set_string(object, "timestamp", step->timestamp);
        G_json_object_set_number(object, "walkers_active",
                                 step->walkers_active);
        if (sediment) {
            set_string_or_null(object, "transport_capacity", step->tc);
            set_string_or_null(object, "tlimit_erosion_deposition", step->et);
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
        G_json_array_append_value(outputs, step_value);
    }
    G_json_object_set_value(root, "outputs", outputs_value);

    serialized = G_json_serialize_to_string_pretty(root_value);
    if (!serialized) {
        G_json_value_free(root_value);
        G_fatal_error(_("Failed to serialize JSON to pretty format."));
    }
    puts(serialized);
    G_json_free_serialized_string(serialized);
    G_json_value_free(root_value);
}

static void print_map_name(const char *key, const char *name)
{
    if (name)
        printf("  %s: %s\n", key, name);
}

static void print_plain(const Setup *setup, const Settings *settings,
                        const Simulation *sim, const Inputs *inputs,
                        const Summary *summary)
{
    bool sediment = inputs->wdepth != NULL;
    int i;

    printf("walkers_requested: %d\n", sim->maxwa);
    printf("walkers_generated: %d\n", sim->nwalk);
    printf("walkers_active: %d\n", sim->nwalka);
    printf("duration: %d\n", settings->timesec);
    printf("simulated_time: %d\n", simulated_time(setup, summary));
    printf("time_step: %g\n", setup->deltap);
    if (sediment)
        printf("time_step_sediment: %g\n", setup->deltaw);
    printf("time_coefficient: %g\n", setup->timec);
    printf("iterations_planned: %d\n", setup->miter);
    printf("iterations_completed: %d\n", summary->iterations_completed);
    printf("iterations_per_output: %d\n", setup->iterout);
    printf("stopped_early: %s\n", summary->stopped_early ? "true" : "false");
    printf("elevation_min: %g\n", setup->zmin);
    printf("elevation_max: %g\n", setup->zmax);
    printf("mean_velocity: %g\n", setup->vmean);
    if (sediment) {
        printf("velocity_max: %g\n", setup->vmax);
        printf("sigma_max: %g\n", setup->sigmax);
    }
    if (setup->chmean != 0.0)
        printf("mean_mannings_n: %g\n", 1.0 / setup->chmean);
    else
        printf("mean_mannings_n: undefined\n");
    printf("mean_source_rate: %g\n", setup->si0);
    if (!sediment)
        printf("mean_infiltration: %g\n", setup->infmean);
    printf("threads: %d\n", summary->threads);

    for (i = 0; i < summary->nsteps; i++) {
        const OutputStep *step = &summary->steps[i];

        printf("output:\n");
        printf("  simulated_time: %d\n", step->simulated_time);
        printf("  timestamp: %s\n", step->timestamp);
        printf("  walkers_active: %d\n", step->walkers_active);
        if (sediment) {
            print_map_name("transport_capacity", step->tc);
            print_map_name("tlimit_erosion_deposition", step->et);
            print_map_name("sediment_concentration", step->conc);
            print_map_name("sediment_flux", step->flux);
            print_map_name("erosion_deposition", step->erdep);
        }
        else {
            print_map_name("depth", step->depth);
            print_map_name("discharge", step->disch);
            print_map_name("error", step->err);
        }
        print_map_name("walkers", step->outwalk);
    }
}

void print_summary(SummaryFormat format, const Setup *setup,
                   const Settings *settings, const Simulation *sim,
                   const Inputs *inputs, const Summary *summary)
{
    switch (format) {
    case SUMMARY_JSON:
        print_json(setup, settings, sim, inputs, summary);
        break;
    case SUMMARY_PLAIN:
        print_plain(setup, settings, sim, inputs, summary);
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
        G_free(step->tc);
        G_free(step->et);
        G_free(step->conc);
        G_free(step->flux);
        G_free(step->erdep);
    }
    G_free(summary->steps);
    summary->steps = NULL;
    summary->nsteps = 0;
    summary->nsteps_alloc = 0;
}
