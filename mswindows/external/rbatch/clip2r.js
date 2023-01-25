// (c) 2009.  Gabor Grothendieck. 
// This is free software licensed under GPL.
//
// To use with vim place these two lines in your _vimrc file
// To find the directory where _vimrc goes enter this in vim   :echo $HOME
// 
//    map <F3> <C-C><ESC>:!start cmd /c clip2r.js<CR><CR>
//    imap <F3> <ESC><C-C><ESC>:!start cmd /c clip2r.js<CR><CR>

var wsh = new ActiveXObject("Wscript.Shell");
wsh.AppActivate("Rgui");
wsh.SendKeys("% x%{w}1^{v}");

