#!/usr/bin/env python3

"""This is a yacc grammar for C.

Derived from ANSI C grammar:
  * Lexicon: http://www.lysator.liu.se/c/ANSI-C-grammar-l.html
  * Grammar: http://www.lysator.liu.se/c/ANSI-C-grammar-y.html

Reference is C99:
  * http://www.open-std.org/JTC1/SC22/WG14/www/docs/n1124.pdf

"""

__docformat__ = "restructuredtext"

if __name__ == "__main__":
    # NOTE if this file is modified, run to generate a new parsetab.py
    #   E.g.:
    #       env PYTHONPATH=. python ctypesgen/parser/cgrammar.py
    # new_parsetab.py is generated in the current directory and needs to be
    # manually copied (after inspection) to ctypesgen/parser/parsetab.py
    import sys, os

    sys.path.insert(0, os.path.join(os.path.pardir, os.path.pardir))
    from ctypesgen.parser.cgrammar import main

    main()
    sys.exit()

import operator
import os.path
import re
import sys
import time
import warnings

from . import yacc
from .. import expressions
from ..ctypedescs import anonymous_struct_tagnum
from . import cdeclarations

tokens = (
    "PP_DEFINE",
    "PP_DEFINE_NAME",
    "PP_DEFINE_MACRO_NAME",
    "PP_UNDEFINE",
    "PP_MACRO_PARAM",
    "PP_STRINGIFY",
    "PP_IDENTIFIER_PASTE",
    "PP_END_DEFINE",
    "PRAGMA",
    "PRAGMA_PACK",
    "PRAGMA_END",
    "IDENTIFIER",
    "CONSTANT",
    "CHARACTER_CONSTANT",
    "STRING_LITERAL",
    "SIZEOF",
    "PTR_OP",
    "INC_OP",
    "DEC_OP",
    "LEFT_OP",
    "RIGHT_OP",
    "LE_OP",
    "GE_OP",
    "EQ_OP",
    "NE_OP",
    "AND_OP",
    "OR_OP",
    "MUL_ASSIGN",
    "DIV_ASSIGN",
    "MOD_ASSIGN",
    "ADD_ASSIGN",
    "SUB_ASSIGN",
    "LEFT_ASSIGN",
    "RIGHT_ASSIGN",
    "AND_ASSIGN",
    "XOR_ASSIGN",
    "OR_ASSIGN",
    "PERIOD",
    "TYPE_NAME",
    "TYPEDEF",
    "EXTERN",
    "STATIC",
    "AUTO",
    "REGISTER",
    "_BOOL",
    "CHAR",
    "SHORT",
    "INT",
    "LONG",
    "SIGNED",
    "UNSIGNED",
    "FLOAT",
    "DOUBLE",
    "CONST",
    "VOLATILE",
    "VOID",
    "STRUCT",
    "UNION",
    "ENUM",
    "ELLIPSIS",
    "CASE",
    "DEFAULT",
    "IF",
    "ELSE",
    "SWITCH",
    "WHILE",
    "DO",
    "FOR",
    "GOTO",
    "CONTINUE",
    "BREAK",
    "RETURN",
    "__ASM__",
    "__ATTRIBUTE__",
    # with new code that accepts arbitrary attributes-->must not be keywords
    # "PACKED",
    # "ALIGNED",
    # "TRANSPARENT_UNION",
    # "STDCALL",
    # "CDECL",
)

keywords = [
    "auto",
    "_Bool",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "int",
    "long",
    "register",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "void",
    "volatile",
    "while",
    "__asm__",
    "__attribute__",
    # with new code that accepts arbitrary attributes-->must not be keywords
    # "packed",
    # "aligned",
    # "transparent_union",
    # "stdcall",
    # "cdecl",
]

precedence = (("nonassoc", "IF"), ("nonassoc", "ELSE"))


def p_translation_unit(p):
    """translation_unit :
                        | translation_unit external_declaration
                        | translation_unit directive
    """
    # Starting production.
    # Allow empty production so that files with no declarations are still
    #    valid.
    # Intentionally empty


def p_identifier(p):
    """identifier : IDENTIFIER
                  | IDENTIFIER PP_IDENTIFIER_PASTE identifier
                  | PP_MACRO_PARAM PP_IDENTIFIER_PASTE identifier
                  | IDENTIFIER PP_IDENTIFIER_PASTE PP_MACRO_PARAM
                  | PP_MACRO_PARAM PP_IDENTIFIER_PASTE PP_MACRO_PARAM
    """
    if len(p) == 2:
        p[0] = expressions.IdentifierExpressionNode(p[1])
    else:
        # Should it be supported? It wouldn't be very hard to add support.
        # Basically, it would involve a new ExpressionNode called
        # an IdentifierPasteExpressionNode that took a list of strings and
        # ParameterExpressionNodes. Then it would generate code like
        # "locals()['%s' + '%s' + ...]" where %s was substituted with the
        # elements of the list. I haven't supported it yet because I think
        # it's unnecessary and a little too powerful.
        p[0] = expressions.UnsupportedExpressionNode(
            "Identifier pasting is " "not supported by ctypesgen."
        )


