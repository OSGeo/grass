# Message Standardization

In order to reduce the amount of similar strings please use standardized
messages. This helps with message translation as well.

## How should Errors/Warnings/Messages be formatted

- Only user derived variables should be bracketed, not GRASS derived variables,
  for example:

  ```text
  Yes: Creating raster map <%s>.  Pass 1 of 7 ...
  No:  Creating <raster> map <%s>.  Pass [1] of [7] ...
  ```

- strings `< >` - raster, vector maps, group names, etc., e.g.
  - Raster map `<%s>` not found
- strings `' '` \- paths, SQL queries, etc., e.g.
  - File '%s' not found
  - Unable to create table: '%s'

### Discussion

- The \[bracketed\] parenthetical disrupts the flow of the phrase and doesn't
  help enhance clarity of meaning. IMHO, this reads better without \[\]
  brackets: "_Line %d deleted._" \[\] Brackets should be used when value is
  outside of the phrase: _"Unknown line \[%d\]"_. --HB

### Statistics

<!-- markdownlint-disable line-length -->
| symbol | number of code lines (2007-04-11) | 2008-02-14 (6.3svn) | 2009-08-02 (7.0svn) | 2013-06-21 (7.0svn) |
|--------|-----------------------------------|---------------------|---------------------|---------------------|
| `<%s>` | 637                               | 1406                | 1935                | 2398                |
| `[%s]` | 690                               | 427                 | 222                 | 189                 |
| `'%s'` | 354                               | 370                 | 537                 | 689                 |
| `<%d>` | 12                                | 7                   | 11                  | 18                  |
| `[%d]` | 207                               | 13                  | 137                 | 136                 |
| `'%d'` | 3                                 | 1                   | 1                   | 3                   |
<!-- markdownlint-enable line-length -->

```bash
TYPES="<%s> \[%s\] '%s' <%d> \[%d\] '%d'"

for TYPE in $TYPES ; do
  NUM_HITS=`grep -rI "$TYPE" * | grep -v '/.svn/\|^dist.i686-\|locale/po/' | wc -l`
  echo "$TYPE  $NUM_HITS"
done
```

## Macros to be defined for C library

- `MSG_RASTER_NOT_FOUND_IN_MAPSET` - "Raster map <%s> not found in <%s>"
- `MSG_CANNOT_OPEN_RASTER` - "Unable to open raster map <%s>"
- `MSG_CANNOT_OPEN_FILE` - Unable to open file <%s>

Note: Problem with xgettext package. How to use macros to work with xgettext?

## Standard messages sandbox

- First letter should be capitalized
- Use the present tense
  (cannot instead of could not; **better: unable to**; even better: avoid the
  issue altogether by rewording like "File not found.")
- Avoid contractions (cannot instead of can't)
- Good sentence construction
  ("Cannot find input map <%s>" instead of "It could not be find input map <%s>";
  possibly better: "Input map <%s> not found.")
- Be consistent with periods. Either end all phrases with a period or none.
  Without periods the translators save also some time
  Complete sentences or all parts of a message with multiple sentences should
  end with periods. Short phrases should not. Punctuated events, such as errors,
  deserve a period. e.g. _"Operation complete."_ Phrases which imply ongoing
  action look odd if missing an ellipse or any other form of punctuation.
  Phrase != Sentence. --HB
- Either all **module** descriptions should end with periods or not. As some
  are multi-sentence, and thus should end in a period for consistency within
  the message, so probably they all should end in one. Currently by my count
  237 end with '.', 139 do not. In the multi-sentence case it may be possible
  to put the simple description in the module->label field and additional
  explanitory text into the ->description field.
- **option** and **flag** descriptions generally should not end in a period
  (more likely to be phrases than sentences). But they can suffer the same
  multi-sentence period problem as module descriptions. In this case splitting
  out additional text into a ->label, ->description split may help.
- Suspension points used to indicate some process is being done should be
  placed next to last word, without space. e.g.

  ```text
  Reading raster map...
  ```

  instead of

  ```text
  Reading raster map ...
  ```

  HB: FWIW & my 2c, 1) to me keeping the space before the ellipse looks better,
  is this a purely cosmetic choice or is there some style logic? \[wikipedia
  article on the ellipse was cited on grass-dev, I would argue that refers to
  printed typeset text not monospace terminal text, strangely suggests
  punctuation+3 dots (....), is just one other guy's opinion, and I'm still not
  swayed\] 2) these messages may be good candidates for G\_verbose\_message().

  HB 2c more: (i.landsat.rgb example)

  ```text
    Processing <$i>...

    Processing <$i> ...
    ```

    The version with the space just looks better. The version without just
    looks wrong. But when the line ends with a word, no-space doesn't look
    that bad: (v.in.ascii)

    ```text
      Importing points...

      Importing points ...
    ```

