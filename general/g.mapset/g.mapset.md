## DESCRIPTION

*g.mapset* changes the current working mapset, project (formerly known
as location), or GISDBASE (directory with one or more projects).

With *g.mapset*, the shell history (i.e. `.bash_history` file of the
initial project will be used to record the command history.

The *g.mapset* tool can also report the current mapset and all mapsets in
the current project.

## NOTES

By default, the shell continues to use the history for the old mapset.
To change this behaviour the history can be switched to record in the
new mapset's history file as follows:

```sh
# bash example
history -w
history -r /"$GISDBASE/$LOCATION/$MAPSET"/.bash_history
HISTFILE=/"$GISDBASE/$LOCATION/$MAPSET"/.bash_history
```

For parsing the outputs, always use the JSON output. The current *plain* format
may change in a future major release. Please, open an issue if you need a
stable parsable format which is not JSON.

## EXAMPLES

### Print the name of the current mapset

To print the name of the current mapset, use the **-p** command as shown
below:

```sh
# In plain format:
g.mapset -p

# In JSON format:
g.mapset -p format=json
```

To print the name of the current mapset in JSON format using python:

```python
import grass.script as gs

# Run the g.mapset command with -p flag to print the current mapset using JSON
# output format
data = gs.parse_command(
    "g.mapset",
    flags="p",
    format="json",
)

print(f"project: {data['project']}")
print(f"mapset: {data['mapset']}")
```

### List available mapsets

To list available mapsets, use the **-l** command as shown below:

```sh
# In plain format:
g.mapset -l

# In JSON format:
g.mapset -l format=json
```

This should list all the mapsets, such as: "landsat new PERMANENT
user1."

To print the list of available mapsets in JSON format using python:

```python
import grass.script as gs

# Run the g.mapset command with -l flag to list available mapsets using JSON
# output format
data = gs.parse_command(
    "g.mapset",
    flags="l",
    format="json",
)

print(f"project: {data['project']}")
print(f"mapsets: {' '.join(data['mapsets'])}")
```

### Change the current mapset

To change the current mapset to "user1" use the following command:

```sh
g.mapset mapset=user1 project=nc_spm_08_grass7
```

You should receive the following message: "Mapset switched. Your shell
continues to use the history for the old mapset."

### Create a new mapset

To create a new mapset, use the **-c** tag as shown below:

```sh
g.mapset -c mapset=new project=nc_spm_08_grass7
```

## SEE ALSO

*[g.gisenv](g.gisenv.md), [g.mapsets](g.mapsets.md)*

## AUTHOR

Radim Blazek
