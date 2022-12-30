"""Preprocess a C source file using gcc and convert the result into
   a token stream

Reference is C99 with additions from C11 and C2x:
* http://www.open-std.org/JTC1/SC22/WG14/www/docs/n1124.pdf
* http://www.quut.com/c/ANSI-C-grammar-l-2011.html
* http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2731.pdf
"""

__docformat__ = "restructuredtext"

from ctypesgen.parser.lex import TOKEN
from ctypesgen.parser import cgrammar


tokens = cgrammar.tokens
keywords = cgrammar.keywords


states = [("DEFINE", "exclusive"), ("PRAGMA", "exclusive")]


_B_ = r"[0-1]"
_O_ = r"[0-7]"
_D_ = r"[0-9]"
_NZ_ = r"[1-9]"
_L_ = r"[a-zA-Z_]"
_A_ = r"[a-zA-Z_0-9]"
_H_ = r"[a-fA-F0-9]"
_HP_ = r"0[xX]"
_BP_ = r"0[bB]"
_E_ = r"([Ee][+-]?" + _D_ + r"+)"
_P_ = r"([Pp][+-]?" + _D_ + r"+)"
_FS_ = r"(f|F|l|L)"
_IS_ = r"(((u|U)(ll|LL|l|L)?)|((ll|LL|l|L)(u|U)?))"
_CP_ = r"(u|U|L)"
_SP_ = r"(u8|u|U|L)"
_ES_ = r"(\\([\'\"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))"
_WS_ = r"[ \t\v\n\f]"

I_CONST_HEX = r"(?P<p1>" + _HP_ + _H_ + r"+" + r")" + _IS_ + r"?"
I_CONST_DEC = r"(?P<p1>" + _NZ_ + _D_ + r"*" + r")" + _IS_ + r"?"
I_CONST_OCT = r"0" + r"(?P<p1>" + _O_ + r"*)" + _IS_ + r"?"
I_CONST_BIN = r"(?P<p1>" + _BP_ + _B_ + "*" + r")" + _IS_ + "?"

F_CONST_1 = r"(?P<sig>" + _D_ + r"+" + r")(?P<exp>" + _E_ + r")" + _FS_ + r"?"
F_CONST_2 = r"(?P<sig>" + _D_ + r"*\." + _D_ + r"+" + r")(?P<exp>" + _E_ + r"?)" + _FS_ + r"?"
F_CONST_3 = r"(?P<sig>" + _D_ + r"+\." + r")(?P<exp>" + _E_ + r"?)" + _FS_ + r"?"
F_CONST_4 = r"(?P<hex>" + _HP_ + _H_ + r"+" + _P_ + r")" + _FS_ + r"?"
F_CONST_5 = r"(?P<hex>" + _HP_ + _H_ + r"*\." + _H_ + r"+" + _P_ + r")" + _FS_ + r"?"
F_CONST_6 = r"(?P<hex>" + _HP_ + _H_ + r"+\." + _P_ + r")" + _FS_ + r"?"

CHARACTER_CONSTANT = _SP_ + r"?'(?P<p1>\\.|[^\\'])+'"
IDENTIFIER = _L_ + _A_ + r"*"

escape_sequence_start_in_string = r"""(\\[0-9a-zA-Z._~!=&\^\-\\?'"])"""
string_char = r"""([^"\\\n]|""" + escape_sequence_start_in_string + ")"
STRING_LITERAL = '"' + string_char + '*"'

# Process line-number directives from the preprocessor
# See https://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html
DIRECTIVE = r'\#\s+(?P<lineno>\d+)\s+"(?P<filename>[^"]+)"[ \d]*\n'


# --------------------------------------------------------------------------
# Token value types
# --------------------------------------------------------------------------

# Numbers represented as int and float types.
# For all other tokens, type is just str representation.


class StringLiteral(str):
    def __new__(cls, value):
        assert value[0] == '"' and value[-1] == '"'
        # Unescaping probably not perfect but close enough.
        value = value[1:-1]  # .decode('string_escape')
        return str.__new__(cls, value)


# --------------------------------------------------------------------------
# Token declarations
# --------------------------------------------------------------------------


