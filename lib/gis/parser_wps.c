
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#if defined(HAVE_LANGINFO_H)
#include <langinfo.h>
#endif
#if defined(__MINGW32__) && defined(USE_NLS)
#include <localcharset.h>
#endif

#include "parser_local_proto.h"

/* Defines and prototypes for WPS process_description XML document generation
 */
#define TYPE_OTHER -1
#define TYPE_RASTER 0
#define TYPE_VECTOR 1
#define TYPE_PLAIN_TEXT 2
#define TYPE_RANGE 3
#define TYPE_LIST 4
#define TYPE_STDS 5 /* Space time datasets of type raster, raster3d and vector */
#define TYPE_STRDS 6 /* Space time raster datasets */
#define TYPE_STVDS 7 /* Space time vector datasets */
#define WPS_INPUT 0
#define WPS_OUTPUT 1

static void wps_print_mimetype_text_plain(void);
static void wps_print_mimetype_raster_tiff(void);
static void wps_print_mimetype_raster_tiff_other(void);
static void wps_print_mimetype_raster_png(void);
static void wps_print_mimetype_raster_gif(void);
static void wps_print_mimetype_raster_jpeg(void);
static void wps_print_mimetype_raster_hfa(void);
static void wps_print_mimetype_raster_netCDF(void);
static void wps_print_mimetype_raster_netCDF_other(void);
static void wps_print_mimetype_raster_grass_binary(void);
static void wps_print_mimetype_raster_grass_ascii(void);
static void wps_print_mimetype_vector_gml311(void);
static void wps_print_mimetype_vector_gml311_appl(void);
static void wps_print_mimetype_vector_gml212(void);
static void wps_print_mimetype_vector_gml212_appl(void);
static void wps_print_mimetype_vector_kml22(void);
static void wps_print_mimetype_vector_dgn(void);
static void wps_print_mimetype_vector_shape(void);
static void wps_print_mimetype_vector_zipped_shape(void);
static void wps_print_mimetype_vector_grass_ascii(void);
static void wps_print_mimetype_vector_grass_binary(void);
static void wps_print_mimetype_space_time_datasets(void);
static void wps_print_mimetype_space_time_raster_datasets(void);
static void wps_print_mimetype_space_time_vector_datasets(void);
static void wps_print_mimetype_space_time_vector_datasets_tar(void);
static void wps_print_mimetype_space_time_raster_datasets_tar(void);
static void wps_print_mimetype_space_time_vector_datasets_tar_gz(void);
static void wps_print_mimetype_space_time_raster_datasets_tar_gz(void);
static void wps_print_mimetype_space_time_vector_datasets_tar_bz2(void);
static void wps_print_mimetype_space_time_raster_datasets_tar_bz2(void);

static void wps_print_process_descriptions_begin(void);
static void wps_print_process_descriptions_end(void);
static void wps_print_process_description_begin(int , int , const char *, const char *, const char *, const char **, int );
static void wps_print_process_description_end(void);
static void wps_print_data_inputs_begin(void);
static void wps_print_data_inputs_end(void);
static void wps_print_process_outputs_begin(void);
static void wps_print_process_outputs_end(void);
static void wps_print_bounding_box_data(void);
static void wps_print_ident_title_abstract(const char *, const char *, const char *);
static void wps_print_complex_input(int , int , const char *, const char *, const char *, int , int );
static void wps_print_complex_output(const char *, const char *, const char *, int );
static void wps_print_comlpex_input_output(int , int , int , const char *, const char *, const char *, int , int );
static void wps_print_literal_input_output(int , int , int , const char *,
                                const char *, const char *, const char *, int ,
                                const char **, int , const char *, int );

static void print_escaped_for_xml(FILE * fp, const char *str)
{
    for (; *str; str++) {
	switch (*str) {
	case '&':
	    fputs("&amp;", fp);
	    break;
	case '<':
	    fputs("&lt;", fp);
	    break;
	case '>':
	    fputs("&gt;", fp);
	    break;
	default:
	    fputc(*str, fp);
	}
    }
}

