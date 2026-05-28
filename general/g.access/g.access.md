## DESCRIPTION

This program allows the user to control access to the current mapset.
Normally, any user can read data from any GRASS mapset. But sometimes it
is desirable to prohibit access to certain sensitive data. The
*g.access* command allows a user to restrict read and execute access to
the current mapset (see UNIX *chmod* command). *g.access* will not
modify write access to the current mapset.

The user may, for example, allow only users in the same UNIX group to
read data files in the mapset, or restrict the mapset to personal use
only.

## NOTES

Under GRASS, access to the mapset PERMANENT must be open to all users.
This is because GRASS looks for the user's default geographic region
definition settings and the project TITLE in files that are stored under
the PERMANENT mapset directory. The *g.access* command, therefore, will
not allow you to restrict access to the PERMANENT mapset.

The *[g.mapsets](g.mapsets.md)* command isn't smart enough to tell if
access to a specified mapset is restricted, and the user is therefore
allowed to include the names of restricted mapsets in his search path.
However, the data in a restricted mapset is still protected; any
attempts to look for or use data in a restricted mapset will fail. The
user will simply not see any data listed for a restricted mapset.

UNIX filesystem access controls and *g.access* actions are not supported
by MS-Windows.

## SEE ALSO

UNIX manual entries for *chmod(1)* and *group(5)*  
*[g.mapsets](g.mapsets.md)*

## AUTHOR

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
