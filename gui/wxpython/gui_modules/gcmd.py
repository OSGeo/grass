"""
@package gcmd

@brief GRASS command interface

Classes:
 - GException
 - GStdError
 - CmdError
 - SettingsError
 - DigitError
 - DBMError
 - NvizError
 - Popen (from http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/440554)
 - Command
 - CommandThread

(C) 2007-2008 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Jachym Cepicky
Martin Landa <landa.martin gmail.com>

@date 2007-2008
"""

import os
import sys
import time
import errno
import signal

import wx

try:
    import subprocess
except:
    compatPath = os.path.join(globalvar.ETCWXDIR, "compat")
    sys.path.append(compatPath)
    import subprocess
if subprocess.mswindows:
    from win32file import ReadFile, WriteFile
    from win32pipe import PeekNamedPipe
    import msvcrt
else:
    import select
    import fcntl
from threading import Thread

import globalvar
grassPath = os.path.join(globalvar.ETCDIR, "python")
sys.path.append(grassPath)
import grass

import utils
from debug import Debug as Debug

class GException(Exception):
    """Generic exception"""
    def __init__(self, message, title=_("Error"), parent=None):
        self.message = message
        self.parent = parent
        self.title = title
        
    def __str__(self):
        dlg = wx.MessageDialog(parent=self.parent,
                               caption=self.title,
                               message=self.message,
                               style=wx.ICON_ERROR | wx.CENTRE)
        dlg.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_error.ico'), wx.BITMAP_TYPE_ICO))
        if self.parent:
            dlg.CentreOnParent()
        else:
            dlg.CentreOnScreen()

        dlg.ShowModal()
        
        return ''

class GStdError(GException):
    """Generic exception"""

    def __init__(self, message, title=_("Error"), parent=None):
        GException.__init__(self, message, title=title, parent=parent)

class CmdError(GException):
    """Exception used for GRASS commands.

    See Command class (command exits with EXIT_FAILURE,
    G_fatal_error() is called)."""
    def __init__(self, cmd, message, parent=None):
        self.cmd = cmd
        GException.__init__(self, message,
                            title=_("Error in command execution %s" % self.cmd[0]),
                            parent=parent)

class SettingsError(GException):
    """Exception used for GRASS settings, see
    gui_modules/preferences.py."""
    def __init__(self, message, parent=None):
        GException.__init__(self, message,
                            title=_("Preferences error"),
                            parent=parent)

class DigitError(GException):
    """Exception raised during digitization session"""
    def __init__(self, message, parent=None):
        GException.__init__(self, message,
                            title=_("Vector digitizer error"),
                            parent=parent)

class DBMError(GException):
    """Attribute Table Manager exception class"""
    def __init__(self, message, parent=None):
        GException.__init__(self, message,
                            title=_("Attribute table manager error"),
                            parent=parent)

class NvizError(GException):
    """Nviz exception class"""
    def __init__(self, message, parent=None):
        GException.__init__(self, message,
                            title=_("Nviz error"),
                            parent=parent)

class Popen(subprocess.Popen):
    """Subclass subprocess.Popen"""
    def recv(self, maxsize=None):
        return self._recv('stdout', maxsize)
    
    def recv_err(self, maxsize=None):
        return self._recv('stderr', maxsize)

    def send_recv(self, input='', maxsize=None):
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
        if subprocess.mswindows:
            import win32api
            handle = win32api.OpenProcess(1, 0, self.pid)
            return (0 != win32api.TerminateProcess(handle, 0))
	else:
            try:
                os.kill(-self.pid, signal.SIGTERM) # kill whole group
            except OSError:
                pass

    if subprocess.mswindows:
        def send(self, input):
            if not self.stdin:
                return None

            try:
                x = msvcrt.get_osfhandle(self.stdin.fileno())
                (errCode, written) = WriteFile(x, input)
            except ValueError:
                return self._close('stdin')
            except (subprocess.pywintypes.error, Exception), why:
                if why[0] in (109, errno.ESHUTDOWN):
                    return self._close('stdin')
                raise

            return written

        def _recv(self, which, maxsize):
            conn, maxsize = self.get_conn_maxsize(which, maxsize)
            if conn is None:
                return None
            
            try:
                x = msvcrt.get_osfhandle(conn.fileno())
                (read, nAvail, nMessage) = PeekNamedPipe(x, 0)
                if maxsize < nAvail:
                    nAvail = maxsize
                if nAvail > 0:
                    (errCode, read) = ReadFile(x, nAvail, None)
            except ValueError:
                return self._close(which)
            except (subprocess.pywintypes.error, Exception), why:
                if why[0] in (109, errno.ESHUTDOWN):
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
            except OSError, why:
                if why[0] == errno.EPIPE: #broken pipe
                    return self._close('stdin')
                raise

            return written

        def _recv(self, which, maxsize):
            conn, maxsize = self.get_conn_maxsize(which, maxsize)
            if conn is None:
                return None
            
            flags = fcntl.fcntl(conn, fcntl.F_GETFL)
            if not conn.closed:
                fcntl.fcntl(conn, fcntl.F_SETFL, flags| os.O_NONBLOCK)
            
            try:
                if not select.select([conn], [], [], 0)[0]:
                    return ''
                
                r = conn.read(maxsize)
                
                if not r:
                    return self._close(which)
    
                if self.universal_newlines:
                    r = self._translate_newlines(r)
                return r
            finally:
                if not conn.closed:
                    fcntl.fcntl(conn, fcntl.F_SETFL, flags)

