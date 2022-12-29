"""
@package core.gcmd

@brief wxGUI command interface

Classes:
 - gcmd::GError
 - gcmd::GWarning
 - gcmd::GMessage
 - gcmd::GException
 - gcmd::Popen (from http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/440554)
 - gcmd::Command
 - gcmd::CommandThread

Functions:
 - RunCommand
 - GetDefaultEncoding

(C) 2007-2008, 2010-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
"""

from __future__ import print_function

import os
import sys
import time
import errno
import signal
import traceback
import locale
import subprocess
from threading import Thread
import wx

is_mswindows = sys.platform == "win32"
if is_mswindows:
    from win32file import ReadFile, WriteFile
    from win32pipe import PeekNamedPipe
    import msvcrt
else:
    import select
    import fcntl

from core.debug import Debug
from core.globalvar import SCT_EXT

from grass.script import core as grass
from grass.script.utils import decode, encode

if sys.version_info.major == 2:
    bytes = str


def DecodeString(string):
    """Decode string using system encoding

    :param string: string to be decoded

    :return: decoded string
    """
    if not string:
        return string

    if _enc and isinstance(string, bytes):
        Debug.msg(5, "DecodeString(): enc=%s" % _enc)
        return string.decode(_enc)
    return string


def EncodeString(string):
    """Return encoded string using system locales

    :param string: string to be encoded

    :return: encoded string
    """
    if not string:
        return string
    if _enc:
        Debug.msg(5, "EncodeString(): enc=%s" % _enc)
        return string.encode(_enc)
    return string


class GError:
    def __init__(self, message, parent=None, caption=None, showTraceback=True):
        """Show error message window

        :param message: error message
        :param parent: centre window on parent if given
        :param caption: window caption (if not given "Error")
        :param showTraceback: True to show also Python traceback
        """
        if not caption:
            caption = _("Error")
        style = wx.OK | wx.ICON_ERROR | wx.CENTRE
        exc_type, exc_value, exc_traceback = sys.exc_info()
        if exc_traceback:
            exception = traceback.format_exc()
            reason = exception.splitlines()[-1].split(":", 1)[-1].strip()

        if Debug.GetLevel() > 0 and exc_traceback:
            sys.stderr.write(exception)

        if showTraceback and exc_traceback:
            wx.MessageBox(
                parent=parent,
                message=message + "\n\n%s: %s\n\n%s" % (_("Reason"), reason, exception),
                caption=caption,
                style=style,
            )
        else:
            wx.MessageBox(parent=parent, message=message, caption=caption, style=style)


class GWarning:
    def __init__(self, message, parent=None):
        caption = _("Warning")
        style = wx.OK | wx.ICON_WARNING | wx.CENTRE
        wx.MessageBox(parent=parent, message=message, caption=caption, style=style)


class GMessage:
    def __init__(self, message, parent=None):
        caption = _("Message")
        style = wx.OK | wx.ICON_INFORMATION | wx.CENTRE
        wx.MessageBox(parent=parent, message=message, caption=caption, style=style)


class GException(Exception):
    def __init__(self, value=""):
        self.value = value

    def __str__(self):
        return self.value

    def __unicode__(self):
        return self.value


