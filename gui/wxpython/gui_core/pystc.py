"""!
@package gui_core.pystc

@brief Python styled text control widget

Classes:
 - pystc::PyStc

(C) 2012-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""


import keyword

import wx
from wx import stc
from core.utils import _


class PyStc(stc.StyledTextCtrl):
    """!Styled Python output (see gmodeler::frame::PythonPanel for
    usage)

    Based on StyledTextCtrl_2 from wxPython demo
    """    
    def __init__(self, parent, id = wx.ID_ANY, statusbar = None):
        stc.StyledTextCtrl.__init__(self, parent, id)
        
        self.parent = parent
        self.statusbar = statusbar
        
        self.modified = False # content modified ?
        
        self.faces = { 'times': 'Times New Roman',
                       'mono' : 'Courier New',
                       'helv' : 'Arial',
                       'other': 'Comic Sans MS',
                       'size' : 10,
                       'size2': 8,
                       }
        
        self.CmdKeyAssign(ord('B'), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMIN)
        self.CmdKeyAssign(ord('N'), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMOUT)
        
        self.SetLexer(stc.STC_LEX_PYTHON)
        self.SetKeyWords(0, " ".join(keyword.kwlist))
        
        self.SetProperty("fold", "1")
        self.SetProperty("tab.timmy.whinge.level", "1")
        self.SetMargins(0, 0)
        self.SetTabWidth(4)
        self.SetUseTabs(False)
        
        self.SetEdgeMode(stc.STC_EDGE_BACKGROUND)
        self.SetEdgeColumn(78)
        
        # setup a margin to hold fold markers
        self.SetMarginType(2, stc.STC_MARGIN_SYMBOL)
        self.SetMarginMask(2, stc.STC_MASK_FOLDERS)
        self.SetMarginSensitive(2, True)
        self.SetMarginWidth(2, 12)
        
        # like a flattened tree control using square headers
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPEN,    stc.STC_MARK_BOXMINUS,          "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDER,        stc.STC_MARK_BOXPLUS,           "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERSUB,     stc.STC_MARK_VLINE,             "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERTAIL,    stc.STC_MARK_LCORNER,           "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEREND,     stc.STC_MARK_BOXPLUSCONNECTED,  "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPENMID, stc.STC_MARK_BOXMINUSCONNECTED, "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERMIDTAIL, stc.STC_MARK_TCORNER,           "white", "#808080")
        
        self.Bind(stc.EVT_STC_UPDATEUI, self.OnUpdateUI)
        self.Bind(stc.EVT_STC_MARGINCLICK, self.OnMarginClick)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyPressed)
        
        # Make some styles, the lexer defines what each style is used
        # for, we just have to define what each style looks like.
        # This set is adapted from Scintilla sample property files.
        
        # global default styles for all languages
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "face:%(helv)s,size:%(size)d" % self.faces)
        self.StyleClearAll()  # reset all to be like the default
        
        # global default styles for all languages
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "face:%(helv)s,size:%(size)d" % self.faces)
        self.StyleSetSpec(stc.STC_STYLE_LINENUMBER,  "back:#C0C0C0,face:%(helv)s,size:%(size2)d" % self.faces)
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "face:%(other)s" % self.faces)
        self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT,  "fore:#FFFFFF,back:#0000FF,bold")
        self.StyleSetSpec(stc.STC_STYLE_BRACEBAD,    "fore:#000000,back:#FF0000,bold")
        
        # Python styles
        # Default 
        self.StyleSetSpec(stc.STC_P_DEFAULT, "fore:#000000,face:%(helv)s,size:%(size)d" % self.faces)
        # Comments
        self.StyleSetSpec(stc.STC_P_COMMENTLINE, "fore:#007F00,face:%(other)s,size:%(size)d" % self.faces)
        # Number
        self.StyleSetSpec(stc.STC_P_NUMBER, "fore:#007F7F,size:%(size)d" % self.faces)
        # String
        self.StyleSetSpec(stc.STC_P_STRING, "fore:#7F007F,face:%(helv)s,size:%(size)d" % self.faces)
        # Single quoted string
        self.StyleSetSpec(stc.STC_P_CHARACTER, "fore:#7F007F,face:%(helv)s,size:%(size)d" % self.faces)
        # Keyword
        self.StyleSetSpec(stc.STC_P_WORD, "fore:#00007F,bold,size:%(size)d" % self.faces)
        # Triple quotes
        self.StyleSetSpec(stc.STC_P_TRIPLE, "fore:#7F0000,size:%(size)d" % self.faces)
        # Triple double quotes
        self.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, "fore:#7F0000,size:%(size)d" % self.faces)
        # Class name definition
        self.StyleSetSpec(stc.STC_P_CLASSNAME, "fore:#0000FF,bold,underline,size:%(size)d" % self.faces)
        # Function or method name definition
        self.StyleSetSpec(stc.STC_P_DEFNAME, "fore:#007F7F,bold,size:%(size)d" % self.faces)
        # Operators
        self.StyleSetSpec(stc.STC_P_OPERATOR, "bold,size:%(size)d" % self.faces)
        # Identifiers
        self.StyleSetSpec(stc.STC_P_IDENTIFIER, "fore:#000000,face:%(helv)s,size:%(size)d" % self.faces)
        # Comment-blocks
        self.StyleSetSpec(stc.STC_P_COMMENTBLOCK, "fore:#7F7F7F,size:%(size)d" % self.faces)
        # End of line where string is not closed
        self.StyleSetSpec(stc.STC_P_STRINGEOL, "fore:#000000,face:%(mono)s,back:#E0C0E0,eol,size:%(size)d" % self.faces)
        
        self.SetCaretForeground("BLUE")
        
    def OnKeyPressed(self, event):
        """!Key pressed
        
        @todo implement code completion (see wxPython demo)
        """
        if not self.modified:
            self.modified = True
            if self.statusbar:
                self.statusbar.SetStatusText(_('Python script contains local modifications'), 0)
        
        event.Skip()
        
    def OnUpdateUI(self, evt):
        # check for matching braces
        braceAtCaret = -1
        braceOpposite = -1
        charBefore = None
        caretPos = self.GetCurrentPos()
        
        if caretPos > 0:
            charBefore  = self.GetCharAt(caretPos - 1)
            styleBefore = self.GetStyleAt(caretPos - 1)
        
        # check before
        if charBefore and chr(charBefore) in "[]{}()" and styleBefore == stc.STC_P_OPERATOR:
            braceAtCaret = caretPos - 1
        
        # check after
        if braceAtCaret < 0:
            charAfter = self.GetCharAt(caretPos)
            styleAfter = self.GetStyleAt(caretPos)
            
            if charAfter and chr(charAfter) in "[]{}()" and styleAfter == stc.STC_P_OPERATOR:
                braceAtCaret = caretPos
        
        if braceAtCaret >= 0:
            braceOpposite = self.BraceMatch(braceAtCaret)
        
        if braceAtCaret != -1  and braceOpposite == -1:
            self.BraceBadLight(braceAtCaret)
        else:
            self.BraceHighlight(braceAtCaret, braceOpposite)
        
    def OnMarginClick(self, evt):
        # fold and unfold as needed
        if evt.GetMargin() == 2:
            if evt.GetShift() and evt.GetControl():
                self.FoldAll()
            else:
                lineClicked = self.LineFromPosition(evt.GetPosition())
                
                if self.GetFoldLevel(lineClicked) & stc.STC_FOLDLEVELHEADERFLAG:
                    if evt.GetShift():
                        self.SetFoldExpanded(lineClicked, True)
                        self.Expand(lineClicked, True, True, 1)
                    elif evt.GetControl():
                        if self.GetFoldExpanded(lineClicked):
                            self.SetFoldExpanded(lineClicked, False)
                            self.Expand(lineClicked, False, True, 0)
                        else:
                            self.SetFoldExpanded(lineClicked, True)
                            self.Expand(lineClicked, True, True, 100)
                    else:
                        self.ToggleFold(lineClicked)
        
    def FoldAll(self):
        lineCount = self.GetLineCount()
        expanding = True
        
        # find out if we are folding or unfolding
        for lineNum in range(lineCount):
            if self.GetFoldLevel(lineNum) & stc.STC_FOLDLEVELHEADERFLAG:
                expanding = not self.GetFoldExpanded(lineNum)
                break
        
        lineNum = 0
        while lineNum < lineCount:
            level = self.GetFoldLevel(lineNum)
            if level & stc.STC_FOLDLEVELHEADERFLAG and \
               (level & stc.STC_FOLDLEVELNUMBERMASK) == stc.STC_FOLDLEVELBASE:
                
                if expanding:
                    self.SetFoldExpanded(lineNum, True)
                    lineNum = self.Expand(lineNum, True)
                    lineNum = lineNum - 1
                else:
                    lastChild = self.GetLastChild(lineNum, -1)
                    self.SetFoldExpanded(lineNum, False)
                    
                    if lastChild > lineNum:
                        self.HideLines(lineNum+1, lastChild)
            
            lineNum = lineNum + 1
        
    def Expand(self, line, doExpand, force=False, visLevels=0, level=-1):
        lastChild = self.GetLastChild(line, level)
        line = line + 1
        
        while line <= lastChild:
            if force:
                if visLevels > 0:
                    self.ShowLines(line, line)
                else:
                    self.HideLines(line, line)
            else:
                if doExpand:
                    self.ShowLines(line, line)
            
            if level == -1:
                level = self.GetFoldLevel(line)
            
            if level & stc.STC_FOLDLEVELHEADERFLAG:
                if force:
                    if visLevels > 1:
                        self.SetFoldExpanded(line, True)
                    else:
                        self.SetFoldExpanded(line, False)
                    
                    line = self.Expand(line, doExpand, force, visLevels-1)
                else:
                    if doExpand and self.GetFoldExpanded(line):
                        line = self.Expand(line, True, force, visLevels-1)
                    else:
                        line = self.Expand(line, False, force, visLevels-1)
            else:
                line = line + 1
        
        return line
