"""Python interface to launch GRASS GIS modules in scripts
"""

from .core import (
    call,
    create_environment,
    create_location,
    create_project,
    compare_key_value_text_files,
    debug,
    debug_level,
    del_temp_region,
    error,
    exec_command,
    fatal,
    feed_command,
    find_file,
    find_program,
    gisenv,
    get_commands,
    get_real_command,
    handle_errors,
    get_raise_on_error,
    get_capture_stderr,
    info,
    legal_name,
    list_grouped,
    list_pairs,
    list_strings,
    locn_is_latlong,
    make_command,
    mapsets,
    message,
    overwrite,
    parse_command,
    parse_color,
    parser,
    percent,
    pipe_command,
    read_command,
    region,
    region_env,
    run_command,
    set_raise_on_error,
    start_command,
    sanitize_mapset_environment,
    set_capture_stderr,
    tempdir,
    tempfile,
    tempname,
    use_temp_region,
    version,
    verbose,
    verbosity,
    warning,
    write_command,
)

from .db import (
    db_begin_transaction,
    db_commit_transaction,
    db_connection,
    db_describe,
    db_select,
    db_table_exist,
    db_table_in_vector,
)

from .raster import mapcalc, mapcalc_start, raster_history, raster_info, raster_what

from .raster3d import mapcalc3d, raster3d_info

from .utils import (
    KeyValue,
    basename,
    decode,
    encode,
    float_or_dms,
    parse_key_val,
    try_remove,
    diff_files,
    try_rmdir,
    text_to_string,
    get_num_suffix,
    split,
    natural_sort,
    naturally_sorted,
    get_lib_path,
    set_path,
    clock,
    legalize_vector_name,
    append_node_pid,
    append_uuid,
    append_random,
)

from .vector import (
    vector_db,
    vector_layer_db,
    vector_columns,
    vector_history,
    vector_info_topo,
    vector_info,
    vector_db_select,
    vector_what,
)
from . import setup  # noqa: F401

__all__ = [
    # utils imports
    "KeyValue",
    "append_node_pid",
    "append_random",
    "append_uuid",
    "basename",
    # core imports
    "call",
    "clock",
    "compare_key_value_text_files",
    "create_environment",
    "create_location",
    "create_project",
    # db imports
    "db_begin_transaction",
    "db_commit_transaction",
    "db_connection",
    "db_describe",
    "db_select",
    "db_table_exist",
    "db_table_in_vector",
    "debug",
    "debug_level",
    "decode",
    "decode",
    "del_temp_region",
    "diff_files",
    "encode",
    "encode",
    "error",
    "exec_command",
    "fatal",
    "feed_command",
    "find_file",
    "find_program",
    "float_or_dms",
    "float_or_dms",
    "get_capture_stderr",
    "get_commands",
    "get_lib_path",
    "get_num_suffix",
    "get_raise_on_error",
    "get_real_command",
    "gisenv",
    "handle_errors",
    "info",
    "legal_name",
    "legalize_vector_name",
    "list_grouped",
    "list_pairs",
    "list_strings",
    "locn_is_latlong",
    "make_command",
    # raster imports
    "mapcalc",
    # raster3d imports
    "mapcalc3d",
    "mapcalc_start",
    "mapsets",
    "message",
    "natural_sort",
    "naturally_sorted",
    "overwrite",
    "parse_color",
    "parse_command",
    "parse_key_val",
    "parse_key_val",
    "parse_key_val",
    "parser",
    "percent",
    "pipe_command",
    "raster3d_info",
    "raster_history",
    "raster_info",
    "raster_what",
    "read_command",
    "region",
    "region_env",
    "run_command",
    "sanitize_mapset_environment",
    "set_capture_stderr",
    "set_path",
    "set_raise_on_error",
    "split",
    "start_command",
    "tempdir",
    "tempfile",
    "tempname",
    "text_to_string",
    "try_remove",
    "try_remove",
    "try_rmdir",
    "use_temp_region",
    "vector_columns",
    # vector imports
    "vector_db",
    "vector_db",
    "vector_db_select",
    "vector_history",
    "vector_info",
    "vector_info_topo",
    "vector_layer_db",
    "vector_what",
    "verbose",
    "verbosity",
    "version",
    "warning",
    "write_command",
]