class Popen(subprocess.Popen):
    """Subclass subprocess.Popen"""

    def __init__(self, args, **kwargs):
        if is_mswindows:
            # The Windows shell (cmd.exe) requires some special characters to
            # be escaped by preceding them with 3 carets (^^^). cmd.exe /?
            # mentions <space> and &()[]{}^=;!'+,`~. A quick test revealed that
            # only ^|&<> need to be escaped. A single quote can be escaped by
            # enclosing it with double quotes and vice versa.
            for i in range(2, len(args)):
                # "^" must be the first character in the list to avoid double
                # escaping.
                for c in ("^", "|", "&", "<", ">"):
                    if c in args[i]:
                        if "=" in args[i]:
                            a = args[i].split("=")
                            k = a[0] + "="
                            v = "=".join(a[1 : len(a)])
                        else:
                            k = ""
                            v = args[i]

                        # If there are spaces, the argument was already
                        # esscaped with double quotes, so don't escape it
                        # again.
                        if c in v and " " not in v:
                            # Here, we escape each ^ in ^^^ with ^^ and a
                            # <special character> with ^ + <special character>,
                            # so we need 7 carets.

                            v = v.replace(c, "^^^^^^^" + c)
                            args[i] = k + v

        subprocess.Popen.__init__(self, args, **kwargs)

    def recv(self, maxsize=None):
        return self._recv("stdout", maxsize)

    def recv_err(self, maxsize=None):
        return self._recv("stderr", maxsize)

    def send_recv(self, input="", maxsize=None):
        return self.send(input), self.recv(maxsize), self.recv_err(maxsize)

    def get_conn_maxsize(self, which, maxsize):
        if maxsize is None:
            maxsize = 1024
        elif maxsize < 1:
            maxsize = 1
        return getattr(self, which), maxsize

    def _close(self, which):
        getattr(self, which).close()
        setattr(self, which, None)

    def kill(self):
        """Try to kill running process"""
        if is_mswindows:
            import win32api

            handle = win32api.OpenProcess(1, 0, self.pid)
            return 0 != win32api.TerminateProcess(handle, 0)
        else:
            try:
                os.kill(-self.pid, signal.SIGTERM)  # kill whole group
            except OSError:
                pass

    if sys.platform == "win32":

        def send(self, input):
            if not self.stdin:
                return None

            import pywintypes

            try:
                x = msvcrt.get_osfhandle(self.stdin.fileno())
                (errCode, written) = WriteFile(x, input)
            except ValueError:
                return self._close("stdin")
            except (pywintypes.error, Exception) as why:
                if why.winerror in (109, errno.ESHUTDOWN):
                    return self._close("stdin")
                raise

            return written

        def _recv(self, which, maxsize):
            conn, maxsize = self.get_conn_maxsize(which, maxsize)
            if conn is None:
                return None

            import pywintypes

            try:
                x = msvcrt.get_osfhandle(conn.fileno())
                (read, nAvail, nMessage) = PeekNamedPipe(x, 0)
                if maxsize < nAvail:
                    nAvail = maxsize
                if nAvail > 0:
                    (errCode, read) = ReadFile(x, nAvail, None)
            except ValueError:
                return self._close(which)
            except (pywintypes.error, Exception) as why:
                if why.winerror in (109, errno.ESHUTDOWN):
                    return self._close(which)
                raise

            if self.universal_newlines:
                read = self._translate_newlines(read)
            return read

    else:

        def send(self, input):
            if not self.stdin:
                return None

            if not select.select([], [self.stdin], [], 0)[1]:
                return 0

            try:
                written = os.write(self.stdin.fileno(), input)
            except OSError as why:
                if why[0] == errno.EPIPE:  # broken pipe
                    return self._close("stdin")
                raise

            return written

        def _recv(self, which, maxsize):
            conn, maxsize = self.get_conn_maxsize(which, maxsize)
            if conn is None:
                return None

            flags = fcntl.fcntl(conn, fcntl.F_GETFL)
            if not conn.closed:
                fcntl.fcntl(conn, fcntl.F_SETFL, flags | os.O_NONBLOCK)

            try:
                if not select.select([conn], [], [], 0)[0]:
                    return ""

                r = conn.read()

                if not r:
                    return self._close(which)

                if self.universal_newlines:
                    r = self._translate_newlines(r)
                return r
            finally:
                if not conn.closed:
                    fcntl.fcntl(conn, fcntl.F_SETFL, flags)


message = "Other end disconnected!"


def recv_some(p, t=0.1, e=1, tr=5, stderr=0):
    if tr < 1:
        tr = 1
    x = time.time() + t
    y = []
    r = ""
    pr = p.recv
    if stderr:
        pr = p.recv_err
    while time.time() < x or r:
        r = pr()
        if r is None:
            if e:
                raise Exception(message)
            else:
                break
        elif r:
            y.append(decode(r))
        else:
            time.sleep(max((x - time.time()) / tr, 0))
    return "".join(y)


