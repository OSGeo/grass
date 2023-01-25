// elevate.js -- runs target command line elevated
// Arguments should not have embedded spaces.
// http://blogs.msdn.com/b/aaron_margosis/archive/2007/07/01/scripting-elevation-on-vista.aspx
if (WScript.Arguments.Length >= 1) {
    Application = WScript.Arguments(0);
    Arguments = "";
    for (Index = 1; Index < WScript.Arguments.Length; Index += 1) {
        if (Index > 1) {
            Arguments += " ";
        }
        Arguments += WScript.Arguments(Index);
    }
    new ActiveXObject("Shell.Application").ShellExecute(Application, Arguments, "", "runas");
} else {
    WScript.Echo("Usage:");
    WScript.Echo("elevate Application Arguments");
}