def p_constant(p):
    """constant : CONSTANT
                | CHARACTER_CONSTANT
    """
    constant = p[1]

    if constant[0] == "'":
        # Character constant
        value = constant[1:-1]
        if isinstance(value, str):
            value = value.encode()
    else:
        # This is a value formatted the way that the preprocessor formats
        # numeric constants. It puts a prefix "l", "i", or "f" to indicate
        # if it should be converted into an integer, long or float.
        prefix = constant[0]
        constant = constant[1:]
        if prefix == "i":
            value = int(constant)
        elif prefix == "l":
            value = int(constant)
        else:
            value = float(constant)

    p[0] = expressions.ConstantExpressionNode(value)


def p_string_literal(p):
    """string_literal : STRING_LITERAL"""
    p[0] = expressions.ConstantExpressionNode(p[1])


def p_multi_string_literal(p):
    """multi_string_literal : string_literal
                            | macro_param
                            | multi_string_literal string_literal
                            | multi_string_literal macro_param
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = expressions.BinaryExpressionNode(
            "string concatenation", (lambda x, y: x + y), "(%s + %s)", (False, False), p[1], p[2]
        )


def p_macro_param(p):
    """macro_param : PP_MACRO_PARAM
                   | PP_STRINGIFY PP_MACRO_PARAM
    """
    if len(p) == 2:
        p[0] = expressions.ParameterExpressionNode(p[1])
    else:
        p[0] = expressions.ParameterExpressionNode(p[2])


def p_primary_expression(p):
    """primary_expression : identifier
                          | constant
                          | multi_string_literal
                          | '(' expression ')'
    """
    if p[1] == "(":
        p[0] = p[2]
    else:
        p[0] = p[1]


def p_postfix_expression(p):
    """postfix_expression : primary_expression
                  | postfix_expression '[' expression ']'
                  | postfix_expression '(' ')'
                  | postfix_expression '(' argument_expression_list ')'
                  | postfix_expression PERIOD IDENTIFIER
                  | postfix_expression PTR_OP IDENTIFIER
                  | postfix_expression INC_OP
                  | postfix_expression DEC_OP
    """

    if len(p) == 2:
        p[0] = p[1]

    elif p[2] == "[":
        p[0] = expressions.BinaryExpressionNode(
            "array access", (lambda a, b: a[b]), "(%s [%s])", (True, False), p[1], p[3]
        )

    elif p[2] == "(":
        if p[3] == ")":
            p[0] = expressions.CallExpressionNode(p[1], [])
        else:
            p[0] = expressions.CallExpressionNode(p[1], p[3])

    elif p[2] == ".":
        p[0] = expressions.AttributeExpressionNode(
            (lambda x, a: getattr(x, a)), "(%s.%s)", p[1], p[3]
        )

    elif p[2] == "->":
        p[0] = expressions.AttributeExpressionNode(
            (lambda x, a: getattr(x.contents, a)), "(%s.contents.%s)", p[1], p[3]
        )

    elif p[2] == "++":
        p[0] = expressions.UnaryExpressionNode(
            "increment", (lambda x: x + 1), "(%s + 1)", False, p[1]
        )

    elif p[2] == "--":
        p[0] = expressions.UnaryExpressionNode(
            "decrement", (lambda x: x - 1), "(%s - 1)", False, p[1]
        )


def p_argument_expression_list(p):
    """argument_expression_list : assignment_expression
                        | argument_expression_list ',' assignment_expression
                        | type_name
                        | argument_expression_list ',' type_name
    """
    if len(p) == 4:
        p[1].append(p[3])
        p[0] = p[1]
    else:
        p[0] = [p[1]]


def p_asm_expression(p):
    """asm_expression : __ASM__ volatile_opt '(' string_literal ')'
                      | __ASM__ volatile_opt '(' string_literal ':' str_opt_expr_pair_list ')'
                      | __ASM__ volatile_opt '(' string_literal ':' str_opt_expr_pair_list ':' str_opt_expr_pair_list ')'
                      | __ASM__ volatile_opt '(' string_literal ':' str_opt_expr_pair_list ':' str_opt_expr_pair_list ':' str_opt_expr_pair_list ')'
    """

    # Definitely not ISO C, adapted from example ANTLR GCC parser at
    #  http://www.antlr.org/grammar/cgram//grammars/GnuCParser.g
    # but more lenient (expressions permitted in optional final part, when
    # they shouldn't be -- avoids shift/reduce conflict with
    # str_opt_expr_pair_list).

    p[0] = expressions.UnsupportedExpressionNode("This node is ASM assembler.")


def p_str_opt_expr_pair_list(p):
    """str_opt_expr_pair_list :
                              | str_opt_expr_pair
                              | str_opt_expr_pair_list ',' str_opt_expr_pair
    """


def p_str_opt_expr_pair(p):
    """str_opt_expr_pair : string_literal
                         | string_literal '(' expression ')'
     """


def p_volatile_opt(p):
    """volatile_opt :
                    | VOLATILE
    """


prefix_ops_dict = {
    "++": ("increment", (lambda x: x + 1), "(%s + 1)", False),
    "--": ("decrement", (lambda x: x - 1), "(%s - 1)", False),
    "&": ("reference ('&')", None, "pointer(%s)", True),
    "*": ("dereference ('*')", None, "(%s[0])", True),
    "+": ("unary '+'", (lambda x: x), "%s", True),
    "-": ("negation", (lambda x: -x), "(-%s)", False),
    "~": ("inversion", (lambda x: ~x), "(~%s)", False),
    "!": ("logical not", (lambda x: not x), "(not %s)", True),
}


def p_unary_expression(p):
    """unary_expression : postfix_expression
                        | INC_OP unary_expression
                        | DEC_OP unary_expression
                        | unary_operator cast_expression
                        | SIZEOF unary_expression
                        | SIZEOF '(' type_name ')'
                        | asm_expression
    """
    if len(p) == 2:
        p[0] = p[1]

    elif p[1] == "sizeof":
        if len(p) == 5:
            p[0] = expressions.SizeOfExpressionNode(p[3])
        else:
            p[0] = expressions.SizeOfExpressionNode(p[2])

    else:
        name, op, format, can_be_ctype = prefix_ops_dict[p[1]]
        p[0] = expressions.UnaryExpressionNode(name, op, format, can_be_ctype, p[2])


def p_unary_operator(p):
    """unary_operator : '&'
                      | '*'
                      | '+'
                      | '-'
                      | '~'
                      | '!'
    """
    p[0] = p[1]


def p_cast_expression(p):
    """cast_expression : unary_expression
                       | '(' type_name ')' cast_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = expressions.TypeCastExpressionNode(p[4], p[2])


