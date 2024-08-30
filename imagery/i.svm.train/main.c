
/****************************************************************************
 *
 * MODULE:       i.svm.train
 * AUTHOR(S):    Maris Nartiss - maris.gis gmail.com
 * PURPOSE:      Trains Support Vector Machine classifier
 *
 * COPYRIGHT:    (C) 2023 by Maris Nartiss and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *               Development of this module was supported from
 *               science funding of University of Latvia (2020-2023).
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>

#include <grass/config.h>
#if HAVE_SVM_H
#include <svm.h>
#elif HAVE_LIBSVM_SVM_H
#include <libsvm/svm.h>
#endif

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

#include "fill.h"

/* LIBSVM message wrapper */
void print_func(const char *s)
{
    G_verbose_message("%s", s);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *opt_group, *opt_subgroup, *opt_sigfile, *opt_labels;
    struct Option *opt_svm_type, *opt_svm_kernel;
    struct Option *opt_svm_cache_size, *opt_svm_degree, *opt_svm_gamma,
        *opt_svm_coef0, *opt_svm_eps, *opt_svm_cost, *opt_svm_nu, *opt_svm_p;
    struct Flag *flag_svm_shrink, *flag_svm_prob;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("svm"));
    G_add_keyword(_("classification"));
    G_add_keyword(_("training"));
    module->label = _("Train a SVM");
    module->description = _("Train a Support Vector Machine");

    opt_group = G_define_standard_option(G_OPT_I_GROUP);
    /* GTC: SVM training input */
    opt_group->description = _("Maps with feature values (attributes)");

    opt_subgroup = G_define_standard_option(G_OPT_I_SUBGROUP);
    opt_subgroup->required = NO;

    opt_labels = G_define_standard_option(G_OPT_R_INPUT);
    opt_labels->key = "trainingmap";
    opt_labels->description = _("Map with training labels or target values");

    opt_sigfile = G_define_option();
    opt_sigfile->key = "signaturefile";
    opt_sigfile->type = TYPE_STRING;
    opt_sigfile->key_desc = "name";
    opt_sigfile->required = YES;
    opt_sigfile->gisprompt = "new,signatures/libsvm,sigfile";
    opt_sigfile->description =
        _("Name for output file containing result signatures");

    opt_svm_type = G_define_option();
    opt_svm_type->key = "type";
    opt_svm_type->type = TYPE_STRING;
    opt_svm_type->key_desc = "name";
    opt_svm_type->required = NO;
    opt_svm_type->options = "c_svc,nu_svc,one_class,epsilon_svr,nu_svr";
    opt_svm_type->answer = "c_svc";
    opt_svm_type->description = _("Type of SVM");
    opt_svm_type->guisection = _("SVM parameters");
    G_asprintf((char **)&(opt_svm_type->descriptions),
               "c_svc;%s;"
               "nu_svc;%s;"
               "one_class;%s;"
               "epsilon_svr;%s;"
               "nu_svr;%s;",
               /* GTC: SVM type */
               _("C-SVM classification"),
               /* GTC: SVM type */
               _("nu-SVM classification"),
               /* GTC: SVM type */
               _("one-class SVM"),
               /* GTC: SVM type */
               _("epsilon-SVM regression"),
               /* GTC: SVM type */
               _("nu-SVM regression"));

    opt_svm_kernel = G_define_option();
    opt_svm_kernel->key = "kernel";
    opt_svm_kernel->type = TYPE_STRING;
    opt_svm_kernel->key_desc = "name";
    opt_svm_kernel->required = NO;
    opt_svm_kernel->options = "linear,poly,rbf,sigmoid";
    opt_svm_kernel->answer = "rbf";
    opt_svm_kernel->description = _("SVM kernel type");
    opt_svm_kernel->guisection = _("SVM parameters");
    G_asprintf((char **)&(opt_svm_kernel->descriptions),
               "linear;%s;"
               "poly;%s;"
               "rbf;%s;"
               "sigmoid;%s;" /* "precomputed;%s;" */,
               /* GTC: SVM kernel type */
               _("u'*v"),
               /* GTC: SVM kernel type */
               _("(gamma*u'*v + coef0)^degree"),
               /* GTC: SVM kernel type */
               _("exp(-gamma*|u-v|^2)"),
               /* GTC: SVM kernel type */
               _("tanh(gamma*u'*v + coef0)"));
    /* TODO: precomputed */

    opt_svm_cache_size = G_define_option();
    opt_svm_cache_size->key = "cache";
    opt_svm_cache_size->type = TYPE_INTEGER;
    opt_svm_cache_size->key_desc = "cache size";
    opt_svm_cache_size->required = NO;
    opt_svm_cache_size->options = "1-";
    opt_svm_cache_size->answer = "512";
    opt_svm_cache_size->description = _("LIBSVM kernel cache size in MB");

    opt_svm_degree = G_define_option();
    opt_svm_degree->key = "degree";
    opt_svm_degree->type = TYPE_INTEGER;
    opt_svm_degree->key_desc = "value";
    opt_svm_degree->required = NO;
    opt_svm_degree->options = "0-";
    opt_svm_degree->answer = "3";
    opt_svm_degree->description = _("Degree in kernel function");
    opt_svm_degree->guisection = _("SVM options");

    opt_svm_gamma = G_define_option();
    opt_svm_gamma->key = "gamma";
    opt_svm_gamma->type = TYPE_DOUBLE;
    opt_svm_gamma->key_desc = "value";
    opt_svm_gamma->required = NO;
    opt_svm_gamma->answer = "1";
    opt_svm_gamma->description = _("Gamma in kernel function");
    opt_svm_gamma->guisection = _("SVM options");

    opt_svm_coef0 = G_define_option();
    opt_svm_coef0->key = "coef0";
    opt_svm_coef0->type = TYPE_DOUBLE;
    opt_svm_coef0->key_desc = "value";
    opt_svm_coef0->required = NO;
    opt_svm_coef0->answer = "0";
    opt_svm_coef0->description = _("coef0 in kernel function");
    opt_svm_coef0->guisection = _("SVM options");

    opt_svm_eps = G_define_option();
    opt_svm_eps->key = "eps";
    opt_svm_eps->type = TYPE_DOUBLE;
    opt_svm_eps->key_desc = "value";
    opt_svm_eps->required = NO;
    /* GTC: SVM epsilon */
    opt_svm_eps->label = _("Tolerance of termination criterion");
    opt_svm_eps->description =
        _("Defaults to 0.00001 for nu-SVC and 0.001 for others");
    opt_svm_eps->guisection = _("SVM options");

    opt_svm_cost = G_define_option();
    opt_svm_cost->key = "cost";
    opt_svm_cost->type = TYPE_DOUBLE;
    opt_svm_cost->key_desc = "value";
    opt_svm_cost->required = NO;
    opt_svm_cost->answer = "1";
    /* GTC: SVM C */
    opt_svm_cost->label = _("Cost of constraints violation");
    opt_svm_cost->description =
        _("The parameter C of C-SVC, epsilon-SVR, and nu-SVR");
    opt_svm_cost->guisection = _("SVM options");

    opt_svm_nu = G_define_option();
    opt_svm_nu->key = "nu";
    opt_svm_nu->type = TYPE_DOUBLE;
    opt_svm_nu->key_desc = "value";
    opt_svm_nu->required = NO;
    opt_svm_nu->answer = "0.5";
    opt_svm_nu->description =
        _("The parameter nu of nu-SVC, one-class SVM, and nu-SVR");
    opt_svm_nu->guisection = _("SVM options");

    opt_svm_p = G_define_option();
    opt_svm_p->key = "p";
    opt_svm_p->type = TYPE_DOUBLE;
    opt_svm_p->key_desc = "value";
    opt_svm_p->required = NO;
    opt_svm_p->answer = "0.1";
    opt_svm_p->description = _("The epsilon in epsilon-insensitive loss "
                               "function of epsilon-SVM regression");
    opt_svm_p->guisection = _("SVM options");

    flag_svm_shrink = G_define_flag();
    flag_svm_shrink->key = 's';
    flag_svm_shrink->label = _("Do not use the shrinking heuristics");
    /* GTC: SVM flag description */
    flag_svm_shrink->description =
        _("Defaults to use the shrinking heuristics");
    flag_svm_shrink->guisection = _("SVM options");

    flag_svm_prob = G_define_flag();
    flag_svm_prob->key = 'p';
    flag_svm_prob->label =
        _("Train a SVC or SVR model for probability estimates");
    /* GTC: SVM flag description */
    flag_svm_prob->description = _("Defaults to no probabilities in model");
    flag_svm_prob->guisection = _("SVM options");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* Input validation */
    /* Input maps */
    char name_group[GNAME_MAX], name_subgroup[GNAME_MAX],
        name_sigfile[GNAME_MAX];
    char mapset_group[GMAPSET_MAX], mapset_subgroup[GMAPSET_MAX],
        mapset_sigfile[GMAPSET_MAX];
    char sigfile_dir[GPATH_MAX];
    char in_path[GPATH_MAX], out_path[GPATH_MAX];
    if (G_unqualified_name(opt_group->answer, NULL, name_group, mapset_group) ==
        0)
        strcpy(mapset_group, G_mapset());
    if (opt_subgroup->answer &&
        G_unqualified_name(opt_subgroup->answer, NULL, name_subgroup,
                           mapset_subgroup) != 0 &&
        strcmp(mapset_subgroup, mapset_group) != 0)
        G_fatal_error(_("Invalid subgroup <%s> provided"),
                      opt_subgroup->answer);
    if (!I_find_group2(name_group, mapset_group)) {
        G_fatal_error(_("Group <%s> not found in mapset <%s>"), name_group,
                      mapset_group);
    }
    if (opt_subgroup->answer &&
        !I_find_subgroup2(name_group, name_subgroup, mapset_group)) {
        G_fatal_error(_("Subgroup <%s> in group <%s@%s> not found"),
                      name_subgroup, name_group, mapset_group);
    }

    const char *mapset_labels;
    char name_labels[GNAME_MAX + GMAPSET_MAX];
    strcpy(name_labels, opt_labels->answer);
    if ((mapset_labels = G_find_raster(name_labels, "")) == NULL) {
        G_fatal_error(_("Raster map <%s> not found"), opt_labels->answer);
    }

    if (G_unqualified_name(opt_sigfile->answer, G_mapset(), name_sigfile,
                           mapset_sigfile) < 0)
        G_fatal_error(_("<%s> does not match the current mapset"),
                      mapset_sigfile);
    if (G_legal_filename(name_sigfile) < 0)
        G_fatal_error(_("<%s> is an illegal file name"), name_sigfile);

    /* Input SVM parameters */
    /* TODO: Implement parameter checking duplicating svm_check_parameter() to
     * generate translatable errors */
    struct svm_parameter parameters;
    parameters.cache_size = atoi(opt_svm_cache_size->answer);
    parameters.degree = atoi(opt_svm_degree->answer);
    parameters.gamma = atof(opt_svm_gamma->answer);
    parameters.coef0 = atof(opt_svm_coef0->answer);
    parameters.C = atof(opt_svm_cost->answer);
    parameters.nu = atof(opt_svm_nu->answer);
    parameters.p = atof(opt_svm_p->answer);

    if (strcmp(opt_svm_type->answer, "c_svc") == 0)
        parameters.svm_type = C_SVC;
    else if (strcmp(opt_svm_type->answer, "nu_svc") == 0)
        parameters.svm_type = NU_SVC;
    else if (strcmp(opt_svm_type->answer, "one_class") == 0)
        parameters.svm_type = ONE_CLASS;
    else if (strcmp(opt_svm_type->answer, "epsilon_svr") == 0)
        parameters.svm_type = EPSILON_SVR;
    else if (strcmp(opt_svm_type->answer, "nu_svr") == 0)
        parameters.svm_type = NU_SVR;
    else
        G_fatal_error(_("Wrong SVM type"));

    if (strcmp(opt_svm_kernel->answer, "linear") == 0)
        parameters.kernel_type = LINEAR;
    else if (strcmp(opt_svm_kernel->answer, "poly") == 0)
        parameters.kernel_type = POLY;
    else if (strcmp(opt_svm_kernel->answer, "rbf") == 0)
        parameters.kernel_type = RBF;
    else if (strcmp(opt_svm_kernel->answer, "sigmoid") == 0)
        parameters.kernel_type = SIGMOID;
    else if (strcmp(opt_svm_kernel->answer, "precomputed") == 0)
        parameters.kernel_type = PRECOMPUTED;
    else
        G_fatal_error(_("Wrong kernel type"));

    if (opt_svm_eps->answer)
        parameters.eps = atof(opt_svm_eps->answer);
    else {
        if (parameters.svm_type == NU_SVC)
            parameters.eps = 0.00001;
        else
            parameters.eps = 0.001;
    }

    if (flag_svm_shrink->answer)
        parameters.shrinking = 0;
    else
        parameters.shrinking = 1;

    if (flag_svm_prob->answer)
        parameters.probability = 1;
    else
        parameters.probability = 0;

    /* TODO: implement weight support */
    parameters.nr_weight = 0;

    /* Get bands */
    struct Ref group_ref;
    if (opt_subgroup->answer) {
        if (!I_get_subgroup_ref2(name_group, opt_subgroup->answer, mapset_group,
                                 &group_ref)) {
            G_fatal_error(
                _("There was an error reading subgroup <%s> in group <%s@%s>"),
                opt_subgroup->answer, name_group, mapset_group);
        }
    }
    else {
        if (!I_get_group_ref2(name_group, mapset_group, &group_ref)) {
            G_fatal_error(_("There was an error reading group <%s@%s>"),
                          name_group, mapset_group);
        }
    }
    if (group_ref.nfiles <= 0) {
        if (opt_subgroup->answer)
            G_fatal_error(
                _("Subgroup <%s> in group <%s@%s> contains no raster maps."),
                opt_subgroup->answer, name_group, mapset_group);
        else
            G_fatal_error(_("Group <%s@%s> contains no raster maps."),
                          name_group, mapset_group);
    }
    const char **semantic_labels = G_malloc(group_ref.nfiles * sizeof(char *));

    /* Precompute values for mean normalization */
    DCELL *means, *ranges;
    means = G_malloc(group_ref.nfiles * sizeof(DCELL));
    ranges = G_malloc(group_ref.nfiles * sizeof(DCELL));
    for (int n = 0; n < group_ref.nfiles; n++) {
        struct Range crange;
        struct FPRange fprange;
        int cmin, cmax;
        double dmin, dmax;
        int ret;
        semantic_labels[n] = Rast_get_semantic_label_or_name(
            group_ref.file[n].name, group_ref.file[n].mapset);
        /* Use raster range for value rescaling */
        ret = Rast_read_range(group_ref.file[n].name, group_ref.file[n].mapset,
                              &crange);
        if (ret == 1) {
            Rast_get_range_min_max(&crange, &cmin, &cmax);
            /* Calculate mean without a risk of integer overflow */
            means[n] =
                (cmin / 2.0) + (cmax / 2.0) + ((cmin % 2 + cmax % 2) / 2.0);
            ranges[n] = (cmax - cmin) / 2.0;
        }
        else if (ret == 3) {
            ret = Rast_read_fp_range(group_ref.file[n].name,
                                     group_ref.file[n].mapset, &fprange);
            if (ret != 1) {
                G_fatal_error(
                    _("Unable to get value range for raster map <%s@%s>"),
                    group_ref.file[n].name, group_ref.file[n].mapset);
            }
            Rast_get_fp_range_min_max(&fprange, &dmin, &dmax);
            means[n] = (dmin + dmax) / 2.0;
            ranges[n] = (dmax - dmin) / 2.0;
        }
        else {
            G_fatal_error(_("Unable to get value range for raster map <%s@%s>"),
                          group_ref.file[n].name, group_ref.file[n].mapset);
        }
        if (ranges[n] < GRASS_EPSILON) {
            G_fatal_error(_("Invalid value range for raster map <%s@%s>"),
                          group_ref.file[n].name, group_ref.file[n].mapset);
        }
    }

    /* Pass LIBSVM messages through GRASS */
    svm_set_print_string_function(&print_func);

    /* Fill svm_problem struct with training data */
    struct svm_problem problem;
    G_message(_("Reading training data"));
    fill_problem(name_labels, mapset_labels, group_ref, means, ranges,
                 &problem);

    /* svm_check_parameter needs filled svm_problem struct thus checking only
     * now */
    G_verbose_message("Checking SVM parametrization");
    const char *parameters_error = svm_check_parameter(&problem, &parameters);
    if (parameters_error)
        G_fatal_error(_("SVM parameter validation returned an error: %s\n"),
                      parameters_error);

    /* Train model. Might take some time. */
    struct svm_model *model;
    G_message(_("Starting training process (it will take some time; "
                "no progress is printed, be patient)"));
    model = svm_train(&problem, &parameters);

    /* Write out training results */
    G_verbose_message("Writing out trained SVM");
    /* This is a specific case as file is not written by GRASS but
       by LIBSVM and thus "normal" GRASS lib functions can not be used. */
    I_make_signatures_dir(I_SIGFILE_TYPE_LIBSVM);
    I_get_signatures_dir(sigfile_dir, I_SIGFILE_TYPE_LIBSVM);
    /* G_fopen_new_misc should create a directory for later use */
    FILE *misc_file = G_fopen_new_misc(sigfile_dir, "version", name_sigfile);
    if (!misc_file)
        G_fatal_error(_("Unable to write trained model to file '%s'."),
                      name_sigfile);
    fprintf(misc_file, "1\n");
    fclose(misc_file);

    /* Write out SVM values in a signature file */
    G_file_name_misc(out_path, sigfile_dir, "sig", name_sigfile, G_mapset());
    int out_status = svm_save_model(out_path, model);
    if (out_status != 0) {
        G_fatal_error(
            _("Unable to write trained model to file '%s'. Error code: %d"),
            out_path, out_status);
    }
    svm_free_and_destroy_model(&model);
    /* Write out semantic label info */
    misc_file = G_fopen_new_misc(sigfile_dir, "semantic_label", name_sigfile);
    if (!misc_file)
        G_fatal_error(_("Unable to write trained model to file '%s'."),
                      name_sigfile);
    for (int n = 0; n < group_ref.nfiles; n++) {
        fprintf(misc_file, "%s\n", semantic_labels[n]);
    }
    fclose(misc_file);
    G_free(semantic_labels);

    /* Write out rescaling value as the same value has to be used for prediction
     */
    misc_file = G_fopen_new_misc(sigfile_dir, "scale", name_sigfile);
    if (!misc_file)
        G_fatal_error(_("Unable to write trained model to file '%s'."),
                      name_sigfile);
    for (int n = 0; n < group_ref.nfiles; n++) {
        fprintf(misc_file, "%lf %lf\n", means[n], ranges[n]);
    }
    fclose(misc_file);
    G_free(means);
    G_free(ranges);

    /* Copy CATs file. Will be used for prediction result maps */
    struct Categories cats;
    G_verbose_message("Copying category information");
    if (Rast_read_cats(name_labels, mapset_labels, &cats) == 0) {
        /* Path to training label map CATs file */
        G_file_name(in_path, "cats", name_labels, mapset_labels);
        G_file_name_misc(out_path, sigfile_dir, "cats", name_sigfile,
                         G_mapset());
        /* It is OK to call G_copy if source file doesn't exist */
        G_copy_file(in_path, out_path);
    }

    /* Copy color file. Will be used for prediction result maps */
    G_verbose_message("Copying colour information");
    if (G_find_file2("colr", name_labels, mapset_labels)) {
        /* Path to training label map colr file */
        G_file_name(in_path, "colr", name_labels, mapset_labels);
        G_file_name_misc(out_path, sigfile_dir, "colr", name_sigfile,
                         G_mapset());
        /* It is OK to call G_copy if source file doesn't exist */
        G_copy_file(in_path, out_path);
    }

    /* History will be appended to a prediction result map history */
    struct History history;
    G_verbose_message("Writing out history");
    misc_file = G_fopen_new_misc(sigfile_dir, "history", name_sigfile);
    if (misc_file != NULL) {
        G_zero(&history, sizeof(struct History));
        /* Rast_command_history performs command wrapping */
        Rast_command_history(&history);
        for (int i = 0; i < history.nlines; i++)
            fprintf(misc_file, "%s\n", history.lines[i]);
        fclose(misc_file);
    }
    else {
        G_warning(_("Unable to write history information for <%s>"),
                  name_sigfile);
    }

    G_message(_("Training successfully complete"));
    exit(EXIT_SUCCESS);
}