- Module descriptions should use all the same verbal tense. Currently some
  of them can be found in infinitive and others in present.

### DB messages

db\_open\_database(), db\_start\_driver\_open\_database()

> Unable to open database <%s> by driver <%s>

db\_execute\_immediate()

> Unable to insert new record: '%s'
> Unable to create table: '%s'
> Unable to drop table: '%s'

db\_grant\_on\_table()

> Unable to grant privileges on table <%s>

db\_start\_driver()

> Unable to start driver <%s>

db\_describe\_table()

> Unable to describe table <%s>

db\_select\_value()

> Unable to select record from table <%s> (key %s, column %s)
> No records selected from table <%s>

db\_fetch()

> Unable to fetch data from table <%s>

db\_create\_index(), db\_create\_index2()

> Unable to create index for table <%s>, key <%s>

db\_copy\_table(), db\_copy\_table\_by\_ints()

> Unable to copy table <%s>

db\_delete\_table()

> Unable to delete table <%s>

db\_table\_exists()

> Unable to find table <%s> linked to vector map <%s>

db\_get\_column()

> Column <%s> not found in table <%s>

db\_list\_tables()

> Unable to get list tables in database <%s>

db\_open\_select\_cursor()

> Unable to open select cursor: '%s'

db\_create\_table()

> Unable to create table <%s>

db\_get\_num\_rows()

> Unable select records from table <%s>

### General messages

G\_legal\_filename()

> <%s> is an illegal file name

G\_set\_window()

> Invalid graphics coordinates
> Unable to set region

D\_do\_conversions()

> Error calculating graphics-region conversions

G\_tempfile()

> Unable to open temporary file <%s>

G\_get\_projinfo()

> Unable to get projection info of raster map

G\_get\_projunits()

> Unable to get projection units of raster map

### Raster messages

G\_find\_raster2()

> Raster map <%s> not found
> Raster map <%s> not found in the current mapset
> Raster map <%s> not found in mapset <%s>

Rast\_open\_old() \[in GRASS 6: G\_open\_cell\_old()\]

> Unable to open raster map <%s>

Rats\_open\_new \[in GRASS 6: G\_open\_cell\_new()\]

> Unable to create raster map <%s>

Rast\_close() \[in GRASS 6: G\_close\_cell()\]

> Unable to close raster map <%s>

Rast\_get\_cellhd() \[in GRASS 6: G\_get\_cellhd()\]

> Unable to read header of raster map <%s>

Rast\_get\_row() \[in GRASS 6: G\_get\_raster\_row()\]

> Unable to read raster map <%s> row %d

Rast\_put\_row() \[in GRASS 6: G\_put\_raster\_row()\]

> Failed writing raster map <%s> row %d

Rast\_write\_cats \[in GRASS 6: G\_write\_cats()\]

> Unable to write category file for raster map <%s>

Rast\_read\_colors() \[in GRASS 6: G\_read\_colors()\]

> Unable to read color file of raster map <%s>

Rast\_read\_cats() \[in GRASS 6: G\_read\_cats()\]

> Unable to read category file of raster map <%s>

Rast\_fp\_range() \[in GRASS 6: G\_read\_fp\_range()\]

> Unable to read fp range of raster map <%s>

Rast\_read\_range() \[in GRASS 6: G\_read\_range()\]

> Unable to read range of raster map <%s>

### 3D raster messages

G\_find\_grid3()

> 3D raster map <%s> not found
> 3D raster map <%s> not found in the current mapset
> 3D raster map <%s> not found in mapset <%s>

G3d\_openCellOld()

> Unable to open 3D raster map <%s>

G3d\_range\_load()

> Unable to read range of 3D raster map <%s>

G3d\_closeCell()

> Unable to close 3D raster map <%s>

### Vector messages

G\_find\_vector2()

> Vector map <%s> not found
> Vector map <%s> not found in the current mapset
> Vector map <%s> not found in mapset <%s>

Vect\_open\_old()

> Unable to open vector map <%s>
> Unable to open vector map <%s> at topology level %d

Vect\_open\_new()

> Unable to create vector map <%s>

Vect\_get\_field(), Vect\_get\_dblink()

> Database connection not defined for layer %d

Vect\_get\_area\_centroid()

> Area %d without centroid or Skipping area %d without centroid

Vect\_open\_old\_head()

> Unable to open header file of vector map <%s>

Vect\_map\_add\_dblink()

> Unable to add database link for vector map <%s>

Vect\_read\_next\_line

> Unable to read vector map <%s> feature id %d

### Discussion (Standard messages sandbox)