message = "Other end disconnected!"

def recv_some(p, t=.1, e=1, tr=5, stderr=0):
    if tr < 1:
        tr = 1
    x = time.time()+t
    y = []
    r = ''
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
            y.append(r)
        else:
            time.sleep(max((x-time.time())/tr, 0))
    return ''.join(y)
    
def send_all(p, data):
    while len(data):
        sent = p.send(data)
        if sent is None:
            raise Exception(message)
        data = buffer(data, sent)

class Command:
    """
    Run GRASS command in separate thread

    If stdout/err is redirected, write() method is required for the
    given classes.

    @code
        cmd = Command(cmd=['d.rast', 'elevation.dem'], verbose=3, wait=True)

        if cmd.returncode == None:
            print 'RUNNING?'
        elif cmd.returncode == 0:
            print 'SUCCESS'
        else:
            print 'FAILURE (%d)' % cmd.returncode
    @endcode

    @param cmd     command given as list
    @param stdin   standard input stream
    @param verbose verbose level [0, 3] (--q, --v)
    @param wait    wait for child execution terminated
    @param rerr    error handling (when CmdError raised).
    True for redirection to stderr, False for GUI dialog,
    None for no operation (quiet mode)
    @param stdout  redirect standard output or None
    @param stderr  redirect standard error output or None
    """
    def __init__ (self, cmd, stdin=None,
                  verbose=None, wait=True, rerr=False,
                  stdout=None, stderr=None):

        self.cmd = cmd
        self.stderr = stderr

	# hack around platform-specific extension for binaries
	if self.cmd[0] in globalvar.grassCmd['script']:
	    self.cmd[0] = self.cmd[0] + globalvar.EXT_SCT
	else:
	    self.cmd[0] = self.cmd[0] + globalvar.EXT_BIN

        #
        # set verbosity level
        #
        verbose_orig = None
        if ('--q' not in self.cmd and '--quiet' not in self.cmd) and \
                ('--v' not in self.cmd and '--verbose' not in self.cmd):
            if verbose is not None:
                if verbose == 0:
                    self.cmd.append('--quiet')
                elif verbose == 3:
                    self.cmd.append('--verbose')
                else:
                    verbose_orig = os.getenv("GRASS_VERBOSE")
                    os.environ["GRASS_VERBOSE"] = str(verbose)

        #
        # create command thread
        #
        self.cmdThread = CommandThread(cmd, stdin,
                                       stdout, stderr)
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
            Debug.msg (3, "Command(): cmd='%s', wait=%s, returncode=%d, alive=%s" % \
                           (' '.join(cmd), wait, self.returncode, self.cmdThread.isAlive()))
            if rerr is not None and self.returncode != 0:
                if rerr is False: # GUI dialog
                    raise CmdError(cmd=self.cmd,
                                   message="%s '%s'%s%s%s %s%s" %
                                   (_("Execution failed:"),
                                    ' '.join(self.cmd),
                                    os.linesep, os.linesep,
                                    _("Details:"),
                                    os.linesep,
                                    _("Error: ") + self.GetError()))
                elif rerr == sys.stderr: # redirect message to sys
                    stderr.write("Execution failed: '%s'" % (' '.join(self.cmd)))
                    stderr.write("%sDetails:%s%s" % (os.linesep,
                                                     _("Error: ") + self.GetError(),
                                                     os.linesep))
            else:
                pass # nop
        else:
            Debug.msg (3, "Command(): cmd='%s', wait=%s, returncode=?, alive=%s" % \
                           (' '.join(cmd), wait, self.cmdThread.isAlive()))

        if verbose_orig:
            os.environ["GRASS_VERBOSE"] = verbose_orig
        elif os.environ.has_key("GRASS_VERBOSE"):
            del os.environ["GRASS_VERBOSE"]
            
    def __ReadOutput(self, stream):
        """Read stream and return list of lines

        @param stream stream to be read
        """
        lineList = []

        if stream is None:
            return lineList

        while True:
            line = stream.readline()
            if not line:
                break
            line = line.replace('%s' % os.linesep, '').strip()
            lineList.append(line)

        return lineList
                    
    def ReadStdOutput(self):
        """Read standard output and return list of lines"""
        if self.cmdThread.stdout:
            stream = self.cmdThread.stdout # use redirected stream instead
            stream.seek(0)
        else:
            stream = self.cmdThread.module.stdout

        return self.__ReadOutput(stream)
    
    def ReadErrOutput(self):
        """Read standard error output and return list of lines"""
        return self.__ReadOutput(self.cmdThread.module.stderr)

    def __ProcessStdErr(self):
        """
        Read messages/warnings/errors from stderr

        @return list of (type, message)
        """
        if self.stderr is None:
            lines = self.ReadErrOutput()
        else:
            lines = self.cmdThread.error.strip('%s' % os.linesep). \
                split('%s' % os.linesep)
        
        msg = []

        type    = None
        content = ""
        for line in lines:
            if len(line) == 0:
                continue
            if 'GRASS_' in line: # error or warning
                if 'GRASS_INFO_WARNING' in line: # warning
                    type = "WARNING"
                elif 'GRASS_INFO_ERROR' in line: # error
                    type = "ERROR"
                elif 'GRASS_INFO_END': # end of message
                    msg.append((type, content))
                    type = None
                    content = ""
                
                if type:
                    content += line.split(':', 1)[1].strip()
            else: # stderr
                msg.append((None, line.strip()))

        return msg

    def GetError(self):
        """Get error message or ''"""
        if not self.cmdThread.module:
            return _("Unable to exectute command: '%s'") % ' '.join(self.cmd)

        for type, msg in self.__ProcessStdErr():
            if type == 'ERROR':
                return unicode(msg, "utf-8")

        return ''
    
    def PrintModuleOutput(self, error=True, warning=False, message=False):
        """Print module errors, warnings, messages to output

        @param error print errors
        @param warning print warnings
        @param message print messages

        @return string
        """

        msgString = ""
        for type, msg in self.__ProcessStdErr():
            if type:
                if (type == 'ERROR' and error) or \
                        (type == 'WARNING' and warning) or \
                        (type == 'MESSAGE' and message):
                    msgString += " " + type + ": " + msg + "%s" % os.linesep
            else:
                msgString += " " + msg + "%s" % os.linesep

        return msgString

