// (c) 2009.  Gabor Grothendieck. 
// This is free software licensed under GPL.
//
// Based on TpR.bas by Philippe Grosjean.
//
// 1. To find the directory where _vimrc goes enter this in vim   :echo $HOME
// 2. Place these two lines in your _vimrc file in which case F3 will 
//    transmit current selection to the clipboard and then paste it into R
//    e.g. ctrl-A F3 copies entire file to R
//
//    map <F3> <C-C><ESC>:!start clip2r.js<CR><CR>
//    imap <F3> <ESC><C-C><ESC>:!start clip2r.js<CR><CR>
//
//    Alternately use these two lines copy entire file when F3 is pressed:
// 
//    map <F3> <C-A><C-C><ESC>:!start clip2r.js<CR><CR>
//    imap <F3> <ESC><C-A><C-C><ESC>:!start clip2r.js<CR><CR>
//
// Note: In the wsh.SendKeys command the strings: 
//       "% x" stands for Alt-space x and has the effect of maximizing Rgui
//       "%{w}1" stands for Alt-w 1 and activates the R console
// 		 "^{v}" stands for ctrl-v and pastes clipboard.

var wsh = new ActiveXObject("Wscript.Shell");
wsh.AppActivate("Rgui");
wsh.SendKeys("% x%{w}1^{v}");

