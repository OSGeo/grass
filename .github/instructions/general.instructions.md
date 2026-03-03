---
applyTo: "**/*"
---
## Best Practices

### General

#### Computational Region

Tools typically **do not change the computational region based on the input
data**. Raster processing tools should respect the current computational region.

**Why?** Users should be able to re-run a command or workflow with different
computational regions to, e.g., test processing in a small area and then move to
a larger one. Also, changing the current region globally can break parallel
processing.

**Exceptions**: Tools importing data typically import the entire dataset, respecting
of the region may be implemented as an optional feature (e.g., r.in.gdal). This
is to avoid, e.g., importing data under finer resolution than their native
resolution. Another exception is raster processing where alignment
of the cells plays a crucial role and there is a clear answer to how the
alignment should be done. In that case, the tool may change the resolution. Some
tools, such as r.mapcalc, opt for providing additional computational region
handling policies. Finally, some operations are meant to use all the data, e.g.,
creating metadata, these operations should not use the current computational
region.

If you need to change the computational region, there are ways to change it only
within your script, not affecting the current region.
See [Changing computational region](python.instructions.md#changing-computational-region)
for more details.

#### Mapsets

**Output data should be always written to the current mapset**. This is ensured
by build-in GRASS mechanisms, so there is nothing which needs to be done in the
tool. If a tool modifies inputs, the input must be in the current mapset.

The tool should accept inputs from any mapset in the current project. The
user-provided name **may or may not include mapset name** and the tool needs to
respect that.

#### Input and Output Geospatial Data Format

An analytical tool should read and write geospatial data as GRASS raster or
vector maps. Importing from and exporting to other data formats should be left
to dedicated import and export tools, e.g., _v.import_. The exceptions are
import and export of data, e.g., _r.in.xyz_.

Processing and analytical tools can then use simple names to refer to the
data within GRASS projects instead of file paths. This follows the separation of
concerns principle: format conversion and CRS transformations are separate from
analysis.

#### Overwriting Existing Data

A tool should not overwrite existing data unless specified by the user using the
`--overwrite` flag. The GRASS command line parser automatically checks for
output data (raster, vector maps) existence and ends the tool execution with a
proper error message in case the output already exists. If the flag is set by
the user (`--overwrite` in command line, `overwrite=True` in Python), the parser
enables the overwriting for the tool.

The `--overwrite` flag can be globally enabled by setting the environment variable
`GRASS_OVERWRITE` to 1. Notably, the GRASS session from _grass.jupyter_ sets
`GRASS_OVERWRITE` to 1 to enable re-running of the cells and notebooks.

#### Mask

GRASS has a global mask managed by the _r.mask_ tool and represented by a
raster called MASK by default. Raster tools called as a subprocess will automatically
respect the globally set mask when reading the data. For outputs, respecting of
the mask is optional.

Tools should generally respect the global mask set by a user. If the mask set
by the user is not respected by a tool, the exact behavior should be described
in the documentation. On the other hand, ignoring mask is usually the desired
behavior for import tools which corresponds with the mask being applied only
when reading existing raster data in a project.

Tools **should not set or remove the global mask** to prevent unintended
behavior during interactive sessions and to maintain parallel processing
integrity. If a tool requires a mask for its operation, it should implement
a temporary mask using _MaskManager_ in Python or by setting the `GRASS_MASK`
environment variable.

Generally, any mask behavior should be documented unless it is the standard case
where masked cells do not participate in the computation and are represented as
NULL cells (no data) in the output.

#### Formatting messages

Put raster, vector maps, imagery groups etc. in brackets:

```text
Raster map <elevation> not found.
```

Put file paths, SQL queries into single quotes:

```text
File '/path/to/file.txt' not found.
```

First letter should be capitalized.

Avoid contractions (cannot instead of can't).

Be consistent with periods. Complete sentences or all parts of a message with
multiple sentences should end with periods. Short phrases should not.
Punctuated events, such as errors, deserve a period, e.g., _"Operation
complete."_ Phrases which imply ongoing action should have an ellipse, e.g.,
 _"Reading raster map..."_.