/*!
 * \brief Print the WPS 1.0.0 process description XML document to stdout
 *
 * A module started with the parameter "--wps-process-description"
 * will write a process description XML document to stdout and exit.
 *
 * Currently only raster and vector modules are supported, but the
 * generation works with any module (more or less meaningful).
 * Most of the input options are caught:
 * * single and multiple raster and vector maps
 * * single and multiple string, float and integer data with default
 * values and value options (range is missing)
 * Flags are supported as boolean values.
 *
 * The mime types for vector maps are GML, KML, dgn, shape and zipped shape. 
 *
 * The mime types for raster maps are tiff, geotiff, hfa, netcdf, gif, jpeg and png.
 *
 * Mime types for space time datasets are tar archives with gz, bzip or without compression
 *
 * The mime types are reflecting the capabilities of grass and gdal and may be extended.
 *
 * BoundignBox support is currently not available for inputs and outputs.
 * Literal data output (string, float or integer)  is currently not supported.
 *
 * In case no output parameter was set (new raster of vector map) the stdout output
 * is noticed as output parameter of mime type text/plain.
 *
 * Multiple vector or raster map outputs marked as one option are not supported (wps 1.0.0 specification
 * does not allow multiple outputs with only one identifier).
 * Multiple outputs must be wrapped via a python script or created as group.
 *
 * In future the following mimetypes may be supported
 * mime type: application/grass-vector-ascii  -> a text file generated with v.out.asci
 * Example.: urn:file:///path/name
 * mime type: application/grass-vector-binary -> the binary vectors must be addressed with a non standard urn:
 * Example: urn:grass:vector:location/mapset/name
 * */

