"""@package grass.temporal

Temporal operator evaluation with PLY

(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Thomas Leppelt and Soeren Gebbert

.. code-block:: python

    >>> p = TemporalOperatorParser()
    >>> expression =  "{equal| during}"
    >>> p.parse(expression, optype = 'relation')
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'during'], None, None)
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{contains | starts}"
    >>> p.parse(expression)
    >>> print(p.relations, p.temporal, p.function)
    (['contains', 'starts'], None, None)
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{&&, during}"
    >>> p.parse(expression, optype = 'boolean')
    >>> print(p.relations, p.temporal, p.function,p.aggregate)
    (['during'], 'l', '&&', '&')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{||, equal | during}"
    >>> p.parse(expression, optype = 'boolean')
    >>> print(p.relations, p.temporal, p.function,p.aggregate)
    (['equal', 'during'], 'l', '||', '|')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{||, equal | during, &}"
    >>> p.parse(expression, optype = 'boolean')
    >>> print(p.relations, p.temporal, p.function,p.aggregate)
    (['equal', 'during'], 'l', '||', '&')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{&&, during, |}"
    >>> p.parse(expression, optype = 'boolean')
    >>> print(p.relations, p.temporal, p.function,p.aggregate)
    (['during'], 'l', '&&', '|')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{&&, during, |, r}"
    >>> p.parse(expression, optype = 'boolean')
    >>> print(p.relations, p.temporal, p.function,p.aggregate)
    (['during'], 'r', '&&', '|')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{&&, during, u}"
    >>> p.parse(expression, optype = 'boolean')
    >>> print(p.relations, p.temporal, p.function,p.aggregate)
    (['during'], 'u', '&&', '&')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{:, during, r}"
    >>> p.parse(expression, optype = 'select')
    >>> print(p.relations, p.temporal, p.function)
    (['during'], 'r', ':')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{!:, equal | contains, d}"
    >>> p.parse(expression, optype = 'select')
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'contains'], 'd', '!:')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{#, during, r}"
    >>> p.parse(expression, optype = 'hash')
    >>> print(p.relations, p.temporal, p.function)
    (['during'], 'r', '#')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{#, equal | contains}"
    >>> p.parse(expression, optype = 'hash')
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'contains'], 'l', '#')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{+, during, r}"
    >>> p.parse(expression, optype = 'raster')
    >>> print(p.relations, p.temporal, p.function)
    (['during'], 'r', '+')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{/, equal | contains}"
    >>> p.parse(expression, optype = 'raster')
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'contains'], 'l', '/')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{+, equal | contains,intersect}"
    >>> p.parse(expression, optype = 'raster')
    >>> print(p.relations, p.temporal, p.function)
    (['equal', 'contains'], 'i', '+')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{*, contains,disjoint}"
    >>> p.parse(expression, optype = 'raster')
    >>> print(p.relations, p.temporal, p.function)
    (['contains'], 'd', '*')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{~, equal,left}"
    >>> p.parse(expression, optype = 'overlay')
    >>> print(p.relations, p.temporal, p.function)
    (['equal'], 'l', '~')
    >>> p = TemporalOperatorParser()    
    >>> expression =  "{^, over,right}"
    >>> p.parse(expression, optype = 'overlay')
    >>> print(p.relations, p.temporal, p.function)
    (['overlaps', 'overlapped'], 'r', '^')

"""

try:
    import ply.lex as lex
    import ply.yacc as yacc
except:
    pass