# Assignment operators
t_ANY_EQUALS = r"="
t_ANY_RIGHT_ASSIGN = r">>="
t_ANY_LEFT_ASSIGN = r"<<="
t_ANY_ADD_ASSIGN = r"\+="
t_ANY_SUB_ASSIGN = r"-="
t_ANY_MUL_ASSIGN = r"\*="
t_ANY_DIV_ASSIGN = r"/="
t_ANY_MOD_ASSIGN = r"%="
t_ANY_AND_ASSIGN = r"&="
t_ANY_XOR_ASSIGN = r"\^="
t_ANY_OR_ASSIGN = r"\|="

# Operators
t_ANY_PLUS = r"\+"
t_ANY_MINUS = r"-"
t_ANY_TIMES = r"\*"
t_ANY_DIVIDE = r"/"
t_ANY_MOD = r"%"
t_ANY_AND = r"&"
t_ANY_OR = r"\|"
t_ANY_NOT = r"~"
t_ANY_XOR = r"\^"
t_ANY_RIGHT_OP = r">>"
t_ANY_LEFT_OP = r"<<"
t_ANY_INC_OP = r"\+\+"
t_ANY_DEC_OP = r"--"
t_ANY_PTR_OP = r"->"
t_ANY_AND_OP = r"&&"
t_ANY_OR_OP = r"\|\|"
t_ANY_LE_OP = r"<="
t_ANY_GE_OP = r">="
t_ANY_EQ_OP = r"=="
t_ANY_NE_OP = r"!="
t_ANY_LNOT = r"!"
t_ANY_LT = r"<"
t_ANY_GT = r">"
t_ANY_CONDOP = r"\?"


<<<<<<< HEAD
# Delimiters
=======
# Delimeters
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
t_ANY_PERIOD = r"\."
t_ANY_LPAREN = r"\("
t_ANY_RPAREN = r"\)"
t_ANY_ELLIPSIS = r"\.\.\."
t_ANY_LBRACKET = r"\["
t_ANY_RBRACKET = r"\]"
t_ANY_LBRACE = r"\{"
t_ANY_RBRACE = r"\}"
t_ANY_COMMA = r","
t_ANY_SEMI = r";"
t_ANY_COLON = r":"


@TOKEN(DIRECTIVE)
def t_ANY_directive(t):
    m = t.lexer.lexmatch
    t.lexer.filename = m.group("filename")
    t.lexer.lineno = int(m.group("lineno"))
    return None


@TOKEN(F_CONST_1)
def t_ANY_f_const_1(t):
    t.type = "F_CONST_1"
    m = t.lexer.lexmatch
    sig = m.group("sig")
    exp = m.group("exp")
    t.value = sig + exp
    return t


@TOKEN(F_CONST_2)
def t_ANY_f_const_2(t):
    t.type = "F_CONST_2"
    m = t.lexer.lexmatch
    sig = m.group("sig")
    exp = m.group("exp")
    t.value = sig + exp
    return t


@TOKEN(F_CONST_3)
def t_ANY_f_const_3(t):
    t.type = "F_CONST_3"
    m = t.lexer.lexmatch
    sig = m.group("sig")
    exp = m.group("exp")
    t.value = sig + exp
    return t


@TOKEN(F_CONST_4)
def t_ANY_f_const_4(t):
    t.type = "F_CONST_4"
    m = t.lexer.lexmatch
    t.value = 'float.fromhex("' + m.group("hex") + '")'
    return t


@TOKEN(F_CONST_5)
def t_ANY_f_const_5(t):
    t.type = "F_CONST_5"
    m = t.lexer.lexmatch
    t.value = 'float.fromhex("' + m.group("hex") + '")'
    return t


@TOKEN(F_CONST_6)
def t_ANY_f_const_6(t):
    t.type = "F_CONST_6"
    m = t.lexer.lexmatch
    t.value = 'float.fromhex("' + m.group("hex") + '")'
    return t


@TOKEN(I_CONST_BIN)
def t_ANY_i_const_bin(t):
    t.type = "I_CONST_BIN"
    m = t.lexer.lexmatch
    t.value = m.group("p1")
    return t


@TOKEN(I_CONST_HEX)
def t_ANY_i_const_hex(t):
    t.type = "I_CONST_HEX"
    m = t.lexer.lexmatch
    t.value = m.group("p1")
    return t


@TOKEN(I_CONST_DEC)
def t_ANY_i_const_dec(t):
    t.type = "I_CONST_DEC"
    m = t.lexer.lexmatch
    t.value = m.group("p1")
    return t


