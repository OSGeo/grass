# Updating Version File

Version file (`include/VERSION`) can be updated using the _update_version.py_ script.

The script captures the logic of updating the version file incorporating
the common actions and workflow checks.

## Usage

Use `--help` to get the information about actions available as sub-commands:

```sh
./utils/update_version.py --help
```

Some sub-commands have short documentation on their own:

```sh
./utils/update_version.py minor --help
```

All commands return YAML output on success and return non-zero return code on failure.

## Examples

### Checking Current Status

The _status_ command prints content of the version file as YAML
and adds today's date and constructs a version string:

```sh
./utils/update_version.py status
```

Example output:

```yaml
today: 2022-04-27
year: 2022
major: 3
minor: 2
micro: 0dev
version: 3.2.0dev
```

Naturally, this also checks that the version if is accessible and fails otherwise.

The _status_ command prints input for Bash _eval_ with `--bash`:

```bash
eval `./utils/update_version.py status --bash`
echo $VERSION
```

### Updating Minor Version

Let's say we are at the main branch, version 3.2.0dev, and just created
a new branch for 3.2 release, so we want to update the minor version
on the main branch to the next minor version:

```sh
./utils/update_version.py minor
```

Separately, or as part of other changes, now is the time to commit,
so the script suggests a commit message in the output, e.g.:

```yaml
read:
  user_message: Use the provided message for the commit
use:
  commit_message: 'version: Start 3.2.0dev'
```

### Error Handling

The commands detect invalid states and report error messages.
Continuing in the previous example, an attempt to increase
the micro version will fail:

```sh
./utils/update_version.py micro
```

The error message explains the reason, a micro version should be increased
only after the release:

```text
Already dev with micro '0dev'. Release first before update.
```