void G__wps_print_process_description(void)
{
    struct Option *opt;
    struct Flag *flag;
    char *type;
    char *s, *top;
    const char *value = NULL;
    int i;
    const char *encoding;
    int new_prompt = 0;
    int store = 1;
    int status = 1;
    const char *identifier = NULL;
    const char *title = NULL;
    const char *abstract = NULL;
    const char **keywords = NULL;
    int data_type, is_input, is_output;
    int num_raster_inputs = 0, num_raster_outputs = 0;
    int num_vector_inputs = 0, num_vector_outputs = 0;
    int num_strds_inputs = 0, num_strds_outputs = 0;
    int num_stvds_inputs = 0, num_stvds_outputs = 0;
    int min = 0, max = 0;
    int num_keywords = 0;
    int found_output = 0;
    int is_tuple; /* Checks the key_descr for comma separated values */
    int num_tuples; /* Counts the "," in key_descr */

    new_prompt = G__uses_new_gisprompt();

    /* gettext converts strings to encoding returned by nl_langinfo(CODESET) */

#if defined(HAVE_LANGINFO_H)
    encoding = nl_langinfo(CODESET);
    if (!encoding || strlen(encoding) == 0) {
	encoding = "UTF-8";
    }
#elif defined(__MINGW32__) && defined(USE_NLS)
    encoding = locale_charset();
    if (!encoding || strlen(encoding) == 0) {
	encoding = "UTF-8";
    }
#else
    encoding = "UTF-8";
#endif

    if (!st->pgm_name)
	st->pgm_name = G_program_name();
    if (!st->pgm_name)
	st->pgm_name = "??";

    /* the identifier of the process is the module name */
    identifier = st->pgm_name;

    if (st->module_info.description) {
        title = st->module_info.description;
        abstract = st->module_info.description;
    }

    if (st->module_info.keywords) {
        keywords = st->module_info.keywords;
        num_keywords = st->n_keys;
    }

    wps_print_process_descriptions_begin();
    /* store and status are supported as default. The WPS server should change this if necessary */
    wps_print_process_description_begin(store, status, identifier, title, abstract, keywords, num_keywords);
    wps_print_data_inputs_begin();

    /* Print the bounding box element with all the coordinate reference systems, which are supported by grass*/
    /* Currently Disabled! A list of all proj4 supported EPSG coordinate reference systems must be implemented*/
    if(1 == 0)
        wps_print_bounding_box_data();

    /* We parse only the inputs at the beginning */
    if (st->n_opts) {
	opt = &st->first_option;
	while (opt != NULL) {

            identifier = NULL;
            title = NULL;
            abstract = NULL;
            keywords = NULL;
            num_keywords = 0;
            value = NULL;
            is_input = 1;
            is_output = 0;
	    is_tuple = 0;
	    num_tuples = 0;
            data_type = TYPE_OTHER;

	    /* Check the gisprompt */
	    if (opt->gisprompt) {
		const char *atts[] = { "age", "element", "prompt", NULL };
		top = G_calloc(strlen(opt->gisprompt) + 1, 1);
		strcpy(top, opt->gisprompt);
		s = strtok(top, ",");
		for (i = 0; s != NULL && atts[i] != NULL; i++) {

                    char *token = G_store(s);

                    /* we print only input parameter, sort out the output parameter */
                    if(strcmp(token, "new") == 0) {
                        is_input = 0;
                        is_output = 1;
                    }
                    if(strcmp(token, "raster") == 0)
                    {
                        data_type = TYPE_RASTER;
                        /* Count the raster inputs and outputs for default option creation */
                        if(is_input == 1)
                            num_raster_inputs++;
                        if(is_output == 1)
                            num_raster_outputs++;
                    }
                    if(strcmp(token, "vector") == 0)
                    {
                        data_type = TYPE_VECTOR;
			if(is_input == 1)
                            num_vector_inputs++;
                        if(is_output == 1)
                            num_vector_outputs++;
                    }
		    /* Modules may have different types of space time datasets as inputs */
                    if(strcmp(token, "stds") == 0)
                    {
                        data_type = TYPE_STDS;
                    }
                    if(strcmp(token, "strds") == 0)
                    {
                        data_type = TYPE_STRDS;
                        if(is_input == 1)
                            num_strds_inputs++;
                        if(is_output == 1)
                            num_strds_outputs++;
                    }
                    if(strcmp(token, "stvds") == 0)
                    {
                        data_type = TYPE_STVDS;
                        if(is_input == 1)
                            num_stvds_inputs++;
                        if(is_output == 1)
                            num_stvds_outputs++;
                    }
                    if(strcmp(token, "file") == 0)
                    {
                        data_type = TYPE_PLAIN_TEXT;
                    }
                    s = strtok(NULL, ",");
                    G_free(token);
		}
		G_free(top);
	    }

	    /* Check the key description */
	    if (opt->key_desc) {
		top = G_calloc(strlen(opt->key_desc) + 1, 1);
		strcpy(top, opt->key_desc);
		s = strtok(top, ",");
		/* Count comma's */
                for (i = 0; s != NULL; i++) {
                    num_tuples++;
                    s = strtok(NULL, ",");
		}
                if(num_tuples > 1)
                    is_tuple = 1;
                
		G_free(top);
	    }
            /* We have an input option */
            if(is_input == 1)
            {
                switch (opt->type) {
                case TYPE_INTEGER:
                    type = "integer";
                    break;
                case TYPE_DOUBLE:
                    type = "float";
                    break;
                case TYPE_STRING:
                    type = "string";
                    break;
                default:
                    type = "string";
                    break;
                }

                identifier = opt->key;

                if(opt->required == YES) {
                    if(is_tuple)
                        min = num_tuples;
                    else
                        min = 1;
                } else {
                    min = 0;
                }

                if(opt->multiple == YES) {
                    max = 1024;
                } else {
                    if(is_tuple)
                        max = num_tuples;
                    else
                        max = 1;
                }
                
                if(opt->label) {
                    title = opt->label;
		}
                if (opt->description) {
		    if(!opt->label)
			title = opt->description;
		    else
			abstract = opt->description;
                }
                if (opt->def) {
                    value = opt->def;
                }
                if (opt->options) {
                    /* TODO:
                     * add something like
                     *       <range min="xxx" max="xxx"/>
                     * to <values> */
                    i = 0;
                    while (opt->opts[i]) {
                        i++;
                    }
                    keywords = opt->opts;
                    num_keywords = i;
                }
                if(data_type == TYPE_RASTER || data_type == TYPE_VECTOR || 
		   data_type == TYPE_STRDS  || data_type == TYPE_STVDS  || 
		   data_type == TYPE_STDS || data_type == TYPE_PLAIN_TEXT)
                {
                    /* 2048 is the maximum size of the map in mega bytes */
                    wps_print_complex_input(min, max, identifier, title, abstract, 2048, data_type);
                }
                else
                {
                    /* The keyword array is missused for options, type means the type of the value (integer, float ... )*/
                    wps_print_literal_input_output(WPS_INPUT, min, max, identifier, title, 
						    abstract, type, 0, keywords, num_keywords, value, TYPE_OTHER);
                }
            }
	    opt = opt->next_opt;
	}
    }

    /* Flags are always input options and can be false or true (boolean) */
    if (st->n_flags) {
	flag = &st->first_flag;
	while (flag != NULL) {

            /* The identifier is the flag "-x" */
            char* ident = (char*)G_calloc(3, sizeof(char));
            ident[0] = '-';
            ident[1] = flag->key;
            ident[2] = '\0';
            title = NULL;
            abstract = NULL;

	    if (flag->description) {
                title = flag->description;
                abstract = flag->description;
	    }
            const char *val[] = {"true","false"};
            wps_print_literal_input_output(WPS_INPUT, 0, 1, ident, title, NULL, "boolean", 0, val, 2, "false", TYPE_OTHER);
	    flag = flag->next_flag;
	}
    }

    /* We have two default options, which define the resolution of the created mapset */
    if(num_raster_inputs > 0 || num_raster_outputs > 0 || num_strds_inputs > 0 || num_strds_outputs > 0) {
        wps_print_literal_input_output(WPS_INPUT, 0, 1, "grass_resolution_ns", "Resolution of the mapset in north-south direction in meters or degrees",
            "This parameter defines the north-south resolution of the mapset in meter or degrees, which should be used to process the input and output raster data. To enable this setting, you need to specify north-south and east-west resolution.",
            "float", 1, NULL, 0, NULL, TYPE_OTHER);
        wps_print_literal_input_output(WPS_INPUT, 0, 1, "grass_resolution_ew", "Resolution of the mapset in east-west direction in meters or degrees",
            "This parameter defines the east-west resolution of the mapset in meters or degrees, which should be used to process the input and output raster data.  To enable this setting, you need to specify north-south and east-west resolution.",
            "float", 1, NULL, 0, NULL, TYPE_OTHER);
    }
    /* In case multi band raster maps should be imported, the band number must be provided */
    if(num_raster_inputs > 0)
        wps_print_literal_input_output(WPS_INPUT, 0, 1, "grass_band_number", "Band to select for processing (default is all bands)",
            "This parameter defines band number of the input raster files which should be processed. As default all bands are processed and used as single and multiple inputs for raster modules.",
            "integer", 0, NULL, 0, NULL, TYPE_OTHER);

    /* End of inputs */
    wps_print_data_inputs_end();
    /* Start of the outputs */
    wps_print_process_outputs_begin();

    found_output = 0;

    /*parse the output. only raster maps, vector maps, space time raster and vector datasets plus stdout are supported */
    if (st->n_opts) {
	opt = &st->first_option;
	while (opt != NULL) {

            identifier = NULL;
            title = NULL;
            abstract = NULL;
            value = NULL;
            is_output = 0;
            data_type = TYPE_OTHER;

	    if (opt->gisprompt) {
		const char *atts[] = { "age", "element", "prompt", NULL };
		top = G_calloc(strlen(opt->gisprompt) + 1, 1);
		strcpy(top, opt->gisprompt);
		s = strtok(top, ",");
		for (i = 0; s != NULL && atts[i] != NULL; i++) {

                    char *token = G_store(s);

                    /* we print only the output parameter */
                    if(strcmp(token, "new") == 0)
                        is_output = 1;
                    if(strcmp(token, "raster") == 0)
                    {
                        data_type = TYPE_RASTER;
                    }
                    if(strcmp(token, "vector") == 0)
                    {
                        data_type = TYPE_VECTOR;
                    }
                    if(strcmp(token, "stds") == 0)
                    {
                        data_type = TYPE_STDS;
                    }
                    if(strcmp(token, "strds") == 0)
                    {
                        data_type = TYPE_STRDS;
                    }
                    if(strcmp(token, "stvds") == 0)
                    {
                        data_type = TYPE_STVDS;
                    }
                    if(strcmp(token, "file") == 0)
                    {
                        data_type = TYPE_PLAIN_TEXT;
                    }
                    s = strtok(NULL, ",");
                    G_free(token);
		}
		G_free(top);
	    }
            /* Only single module output is supported!! */
            if(is_output == 1)
            {
		if(opt->multiple == YES)
		    G_warning(_("Multiple outputs are not supported by WPS 1.0.0"));
                identifier = opt->key;
 
                if(opt->label) {
                    title = opt->label;
		}
                if (opt->description) {
		    if(!opt->label)
			title = opt->description;
		    else
			abstract = opt->description;
		}

                if(data_type == TYPE_RASTER || data_type == TYPE_VECTOR || 
		   data_type == TYPE_STRDS  || data_type == TYPE_STVDS || 
		   data_type == TYPE_STDS  || data_type == TYPE_PLAIN_TEXT) {
                    wps_print_complex_output(identifier, title, abstract, data_type);
                    found_output = 1;
                }
            }
	    opt = opt->next_opt;
	}
        /* we assume the computatuon output on stdout, if no raster/vector output was found*/
        if(found_output == 0)
            wps_print_complex_output("stdout", "Module output on stdout", "The output of the module written to stdout", TYPE_PLAIN_TEXT);
    }

    wps_print_process_outputs_end();
    wps_print_process_description_end();
    wps_print_process_descriptions_end();
}