class Command:
    """Run command in separate thread. Used for commands launched
    on the background.

    If stdout/err is redirected, write() method is required for the
    given classes.

        cmd = Command(cmd=['d.rast', 'elevation.dem'], verbose=3, wait=True)

        if cmd.returncode == None:
            print 'RUNNING?'
        elif cmd.returncode == 0:
            print 'SUCCESS'
        else:
            print 'FAILURE (%d)' % cmd.returncode
    """

    def __init__(
        self,
        cmd,
        stdin=None,
        verbose=None,
        wait=True,
        rerr=False,
        stdout=None,
        stderr=None,
    ):
        """
        :param cmd: command given as list
        :param stdin: standard input stream
        :param verbose: verbose level [0, 3] (--q, --v)
        :param wait: wait for child execution terminated
        :param rerr: error handling (when GException raised).
                     True for redirection to stderr, False for GUI
                     dialog, None for no operation (quiet mode)
        :param stdout:  redirect standard output or None
        :param stderr:  redirect standard error output or None
        """
        Debug.msg(1, "gcmd.Command(): %s" % " ".join(cmd))
        self.cmd = cmd
        self.stderr = stderr

        #
        # set verbosity level
        #
        verbose_orig = None
        if ("--q" not in self.cmd and "--quiet" not in self.cmd) and (
            "--v" not in self.cmd and "--verbose" not in self.cmd
        ):
            if verbose is not None:
                if verbose == 0:
                    self.cmd.append("--quiet")
                elif verbose == 3:
                    self.cmd.append("--verbose")
                else:
                    verbose_orig = os.getenv("GRASS_VERBOSE")
                    os.environ["GRASS_VERBOSE"] = str(verbose)

        #
        # create command thread
        #
        self.cmdThread = CommandThread(cmd, stdin, stdout, stderr)
        self.cmdThread.start()

        if wait:
            self.cmdThread.join()
            if self.cmdThread.module:
                self.cmdThread.module.wait()
                self.returncode = self.cmdThread.module.returncode
            else:
                self.returncode = 1
        else:
            self.cmdThread.join(0.5)
            self.returncode = None

        if self.returncode is not None:
            Debug.msg(
                3,
                "Command(): cmd='%s', wait=%s, returncode=%d, alive=%s"
                % (" ".join(cmd), wait, self.returncode, self.cmdThread.isAlive()),
            )
            if rerr is not None and self.returncode != 0:
                if rerr is False:  # GUI dialog
                    raise GException(
                        "%s '%s'%s%s%s %s%s"
                        % (
                            _("Execution failed:"),
                            " ".join(self.cmd),
                            os.linesep,
                            os.linesep,
                            _("Details:"),
                            os.linesep,
                            _("Error: ") + self.__GetError(),
                        )
                    )
                elif rerr == sys.stderr:  # redirect message to sys
                    stderr.write("Execution failed: '%s'" % (" ".join(self.cmd)))
                    stderr.write(
                        "%sDetails:%s%s"
                        % (os.linesep, _("Error: ") + self.__GetError(), os.linesep)
                    )
            else:
                pass  # nop
        else:
            Debug.msg(
                3,
                "Command(): cmd='%s', wait=%s, returncode=?, alive=%s"
                % (" ".join(cmd), wait, self.cmdThread.isAlive()),
            )

        if verbose_orig:
            os.environ["GRASS_VERBOSE"] = verbose_orig
        elif "GRASS_VERBOSE" in os.environ:
            del os.environ["GRASS_VERBOSE"]

    def __ReadOutput(self, stream):
        """Read stream and return list of lines

        :param stream: stream to be read
        """
        lineList = []

        if stream is None:
            return lineList

        while True:
            line = stream.readline()
            if not line:
                break
            line = line.replace("%s" % os.linesep, "").strip()
            lineList.append(line)

        return lineList

    def __ReadErrOutput(self):
        """Read standard error output and return list of lines"""
        return self.__ReadOutput(self.cmdThread.module.stderr)

    def __ProcessStdErr(self):
        """
        Read messages/warnings/errors from stderr

        :return: list of (type, message)
        """
        if self.stderr is None:
            lines = self.__ReadErrOutput()
        else:
            lines = self.cmdThread.error.strip("%s" % os.linesep).split(
                "%s" % os.linesep
            )

        msg = []

        type = None
        content = ""
        for line in lines:
            if len(line) == 0:
                continue
            if "GRASS_" in line:  # error or warning
                if "GRASS_INFO_WARNING" in line:  # warning
                    type = "WARNING"
                elif "GRASS_INFO_ERROR" in line:  # error
                    type = "ERROR"
                elif "GRASS_INFO_END":  # end of message
                    msg.append((type, content))
                    type = None
                    content = ""

                if type:
                    content += line.split(":", 1)[1].strip()
            else:  # stderr
                msg.append((None, line.strip()))

        return msg

    def __GetError(self):
        """Get error message or ''"""
        if not self.cmdThread.module:
            return _("Unable to exectute command: '%s'") % " ".join(self.cmd)

        for type, msg in self.__ProcessStdErr():
            if type == "ERROR":
                return msg

        return ""


