"""
Parse a C source file.

To use, subclass CParser and override its handle_* methods.  Then instantiate
the class with a string to parse.
"""

__docformat__ = "restructuredtext"

import os.path
import sys

from ctypesgen.parser import cgrammar, preprocessor, yacc

# --------------------------------------------------------------------------
# Lexer
# --------------------------------------------------------------------------


class CLexer(object):
    def __init__(self, cparser):
        self.cparser = cparser
        self.type_names = set()
        self.in_define = False
        self.lineno = -1
        self.lexpos = -1

    def input(self, tokens):
        self.tokens = tokens
        self.pos = 0

    def token(self):
        while self.pos < len(self.tokens):
            t = self.tokens[self.pos]

            self.pos += 1

            if not t:
                break

            if t.type == "PP_DEFINE":
                self.in_define = True
            elif t.type == "PP_END_DEFINE":
                self.in_define = False

            # Transform PP tokens into C tokens
            elif t.type == "IDENTIFIER" and t.value in cgrammar.keywords:
                t.type = cgrammar.keyword_map[t.value]
            elif t.type == "IDENTIFIER" and t.value in self.type_names:
                if self.pos < 2 or self.tokens[self.pos - 2].type not in (
                    "VOID",
                    "_BOOL",
                    "CHAR",
                    "SHORT",
                    "INT",
                    "LONG",
                    "FLOAT",
                    "DOUBLE",
                    "SIGNED",
                    "UNSIGNED",
                    "ENUM",
                    "STRUCT",
                    "UNION",
                    "TYPE_NAME",
                ):
                    t.type = "TYPE_NAME"

            t.lexer = self
            t.clexpos = self.pos - 1

            return t
        return None


# --------------------------------------------------------------------------
# Parser
# --------------------------------------------------------------------------


class CParser(object):
    """Parse a C source file.

    Subclass and override the handle_* methods.  Call `parse` with a string
    to parse.
    """

    def __init__(self, options):
        super(CParser, self).__init__()
        self.preprocessor_parser = preprocessor.PreprocessorParser(options, self)
        self.parser = yacc.yacc(
            method="LALR",
            debug=False,
            module=cgrammar,
            write_tables=True,
            outputdir=os.path.dirname(__file__),
            optimize=True,
        )

        self.parser.errorfunc = cgrammar.p_error
        self.parser.cparser = self

        self.lexer = CLexer(self)
        if not options.no_stddef_types:
            self.lexer.type_names.add("wchar_t")
            self.lexer.type_names.add("ptrdiff_t")
            self.lexer.type_names.add("size_t")
        if not options.no_gnu_types:
            self.lexer.type_names.add("__builtin_va_list")
        if sys.platform == "win32" and not options.no_python_types:
            self.lexer.type_names.add("__int64")

    def parse(self, filename, debug=False):
        """Parse a file.

        If `debug` is True, parsing state is dumped to stdout.
        """

        self.handle_status("Preprocessing %s" % filename)
        self.preprocessor_parser.parse(filename)
        self.lexer.input(self.preprocessor_parser.output)
        self.handle_status("Parsing %s" % filename)
        self.parser.parse(lexer=self.lexer, debug=debug, tracking=True)

    # ----------------------------------------------------------------------
    # Parser interface.  Override these methods in your subclass.
    # ----------------------------------------------------------------------

    def handle_error(self, message, filename, lineno):
        """A parse error occured.

        The default implementation prints `lineno` and `message` to stderr.
        The parser will try to recover from errors by synchronising at the
        next semicolon.
        """
        sys.stderr.write("%s:%s %s\n" % (filename, lineno, message))

    def handle_pp_error(self, message):
        """The C preprocessor emitted an error.

        The default implementatin prints the error to stderr. If processing
        can continue, it will.
        """
        sys.stderr.write("Preprocessor: {}\n".format(message))

    def handle_status(self, message):
        """Progress information.

        The default implementationg prints message to stderr.
        """
        sys.stderr.write("{}\n".format(message))

    def handle_define(self, name, params, value, filename, lineno):
        """#define `name` `value`
        or #define `name`(`params`) `value`

        name is a string
        params is None or a list of strings
        value is a ...?
        """

    def handle_define_constant(self, name, value, filename, lineno):
        """#define `name` `value`

        name is a string
        value is an ExpressionNode or None
        """

    def handle_define_macro(self, name, params, value, filename, lineno):
        """#define `name`(`params`) `value`

        name is a string
        params is a list of strings
        value is an ExpressionNode or None
        """

    def handle_undefine(self, name, filename, lineno):
        """#undef `name`

        name is a string
        """

    def impl_handle_declaration(self, declaration, filename, lineno):
        """Internal method that calls `handle_declaration`.  This method
        also adds any new type definitions to the lexer's list of valid type
        names, which affects the parsing of subsequent declarations.
        """
        if declaration.storage == "typedef":
            declarator = declaration.declarator
            if not declarator:
                # XXX TEMPORARY while struct etc not filled
                return
            while declarator.pointer:
                declarator = declarator.pointer
            self.lexer.type_names.add(declarator.identifier)
        self.handle_declaration(declaration, filename, lineno)

    def handle_declaration(self, declaration, filename, lineno):
        """A declaration was encountered.

        `declaration` is an instance of Declaration.  Where a declaration has
        multiple initialisers, each is returned as a separate declaration.
        """
        pass


class DebugCParser(CParser):
    """A convenience class that prints each invocation of a handle_* method to
    stdout.
    """

    def handle_define(self, name, value, filename, lineno):
        print("#define name=%r, value=%r" % (name, value))

    def handle_define_constant(self, name, value, filename, lineno):
        print("#define constant name=%r, value=%r" % (name, value))

    def handle_declaration(self, declaration, filename, lineno):
        print(declaration)

    def get_ctypes_type(self, typ, declarator):
        return typ

    def handle_define_unparseable(self, name, params, value, filename, lineno):
        if params:
            original_string = "#define %s(%s) %s" % (name, ",".join(params), " ".join(value))
        else:
            original_string = "#define %s %s" % (name, " ".join(value))
        print(original_string)


if __name__ == "__main__":
    DebugCParser().parse(sys.argv[1], debug=True)