mult_ops_dict = {
    "*": ("multiplication", (lambda x, y: x * y), "(%s * %s)"),
    "/": ("division", (lambda x, y: x / y), "(%s / %s)"),
    "%": ("modulo", (lambda x, y: x % y), "(%s %% %s)"),
}


def p_multiplicative_expression(p):
    """multiplicative_expression : cast_expression
                                 | multiplicative_expression '*' cast_expression
                                 | multiplicative_expression '/' cast_expression
                                 | multiplicative_expression '%' cast_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        name, op, format = mult_ops_dict[p[2]]
        p[0] = expressions.BinaryExpressionNode(name, op, format, (False, False), p[1], p[3])


add_ops_dict = {
    "+": ("addition", (lambda x, y: x + y), "(%s + %s)"),
    "-": ("subtraction", (lambda x, y: x - y), "(%s - %s)"),
}


def p_additive_expression(p):
    """additive_expression : multiplicative_expression
                           | additive_expression '+' multiplicative_expression
                           | additive_expression '-' multiplicative_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        name, op, format = add_ops_dict[p[2]]
        p[0] = expressions.BinaryExpressionNode(name, op, format, (False, False), p[1], p[3])


shift_ops_dict = {
    ">>": ("right shift", (lambda x, y: x >> y), "(%s >> %s)"),
    "<<": ("left shift", (lambda x, y: x << y), "(%s << %s)"),
}


def p_shift_expression(p):
    """shift_expression : additive_expression
                        | shift_expression LEFT_OP additive_expression
                        | shift_expression RIGHT_OP additive_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        name, op, format = shift_ops_dict[p[2]]
        p[0] = expressions.BinaryExpressionNode(name, op, format, (False, False), p[1], p[3])


rel_ops_dict = {
    ">": ("greater-than", (lambda x, y: x > y), "(%s > %s)"),
    "<": ("less-than", (lambda x, y: x < y), "(%s < %s)"),
    ">=": ("greater-than-equal", (lambda x, y: x >= y), "(%s >= %s)"),
    "<=": ("less-than-equal", (lambda x, y: x <= y), "(%s <= %s)"),
}


def p_relational_expression(p):
    """relational_expression : shift_expression
                             | relational_expression '<' shift_expression
                             | relational_expression '>' shift_expression
                             | relational_expression LE_OP shift_expression
                             | relational_expression GE_OP shift_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        name, op, format = rel_ops_dict[p[2]]
        p[0] = expressions.BinaryExpressionNode(name, op, format, (False, False), p[1], p[3])


equality_ops_dict = {
    "==": ("equals", (lambda x, y: x == y), "(%s == %s)"),
    "!=": ("not equals", (lambda x, y: x != y), "(%s != %s)"),
}


def p_equality_expression(p):
    """equality_expression : relational_expression
                           | equality_expression EQ_OP relational_expression
                           | equality_expression NE_OP relational_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        name, op, format = equality_ops_dict[p[2]]
        p[0] = expressions.BinaryExpressionNode(name, op, format, (False, False), p[1], p[3])


def p_and_expression(p):
    """and_expression : equality_expression
                      | and_expression '&' equality_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = expressions.BinaryExpressionNode(
            "bitwise and", (lambda x, y: x & y), "(%s & %s)", (False, False), p[1], p[3]
        )


def p_exclusive_or_expression(p):
    """exclusive_or_expression : and_expression
                               | exclusive_or_expression '^' and_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = expressions.BinaryExpressionNode(
            "bitwise xor", (lambda x, y: x ^ y), "(%s ^ %s)", (False, False), p[1], p[3]
        )


def p_inclusive_or_expression(p):
    """inclusive_or_expression : exclusive_or_expression
                   | inclusive_or_expression '|' exclusive_or_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = expressions.BinaryExpressionNode(
            "bitwise or", (lambda x, y: x | y), "(%s | %s)", (False, False), p[1], p[3]
        )


def p_logical_and_expression(p):
    """logical_and_expression : inclusive_or_expression
                  | logical_and_expression AND_OP inclusive_or_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = expressions.BinaryExpressionNode(
            "logical and", (lambda x, y: x and y), "(%s and %s)", (True, True), p[1], p[3]
        )


def p_logical_or_expression(p):
    """logical_or_expression : logical_and_expression
                  | logical_or_expression OR_OP logical_and_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = expressions.BinaryExpressionNode(
            "logical and", (lambda x, y: x or y), "(%s or %s)", (True, True), p[1], p[3]
        )