@TOKEN(I_CONST_OCT)
def t_ANY_i_const_oct(t):
    t.type = "I_CONST_OCT"
    m = t.lexer.lexmatch
    p1 = m.group("p1")
    if not p1:
        t.value = "0"
    else:
        t.value = "0o" + m.group("p1")
    return t


@TOKEN(CHARACTER_CONSTANT)
def t_ANY_character_constant(t):
    t.type = "CHARACTER_CONSTANT"
    m = t.lexer.lexmatch
    p1 = m.group("p1")
    t.value = p1
    return t


@TOKEN(STRING_LITERAL)
def t_ANY_string_literal(t):
    t.type = "STRING_LITERAL"
    t.value = StringLiteral(t.value)
    return t


@TOKEN(IDENTIFIER)
def t_INITIAL_identifier(t):
    t.type = "IDENTIFIER"
    return t


@TOKEN(IDENTIFIER)
def t_DEFINE_identifier(t):
    if t.lexer.next_is_define_name:
        # This identifier is the name of a macro
        # We need to look ahead and see if this macro takes parameters or not.
        if (
            t.lexpos + len(t.value) < t.lexer.lexlen
            and t.lexer.lexdata[t.lexpos + len(t.value)] == "("
        ):

            t.type = "PP_DEFINE_MACRO_NAME"

            # Look ahead and read macro parameter list
            lexdata = t.lexer.lexdata
            pos = t.lexpos + len(t.value) + 1
            while lexdata[pos] not in "\n)":
                pos += 1
            params = lexdata[t.lexpos + len(t.value) + 1 : pos]
            paramlist = [x.strip() for x in params.split(",") if x.strip()]
            t.lexer.macro_params = paramlist

        else:
            t.type = "PP_DEFINE_NAME"

        t.lexer.next_is_define_name = False
    elif t.value in t.lexer.macro_params:
        t.type = "PP_MACRO_PARAM"
    else:
        t.type = "IDENTIFIER"
    return t


@TOKEN(r"\n")
def t_INITIAL_newline(t):
    t.lexer.lineno += 1
    return None


@TOKEN(r"\#undef")
def t_INITIAL_pp_undefine(t):
    t.type = "PP_UNDEFINE"
    t.lexer.begin("DEFINE")
    t.lexer.next_is_define_name = True
    t.lexer.macro_params = set()
    return t


@TOKEN(r"\#define")
def t_INITIAL_pp_define(t):
    t.type = "PP_DEFINE"
    t.lexer.begin("DEFINE")
    t.lexer.next_is_define_name = True
    t.lexer.macro_params = set()
    return t


@TOKEN(r"\#pragma")
def t_INITIAL_pragma(t):
    t.type = "PRAGMA"
    t.lexer.begin("PRAGMA")
    return t


@TOKEN(r"pack")
def t_PRAGMA_pack(t):
    t.type = "PRAGMA_PACK"
    return t


@TOKEN(r"\n")
def t_PRAGMA_newline(t):
    t.type = "PRAGMA_END"
    t.lexer.begin("INITIAL")
    t.lexer.lineno += 1
    return t


@TOKEN(IDENTIFIER)
def t_PRAGMA_identifier(t):
    t.type = "IDENTIFIER"
    return t


def t_PRAGMA_error(t):
    t.type = "OTHER"
    t.value = t.value[0:30]
    t.lexer.lexpos += 1  # Skip it if it's an error in a #pragma
    return t


@TOKEN(r"\n")
def t_DEFINE_newline(t):
    t.type = "PP_END_DEFINE"
    t.lexer.begin("INITIAL")
    t.lexer.lineno += 1
    del t.lexer.macro_params

    # Damage control in case the token immediately after the #define failed
    # to handle this
    t.lexer.next_is_define_name = False
    return t


@TOKEN(r"(\#\#)|(\#)")
def t_DEFINE_pp_param_op(t):
    if t.value == "#":
        t.type = "PP_STRINGIFY"
    else:
        t.type = "PP_IDENTIFIER_PASTE"
    return t


def t_INITIAL_error(t):
    t.type = "OTHER"
    return t


def t_DEFINE_error(t):
    t.type = "OTHER"
    t.value = t.value[0]
    t.lexer.lexpos += 1  # Skip it if it's an error in a #define
    return t


t_ANY_ignore = " \t\v\f\r"
