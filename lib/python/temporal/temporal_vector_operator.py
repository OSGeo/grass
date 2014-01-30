"""!@package grass.temporal

Temporal vector operator evaluation with PLY

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@authors Thomas Leppelt and Soeren Gebbert

@code
    >>> import grass.temporal as tgis
    >>> tgis.init(True)
    >>> p = tgis.TemporalVectorOperatorParser()
    >>> expression =  "{equal| during,&&}"
    >>> p.parse(expression, True)
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'during'], '=', '&&')
    >>> expression =  "{equal| during | follows,&&}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'during', 'follows'], '&', '&')
    >>> expression =  "{equal| during | follows,+!:}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'during', 'follows'], '+', '!:')
    >>> expression =  "{equal| during,!:}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'during'], '=', '!:')
    >>> expression =  "{!:}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    (['equal'], '=', '!:')
    >>> expression =  "{&&}"
    >>> p.parse(expression, True)
    >>> print(p.relations, p.temporal, p.function)
    (['equal'], '=', '&&')
    >>> expression =  "{&&}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    (['equal'], '&', '&')
    >>> expression =  "{|#}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    (['equal'], '|', '#')
    >>> expression =  "{equal,||}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    (['equal'], '|', '|')
    >>> expression =  "{equal|during,=!:}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'during'], '=', '!:')
    >>> expression =  "{equal|during|starts}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'during', 'starts'], None, None)
    >>> expression =  "{over| equal,||}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    ([['overlaps', 'overlapped'], 'equal'], '|', '|')
    
@endcode
"""
try:
    import ply.lex as lex
    import ply.yacc as yacc
except:
    pass

class TemporalVectorOperatorLexer(object):
    """!Lexical analyzer for the GRASS GIS temporal vector operators"""
    
    # Functions that defines topological relations.
    relations = {
        'equal'      : "EQUAL",
        'follows'    : "FOLLOWS",
        'precedes'   : "PRECEDES",
        'overlaps'   : "OVERLAPS",
        'overlapped' : "OVERLAPPED",
        'during'     : "DURING",
        'starts'     : "STARTS",
        'finishes'   : "FINISHES",
        'contains'   : "CONTAINS",
        'started'    : "STARTED",
        'finished'   : "FINISHED",
        'over'       : "OVER"
        }
        
    # This is the list of token names.
    tokens = (
        'COMMA',
        'LEFTREF',
        'HASH',
        'OR',
        'AND',
        'DISOR',
        'XOR',
        'NOT',
        'T_SELECT',
        'T_NOT_SELECT',
        'CLPAREN',
        'CRPAREN',
    )
    
    # Build the token list
    tokens = tokens + tuple(relations.values())

    # Regular expression rules for simple tokens
    t_T_SELECT           = r':'
    t_T_NOT_SELECT       = r'!:'
    t_COMMA              = r','
    t_LEFTREF             = r'='
    t_HASH               = r'\#'
    t_OR                 = r'[\|]'
    t_AND                = r'[&]'
    t_DISOR              = r'\+'
    t_XOR                = r'\^'
    t_NOT                = r'\~'
    t_CLPAREN             = r'\{'
    t_CRPAREN             = r'\}'
    
    # These are the things that should be ignored.
    t_ignore = ' \t'

    # Track line numbers.
    def t_newline(self, t):
        r'\n+'
        t.lineno += len(t.value)
        
    def t_NAME(self, t):
        r'[a-zA-Z_][a-zA-Z_0-9]*'
        self.temporal_symbol(t)
        return t
        
    # Parse symbols
    def temporal_symbol(self, t):
        # Check for reserved words
        if t.value in TemporalVectorOperatorLexer.relations.keys():
            t.type = TemporalVectorOperatorLexer.relations.get(t.value)
        #else:
        #    t.type = 'NAME'
        return(t)

    # Handle errors.
    def t_error(self, t):
        raise SyntaxError("syntax error on line %d near '%s'" % 
            (t.lineno, t.value))

    # Build the lexer
    def build(self,**kwargs):
        self.lexer = lex.lex(module=self, **kwargs)
        
    # Just for testing
    def test(self,data):
        self.name_list = {}
        print(data)
        self.lexer.input(data)
        while True:
             tok = self.lexer.token()
             if not tok: break
             print tok
             