def p_conditional_expression(p):
    """conditional_expression : logical_or_expression
          | logical_or_expression '?' expression ':' conditional_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = expressions.ConditionalExpressionNode(p[1], p[3], p[5])


assign_ops_dict = {
    "*=": ("multiply", (lambda x, y: x * y), "(%s * %s)"),
    "/=": ("divide", (lambda x, y: x / y), "(%s / %s)"),
    "%=": ("modulus", (lambda x, y: x % y), "(%s % %s)"),
    "+=": ("addition", (lambda x, y: x + y), "(%s + %s)"),
    "-=": ("subtraction", (lambda x, y: x - y), "(%s - %s)"),
    "<<=": ("left shift", (lambda x, y: x << y), "(%s << %s)"),
    ">>=": ("right shift", (lambda x, y: x >> y), "(%s >> %s)"),
    "&=": ("bitwise and", (lambda x, y: x & y), "(%s & %s)"),
    "^=": ("bitwise xor", (lambda x, y: x ^ y), "(%s ^ %s)"),
    "|=": ("bitwise or", (lambda x, y: x | y), "(%s | %s)"),
}


def p_assignment_expression(p):
    """assignment_expression : conditional_expression
                 | unary_expression assignment_operator assignment_expression
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        # In C, the value of (x*=3) is the same as (x*3). We support that here.
        # However, we don't support the change in the value of x.
        if p[2] == "=":
            p[0] = p[3]
        else:
            name, op, format = assign_ops_dict[p[2]]
            p[0] = expressions.BinaryExpressionNode(name, op, format, (True, True), p[1], p[3])


def p_assignment_operator(p):
    """assignment_operator : '='
                           | MUL_ASSIGN
                           | DIV_ASSIGN
                           | MOD_ASSIGN
                           | ADD_ASSIGN
                           | SUB_ASSIGN
                           | LEFT_ASSIGN
                           | RIGHT_ASSIGN
                           | AND_ASSIGN
                           | XOR_ASSIGN
                           | OR_ASSIGN
    """
    p[0] = p[1]


def p_expression(p):
    """expression : assignment_expression
                  | expression ',' assignment_expression
    """
    p[0] = p[1]
    # We don't need to support sequence expressions...


def p_constant_expression(p):
    """constant_expression : conditional_expression
    """
    p[0] = p[1]


def p_declaration(p):
    """declaration : declaration_impl ';'
    """
    # The ';' must be here, not in 'declaration', as declaration needs to
    # be executed before the ';' is shifted (otherwise the next lookahead will
    # be read, which may be affected by this declaration if its a typedef.


def p_declaration_impl(p):
    """declaration_impl : declaration_specifier_list
                        | declaration_specifier_list init_declarator_list
    """
    declaration = cdeclarations.Declaration()
    cdeclarations.apply_specifiers(p[1], declaration)

    if len(p) == 2:
        filename = p.slice[1].filename
        lineno = p.slice[1].lineno
        p.parser.cparser.impl_handle_declaration(declaration, filename, lineno)
        return

    filename = p.slice[2].filename
    lineno = p.slice[2].lineno
    for declarator in p[2]:
        declaration.declarator = declarator
        p.parser.cparser.impl_handle_declaration(declaration, filename, lineno)


# shift/reduce conflict with p_statement_error.
# def p_declaration_error(p):
#    '''declaration : error ';'
#    '''
#    # Error resynchronisation catch-all


def p_declaration_specifier_list(p):
    """declaration_specifier_list : gcc_attributes declaration_specifier gcc_attributes
                      | declaration_specifier_list declaration_specifier gcc_attributes
    """
    if type(p[1]) == cdeclarations.Attrib:
        p[0] = (p[1], p[2], p[3])
        p.slice[0].filename = p.slice[2].filename
        p.slice[0].lineno = p.slice[2].lineno
    else:
        p[0] = p[1] + (p[2], p[3])
        p.slice[0].filename = p.slice[1].filename
        p.slice[0].lineno = p.slice[1].lineno


def p_declaration_specifier(p):
    """declaration_specifier : storage_class_specifier
                             | type_specifier
                             | type_qualifier
    """
    p[0] = p[1]


def p_init_declarator_list(p):
    """init_declarator_list : init_declarator
                            | init_declarator_list ',' init_declarator
    """
    if len(p) > 2:
        p[0] = p[1] + (p[3],)
    else:
        p[0] = (p[1],)


def p_init_declarator(p):
    """init_declarator : declarator gcc_attributes
                       | declarator gcc_attributes '=' initializer
    """
    p[0] = p[1]
    p[0].attrib.update(p[2])
    p.slice[0].filename = p.slice[1].filename
    p.slice[0].lineno = p.slice[1].lineno
    if len(p) > 3:
        p[0].initializer = p[4]


def p_storage_class_specifier(p):
    """storage_class_specifier : TYPEDEF
                               | EXTERN
                               | STATIC
                               | AUTO
                               | REGISTER
    """
    p[0] = cdeclarations.StorageClassSpecifier(p[1])


def p_type_specifier(p):
    """type_specifier : VOID
                      | _BOOL
                      | CHAR
                      | SHORT
                      | INT
                      | LONG
                      | FLOAT
                      | DOUBLE
                      | SIGNED
                      | UNSIGNED
                      | struct_or_union_specifier
                      | enum_specifier
                      | TYPE_NAME
    """
    if type(p[1]) in (cdeclarations.StructTypeSpecifier, cdeclarations.EnumSpecifier):
        p[0] = p[1]
    else:
        p[0] = cdeclarations.TypeSpecifier(p[1])