class TemporalOperatorLexer(object):
    """Lexical analyzer for the GRASS GIS temporal operator"""

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
        'RIGHTREF',
        'UNION', 
        'DISJOINT',
        'INTERSECT',
        'HASH',
        'OR',
        'AND',
        'DISOR',
        'XOR',
        'NOT',        
        'MOD',
        'DIV',
        'MULT',
        'ADD',      
        'SUB',
        'T_SELECT',
        'T_NOT_SELECT',
        'CLPAREN',
        'CRPAREN',
    )

    # Build the token list
    tokens = tokens + tuple(relations.values())

    # Regular expression rules for simple tokens
    t_T_SELECT                = r':'
    t_T_NOT_SELECT        = r'!:'
    t_COMMA                    = r','
    t_LEFTREF                  = '^[l|left]'
    t_RIGHTREF                ='^[r|right]'
    t_UNION                     = '^[u|union]'
    t_DISJOINT                 = '^[d|disjoint]'
    t_INTERSECT             = '^[i|intersect]'
    t_HASH                      = r'\#'
    t_OR                          = r'[\|]'
    t_AND                        = r'[&]'
    t_DISOR                     = r'\+'
    t_XOR                         = r'\^'
    t_NOT                         = r'\~'    
    t_MOD                       = r'[\%]'
    t_DIV                         = r'[\/]'
    t_MULT                      = r'[\*]'
    t_ADD                       = r'[\+]'
    t_SUB                        = r'[-]'
    t_CLPAREN                = r'\{'
    t_CRPAREN                = r'\}'

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
        if t.value in TemporalOperatorLexer.relations.keys():
            t.type = TemporalOperatorLexer.relations.get(t.value)
        elif t.value == 'l' or t.value == 'left':
            t.value = 'l'
            t.type = 'LEFTREF'
        elif t.value == 'r' or t.value == 'right':
            t.value = 'r'
            t.type = 'RIGHTREF'
        elif t.value == 'u' or t.value == 'union':
            t.value = 'u'
            t.type = 'UNION'
        elif t.value == 'd' or t.value == 'disjoint':
            t.value = 'd'
            t.type = 'DISJOINT'
        elif t.value == 'i' or t.value == 'intersect':
            t.value = 'i'
            t.type = 'INTERSECT'
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

###############################################################################