- "Map \<roads\> in \<user1\>" or "Map \<roads\@user1\>"
  - "Map \<roads@user1\>" preferred - \[\[User:Landa|ML\]\]
  - If the element was given on the command line, the user knows which mapset
    it came from, and the @mapset part is just extra noise. If the element is
    taken from the data (how? i.group?) where the user hasn't explicitly
    defined the map, then @mapset it appropriate. Also output maps are always
    created in the current mapset\* so @mapset is redundant and should not be
    used for new maps. \[\* i.rectify, r.in.gdal, v.in.ogr are exceptions that
    can create maps in other mapsets or new locations\] --HB

_Proposed standard responses in bold._

- `G_get_raster_row()`
  "**Unable to read raster map <%s> row %d**"

  ```text
  Cannot get raster raster of input map
  Cannot raster raster of input map
  Cannot read raster row [%d]
  can't read row in input elevation map
  Could not read from <%s>
  Could not read from <%s>, row=%d
  Error getting input null cells
  error reading hue data
  Error reading 'hue' map
  Error reading input
  error reading intensity data
  Error reading 'intensity' map
  Error reading map %s
  Error reading row of data
  error reading saturation data
  Error reading 'saturation' map
  G_get_raster_row failed
  reading map
  Reading map
  Reading row %d
  read_row: error reading data
  %s: Unable to read raster map [%s]
  Unable to get row %d from raster map
  Unable to get row %d from <%s>
  Unable to read map raster row %d
  Unable to read raster row.
  Unable to read raster row %d
  Unable to read row %i\n
  ```

- `G_put_raster_row()`
    "**Failed writing raster map <%s> row %d**"

  ```text
  Cannot write row to raster map
  Cannot write to <%s>
  Error while writing new cell map.
  Error writing cell map during transform.
  error writing data
  Error writing output map
  G_put_raster_row failed (file system full?)
  %s - ERROR writing %s
  Unable to properly write output raster map
  unable to write map row %d
  ```

- `G_open_cell_new()`
  "**Unable to create raster map <%s>**"

  ```text
  Cannot open output raster map <%s>
  can't open %s
  Error in opening output file <%s>
  Unable to create output <%s>
  Unable to create raster map <%s>
  Unable to create raster map [%s]
  Unable to open cell map <%s> for output.\n
  Unable to open output file.
  Unable to open the cell map MASK.
  ```

- `G_open_cell_old()`
  "**Unable to open raster map <%s>**"

  ```text
  Cannot find map %s
  Cannot find mapset for %s
  Cannot open File %s
  Cannot open input raster map <%s>
  Cannot open map %s
  Cannot open raster map
  Cannot open raster map <%s>
  Cannot open raster map [%s]
  Cannot open raster map [%s]!
  Cannot open %s
  Cannot open seed map <%s@%s>!
  Cannot open terrain raster map <%s@%s>!
  Can't open input map
  can't open raster map [%s]
  can't open raster map <%s> in mapset %s
  can't open %s
  Could not open raster <%s>
  Could not open raster '%s'
  Error in opening input file <%s>
  Not able to open cellfile for [%s]
  Problem opening input file [%s]
  Raster file <%s> not found
  Raster map [%s] not found
  <%s> cellfile not found
  %s: Couldn't open raster <%s@%s>
  %s: o_open_file: could not open raster map %s in %s
  Unable to open band files.
  Unable to open cellfile for [%s]
  Unable to open map <%s>
  Unable to open MASK
  Unable to open raster map [%s]
  Unable to open raster <%s>
  unable to open [%s] in [%s]
  Unable to proceed
  ```

- `G_find_raster()`
  "**Raster map <%s> not found**"

  ```text
  albedo raster map <%s> not found
  aspin raster map <%s> not found
  Cannot find map %s
  Cannot find mapset for %s
  Cannot find raster map
  Cannot find %s
  cell file [%s] not found
  coefbh raster map <%s> not found
  elevin raster map <%s> not found
  Input map [%s] in location [%s] in mapset [%s] not found.
  latin raster map <%s> not found
  linkein raster map <%s> not found
  No clump map specified and MASK not set.
  Raster file [%s] not found. Exiting.
  Raster map [%s] already exists.\nPlease try another.
  Raster map <%s> not found
  Raster map [%s] not found
  Raster <%s> already exist.
  Raster <%s> not found
  Raster '%s' not found
  Requested raster map <%s> not found
  [%s] cannot be found!
  %s - exits in Mapset <%s>, select another name
  slopein raster map <%s> not found
  %s: more than %d files not allowed
  %s - not found
  %s: o_open_file: raster map %s not found
  %s: <%s> raster map not found
  %s: %s - raster map not found
  %s: %s - Unable to find the imaginary-image.
  %s: %s - Unable to find the real-image map.
  stringliteral
  Training map [%s] not found.
  Unable to find base map <%s>
  Unable to find cell map <%s>
  Unable to find cover map <%s>
  Unable to find file [%s].
  Unable to find input cell map <%s>
  ```

