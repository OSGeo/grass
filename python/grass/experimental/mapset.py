"Session or subsession for mapsets (subprojects)"

import shutil
import os
from pathlib import Path

import grass.script as gs
from grass.experimental.create import (
    require_create_ensure_mapset,
    create_temporary_mapset,
)


class MapsetSession:
    """Session in another mapset in the same location

    By default, it assumes that the mapset exists and raises ValueError otherwise.
    Use *create* to create a new mapset and add *overwrite* to delete an existing
    one of the same name (with all the data in it) before the new one is created.
    To use an existing mapset if it exist and create it if it doesn't exist,
    use *ensure*.

    Note that *ensure* will not create a new mapset if the current is invalid.
    Invalid mapset may mean corrupt data, so it is not clear what to do.
    Using create with overwrite will work on an invalid mapset because
    the existing mapset is always deleted with overwrite enabled.

    Standard use of the object is to use it as a context manager, i.e., create it
    using the ``with`` statement. Then use its *env* property to pass the environment
    variables for the session to subprocesses:

    >>> with MapsetSession(name, ensure=True) as session:
    ...     run_command("r.surf.fractal", output="surface", env=session.env)

    This session expects an existing GRASS runtime environment.

    The name argument is positional-only.

    .. versionadded:: 8.4
    """

    def __init__(self, name, *, create=False, overwrite=False, ensure=False, env=None):
        """Starts the session and creates the mapset if requested"""
        self._name = name
        self._env = env
        self._session_file = None
        self._active = False
        self._start(create=create, overwrite=overwrite, ensure=ensure)

    @property
    def active(self):
        """True if session is active (i.e., not finished)"""
        return self._active

    @property
    def env(self):
        """Mapping object with environment variables

        This is suitable for subprocess which should run this mapset.
        """
        return self._env

    @property
    def name(self):
        """Mapset name"""
        return self._name

    def _start(self, create, overwrite, ensure):
        """Start the session and create the mapset if requested"""
        gis_env = gs.gisenv(env=self._env)
        require_create_ensure_mapset(
            gis_env["GISDBASE"],
            gis_env["LOCATION_NAME"],
            self._name,
            create=create,
            overwrite=overwrite,
            ensure=ensure,
        )
        self._session_file, self._env = gs.create_environment(
            gis_env["GISDBASE"],
            gis_env["LOCATION_NAME"],
            self._name,
            env=self._env,
        )
        self._active = True

    def finish(self):
        """Finish the session.

        If not used as a context manager, call explicitly to clean and close the mapset
        and finish the session. No GRASS modules can be called afterwards with
        the environment obtained from this object.
        """
        if not self.active:
            msg = "Attempt to finish an already finished session"
            raise ValueError(msg)
        os.remove(self._session_file)
        self._active = False

    def __enter__(self):
        """Enter the context manager context.

        Notably, the session is activated in its *__init__* function.

        :returns: reference to the object (self)
        """
        if not self.active:
            msg = "Attempt to use inactive (finished) session as a context manager"
            raise ValueError(msg)
        return self

    def __exit__(self, type, value, traceback):  # pylint: disable=redefined-builtin
        """Exit the context manager context.

        Finishes the existing session.
        """
        self.finish()


class TemporaryMapsetSession:
    """Session in another mapset in the same location

    By default, it assumes that the mapset exists and raises ValueError otherwise.
    Use *create* to create a new mapset and add *overwrite* to delete an existing
    one of the same name (with all the data in it) before the new one is created.
    To use an existing mapset if it exist and create it if it doesn't exist,
    use *ensure*.

    Note that *ensure* will not create a new mapset if the current is invalid.
    Invalid mapset may mean corrupt data, so it is not clear what to do.
    Using create with overwrite will work on an invalid mapset because
    the existing mapset is always deleted with overwrite enabled.

    Standard use of the object is to use it as a context manager, i.e., create it
    using the ``with`` statement. Then use its *env* property to pass the environment
    variables for the session to subprocesses:

    >>> with MapsetSession(name, ensure=True) as session:
    ...     run_command("r.surf.fractal", output="surface", env=session.env)

    The name argument is positional-only.

    .. versionadded:: 8.4
    """

    def __init__(self, *, location=None, env=None):
        """Starts the session and creates the mapset if requested"""
        if location:
            # Simple resolution of location name versus path to location.
            # Assumes anything which is not a directory (existing files,
            # non-existing paths) are names. Existing directory with the corresponding
            # name works only if GISDBASE is the current directory.
            # Resolving mapsets handled in jupyter.Session.switch_mapset and
            # resolve_mapset_path functions.
            self._location_path = Path(location)
            if not self._location_path.is_dir():
                gis_env = gs.gisenv(env=env)
                self._location_path = Path(gis_env["GISDBASE"]) / location
        else:
            gis_env = gs.gisenv(env=env)
            self._location_path = Path(gis_env["GISDBASE"]) / gis_env["LOCATION_NAME"]
        self._name = None
        self._path = None
        self._session_file = None
        self._active = False
        self._env = None
        self._start(env=env)

    @property
    def active(self):
        """True if session is active (i.e., not finished)"""
        return self._active

    @property
    def env(self):
        """Mapping object with environment variables

        This is suitable for subprocess which should run this mapset.
        """
        # This could be a copy to be read-only, but
        # that may be too much overhead with env=session.env usage.
        return self._env

    @property
    def name(self):
        """Mapset name"""
        return self._name

    @property
    def mapset_path(self):
        """MapsetPath"""
        return self._path

    def _start(self, env):
        """Start the session and create the mapset if requested"""
        self._path = create_temporary_mapset(self._location_path)
        self._name = self._path.mapset
        self._session_file, self._env = gs.create_environment(
            self._location_path.parent,
            self._location_path.name,
            self._name,
            env=env,
        )
        self._active = True

    def finish(self):
        """Finish the session.

        If not used as a context manager, call explicitly to clean and close the mapset
        and finish the session. No GRASS modules can be called afterwards with
        the environment obtained from this object.
        """
        if not self.active:
            msg = "Attempt to finish an already finished session"
            raise ValueError(msg)
        self._active = False
        os.remove(self._session_file)
        shutil.rmtree(self._path.path, ignore_errors=True)

    def __enter__(self):
        """Enter the context manager context.

        Notably, the session is activated in its *__init__* function.

        :returns: reference to the object (self)
        """
        if not self.active:
            msg = "Attempt to use inactive (finished) session as a context manager"
            raise ValueError(msg)
        return self

    def __exit__(self, type, value, traceback):  # pylint: disable=redefined-builtin
        """Exit the context manager context.

        Finishes the existing session.
        """
        self.finish()


def _test():
    """Quick tests of mapset session usage.

    The file should run outside of an existing session, but the grass package
    needs to be on path.
    """
    with gs.setup.init("~/grassdata/nc_spm_08_grass7"):
        with TemporaryMapsetSession() as session:
            gs.run_command("g.region", res=10, rows=100, cols=200, env=session.env)
            gs.run_command(
                "r.surf.random", output="uniform_random", min=1, max=10, env=session.env
            )
            print(
                gs.parse_command(
                    "r.univar", map="uniform_random", flags="g", env=session.env
                )["max"]
            )

        with MapsetSession("user1", ensure=True) as session:
            gs.run_command("g.region", raster="elevation", env=session.env)
            print(
                gs.parse_command(
                    "r.univar", map="elevation", flags="g", env=session.env
                )["range"]
            )
        gs.run_command("g.mapsets", flags="l")


if __name__ == "__main__":
    _test()