def p_struct_or_union_specifier(p):
    """struct_or_union_specifier : struct_or_union gcc_attributes IDENTIFIER '{' struct_declaration_list '}'
         | struct_or_union gcc_attributes TYPE_NAME '{' struct_declaration_list '}'
         | struct_or_union gcc_attributes '{' struct_declaration_list '}'
         | struct_or_union gcc_attributes IDENTIFIER
         | struct_or_union gcc_attributes TYPE_NAME
    """
    # format of grammar for gcc_attributes taken from c-parser.c in GCC source.
    # The TYPE_NAME ones are dodgy, needed for Apple headers
    # CoreServices.framework/Frameworks/CarbonCore.framework/Headers/Files.h.
    # CoreServices.framework/Frameworks/OSServices.framework/Headers/Power.h
    tag = None
    decl = None

    if len(p) == 4:  # struct [attributes] <id/typname>
        tag = p[3]
    elif p[3] == "{":
        tag, decl = anonymous_struct_tagnum(), p[4]
    else:
        tag, decl = p[3], p[5]

    p[0] = cdeclarations.StructTypeSpecifier(p[1], p[2], tag, decl)

    p.slice[0].filename = p.slice[1].filename
    p.slice[0].lineno = p.slice[1].lineno
    p[0].filename = p.slice[1].filename
    p[0].lineno = p.slice[1].lineno


def p_struct_or_union(p):
    """struct_or_union : STRUCT
                       | UNION
    """
    p[0] = p[1] == "union"


def p_gcc_attributes(p):
    """gcc_attributes :
                      | gcc_attributes gcc_attribute
    """
    # Allow empty production on attributes (take from c-parser.c in GCC source)
    if len(p) == 1:
        p[0] = cdeclarations.Attrib()
    else:
        p[0] = p[1]
        p[0].update(p[2])


def p_gcc_attribute(p):
    """gcc_attribute : __ATTRIBUTE__ '(' '(' gcc_attrib_list ')' ')'
    """
    p[0] = cdeclarations.Attrib()
    p[0].update(p[4])


def p_gcc_attrib_list(p):
    """gcc_attrib_list : gcc_attrib
                       | gcc_attrib_list ',' gcc_attrib
    """
    if len(p) == 2:
        p[0] = (p[1],)
    else:
        p[0] = p[1] + (p[3],)


def p_gcc_attrib(p):
    """gcc_attrib :
                  | IDENTIFIER
                  | IDENTIFIER '(' argument_expression_list ')'
    """
    if len(p) == 1:
        p[0] = (None, None)
    elif len(p) == 2:
        p[0] = (p[1], True)
    elif len(p) == 5:
        p[0] = (p[1], p[3])
    else:
        raise RuntimeError("Should never reach this part of the grammar")