class TemporalVectorOperatorParser(object):
    """!The parser for the GRASS GIS temporal vector operators"""
    
    def __init__(self):
        self.lexer = TemporalVectorOperatorLexer()
        self.lexer.build()
        self.parser = yacc.yacc(module=self)

    def parse(self, expression, comparison = False):
        self.comparison = comparison
        self.parser.parse(expression)
        
    # Error rule for syntax errors.
    def p_error(self, t):
        raise SyntaxError("invalid syntax")

    # Get the tokens from the lexer class
    tokens = TemporalVectorOperatorLexer.tokens

    def p_relation_only(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN relation CRPAREN
                 | CLPAREN relationlist CRPAREN
        """
        # Set three operator components.
        if isinstance(t[2], list):
            self.relations = t[2]
        else:
            self.relations = [t[2]]
        self.temporal  = None 
        self.function  = None
        
        t[0] = t[2]


    def p_operator(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN select CRPAREN
                 | CLPAREN HASH CRPAREN
                 | CLPAREN overlay CRPAREN
        """
        # Set three operator components.
        self.relations = ['equal']
        self.temporal  = "=" 
        self.function  = t[2]
        
        t[0] = t[2]
        
    def p_operator_temporal(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN temporal select CRPAREN
                 | CLPAREN temporal HASH CRPAREN
                 | CLPAREN temporal overlay CRPAREN
        """
        # Set three operator components.
        self.relations = ['equal']
        if self.comparison:
            self.temporal = "="
            self.function = t[2] + t[3]
        else:
            self.temporal = t[2]
            self.function = t[3]
        
        t[0] = t[3]

    def p_operator_relation(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN relation COMMA select CRPAREN
                 | CLPAREN relationlist COMMA select CRPAREN
                 | CLPAREN relation COMMA HASH CRPAREN
                 | CLPAREN relationlist COMMA HASH CRPAREN
                 | CLPAREN relation COMMA overlay CRPAREN
                 | CLPAREN relationlist COMMA overlay CRPAREN
        """
        # Set three operator components.
        if isinstance(t[2], list):
            self.relations = t[2]
        else:
            self.relations = [t[2]]
        self.temporal  = "=" 
        self.function  = t[4]
        
        t[0] = t[4]

    def p_operator_relation_temporal(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN relation COMMA temporal select CRPAREN
                 | CLPAREN relationlist COMMA temporal select CRPAREN
                 | CLPAREN relation COMMA temporal HASH CRPAREN
                 | CLPAREN relationlist COMMA temporal HASH CRPAREN
                 | CLPAREN relation COMMA temporal overlay CRPAREN
                 | CLPAREN relationlist COMMA temporal overlay CRPAREN
        """
        # Set three operator components.
        if isinstance(t[2], list):
            self.relations = t[2]
        else:
            self.relations = [t[2]]
        if self.comparison:
            self.temporal = "="
            self.function = t[4] + t[5]
        else:
            self.temporal = t[4]
            self.function = t[5]
        t[0] = t[5]
        
    def p_relation(self, t):
        # The list of relations.
        """
        relation : EQUAL
                 | FOLLOWS
                 | PRECEDES
                 | OVERLAPS
                 | OVERLAPPED
                 | DURING
                 | STARTS
                 | FINISHES
                 | CONTAINS
                 | STARTED
                 | FINISHED
        """
        t[0] = t[1]
        
    def p_over(self, t):
        # The list of relations.
        """
        relation : OVER
        """
        over_list = ["overlaps", "overlapped"]
        t[0] = over_list

    def p_relationlist(self, t):
        # The list of relations.
        """
        relationlist : relation OR relation
                     | relation OR relationlist
        """
        rel_list = []
        rel_list.append(t[1])
        if isinstance(t[3], list):
            rel_list = rel_list + t[3]
        else:
            rel_list.append(t[3])
        t[0] =  rel_list

    def p_temporal_operator(self, t):
        # The list of relations.
        """
        temporal : LEFTREF
                 | OR
                 | AND
                 | DISOR
        """
        t[0] = t[1]

    def p_select_operator(self, t):
        # The list of relations.
        """
        select : T_SELECT
               | T_NOT_SELECT
        """
        t[0] = t[1]

    def p_overlay_operator(self, t):
        # The list of relations.
        """
        overlay : OR
                | AND
                | DISOR
                | XOR
                | NOT
        """
        t[0] = t[1]
###############################################################################             
        
if __name__ == "__main__":
    import doctest
    doctest.testmod()
    