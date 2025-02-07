## DESCRIPTION

*g.mapset* changes the current working mapset, project (formerly known
as location), or GISDBASE (directory with one or more projects).

With *g.mapset*, the shell history (i.e. `.bash_history` file of the
initial project will be used to record the command history.

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

## EXAMPLES

### Print the name of the current mapset

To print the name of the current mapset, use the **-p** command as shown
below:

```sh
g.mapset -p
```

### List available mapsets

To list available mapsets, use the **-l** command as shown below:

```sh
g.mapset -l
```

This should list all the mapsets, such as: "landsat new PERMANENT
user1."

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