class CommandThread(Thread):
    """Create separate thread for command. Used for commands launched
    on the background."""

    def __init__(self, cmd, env=None, stdin=None, stdout=sys.stdout, stderr=sys.stderr):
        """
        :param cmd: command (given as list)
        :param env: environmental variables
        :param stdin: standard input stream
        :param stdout: redirect standard output or None
        :param stderr: redirect standard error output or None
        """
        Thread.__init__(self)

        self.cmd = cmd
        self.stdin = stdin
        self.stdout = stdout
        self.stderr = stderr
        self.env = env

        self.module = None
        self.error = ""

        self._want_abort = False
        self.aborted = False

        self.setDaemon(True)

        # set message formatting
        self.message_format = os.getenv("GRASS_MESSAGE_FORMAT")
        os.environ["GRASS_MESSAGE_FORMAT"] = "gui"

    def __del__(self):
        if self.message_format:
            os.environ["GRASS_MESSAGE_FORMAT"] = self.message_format
        else:
            del os.environ["GRASS_MESSAGE_FORMAT"]

    def run(self):
        """Run command"""
        if len(self.cmd) == 0:
            return

        Debug.msg(1, "gcmd.CommandThread(): %s" % " ".join(self.cmd))

        self.startTime = time.time()

        # TODO: replace ugly hack below
        # this cannot be replaced it can be only improved
        # also unifying this with 3 other places in code would be nice
        # changing from one chdir to get_real_command function
        args = self.cmd
        if sys.platform == "win32":
            if os.path.splitext(args[0])[1] == SCT_EXT:
                args[0] = args[0][:-3]
            # using Python executable to run the module if it is a script
            # expecting at least module name at first position
            # cannot use make_command for this now because it is used in GUI
            # The same code is in grass.script.core already twice.
            args[0] = grass.get_real_command(args[0])
            if args[0].endswith(".py"):
                args.insert(0, sys.executable)

        try:
            self.module = Popen(
                args,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                shell=sys.platform == "win32",
                env=self.env,
            )

        except OSError as e:
            self.error = str(e)
            print(e, file=sys.stderr)
            return 1

        if self.stdin:  # read stdin if requested ...
            self.module.stdin.write(self.stdin)
            self.module.stdin.close()

        # redirect standard outputs...
        self._redirect_stream()

    def _redirect_stream(self):
        """Redirect stream"""
        if self.stdout:
            # make module stdout/stderr non-blocking
            out_fileno = self.module.stdout.fileno()
            if not is_mswindows:
                flags = fcntl.fcntl(out_fileno, fcntl.F_GETFL)
                fcntl.fcntl(out_fileno, fcntl.F_SETFL, flags | os.O_NONBLOCK)

        if self.stderr:
            # make module stdout/stderr non-blocking
            out_fileno = self.module.stderr.fileno()
            if not is_mswindows:
                flags = fcntl.fcntl(out_fileno, fcntl.F_GETFL)
                fcntl.fcntl(out_fileno, fcntl.F_SETFL, flags | os.O_NONBLOCK)

        # wait for the process to end, sucking in stuff until it does end
        while self.module.poll() is None:
            if self._want_abort:  # abort running process
                self.module.terminate()
                self.aborted = True
                return
            if self.stdout:
                line = recv_some(self.module, e=0, stderr=0)
                self.stdout.write(line)
            if self.stderr:
                line = recv_some(self.module, e=0, stderr=1)
                self.stderr.write(line)
                if len(line) > 0:
                    self.error = line

        # get the last output
        if self.stdout:
            line = recv_some(self.module, e=0, stderr=0)
            self.stdout.write(line)
        if self.stderr:
            line = recv_some(self.module, e=0, stderr=1)
            self.stderr.write(line)
            if len(line) > 0:
                self.error = line

    def abort(self):
        """Abort running process, used by main thread to signal an abort"""
        self._want_abort = True