- `G_find_raster2()`
  "**Raster map <%s> not found**"

  ```text
  Raster map <%s> not found in current mapset
  Raster map <%s> not found
  Raster file [%s] not found. Exiting.
  ```

- `G_find_vector()`
  "**Vector map <%s> not found**"

  ```text
  Cannot find vector map
  Could not find input map <%s>
  Could not find input map <%s>.
  Could not find input map '%s'.
  Could not find input map <%s> in current mapset.
  Could not find input map <%s>\n
  Could not find input %s
  Could not find input %s\n
  Could not find input vector map <%s>
  Could not find input vector map %s
  Could not find network input map '%s'.
  Could not find vector map <%s> in current mapset
  ```

- `G_tempfile()`
  "**Unable to open temporary file <%s>**"

  ```text
  Unable to write the temporary file
  Unable to open temp file.
  Unable to open tempfile
  Unable to open temporary file <%s>
  ```

## Systematic Approach

- Collect all possible error states from functions
- assign error codes for macros?
- `G_option()` parameters
  (like output->description = ("Path to resulting ASCII file");)

Note: Would be possible to conctretize your ideas? (MartinL)

### Parameters and flags

Fix the parameters and flags. Make it a concept. See
[proposal(s)](https://trac.osgeo.org/grass/browser/grass/trunk/doc/parms_flags.txt)

## Verbosity levels

### Current abstract concept

<!-- markdownlint-disable line-length -->
| type     | `GRASS_VERBOSE` | libgis      | command line       | result                                               | libgis fn                                             |
|----------|-----------------|-------------|--------------------|------------------------------------------------------|-------------------------------------------------------|
| verbose  | 3               | `MAX_LEVEL` | `--verbose`        | print progress and all messages messages             | `G_verbose_message()`                                 |
| standard | 2               | `STD_LEVEL` |                    | print progress and selected messages                 | `G_message()`                                         |
| brief    | 1               |             |                    | print only progress information (% done)             | `G_percent()`, `G_clicker()`, `G_important_message()` |
| quiet    | 0               | `MIN_LEVEL` | `--quiet`          | print nothing (only ERRORs and WARNINGs)             | `G_warning()`, `G_fatal_error()`                      |
| debug    |                 | 0-5         |                    | print debug message at DEBUG level set with g.gisenv | `G_debug(#, "")`                                      |
| mute     | \-              |             | `2>&1 > /dev/null` | only for use in rare occasions                       |                                                       |
<!-- markdownlint-enable line-length -->

#### Proposal

- module output (should be "parsable") is not controlled by
  `GRASS_VERBOSE` \[use fprintf (stdout, ...)\]
- quiet
  only warnings and fatal errors are printed
- brief
  all `G_percent()` and `G_important_message()` (especially connected to
  progress information??)
- standard
  all `G_percent()` and selected `G_message()`
- verbose
  all `G_percent()` + `G_message()` + `G_verbose_message()`

#### Modification

- **remove** verbose level &rarr; standard == all `G_percent()` + `G_messages()`
  why?? --HB
- remove "--verbose" from help pages, manually mention where it does work.
  it only does something in a few modules
- move some messages to `G_debug()`
- move some messages to `G_verbose_message()`
- move some messages to `G_important_message()`
- change level numbers to 1, 3, 5
- Add new `G_msg(int level, "%format", ...);` lib function. Existing
  `G_message()` could stay and would be the same as `G_msg(LEVEL_STD,...)`.

#### Discussion (Current abstract concept)

- HB: I'm quite happy to leave the current system as it is, but use
  `GRASS_MESSAGE_FORMAT=silent` to switch off `G_percent()`,`G_clicker()` output.
  Mixing a sliding verbosity scale plus a binary switch or >= rule for when
  `G_percent()` happens in the same variable is very messy IMO.

### Alternative abstract concept

(Binary method, 3=1+2)

_How to deal with --verbose messages? add levels 4 (vmsg+msg) and
5(vmsg+msg+progress) ?_

<!-- markdownlint-disable line-length -->
|                     |                    |                |                                 |
|---------------------|--------------------|----------------|---------------------------------|
| standard            | 3                  | `PERMSG_MODE`  | print progress and messages     |
| standard            | 2                  | `MESSAGE_MODE` | print only messages             |
| brief               | 1                  | `PERCENT_MODE` | print only progress information |
| silent but not mute | 0                  | `QUIET_MODE`   | print nothing (but ERR and WAR) |
| mute                | `2>&1 > /dev/null` |                |                                 |
<!-- markdownlint-enable line-length -->