def p_struct_declaration_list(p):
    """struct_declaration_list : struct_declaration
                               | struct_declaration_list struct_declaration
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = p[1] + p[2]


def p_struct_declaration(p):
    """struct_declaration : specifier_qualifier_list struct_declarator_list ';'
                          | specifier_qualifier_list ';'
    """
    # p[0] returned is a tuple, to handle multiple declarators in one
    # declaration.
    r = ()
    if len(p) >= 4:
        for declarator in p[2]:
            declaration = cdeclarations.Declaration()
            cdeclarations.apply_specifiers(p[1], declaration)
            declaration.declarator = declarator
            r += (declaration,)
    else:
        # anonymous field (C11/GCC extension)
        declaration = cdeclarations.Declaration()
        cdeclarations.apply_specifiers(p[1], declaration)
        r = (declaration,)

    p[0] = r


def p_specifier_qualifier_list(p):
    """specifier_qualifier_list : gcc_attributes specifier_qualifier gcc_attributes
                      | specifier_qualifier_list specifier_qualifier gcc_attributes
    """
    if type(p[1]) == cdeclarations.Attrib:
        p[0] = (p[1], p[2], p[3])
    else:
        p[0] = p[1] + (p[2], p[3])


def p_specifier_qualifier(p):
    """specifier_qualifier : type_specifier
                           | type_qualifier
    """
    p[0] = p[1]


def p_struct_declarator_list(p):
    """struct_declarator_list : struct_declarator
                              | struct_declarator_list ',' struct_declarator
    """
    if len(p) == 2:
        p[0] = (p[1],)
    else:
        p[0] = p[1] + (p[3],)


def p_struct_declarator(p):
    """struct_declarator : declarator                         gcc_attributes
                         | ':' constant_expression            gcc_attributes
                         | declarator ':' constant_expression gcc_attributes
    """
    if p[1] == ":":
        p[0] = cdeclarations.Declarator()
        p[0].bitfield = p[2]
    else:
        p[0] = p[1]
        # Bitfield support
        if p[2] == ":":
            p[0].bitfield = p[3]

    p[0].attrib.update(p[len(p) - 1])


def p_enum_specifier(p):
    """enum_specifier : ENUM '{' enumerator_list '}'
                      | ENUM IDENTIFIER '{' enumerator_list '}'
                      | ENUM IDENTIFIER
    """
    if len(p) == 5:
        p[0] = cdeclarations.EnumSpecifier(None, p[3])
    elif len(p) == 6:
        p[0] = cdeclarations.EnumSpecifier(p[2], p[4])
    else:
        p[0] = cdeclarations.EnumSpecifier(p[2], ())

    p[0].filename = p.slice[0].filename
    p[0].lineno = p.slice[0].lineno


def p_enumerator_list(p):
    """enumerator_list : enumerator_list_iso
                       | enumerator_list_iso ','
    """
    # Apple headers sometimes have trailing ',' after enumerants, which is
    # not ISO C.
    p[0] = p[1]


def p_enumerator_list_iso(p):
    """enumerator_list_iso : enumerator
                           | enumerator_list_iso ',' enumerator
    """
    if len(p) == 2:
        p[0] = (p[1],)
    else:
        p[0] = p[1] + (p[3],)


def p_enumerator(p):
    """enumerator : IDENTIFIER
                  | IDENTIFIER '=' constant_expression
    """
    if len(p) == 2:
        p[0] = cdeclarations.Enumerator(p[1], None)
    else:
        p[0] = cdeclarations.Enumerator(p[1], p[3])


def p_type_qualifier(p):
    """type_qualifier : CONST
                      | VOLATILE
    """
    p[0] = cdeclarations.TypeQualifier(p[1])


def p_declarator(p):
    """declarator : pointer direct_declarator
                  | direct_declarator
    """
    if len(p) > 2:
        p[0] = p[1]
        ptr = p[1]
        while ptr.pointer:
            ptr = ptr.pointer
        ptr.pointer = p[2]
        p[2].attrib.update(p[1].attrib)
    else:
        p[0] = p[1]


def p_direct_declarator(p):
    """direct_declarator : IDENTIFIER
                         | '(' gcc_attributes declarator ')'
                         | direct_declarator '[' constant_expression ']'
                         | direct_declarator '[' ']'
                         | direct_declarator '(' parameter_type_list ')'
                         | direct_declarator '(' identifier_list ')'
                         | direct_declarator '(' ')'
    """
    if isinstance(p[1], cdeclarations.Declarator):
        p[0] = p[1]
        if p[2] == "[":
            a = cdeclarations.Array()
            a.array = p[0].array
            p[0].array = a
            if p[3] != "]":
                a.size = p[3]
        else:
            if p[3] == ")":
                p[0].parameters = ()
            else:
                p[0].parameters = p[3]
    elif p[1] == "(":
        p[0] = p[3]
        p[3].attrib.update(p[2])
    else:
        p[0] = cdeclarations.Declarator()
        p[0].identifier = p[1]

    # Check parameters for (void) and simplify to empty tuple.
    if p[0].parameters and len(p[0].parameters) == 1:
        param = p[0].parameters[0]
        if param.type.specifiers == ["void"] and not param.declarator:
            p[0].parameters = ()


def p_pointer(p):
    """pointer : '*'
               | '*' type_qualifier_list
               | '*' pointer
               | '*' type_qualifier_list pointer
    """
    if len(p) == 2:
        p[0] = cdeclarations.Pointer()
    elif len(p) == 3 and isinstance(p[2], cdeclarations.Pointer):
        p[0] = cdeclarations.Pointer()
        p[0].pointer = p[2]
        p[0].attrib.update(p[2].attrib)
    else:
        p[0] = cdeclarations.Pointer()
        for tq in p[2]:
            if isinstance(tq, cdeclarations.Attrib):
                p[0].attrib.update(tq)
            else:
                p[0].qualifiers += (tq,)

        if len(p) == 4:
            p[0].pointer = p[3]
            p[0].attrib.update(p[3].attrib)


def p_type_qualifier_list(p):
    """type_qualifier_list : type_qualifier
                           | gcc_attribute
                           | type_qualifier_list type_qualifier
                           | type_qualifier_list gcc_attribute
    """
    if len(p) > 2:
        p[0] = p[1] + (p[2],)
    else:
        p[0] = (p[1],)


def p_parameter_type_list(p):
    """parameter_type_list : parameter_list
                           | parameter_list ',' ELLIPSIS
    """
    if len(p) > 2:
        p[0] = p[1] + (p[3],)
    else:
        p[0] = p[1]


def p_parameter_list(p):
    """parameter_list : parameter_declaration
                      | parameter_list ',' parameter_declaration
    """
    if len(p) > 2:
        p[0] = p[1] + (p[3],)
    else:
        p[0] = (p[1],)


def p_parameter_declaration(p):
    """parameter_declaration : declaration_specifier_list declarator          gcc_attributes
                             | declaration_specifier_list abstract_declarator
                             | declaration_specifier_list
    """
    p[0] = cdeclarations.Parameter()
    specs = p[1]

    if len(p) == 4:
        # add the attributes as a final specifier
        specs += (p[3],)
        p[0].declarator = p[2]
    elif len(p) == 3:
        p[0].declarator = p[2]

    cdeclarations.apply_specifiers(specs, p[0])


def p_identifier_list(p):
    """identifier_list : IDENTIFIER
                       | identifier_list ',' IDENTIFIER
    """
    param = cdeclarations.Parameter()
    param.declarator = cdeclarations.Declarator()
    if len(p) > 2:
        param.declarator.identifier = p[3]
        p[0] = p[1] + (param,)
    else:
        param.declarator.identifier = p[1]
        p[0] = (param,)


def p_type_name(p):
    """type_name : specifier_qualifier_list
                 | specifier_qualifier_list abstract_declarator
    """
    typ = p[1]
    if len(p) == 3:
        declarator = p[2]
    else:
        declarator = None

    declaration = cdeclarations.Declaration()
    declaration.declarator = declarator
    cdeclarations.apply_specifiers(typ, declaration)
    ctype = p.parser.cparser.get_ctypes_type(declaration.type, declaration.declarator)
    p[0] = ctype


def p_abstract_declarator(p):
    """abstract_declarator : pointer
                           | direct_abstract_declarator         gcc_attributes
                           | pointer direct_abstract_declarator gcc_attributes
    """
    if len(p) == 2:
        p[0] = p[1]
        ptr = p[0]
        while ptr.pointer:
            ptr = ptr.pointer
        # Only if doesn't already terminate in a declarator
        if type(ptr) == cdeclarations.Pointer:
            ptr.pointer = cdeclarations.Declarator()
            ptr.pointer.attrib.update(p[1].attrib)
        else:
            ptr.attrib.update(p[1].attrib)
    elif len(p) == 3:
        p[0] = p[1]
        p[1].attrib.update(p[2])
    else:
        p[0] = p[1]
        ptr = p[0]
        while ptr.pointer:
            ptr = ptr.pointer
        ptr.pointer = p[2]
        p[2].attrib.update(p[1].attrib)
        p[2].attrib.update(p[3])


def p_direct_abstract_declarator(p):
    """direct_abstract_declarator : '(' gcc_attributes abstract_declarator ')'
                      | '[' ']'
                      | '[' constant_expression ']'
                      | direct_abstract_declarator '[' ']'
                      | direct_abstract_declarator '[' constant_expression ']'
                      | '(' ')'
                      | '(' parameter_type_list ')'
                      | direct_abstract_declarator '(' ')'
                      | direct_abstract_declarator '(' parameter_type_list ')'
    """
    if p[1] == "(" and isinstance(p[3], cdeclarations.Declarator):
        p[0] = p[3]
        p[3].attrib.update(p[2])
    else:
        if isinstance(p[1], cdeclarations.Declarator):
            p[0] = p[1]
            if p[2] == "[":
                a = cdeclarations.Array()
                a.array = p[0].array
                p[0].array = a
                if p[3] != "]":
                    p[0].array.size = p[3]
            elif p[2] == "(":
                if p[3] == ")":
                    p[0].parameters = ()
                else:
                    p[0].parameters = p[3]
        else:
            p[0] = cdeclarations.Declarator()
            if p[1] == "[":
                p[0].array = cdeclarations.Array()
                if p[2] != "]":
                    p[0].array.size = p[2]
            elif p[1] == "(":
                if p[2] == ")":
                    p[0].parameters = ()
                else:
                    p[0].parameters = p[2]

    # Check parameters for (void) and simplify to empty tuple.
    if p[0].parameters and len(p[0].parameters) == 1:
        param = p[0].parameters[0]
        if param.type.specifiers == ["void"] and not param.declarator:
            p[0].parameters = ()


def p_initializer(p):
    """initializer : assignment_expression
                   | '{' initializer_list '}'
                   | '{' initializer_list ',' '}'
    """


def p_initializer_list(p):
    """initializer_list : initializer
                        | initializer_list ',' initializer
    """


def p_statement(p):
    """statement : labeled_statement
                 | compound_statement
                 | expression_statement
                 | selection_statement
                 | iteration_statement
                 | jump_statement
    """


def p_labeled_statement(p):
    """labeled_statement : IDENTIFIER ':' statement
                         | CASE constant_expression ':' statement
                         | DEFAULT ':' statement
    """


def p_compound_statement(p):
    """compound_statement : '{' '}'
                          | '{' statement_list '}'
                          | '{' declaration_list '}'
                          | '{' declaration_list statement_list '}'
    """


def p_compound_statement_error(p):
    """compound_statement : '{' error '}'
    """
    # Error resynchronisation catch-all


def p_declaration_list(p):
    """declaration_list : declaration
                        | declaration_list declaration
    """


def p_statement_list(p):
    """statement_list : statement
                      | statement_list statement
    """


def p_expression_statement(p):
    """expression_statement : ';'
                            | expression ';'
    """


def p_expression_statement_error(p):
    """expression_statement : error ';'
    """
    # Error resynchronisation catch-all


def p_selection_statement(p):
    """selection_statement : IF '(' expression ')' statement %prec IF
                           | IF '(' expression ')' statement ELSE statement
                           | SWITCH '(' expression ')' statement
    """


def p_iteration_statement(p):
    """iteration_statement : WHILE '(' expression ')' statement
    | DO statement WHILE '(' expression ')' ';'
    | FOR '(' expression_statement expression_statement ')' statement
    | FOR '(' expression_statement expression_statement expression ')' statement
    """


def p_jump_statement(p):
    """jump_statement : GOTO IDENTIFIER ';'
                      | CONTINUE ';'
                      | BREAK ';'
                      | RETURN ';'
                      | RETURN expression ';'
    """


def p_external_declaration(p):
    """external_declaration : declaration
                            | function_definition
    """
    # Intentionally empty


def p_function_definition(p):
    """function_definition : declaration_specifier_list declarator declaration_list compound_statement
                        | declaration_specifier_list declarator compound_statement
                        | declarator declaration_list compound_statement
                        | declarator compound_statement
    """
    # No impl of function defs


def p_directive(p):
    """directive : define
                 | undefine
                 | pragma
    """


def p_define(p):
    """define : PP_DEFINE PP_DEFINE_NAME PP_END_DEFINE
              | PP_DEFINE PP_DEFINE_NAME type_name PP_END_DEFINE
              | PP_DEFINE PP_DEFINE_NAME constant_expression PP_END_DEFINE
              | PP_DEFINE PP_DEFINE_MACRO_NAME '(' ')' PP_END_DEFINE
              | PP_DEFINE PP_DEFINE_MACRO_NAME '(' ')' constant_expression PP_END_DEFINE
              | PP_DEFINE PP_DEFINE_MACRO_NAME '(' macro_parameter_list ')' PP_END_DEFINE
              | PP_DEFINE PP_DEFINE_MACRO_NAME '(' macro_parameter_list ')' constant_expression PP_END_DEFINE
    """

    filename = p.slice[1].filename
    lineno = p.slice[1].lineno

    if p[3] != "(":
        if len(p) == 4:
            p.parser.cparser.handle_define_constant(p[2], None, filename, lineno)
        else:
            p.parser.cparser.handle_define_constant(p[2], p[3], filename, lineno)
    else:
        if p[4] == ")":
            params = []
            if len(p) == 6:
                expr = None
            elif len(p) == 7:
                expr = p[5]
        else:
            params = p[4]
            if len(p) == 7:
                expr = None
            elif len(p) == 8:
                expr = p[6]

        filename = p.slice[1].filename
        lineno = p.slice[1].lineno

        p.parser.cparser.handle_define_macro(p[2], params, expr, filename, lineno)


def p_define_error(p):
    """define : PP_DEFINE error PP_END_DEFINE"""
    lexer = p[2].lexer
    clexdata = lexer.tokens
    start = end = p[2].clexpos
    while clexdata[start].type != "PP_DEFINE":
        start -= 1
    while clexdata[end].type != "PP_END_DEFINE":
        end += 1

    name = clexdata[start + 1].value
    if clexdata[start + 1].type == "PP_DEFINE_NAME":
        params = None
        contents = [t.value for t in clexdata[start + 2 : end]]
    else:
        end_of_param_list = start
        while clexdata[end_of_param_list].value != ")" and end_of_param_list < end:
            end_of_param_list += 1
        params = [t.value for t in clexdata[start + 3 : end_of_param_list] if t.value != ","]
        contents = [t.value for t in clexdata[end_of_param_list + 1 : end]]

    filename = p.slice[1].filename
    lineno = p.slice[1].lineno

    p[2].lexer.cparser.handle_define_unparseable(name, params, contents, filename, lineno)


def p_undefine(p):
    """undefine : PP_UNDEFINE PP_DEFINE_NAME PP_END_DEFINE"""

    filename = p.slice[1].filename
    lineno = p.slice[1].lineno

    macro = expressions.IdentifierExpressionNode(p[2])
    p.parser.cparser.handle_undefine(macro, filename, lineno)


def p_macro_parameter_list(p):
    """macro_parameter_list : PP_MACRO_PARAM
                            | macro_parameter_list ',' PP_MACRO_PARAM
    """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[1].append(p[3])
        p[0] = p[1]


def p_error(t):
    if t.lexer.in_define:
        # p_define_error will generate an error message.
        pass
    else:
        if t.type == "$end":
            t.parser.cparser.handle_error("Syntax error at end of file.", t.filename, 0)
        else:
            t.lexer.cparser.handle_error("Syntax error at %r" % t.value, t.filename, t.lineno)
    # Don't alter lexer: default behaviour is to pass error production
    # up until it hits the catch-all at declaration, at which point
    # parsing continues (synchronisation).


def p_pragma(p):
    """pragma : pragma_pack
    """


def p_pragma_pack(p):
    """pragma_pack : PRAGMA PRAGMA_PACK '(' ')' PRAGMA_END
                   | PRAGMA PRAGMA_PACK '(' constant ')' PRAGMA_END
                   | PRAGMA PRAGMA_PACK '(' pragma_pack_stack_args ')' PRAGMA_END
    """

    err = None
    if len(p) == 6:
        cdeclarations.pragma_pack.set_default()
    elif isinstance(p[4], tuple):
        op, id, n = p[4]
        if op == "push":
            err = cdeclarations.pragma_pack.push(id, n)
        elif op == "pop":
            err = cdeclarations.pragma_pack.pop(id)
        else:
            err = "Syntax error for #pragma pack at {}:{}".format(
                p.slice[1].filename, p.slice[1].lineno
            )
    else:
        cdeclarations.pragma_pack.current = p[4]

    if err:
        p.lexer.cparser.handle_error(err, p.slice[1].filename, p.slice[1].lineno)


def p_pragma_pack_stack_args(p):
    """pragma_pack_stack_args : IDENTIFIER
                              | IDENTIFIER ',' IDENTIFIER
                              | IDENTIFIER ',' IDENTIFIER ',' constant
                              | IDENTIFIER ',' constant ',' IDENTIFIER
                              | IDENTIFIER ',' constant
    """
    op, id, n = p[1], None, None

    if len(p) > 2:
        if isinstance(p[3], expressions.ConstantExpressionNode):
            n = p[3].value

            if len(p) > 4:
                id = p[5]
        else:
            id = p[3]

            if len(p) > 4:
                n = p[5].value

    p[0] = (op, id, n)


def main():
    yacc.yacc(tabmodule="new_parsetab")