/**************************************************************************
 *
 * The remaining routines are all local (static) routines used to support
 * the the creation of the WPS process_description document.
 *
 **************************************************************************/

static void wps_print_process_descriptions_begin(void)
{
    fprintf(stdout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(stdout, "<wps:ProcessDescriptions xmlns:wps=\"http://www.opengis.net/wps/1.0.0\"\n");
    fprintf(stdout, "xmlns:ows=\"http://www.opengis.net/ows/1.1\"\n");
    fprintf(stdout, "xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
    fprintf(stdout, "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
    fprintf(stdout, "xsi:schemaLocation=\"http://www.opengis.net/wps/1.0.0\n http://schemas.opengis.net/wps/1.0.0/wpsDescribeProcess_response.xsd\"\n service=\"WPS\" version=\"1.0.0\" xml:lang=\"en-US\"> \n");
}

/* ************************************************************************** */

static void wps_print_process_descriptions_end(void)
{
    fprintf(stdout,"</wps:ProcessDescriptions>\n");
}

/* ************************************************************************** */

static void wps_print_process_description_begin(int store, int status, const char *identifier,
                                               const char *title, const char *abstract,
                                               const char **keywords, int num_keywords)
{
    int i;

    fprintf(stdout,"\t<ProcessDescription wps:processVersion=\"1\" storeSupported=\"%s\" statusSupported=\"%s\">\n", (store?"true":"false"), (status?"true":"false"));
    if(identifier)
    {
        fprintf(stdout,"\t\t<ows:Identifier>");
        print_escaped_for_xml(stdout, identifier);
        fprintf(stdout,"</ows:Identifier>\n");
    } else {
	G_fatal_error("Identifier not defined");
    }

    if(title)
    {
        fprintf(stdout,"\t\t<ows:Title>");
        print_escaped_for_xml(stdout, title);
        fprintf(stdout, "</ows:Title>\n");
    } else {
	G_warning("Title not defined!");
        fprintf(stdout,"\t\t<ows:Title>");
        print_escaped_for_xml(stdout, "No title available");
        fprintf(stdout, "</ows:Title>\n");
    }


    if(abstract)
    {
        fprintf(stdout,"\t\t<ows:Abstract>");
        fprintf(stdout, "https://grass.osgeo.org/grass78/manuals/%s.html", identifier);
        fprintf(stdout, "</ows:Abstract>\n");
    }

    for(i = 0; i < num_keywords; i++)
    {
        fprintf(stdout,"\t\t<ows:Metadata xlink:title=\"");
        print_escaped_for_xml(stdout, keywords[i]);
        fprintf(stdout, "\" />\n");
    }
}

/* ************************************************************************** */

static void wps_print_process_description_end(void)
{
    fprintf(stdout,"\t</ProcessDescription>\n");
}

/* ************************************************************************** */

static void wps_print_data_inputs_begin(void)
{
    fprintf(stdout,"\t\t<DataInputs>\n");
}

/* ************************************************************************** */

static void wps_print_data_inputs_end(void)
{
    fprintf(stdout,"\t\t</DataInputs>\n");
}

/* ************************************************************************** */

static void wps_print_process_outputs_begin(void)
{
    fprintf(stdout,"\t\t<ProcessOutputs>\n");
}

/* ************************************************************************** */

static void wps_print_process_outputs_end(void)
{
    fprintf(stdout,"\t\t</ProcessOutputs>\n");
}

/* ************************************************************************** */

static void wps_print_complex_input(int min, int max, const char *identifier, const char *title, const char *abstract, int megs, int type)
{
    wps_print_comlpex_input_output(WPS_INPUT, min, max, identifier, title, abstract, megs, type);
}

/* ************************************************************************** */

static void wps_print_complex_output(const char *identifier, const char *title, const char *abstract, int type)
{
    wps_print_comlpex_input_output(WPS_OUTPUT, 0, 0, identifier, title, abstract, 0, type);
}

/* ************************************************************************** */

static void wps_print_comlpex_input_output(int inout_type, int min, int max, const char *identifier, const char *title, const char *abstract, int megs, int type)
{
    if(inout_type == WPS_INPUT)
        fprintf(stdout,"\t\t\t<Input minOccurs=\"%i\" maxOccurs=\"%i\">\n", min, max);
    else if(inout_type == WPS_OUTPUT)
        fprintf(stdout,"\t\t\t<Output>\n");

    wps_print_ident_title_abstract(identifier, title, abstract);

    if(inout_type == WPS_INPUT)
        fprintf(stdout,"\t\t\t\t<ComplexData maximumMegabytes=\"%i\">\n", megs);
    else if(inout_type == WPS_OUTPUT)
        fprintf(stdout,"\t\t\t\t<ComplexOutput>\n");

    fprintf(stdout,"\t\t\t\t\t<Default>\n");
    if(type == TYPE_RASTER)
    {
            wps_print_mimetype_raster_tiff();
    }
    else if(type == TYPE_VECTOR)
    {
            wps_print_mimetype_vector_gml311();
    }
    else if(type == TYPE_STDS)
    {
	    /* A space time raster dataset is the default an any modules with multiple dataset options */
            wps_print_mimetype_space_time_raster_datasets_tar_gz();
    }
    else if(type == TYPE_STRDS)
    {
            wps_print_mimetype_space_time_raster_datasets_tar_gz();
    }
    else if(type == TYPE_STVDS)
    {
            wps_print_mimetype_space_time_vector_datasets_tar_gz();
    }
    else if(type == TYPE_PLAIN_TEXT)
    {
            wps_print_mimetype_text_plain();
    }
    fprintf(stdout,"\t\t\t\t\t</Default>\n");
    fprintf(stdout,"\t\t\t\t\t<Supported>\n");
    if(type == TYPE_RASTER)
    {
	    /*The supported types for input and output are different*/
            if(inout_type == WPS_INPUT) {
            	wps_print_mimetype_raster_tiff();
            	wps_print_mimetype_raster_tiff_other();
            	wps_print_mimetype_raster_png();
            	wps_print_mimetype_raster_gif();
            	wps_print_mimetype_raster_jpeg();
            	wps_print_mimetype_raster_hfa();
            	wps_print_mimetype_raster_netCDF();
            	wps_print_mimetype_raster_netCDF_other();
	    } else {
            	wps_print_mimetype_raster_tiff();
            	wps_print_mimetype_raster_tiff_other();
		wps_print_mimetype_raster_hfa();
            	wps_print_mimetype_raster_netCDF();
            	wps_print_mimetype_raster_netCDF_other();
	    }
    }
    else if(type == TYPE_VECTOR)
    {
            if(inout_type == WPS_INPUT) {
            	wps_print_mimetype_vector_gml311();
                wps_print_mimetype_vector_gml311_appl();
                wps_print_mimetype_vector_gml212();
                wps_print_mimetype_vector_gml212_appl();
            	wps_print_mimetype_vector_kml22();
            	wps_print_mimetype_vector_dgn();
            	wps_print_mimetype_vector_shape();
            	wps_print_mimetype_vector_zipped_shape();
	    } else {
            	wps_print_mimetype_vector_gml311();
                wps_print_mimetype_vector_gml311_appl();
                wps_print_mimetype_vector_gml212();
                wps_print_mimetype_vector_gml212_appl();
            	wps_print_mimetype_vector_kml22();
	    }
    }
    else if(type == TYPE_STDS)
    {
            wps_print_mimetype_space_time_datasets();
    }
    else if(type == TYPE_STRDS)
    {
            wps_print_mimetype_space_time_raster_datasets();
    }
    else if(type == TYPE_STVDS)
    {
            wps_print_mimetype_space_time_vector_datasets();
    }
    else if(type == TYPE_PLAIN_TEXT)
    {
            wps_print_mimetype_text_plain();
    }
    fprintf(stdout,"\t\t\t\t\t</Supported>\n");

    if(inout_type == WPS_INPUT)
        fprintf(stdout,"\t\t\t\t</ComplexData>\n");
    else if(inout_type == WPS_OUTPUT)
        fprintf(stdout,"\t\t\t\t</ComplexOutput>\n");

    if(inout_type == WPS_INPUT)
        fprintf(stdout,"\t\t\t</Input>\n");
    else if(inout_type == WPS_OUTPUT)
        fprintf(stdout,"\t\t\t</Output>\n");
}

/* ************************************************************************** */

static void wps_print_ident_title_abstract(const char *identifier, const char *title, const char *abstract)
{
    if(identifier)
    {
        fprintf(stdout,"\t\t\t\t<ows:Identifier>");
        print_escaped_for_xml(stdout, identifier);
        fprintf(stdout,"</ows:Identifier>\n");
    } else {
	G_fatal_error("Identifier not defined");
    }

    if(title)
    {
        fprintf(stdout,"\t\t\t\t<ows:Title>");
        print_escaped_for_xml(stdout, title);
        fprintf(stdout, "</ows:Title>\n");
    } else {
	G_warning("Title not defined!");
        fprintf(stdout,"\t\t\t\t<ows:Title>");
        print_escaped_for_xml(stdout, "No title available");
        fprintf(stdout, "</ows:Title>\n");
    }

    if(abstract)
    {
        fprintf(stdout,"\t\t\t\t<ows:Abstract>");
        print_escaped_for_xml(stdout, abstract);
        fprintf(stdout, "</ows:Abstract>\n");
    }
}

/* ************************************************************************** */

static void wps_print_literal_input_output(int inout_type, int min, int max, const char *identifier,
                                const char *title, const char *abstract, const char *datatype, int unitofmesure,
                                const char **choices, int num_choices, const char *default_value, int type)
{
    int i;
    char range[2][24];
    char *str;

    if(inout_type == WPS_INPUT)
        fprintf(stdout,"\t\t\t<Input minOccurs=\"%i\" maxOccurs=\"%i\">\n", min, max);
    else if(inout_type == WPS_OUTPUT)
        fprintf(stdout,"\t\t\t<Output>\n");

    wps_print_ident_title_abstract(identifier, title, abstract);

    fprintf(stdout,"\t\t\t\t<LiteralData>\n");

    if(datatype)
        fprintf(stdout,"\t\t\t\t\t<ows:DataType ows:reference=\"xs:%s\">%s</ows:DataType>\n", datatype, datatype);

    if(unitofmesure)
    {
        fprintf(stdout,"\t\t\t\t\t<UOMs>\n");
        fprintf(stdout,"\t\t\t\t\t\t<Default>\n");
        fprintf(stdout,"\t\t\t\t\t\t\t<ows:UOM>meters</ows:UOM>\n");
        fprintf(stdout,"\t\t\t\t\t\t</Default>\n");
        fprintf(stdout,"\t\t\t\t\t\t<Supported>\n");
        fprintf(stdout,"\t\t\t\t\t\t\t<ows:UOM>meters</ows:UOM>\n");
        fprintf(stdout,"\t\t\t\t\t\t\t<ows:UOM>degrees</ows:UOM>\n");
        fprintf(stdout,"\t\t\t\t\t\t</Supported>\n");
        fprintf(stdout,"\t\t\t\t\t</UOMs>\n");
    }
    if(num_choices == 0 || choices == NULL)
        fprintf(stdout,"\t\t\t\t\t<ows:AnyValue/>\n");
    else
    {
        /* Check for range values */
        if(strcmp(datatype, "integer") == 0 || strcmp(datatype, "float") == 0) {
            str = strtok((char*)choices[0], "-");
            if(str != NULL) {
                G_snprintf(range[0], 24, "%s", str);
                str = strtok(NULL, "-");
                if(str != NULL) {
                    G_snprintf(range[1], 24, "%s", str);
                    type = TYPE_RANGE;
                }
            }
        }

        fprintf(stdout,"\t\t\t\t\t<ows:AllowedValues>\n");
        if(type == TYPE_RANGE)
        {
            fprintf(stdout,"\t\t\t\t\t\t<ows:Range ows:rangeClosure=\"closed\">\n");
            fprintf(stdout,"\t\t\t\t\t\t\t<ows:MinimumValue>%s</ows:MinimumValue>\n", range[0]);
            fprintf(stdout,"\t\t\t\t\t\t\t<ows:MaximumValue>%s</ows:MaximumValue>\n", range[1]);
            fprintf(stdout,"\t\t\t\t\t\t</ows:Range>\n");
        }
        else
        {
            for(i = 0; i < num_choices; i++)
            {
                fprintf(stdout,"\t\t\t\t\t\t<ows:Value>");
                print_escaped_for_xml(stdout, choices[i]);
                fprintf(stdout,"</ows:Value>\n");
            }
        }
        fprintf(stdout,"\t\t\t\t\t</ows:AllowedValues>\n");
    }

    if(default_value)
    {
        fprintf(stdout,"\t\t\t\t\t<DefaultValue>");
        print_escaped_for_xml(stdout, default_value);
        fprintf(stdout,"</DefaultValue>\n");
    }
    fprintf(stdout,"\t\t\t\t</LiteralData>\n");


    if(inout_type == WPS_INPUT)
        fprintf(stdout,"\t\t\t</Input>\n");
    else if(inout_type == WPS_OUTPUT)
        fprintf(stdout,"\t\t\t</Output>\n");
}

/* ************************************************************************** */

static void wps_print_mimetype_text_plain(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>text/plain</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}
/* ************************************************************************** */

static void wps_print_mimetype_raster_tiff(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>image/tiff</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */

static void wps_print_mimetype_raster_png(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>image/png</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* *** Native GRASS raster format urn:grass:raster:location/mapset/raster *** */

static void wps_print_mimetype_raster_grass_binary(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/grass-raster-binary</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* *** GRASS raster maps exported via r.out.ascii ************************** */

static void wps_print_mimetype_raster_grass_ascii(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/grass-raster-ascii</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */

static void wps_print_mimetype_vector_gml311_appl(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/xml</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<Encoding>UTF-8</Encoding>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<Schema>http://schemas.opengis.net/gml/3.1.1/base/gml.xsd</Schema>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */

static void wps_print_mimetype_vector_gml212_appl(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/xml</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<Encoding>UTF-8</Encoding>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<Schema>http://schemas.opengis.net/gml/2.1.2/feature.xsd</Schema>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}


/* ************************************************************************** */

static void wps_print_mimetype_vector_gml311(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>text/xml</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<Encoding>UTF-8</Encoding>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<Schema>http://schemas.opengis.net/gml/3.1.1/base/gml.xsd</Schema>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */

static void wps_print_mimetype_vector_gml212(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>text/xml</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<Encoding>UTF-8</Encoding>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<Schema>http://schemas.opengis.net/gml/2.1.2/feature.xsd</Schema>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* *** GRASS vector format exported via v.out.ascii ************************** */

static void wps_print_mimetype_vector_grass_ascii(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/grass-vector-ascii</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* *** Native GRASS vector format urn:grass:vector:location/mapset/vector *** */

static void wps_print_mimetype_vector_grass_binary(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/grass-vector-binary</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* *** Space time dataset format using tar, tar.gz and tar.bz2 methods for packaging */

static void wps_print_mimetype_space_time_datasets(void)
{
    wps_print_mimetype_space_time_raster_datasets();
    wps_print_mimetype_space_time_vector_datasets();
}

/* *** Space time raster dataset format using tar, tar.gz and tar.bz2 methods for packaging */

static void wps_print_mimetype_space_time_raster_datasets(void)
{
    wps_print_mimetype_space_time_raster_datasets_tar();
    wps_print_mimetype_space_time_raster_datasets_tar_gz();
    wps_print_mimetype_space_time_raster_datasets_tar_bz2();
}

static void wps_print_mimetype_space_time_raster_datasets_tar(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/x-grass-strds-tar</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

static void wps_print_mimetype_space_time_raster_datasets_tar_gz(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/x-grass-strds-tar-gz</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

static void wps_print_mimetype_space_time_raster_datasets_tar_bz2(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/x-grass-strds-tar-bzip</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}


/* *** Space time vector dataset format using tar, tar.gz and tar.bz2 methods for packaging */

static void wps_print_mimetype_space_time_vector_datasets(void)
{
    wps_print_mimetype_space_time_vector_datasets_tar();
    wps_print_mimetype_space_time_vector_datasets_tar_gz();
    wps_print_mimetype_space_time_vector_datasets_tar_bz2();
}

static void wps_print_mimetype_space_time_vector_datasets_tar(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/x-grass-stvds-tar</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

static void wps_print_mimetype_space_time_vector_datasets_tar_gz(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/x-grass-stvds-tar-gz</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

static void wps_print_mimetype_space_time_vector_datasets_tar_bz2(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/x-grass-stvds-tar-bzip</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */
static void wps_print_mimetype_raster_gif(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>image/gif</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */
static void wps_print_mimetype_raster_jpeg(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>image/jpeg</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */
static void wps_print_mimetype_raster_hfa(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/x-erdas-hfa</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}
/* ************************************************************************** */

static void wps_print_mimetype_raster_tiff_other(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>image/geotiff</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");

    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/geotiff</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");

    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/x-geotiff</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */
static void wps_print_mimetype_raster_netCDF(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/netcdf</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */
static void wps_print_mimetype_raster_netCDF_other(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/x-netcdf</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */
static void wps_print_mimetype_vector_kml22(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>text/xml</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<Encoding>UTF-8</Encoding>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<Schema>http://schemas.opengis.net/kml/2.2.0/ogckml22.xsd</Schema>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */
static void wps_print_mimetype_vector_dgn(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/dgn</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */
static void wps_print_mimetype_vector_shape(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/shp</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* ************************************************************************** */
static void wps_print_mimetype_vector_zipped_shape(void)
{
    fprintf(stdout,"\t\t\t\t\t\t<Format>\n");
    fprintf(stdout,"\t\t\t\t\t\t\t<MimeType>application/x-zipped-shp</MimeType>\n");
    fprintf(stdout,"\t\t\t\t\t\t</Format>\n");
}

/* Bounding box data input. Do not use! Under construction. A list of coordinate reference systems must be created.*/

static void wps_print_bounding_box_data(void)
{
    int i;

    fprintf(stdout,"\t\t\t<Input minOccurs=\"0\" maxOccurs=\"1\">\n");
    wps_print_ident_title_abstract("BoundingBox", "Bounding box to process data",
      "The bounding box is uesed to create the reference coordinate system in grass, as well as the lower left and upper right corner of the processing area.");
    fprintf(stdout,"\t\t\t\t<BoundingBoxData>\n");
    /* A meaningful default boundingbox should be chosen*/
    fprintf(stdout,"\t\t\t\t\t<Default>\n");
    fprintf(stdout,"\t\t\t\t\t\t<CRS>urn:ogc:def:crs,crs:EPSG:6.3:32760</CRS>\n");
    fprintf(stdout,"\t\t\t\t\t</Default>\n");
    /* A list of all proj4 supported EPSG coordinate systems should be created */
    fprintf(stdout,"\t\t\t\t\t<Supported>\n");
    for(i = 0; i < 1; i++)
        fprintf(stdout,"\t\t\t\t\t\t<CRS>urn:ogc:def:crs,crs:EPSG:6.3:32760</CRS>\n");
    fprintf(stdout,"\t\t\t\t\t</Supported>\n");
    fprintf(stdout,"\t\t\t\t</BoundingBoxData>\n");
    fprintf(stdout,"\t\t\t</Input>\n");
}