class CommandThread(Thread):
    """Create separate thread for command

    @param cmd GRASS command (given as list)
    @param stdin standard input stream 
    @param stdout  redirect standard output or None
    @param stderr  redirect standard error output or None
    """
    def __init__ (self, cmd, stdin=None,
                  stdout=sys.stdout, stderr=sys.stderr):

        Thread.__init__(self)

        self.cmd    = cmd
        self.stdin  = stdin
        self.stdout = stdout
        self.stderr = stderr
        
        self.module = None
        self.error  = ''
        
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
        if len(self.cmd) == 0:
            return

        self.startTime = time.time()
        
        try:
            self.module = Popen(self.cmd,
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        except OSError, e:
            self.error = str(e)
            return 1
        
        if self.stdin: # read stdin if requested ...
            self.module.stdin.write(self.stdin)
            self.module.stdin.close()
            
        # redirect standard outputs...
        if self.stdout or self.stderr:
            self.__redirect_stream()

    def __redirect_stream(self):
        """Redirect stream"""
        if self.stdout:
            # make module stdout/stderr non-blocking
            out_fileno = self.module.stdout.fileno()
	    if not subprocess.mswindows:
                flags = fcntl.fcntl(out_fileno, fcntl.F_GETFL)
                fcntl.fcntl(out_fileno, fcntl.F_SETFL, flags| os.O_NONBLOCK)
                
        if self.stderr:
            # make module stdout/stderr non-blocking
            out_fileno = self.module.stderr.fileno()
	    if not subprocess.mswindows:
                flags = fcntl.fcntl(out_fileno, fcntl.F_GETFL)
                fcntl.fcntl(out_fileno, fcntl.F_SETFL, flags| os.O_NONBLOCK)
        
        # wait for the process to end, sucking in stuff until it does end
        while self.module.poll() is None:
            if self._want_abort: # abort running process
                self.module.kill()
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
    