class TemporalOperatorParser(object):
    """The temporal operator class"""

    def __init__(self):
        self.lexer = TemporalOperatorLexer()
        self.lexer.build()
        self.parser = yacc.yacc(module=self)
        self.relations = None
        self.temporal  = None
        self.function  = None
        self.aggregate = None

    def parse(self, expression,  optype = 'relation'):
        self.optype = optype        
        self.parser.parse(expression)
        # The parameter optype can be of type: select {:, during, r}, boolean{&&, contains, |}, 
        #                                                            raster{*, equal, |}, vector {|, starts, &},
        #                                                            hash{#, during, l} or relation {during}.
    
    # Error rule for syntax errors.
    def p_error(self, t):
        raise SyntaxError("Unexpected syntax error")

    # Get the tokens from the lexer class
    tokens = TemporalOperatorLexer.tokens
 
    def p_relation_operator(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN relation CRPAREN
                 | CLPAREN relationlist CRPAREN
        """
        # Check for correct type.
        if not self.optype == 'relation':
            raise SyntaxError("invalid syntax")
        else:
            # Set three operator components.
            if isinstance(t[2], list):
                self.relations = t[2]
            else:
                self.relations = [t[2]]
            self.temporal  = None
            self.function  = None

            t[0] = t[2]

    def p_relation_bool_operator(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN OR OR COMMA relation CRPAREN
                | CLPAREN AND AND COMMA relation CRPAREN
                | CLPAREN OR OR COMMA relationlist CRPAREN
                | CLPAREN AND AND COMMA relationlist CRPAREN
        """
        if not self.optype == 'boolean':
            raise SyntaxError("invalid syntax")
        else:        
            # Set three operator components.
            if isinstance(t[5], list):
                self.relations = t[5]
            else:
                self.relations = [t[5]]
            self.temporal  = "l"
            self.function  = t[2] + t[3]
            self.aggregate = t[2]
            
            t[0] = t[2]

    def p_relation_bool_combi_operator(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN OR OR COMMA relation COMMA OR CRPAREN
                | CLPAREN OR OR COMMA relation COMMA AND CRPAREN
                | CLPAREN AND AND COMMA relation COMMA OR CRPAREN
                | CLPAREN AND AND COMMA relation COMMA AND CRPAREN
                | CLPAREN OR OR COMMA relationlist COMMA OR CRPAREN
                | CLPAREN OR OR COMMA relationlist COMMA AND CRPAREN
                | CLPAREN AND AND COMMA relationlist COMMA OR CRPAREN
                | CLPAREN AND AND COMMA relationlist COMMA AND CRPAREN
        """
        if not self.optype == 'boolean':
            raise SyntaxError("invalid syntax")
        else:        
            # Set three operator components.
            if isinstance(t[5], list):
                self.relations = t[5]
            else:
                self.relations = [t[5]]
            self.temporal  = "l"
            self.function  = t[2] + t[3]
            self.aggregate = t[7]
            
            t[0] = t[2]

    def p_relation_bool_combi_operator2(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN OR OR COMMA relation COMMA temporal CRPAREN
                | CLPAREN AND AND COMMA relation COMMA temporal CRPAREN
                | CLPAREN OR OR COMMA relationlist COMMA temporal CRPAREN
                | CLPAREN AND AND COMMA relationlist COMMA temporal CRPAREN
        """
        if not self.optype == 'boolean':
            raise SyntaxError("invalid syntax")
        else:        
            # Set three operator components.
            if isinstance(t[5], list):
                self.relations = t[5]
            else:
                self.relations = [t[5]]
            self.temporal  = t[7]
            self.function  = t[2] + t[3]
            self.aggregate = t[2]
            
            t[0] = t[2]
    
    def p_relation_bool_combi_operator3(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN OR OR COMMA relation COMMA OR COMMA temporal CRPAREN
                | CLPAREN OR OR COMMA relation COMMA AND COMMA temporal CRPAREN
                | CLPAREN AND AND COMMA relation COMMA OR COMMA temporal CRPAREN
                | CLPAREN AND AND COMMA relation COMMA AND COMMA temporal CRPAREN
                | CLPAREN OR OR COMMA relationlist COMMA OR COMMA temporal CRPAREN
                | CLPAREN OR OR COMMA relationlist COMMA AND COMMA temporal CRPAREN
                | CLPAREN AND AND COMMA relationlist COMMA OR COMMA temporal CRPAREN
                | CLPAREN AND AND COMMA relationlist COMMA AND COMMA temporal CRPAREN
        """
        if not self.optype == 'boolean':
            raise SyntaxError("invalid syntax")
        else:        
            # Set three operator components.
            if isinstance(t[5], list):
                self.relations = t[5]
            else:
                self.relations = [t[5]]
            self.temporal  = t[9]
            self.function  = t[2] + t[3]
            self.aggregate = t[7]
            
            t[0] = t[2]
    
    def p_select_relation_operator(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN select CRPAREN
                    | CLPAREN select COMMA relation CRPAREN
                    | CLPAREN select COMMA relationlist CRPAREN
                    | CLPAREN select COMMA relation COMMA temporal CRPAREN
                    | CLPAREN select COMMA relationlist COMMA temporal CRPAREN
        """
        if not self.optype == 'select':
            raise SyntaxError("invalid syntax")
        else:
            if len(t) == 4:
                # Set three operator components.
                self.relations = ['equal']
                self.temporal  = "l"
                self.function  = t[2]
            elif len(t) == 5:
                self.relations = ['equal']
                self.temporal  = t[3]
                self.function  = t[2]
            elif len(t) == 6:
                if isinstance(t[4], list):
                    self.relations = t[4]
                else:
                    self.relations = [t[4]]
                self.temporal  = "l"
                self.function  = t[2]
            elif len(t) == 8:
                if isinstance(t[4], list):
                    self.relations = t[4]
                else:
                    self.relations = [t[4]]
                self.temporal  = t[6]
                self.function  = t[2]
            t[0] = t[2]

    def p_hash_relation_operator(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN HASH CRPAREN
                    | CLPAREN HASH COMMA relation CRPAREN
                    | CLPAREN HASH COMMA relationlist CRPAREN
                    | CLPAREN HASH COMMA relation COMMA temporal CRPAREN
                    | CLPAREN HASH COMMA relationlist COMMA temporal CRPAREN
        """
        if not self.optype == 'hash':
            raise SyntaxError("invalid syntax")
        else:
            if len(t) == 4:
                # Set three operator components.
                self.relations = ['equal']
                self.temporal  = "l"
                self.function  = t[2]
            elif len(t) == 5:
                self.relations = ['equal']
                self.temporal  = t[3]
                self.function  = t[2]
            elif len(t) == 6:
                if isinstance(t[4], list):
                    self.relations = t[4]
                else:
                    self.relations = [t[4]]
                self.temporal  = "l"
                self.function  = t[2]
            elif len(t) == 8:
                if isinstance(t[4], list):
                    self.relations = t[4]
                else:
                    self.relations = [t[4]]
                self.temporal  = t[6]
                self.function  = t[2]
            t[0] = t[2]

    def p_raster_relation_operator(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN arithmetic CRPAREN
                    | CLPAREN arithmetic COMMA relation CRPAREN
                    | CLPAREN arithmetic COMMA relationlist CRPAREN
                    | CLPAREN arithmetic COMMA relation COMMA temporal CRPAREN
                    | CLPAREN arithmetic COMMA relationlist COMMA temporal CRPAREN
        """
        if not self.optype == 'raster':
            raise SyntaxError("invalid syntax")
        else:
            if len(t) == 4:
                # Set three operator components.
                self.relations = ['equal']
                self.temporal  = "l"
                self.function  = t[2]
            elif len(t) == 5:
                self.relations = ['equal']
                self.temporal  = t[3]
                self.function  = t[2]
            elif len(t) == 6:
                if isinstance(t[4], list):
                    self.relations = t[4]
                else:
                    self.relations = [t[4]]
                self.temporal  = "l"
                self.function  = t[2]
            elif len(t) == 8:
                if isinstance(t[4], list):
                    self.relations = t[4]
                else:
                    self.relations = [t[4]]
                self.temporal  = t[6]
                self.function  = t[2]
            t[0] = t[2]

    def p_overlay_relation_operator(self, t):
        # The expression should always return a list of maps.
        """
        operator : CLPAREN overlay CRPAREN
                    | CLPAREN overlay COMMA relation CRPAREN
                    | CLPAREN overlay COMMA relationlist CRPAREN
                    | CLPAREN overlay COMMA relation COMMA temporal CRPAREN
                    | CLPAREN overlay COMMA relationlist COMMA temporal CRPAREN
        """
        if not self.optype == 'overlay':
            raise SyntaxError("invalid syntax")
        else:
            if len(t) == 4:
                # Set three operator components.
                self.relations = ['equal']
                self.temporal  = "l"
                self.function  = t[2]
            elif len(t) == 5:
                self.relations = ['equal']
                self.temporal  = t[3]
                self.function  = t[2]
            elif len(t) == 6:
                if isinstance(t[4], list):
                    self.relations = t[4]
                else:
                    self.relations = [t[4]]
                self.temporal  = "l"
                self.function  = t[2]
            elif len(t) == 8:
                if isinstance(t[4], list):
                    self.relations = t[4]
                else:
                    self.relations = [t[4]]
                self.temporal  = t[6]
                self.function  = t[2]
            t[0] = t[2]

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
                | RIGHTREF
                | UNION
                | DISJOINT
                | INTERSECT
        """
        t[0] = t[1]

    def p_select_operator(self, t):
        # The list of relations.
        """
        select : T_SELECT
               | T_NOT_SELECT
        """
        t[0] = t[1]

    def p_arithmetic_operator(self, t):
        # The list of relations.
        """
        arithmetic : MOD
                   | DIV
                   | MULT
                   | ADD
                   | SUB
        """
        t[0] = t[1]

    def p_overlay_operator(self, t):
        # The list of relations.
        """
        overlay : AND
                   | OR
                   | XOR
                   | DISOR
                   | NOT
        """
        t[0] = t[1]
###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()