def _formatMsg(text):
    """Format error messages for dialogs"""
    message = ""
    for line in text.splitlines():
        if len(line) == 0:
            continue
        elif "GRASS_INFO_MESSAGE" in line:
            message += line.split(":", 1)[1].strip() + "\n"
        elif "GRASS_INFO_WARNING" in line:
            message += line.split(":", 1)[1].strip() + "\n"
        elif "GRASS_INFO_ERROR" in line:
            message += line.split(":", 1)[1].strip() + "\n"
        elif "GRASS_INFO_END" in line:
            return message
        else:
            message += line.strip() + "\n"

    return message


def RunCommand(
    prog,
    flags="",
    overwrite=False,
    quiet=False,
    verbose=False,
    parent=None,
    read=False,
    parse=None,
    stdin=None,
    getErrorMsg=False,
    env=None,
    **kwargs,
):
    """Run GRASS command

    :param prog: program to run
    :param flags: flags given as a string
    :param overwrite, quiet, verbose: flags
    :param parent: parent window for error messages
    :param read: fetch stdout
    :param parse: fn to parse stdout (e.g. grass.parse_key_val) or None
    :param stdin: stdin or None
    :param getErrorMsg: get error messages on failure
    :param env: environment (optional, uses os.environ if not provided)
    :param kwargs: program parameters

    The environment passed to the function (env or os.environ) is not modified (a copy is used internally).

    :return: returncode (read == False and getErrorMsg == False)
    :return: returncode, messages (read == False and getErrorMsg == True)
    :return: stdout (read == True and getErrorMsg == False)
    :return: returncode, stdout, messages (read == True and getErrorMsg == True)
    :return: stdout, stderr
    """
    cmdString = " ".join(
        grass.make_command(prog, flags, overwrite, quiet, verbose, **kwargs)
    )

    Debug.msg(1, "gcmd.RunCommand(): %s" % cmdString)

    kwargs["stderr"] = subprocess.PIPE

    if read:
        kwargs["stdout"] = subprocess.PIPE

    if stdin:
        kwargs["stdin"] = subprocess.PIPE

    # Do not change the environment, only a local copy.
    if env:
        env = env.copy()
    else:
        env = os.environ.copy()

    if parent:
        env["GRASS_MESSAGE_FORMAT"] = "standard"

    start = time.time()

    ps = grass.start_command(prog, flags, overwrite, quiet, verbose, env=env, **kwargs)

    if stdin:
        ps.stdin.write(encode(stdin))
        ps.stdin.close()
        ps.stdin = None

    stdout, stderr = ps.communicate()
    stderr = decode(stderr)
    stdout = decode(stdout) if read else stdout

    ret = ps.returncode
    Debug.msg(
        1,
        "gcmd.RunCommand(): get return code %d (%.6f sec)"
        % (ret, (time.time() - start)),
    )

    if ret != 0:
        if stderr:
            Debug.msg(2, "gcmd.RunCommand(): error %s" % stderr)
        else:
            Debug.msg(2, "gcmd.RunCommand(): nothing to print ???")

        if parent:
            GError(parent=parent, caption=_("Error in %s") % prog, message=stderr)

    if not read:
        if not getErrorMsg:
            return ret
        else:
            return ret, _formatMsg(stderr)

    if stdout:
        Debug.msg(3, "gcmd.RunCommand(): return stdout\n'%s'" % stdout)
    else:
        Debug.msg(3, "gcmd.RunCommand(): return stdout = None")

    if parse:
        stdout = parse(stdout)

    if not getErrorMsg:
        return stdout

    if read and getErrorMsg:
        return ret, stdout, _formatMsg(stderr)

    return stdout, _formatMsg(stderr)


def GetDefaultEncoding(forceUTF8=False):
    """Get default system encoding

    :param bool forceUTF8: force 'UTF-8' if encoding is not defined

    :return: system encoding (can be None)
    """
    enc = locale.getdefaultlocale()[1]
    if forceUTF8 and (enc is None or enc == "UTF8"):
        return "UTF-8"

    if enc is None:
        enc = locale.getpreferredencoding()

    Debug.msg(1, "GetSystemEncoding(): %s" % enc)
    return enc


_enc = GetDefaultEncoding()  # define as global variable
