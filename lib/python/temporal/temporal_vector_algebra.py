"""!@package grass.temporal

Temporal vector algebra

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@authors Thomas Leppelt and Soeren Gebbert

@code

    >>> import grass.temporal as tgis
    >>> tgis.init(True)
    >>> p = tgis.TemporalVectorAlgebraLexer()
    >>> p.build()
    >>> p.debug = True
    >>> expression =  "C = A : B"
    >>> p.test(expression)
    C = A : B
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_SELECT,':',1,6)
    LexToken(NAME,'B',1,8)
    >>> expression =  "C = test1 !: test2"
    >>> p.test(expression)
    C = test1 !: test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_NOT_SELECT,'!:',1,10)
    LexToken(NAME,'test2',1,13)
    >>> expression =  "C = test1 {equal,:} test2"
    >>> p.test(expression)
    C = test1 {equal,:} test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_SELECT_OPERATOR,'{equal,:}',1,10)
    LexToken(NAME,'test2',1,20)
    >>> expression =  "C = test1 {equal,!:} test2"
    >>> p.test(expression)
    C = test1 {equal,!:} test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_SELECT_OPERATOR,'{equal,!:}',1,10)
    LexToken(NAME,'test2',1,21)
    >>> expression =  "C = test1 # test2"
    >>> p.test(expression)
    C = test1 # test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(HASH,'#',1,10)
    LexToken(NAME,'test2',1,12)
    >>> expression =  "C = test1 {#} test2"
    >>> p.test(expression)
    C = test1 {#} test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_HASH_OPERATOR,'{#}',1,10)
    LexToken(NAME,'test2',1,14)
    >>> expression =  "C = test1 {equal,#} test2"
    >>> p.test(expression)
    C = test1 {equal,#} test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_HASH_OPERATOR,'{equal,#}',1,10)
    LexToken(NAME,'test2',1,20)
    >>> expression =  "C = test1 {equal|during,#} test2"
    >>> p.test(expression)
    C = test1 {equal|during,#} test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_HASH_OPERATOR,'{equal|during,#}',1,10)
    LexToken(NAME,'test2',1,27)
    >>> expression =  "E = test1 : test2 !: test1"
    >>> p.test(expression)
    E = test1 : test2 !: test1
    LexToken(NAME,'E',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_SELECT,':',1,10)
    LexToken(NAME,'test2',1,12)
    LexToken(T_NOT_SELECT,'!:',1,18)
    LexToken(NAME,'test1',1,21)
    >>> expression =  'D = buff_t(test1,"10 months")'
    >>> p.test(expression)
    D = buff_t(test1,"10 months")
    LexToken(NAME,'D',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(BUFF_T,'buff_t',1,4)
    LexToken(LPAREN,'(',1,10)
    LexToken(NAME,'test1',1,11)
    LexToken(COMMA,',',1,16)
    LexToken(QUOTE,'"',1,17)
    LexToken(INT,10,1,18)
    LexToken(NAME,'months',1,21)
    LexToken(QUOTE,'"',1,27)
    LexToken(RPAREN,')',1,28)
    >>> expression =  'H = tsnap(test1)'
    >>> p.test(expression)
    H = tsnap(test1)
    LexToken(NAME,'H',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(TSNAP,'tsnap',1,4)
    LexToken(LPAREN,'(',1,9)
    LexToken(NAME,'test1',1,10)
    LexToken(RPAREN,')',1,15)
    >>> expression =  'H = tsnap(test2 {during,:} buff_t(test1, "1 days"))'
    >>> p.test(expression)
    H = tsnap(test2 {during,:} buff_t(test1, "1 days"))
    LexToken(NAME,'H',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(TSNAP,'tsnap',1,4)
    LexToken(LPAREN,'(',1,9)
    LexToken(NAME,'test2',1,10)
    LexToken(T_SELECT_OPERATOR,'{during,:}',1,16)
    LexToken(BUFF_T,'buff_t',1,27)
    LexToken(LPAREN,'(',1,33)
    LexToken(NAME,'test1',1,34)
    LexToken(COMMA,',',1,39)
    LexToken(QUOTE,'"',1,41)
    LexToken(INT,1,1,42)
    LexToken(NAME,'days',1,44)
    LexToken(QUOTE,'"',1,48)
    LexToken(RPAREN,')',1,49)
    LexToken(RPAREN,')',1,50)
    >>> expression =  'H = tshift(test2 {during,:} buff_t(test1, "1 days"), "1 months")'
    >>> p.test(expression)
    H = tshift(test2 {during,:} buff_t(test1, "1 days"), "1 months")
    LexToken(NAME,'H',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(TSHIFT,'tshift',1,4)
    LexToken(LPAREN,'(',1,10)
    LexToken(NAME,'test2',1,11)
    LexToken(T_SELECT_OPERATOR,'{during,:}',1,17)
    LexToken(BUFF_T,'buff_t',1,28)
    LexToken(LPAREN,'(',1,34)
    LexToken(NAME,'test1',1,35)
    LexToken(COMMA,',',1,40)
    LexToken(QUOTE,'"',1,42)
    LexToken(INT,1,1,43)
    LexToken(NAME,'days',1,45)
    LexToken(QUOTE,'"',1,49)
    LexToken(RPAREN,')',1,50)
    LexToken(COMMA,',',1,51)
    LexToken(QUOTE,'"',1,53)
    LexToken(INT,1,1,54)
    LexToken(NAME,'months',1,56)
    LexToken(QUOTE,'"',1,62)
    LexToken(RPAREN,')',1,63)
    >>> expression =  'H = tshift(A , 10)'
    >>> p.test(expression)
    H = tshift(A , 10)
    LexToken(NAME,'H',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(TSHIFT,'tshift',1,4)
    LexToken(LPAREN,'(',1,10)
    LexToken(NAME,'A',1,11)
    LexToken(COMMA,',',1,13)
    LexToken(INT,10,1,15)
    LexToken(RPAREN,')',1,17)
    >>> expression =  'H = if(td(A) > 10, A)'
    >>> p.test(expression)
    H = if(td(A) > 10, A)
    LexToken(NAME,'H',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(IF,'if',1,4)
    LexToken(LPAREN,'(',1,6)
    LexToken(TD,'td',1,7)
    LexToken(LPAREN,'(',1,9)
    LexToken(NAME,'A',1,10)
    LexToken(RPAREN,')',1,11)
    LexToken(GREATER,'>',1,13)
    LexToken(INT,10,1,15)
    LexToken(COMMA,',',1,17)
    LexToken(NAME,'A',1,19)
    LexToken(RPAREN,')',1,20)
    >>> expression =  'H = if(td(A) > 10, A, B)'
    >>> p.test(expression)
    H = if(td(A) > 10, A, B)
    LexToken(NAME,'H',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(IF,'if',1,4)
    LexToken(LPAREN,'(',1,6)
    LexToken(TD,'td',1,7)
    LexToken(LPAREN,'(',1,9)
    LexToken(NAME,'A',1,10)
    LexToken(RPAREN,')',1,11)
    LexToken(GREATER,'>',1,13)
    LexToken(INT,10,1,15)
    LexToken(COMMA,',',1,17)
    LexToken(NAME,'A',1,19)
    LexToken(COMMA,',',1,20)
    LexToken(NAME,'B',1,22)
    LexToken(RPAREN,')',1,23)
    >>> expression =  'I = if(equals,td(A) > 10 {equals,||} td(B) < 10, A)'
    >>> p.test(expression)
    I = if(equals,td(A) > 10 {equals,||} td(B) < 10, A)
    LexToken(NAME,'I',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(IF,'if',1,4)
    LexToken(LPAREN,'(',1,6)
    LexToken(NAME,'equals',1,7)
    LexToken(COMMA,',',1,13)
    LexToken(TD,'td',1,14)
    LexToken(LPAREN,'(',1,16)
    LexToken(NAME,'A',1,17)
    LexToken(RPAREN,')',1,18)
    LexToken(GREATER,'>',1,20)
    LexToken(INT,10,1,22)
    LexToken(T_OVERLAY_OPERATOR,'{equals,||}',1,25)
    LexToken(TD,'td',1,37)
    LexToken(LPAREN,'(',1,39)
    LexToken(NAME,'B',1,40)
    LexToken(RPAREN,')',1,41)
    LexToken(LOWER,'<',1,43)
    LexToken(INT,10,1,45)
    LexToken(COMMA,',',1,47)
    LexToken(NAME,'A',1,49)
    LexToken(RPAREN,')',1,50)
    >>> expression =  'I = if(equals,td(A) > 10 || start_day() < 10, A)'
    >>> p.test(expression)
    I = if(equals,td(A) > 10 || start_day() < 10, A)
    LexToken(NAME,'I',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(IF,'if',1,4)
    LexToken(LPAREN,'(',1,6)
    LexToken(NAME,'equals',1,7)
    LexToken(COMMA,',',1,13)
    LexToken(TD,'td',1,14)
    LexToken(LPAREN,'(',1,16)
    LexToken(NAME,'A',1,17)
    LexToken(RPAREN,')',1,18)
    LexToken(GREATER,'>',1,20)
    LexToken(INT,10,1,22)
    LexToken(OR,'|',1,25)
    LexToken(OR,'|',1,26)
    LexToken(START_DAY,'start_day',1,28)
    LexToken(LPAREN,'(',1,37)
    LexToken(RPAREN,')',1,38)
    LexToken(LOWER,'<',1,40)
    LexToken(INT,10,1,42)
    LexToken(COMMA,',',1,44)
    LexToken(NAME,'A',1,46)
    LexToken(RPAREN,')',1,47)
    >>> expression =  'E = if({equals},td(A) >= 4 {contain,&&} td(B) == 2, C : D)'
    >>> p.test(expression)
    E = if({equals},td(A) >= 4 {contain,&&} td(B) == 2, C : D)
    LexToken(NAME,'E',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(IF,'if',1,4)
    LexToken(LPAREN,'(',1,6)
    LexToken(T_REL_OPERATOR,'{equals}',1,7)
    LexToken(COMMA,',',1,15)
    LexToken(TD,'td',1,16)
    LexToken(LPAREN,'(',1,18)
    LexToken(NAME,'A',1,19)
    LexToken(RPAREN,')',1,20)
    LexToken(GREATER_EQUALS,'>=',1,22)
    LexToken(INT,4,1,25)
    LexToken(T_OVERLAY_OPERATOR,'{contain,&&}',1,27)
    LexToken(TD,'td',1,40)
    LexToken(LPAREN,'(',1,42)
    LexToken(NAME,'B',1,43)
    LexToken(RPAREN,')',1,44)
    LexToken(CEQUALS,'==',1,46)
    LexToken(INT,2,1,49)
    LexToken(COMMA,',',1,50)
    LexToken(NAME,'C',1,52)
    LexToken(T_SELECT,':',1,54)
    LexToken(NAME,'D',1,56)
    LexToken(RPAREN,')',1,57)
    >>> expression =  'F = if({equals},A {equal,#}, B, C : D)'
    >>> p.test(expression)
    F = if({equals},A {equal,#}, B, C : D)
    LexToken(NAME,'F',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(IF,'if',1,4)
    LexToken(LPAREN,'(',1,6)
    LexToken(T_REL_OPERATOR,'{equals}',1,7)
    LexToken(COMMA,',',1,15)
    LexToken(NAME,'A',1,16)
    LexToken(T_HASH_OPERATOR,'{equal,#}',1,18)
    LexToken(COMMA,',',1,27)
    LexToken(NAME,'B',1,29)
    LexToken(COMMA,',',1,30)
    LexToken(NAME,'C',1,32)
    LexToken(T_SELECT,':',1,34)
    LexToken(NAME,'D',1,36)
    LexToken(RPAREN,')',1,37)
    >>> expression =  'E = A : B ^ C : D'
    >>> p.test(expression)
    E = A : B ^ C : D
    LexToken(NAME,'E',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_SELECT,':',1,6)
    LexToken(NAME,'B',1,8)
    LexToken(XOR,'^',1,10)
    LexToken(NAME,'C',1,12)
    LexToken(T_SELECT,':',1,14)
    LexToken(NAME,'D',1,16)
    >>> expression =  'E = A : B {|^} C : D'
    >>> p.test(expression)
    E = A : B {|^} C : D
    LexToken(NAME,'E',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_SELECT,':',1,6)
    LexToken(NAME,'B',1,8)
    LexToken(T_OVERLAY_OPERATOR,'{|^}',1,10)
    LexToken(NAME,'C',1,15)
    LexToken(T_SELECT,':',1,17)
    LexToken(NAME,'D',1,19)
    >>> expression =  'E = buff_a(A, 10)'
    >>> p.test(expression)
    E = buff_a(A, 10)
    LexToken(NAME,'E',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(BUFF_AREA,'buff_a',1,4)
    LexToken(LPAREN,'(',1,10)
    LexToken(NAME,'A',1,11)
    LexToken(COMMA,',',1,12)
    LexToken(INT,10,1,14)
    LexToken(RPAREN,')',1,16)
    >>> p = tgis.TemporalVectorAlgebraParser()
    >>> p.run = False
    >>> p.debug = True
    >>> expression =  "D = A : (B !: C)"
    >>> p.parse(expression)
    B* =  B !: C
    A* =  A : B*
    D = A*
    >>> expression =  "D = A {!:} B {during,:} C"
    >>> print(expression)
    D = A {!:} B {during,:} C
    >>> p.parse(expression)
    A* =  A {!:} B
    A** =  A* {during,:} C
    D = A**
    >>> expression =  "D = A {:} B {during,!:} C"
    >>> print(expression)
    D = A {:} B {during,!:} C
    >>> p.parse(expression)
    A* =  A {:} B
    A** =  A* {during,!:} C
    D = A**
    >>> expression =  "D = A {:} (B {during,!:} (C : E))"
    >>> print(expression)
    D = A {:} (B {during,!:} (C : E))
    >>> p.parse(expression)
    C* =  C : E
    B* =  B {during,!:} C*
    A* =  A {:} B*
    D = A*
    >>> p.run = False
    >>> p.debug = False
    >>> expression =  "C = test1 : test2"
    >>> print(expression)
    C = test1 : test2
    >>> p.parse(expression, 'stvds')
    >>> expression =  'D = buff_t(test1,"10 months")'
    >>> print(expression)
    D = buff_t(test1,"10 months")
    >>> p.parse(expression, 'stvds')
    >>> expression =  'E = test2 {during,:} buff_t(test1,"1 days")'
    >>> print(expression)
    E = test2 {during,:} buff_t(test1,"1 days")
    >>> p.parse(expression, 'stvds')
    >>> expression =  'F = test2 {equal,:} buff_t(test1,"1 days")'
    >>> print(expression)
    F = test2 {equal,:} buff_t(test1,"1 days")
    >>> p.parse(expression, 'stvds')
    >>> p.debug = True
    >>> expression =  'H = tsnap(test2 {during,:} buff_t(test1, "1 days"))'
    >>> p.parse(expression, 'stvds')
    test1* = buff_t( test1 , " 1 days " )
    test2* =  test2 {during,:} test1*
    test2** = tsnap( test2* )
    H = test2**
    >>> expression =  'H = tshift(test2 {during,:} test1, "1 days")'
    >>> p.parse(expression, 'stvds')
    test2* =  test2 {during,:} test1
    test2** = tshift( test2* , " 1 days " )
    H = test2**
    >>> expression =  'H = tshift(H, 3)'
    >>> p.parse(expression, 'stvds')
    H* = tshift( H , 3 )
    H = H*
    >>> expression =  'C = if(td(A) == 2, A)'
    >>> p.parse(expression, 'stvds')
    td(A)
    td(A) == 2
    A* =  if condition True  then  A
    C = A*
    >>> expression =  'C = if(td(A) == 5 || start_date() >= "2010-01-01", A, B)'
    >>> p.parse(expression, 'stvds')
    td(A)
    td(A) == 5
    start_date >= "2010-01-01"
    True || True
    A* =  if condition True  then  A  else  B
    C = A*
    >>> expression =  'C = if(td(A) == 5, A, B)'
    >>> p.parse(expression, 'stvds')
    td(A)
    td(A) == 5
    A* =  if condition True  then  A  else  B
    C = A*

@endcode
"""

import grass.pygrass.modules as pygrass
from temporal_vector_operator import *
from temporal_algebra import *

##############################################################################

class TemporalVectorAlgebraLexer(TemporalAlgebraLexer):
    """!Lexical analyzer for the GRASS GIS temporal vector algebra"""

    def __init__(self):
        TemporalAlgebraLexer.__init__(self)

    # Buffer functions from v.buffer
    vector_buff_functions = {
       'buff_p'  : 'BUFF_POINT',
       'buff_l'   : 'BUFF_LINE',
       'buff_a'   : 'BUFF_AREA',
       }

    # This is the list of token names.
    vector_tokens = (
        'DISOR',
        'XOR',
        'NOT',
        'T_OVERLAY_OPERATOR',
    )

    # Build the token list
    tokens = TemporalAlgebraLexer.tokens \
                    + vector_tokens \
                    + tuple(vector_buff_functions.values())

    # Regular expression rules for simple tokens
    t_DISOR              = r'\+'
    t_XOR                = r'\^'
    t_NOT                = r'\~'
    t_T_OVERLAY_OPERATOR = r'\{([a-zA-Z\|]+[,])?([\|&+=]?[\|&+=\^\~])\}'

    # Parse symbols
    def temporal_symbol(self, t):
        # Check for reserved words
        if t.value in TemporalVectorAlgebraLexer.time_functions.keys():
            t.type = TemporalVectorAlgebraLexer.time_functions.get(t.value)
        elif t.value in TemporalVectorAlgebraLexer.datetime_functions.keys():
            t.type = TemporalVectorAlgebraLexer.datetime_functions.get(t.value)
        elif t.value in TemporalVectorAlgebraLexer.conditional_functions.keys():
            t.type = TemporalVectorAlgebraLexer.conditional_functions.get(t.value)
        elif t.value in TemporalVectorAlgebraLexer.vector_buff_functions.keys():
            t.type = TemporalVectorAlgebraLexer.vector_buff_functions.get(t.value)
        else:
            t.type = 'NAME'
        return t

##############################################################################

class TemporalVectorAlgebraParser(TemporalAlgebraParser):
    """The temporal algebra class"""

    # Get the tokens from the lexer class
    tokens = TemporalVectorAlgebraLexer.tokens

    # Setting equal precedence level for select and hash operations.
    precedence = (
        ('left', 'T_SELECT_OPERATOR', 'T_SELECT', 'T_NOT_SELECT'), # 1
        ('left', 'AND', 'OR', 'T_COMP_OPERATOR', 'T_OVERLAY_OPERATOR', 'DISOR', \
          'NOT', 'XOR'), #2
        )

    def __init__(self, pid=None, run=False, debug=True, spatial = False):
        TemporalAlgebraParser.__init__(self, pid, run, debug, spatial)

        self.m_overlay = pygrass.Module('v.overlay', quiet=True, run_=False)
        self.m_rename = pygrass.Module('g.rename', quiet=True, run_=False)
        self.m_patch = pygrass.Module('v.patch', quiet=True, run_=False)
        self.m_mremove = pygrass.Module('g.mremove', quiet=True, run_=False)
        self.m_buffer = pygrass.Module('v.buffer', quiet=True, run_=False)

    def parse(self, expression, basename = None, overwrite = False):
        self.lexer = TemporalVectorAlgebraLexer()
        self.lexer.build()
        self.parser = yacc.yacc(module=self, debug=self.debug)

        self.overwrite = overwrite
        self.count = 0
        self.stdstype = "stvds"
        self.basename = basename
        self.expression = expression
        self.parser.parse(expression)

    ######################### Temporal functions ##############################

    def remove_intermediate_vector_maps(self):
        """! Removes the intermediate vector maps.
        """
        if self.names != {}:
            namelist = self.names.values()
            max = 100
            chunklist = [namelist[i:i + max] for i in range(0, len(namelist), max)]
            for chunk in chunklist:
                stringlist = ",".join(chunk)
                if self.debug:
                    print "g.mremove type=vect pattern=%s"%(stringlist)

                if self.run:
                    m = copy.deepcopy(self.m_mremove)
                    m.inputs["type"].value = "vect"
                    m.inputs["pattern"].value = stringlist
                    m.flags["f"].value = True
                    m.run()

    def eval_toperator(self, operator, comparison = False):
        """!This function evaluates a string containing temporal operations.

          @param operator String of temporal operations, e.g. {equal|during,=!:}.

          @return List of temporal relations (equal, during), the given function
           (!:) and the interval/instances (=).

          @code
          >>> import grass.temporal as tgis
          >>> tgis.init(True)
          >>> p = tgis.TemporalVectorAlgebraParser()
          >>> operator = "{equal,:}"
          >>> p.eval_toperator(operator)
          (['EQUAL'], '=', ':')
          >>> operator = "{equal|during,:}"
          >>> p.eval_toperator(operator)
          (['EQUAL', 'DURING'], '=', ':')
          >>> operator = "{equal,!:}"
          >>> p.eval_toperator(operator)
          (['EQUAL'], '=', '!:')
          >>> operator = "{equal|during,!:}"
          >>> p.eval_toperator(operator)
          (['EQUAL', 'DURING'], '=', '!:')
          >>> operator = "{equal|during,=!:}"
          >>> p.eval_toperator(operator)
          (['EQUAL', 'DURING'], '=', '!:')
          >>> operator = "{equal|during|starts,#}"
          >>> p.eval_toperator(operator)
          (['EQUAL', 'DURING', 'STARTS'], '=', '#')
          >>> operator = "{!:}"
          >>> p.eval_toperator(operator)
          (['EQUAL'], '=', '!:')
          >>> operator = "{=:}"
          >>> p.eval_toperator(operator)
          (['EQUAL'], '=', ':')
          >>> operator = "{#}"
          >>> p.eval_toperator(operator)
          (['EQUAL'], '=', '#')
          >>> operator = "{equal|during}"
          >>> p.eval_toperator(operator)
          (['EQUAL', 'DURING'], None, None)
          >>> operator = "{equal}"
          >>> p.eval_toperator(operator)
          (['EQUAL'], None, None)
          >>> operator = "{equal,||}"
          >>> p.eval_toperator(operator, True)
          (['EQUAL'], '=', '||')
          >>> operator = "{equal|during,&&}"
          >>> p.eval_toperator(operator, True)
          (['EQUAL', 'DURING'], '=', '&&')
          >>> operator = "{&}"
          >>> p.eval_toperator(operator)
          (['EQUAL'], '=', '&')

          @endcode

        """

        p = TemporalVectorOperatorParser()
        p.parse(operator, comparison)
        p.relations = [rel.upper() for rel in p.relations]

        return(p.relations, p.temporal, p.function)

    def overlay_map_extent(self, mapA, mapB, bool_op = None, temp_op = '=',
                            copy = False):
        """!Compute the spatio-temporal extent of two topological related maps

           @param mapA The first map
           @param mapB The second maps
           @param bool_op The boolean operator specifying the spatial extent
                  operation (intersection, union, disjoint union)
           @param temp_op The temporal operator specifying the temporal
                  exntent operation (intersection, union, disjoint union)
           @param copy Specifies if the temporal extent of mapB should be
                  copied to mapA
        """
        returncode = TemporalAlgebraParser.overlay_map_extent(self, mapA, mapB,
                                                              bool_op, temp_op,
                                                              copy)
        if not copy and returncode == 1:
            # Conditional append of command list.
            if "cmd_list" in dir(mapA) and "cmd_list" in dir(mapB):
                mapA.cmd_list = mapA.cmd_list + mapB.cmd_list
            elif "cmd_list" not in dir(mapA) and "cmd_list" in dir(mapB):
                mapA.cmd_list = mapB.cmd_list

        return(returncode)

    ###########################################################################

    def p_statement_assign(self, t):
        # The expression should always return a list of maps.
        """
        statement : stds EQUALS expr

        """
        # Execute the command lists
        if self.run:
            if isinstance(t[3], list):
                num = len(t[3])
                count = 0
                returncode = 0
                register_list = []
                for i in range(num):
                    # Check if resultmap names exist in GRASS database.
                    vectorname = self.basename + "_" + str(i)
                    vectormap = VectorDataset(vectorname + "@" + get_current_mapset())
                    if vectormap.map_exists() and self.overwrite == False:
                        self.msgr.fatal(_("Error vector maps with basename %s exist. "
                                      "Use --o flag to overwrite existing file") \
                                      %(vectorname))
                for map_i in t[3]:
                    if "cmd_list" in dir(map_i):
                        # Execute command list.
                        for cmd in map_i.cmd_list:
                            try:
                                # We need to check if the input maps have areas in case of v.overlay
                                # otherwise v.overlay will break
                                if cmd.name == "v.overlay":
                                    for name in (cmd.inputs["ainput"].value,
                                                    cmd.inputs["binput"].value):
                                        #self.msgr.message("Check if map <" + name + "> exists")
                                        if name.find("@") < 0:
                                            name = name + "@" + get_current_mapset()
                                        tmp_map = map_i.get_new_instance(name)
                                        if not tmp_map.map_exists():
                                            raise Exception
                                        #self.msgr.message("Check if map <" + name + "> has areas")
                                        tmp_map.load()
                                        if tmp_map.metadata.get_number_of_areas() == 0:
                                            raise Exception
                            except Exception:
                                returncode = 1
                                break

                            # run the command
                            # print the command that will be executed
                            self.msgr.message("Run command:\n" + cmd.get_bash())
                            cmd.run()
                            if cmd.popen.returncode != 0:
                                self.msgr.fatal(_("Error starting %s : \n%s") \
                                                    %(cmd.get_bash(), \
                                                    cmd.popen.stderr))
                            mapname = cmd.outputs['output'].value
                            if mapname.find("@") >= 0:
                                map_test = map_i.get_new_instance(mapname)
                            else:
                                map_test = map_i.get_new_instance(mapname + "@" + self.mapset)
                            if not map_test.map_exists():
                                returncode = 1
                                break
                        if returncode == 0:
                            # We remove the invalid vector name from the remove list.
                            if self.names.has_key(map_i.get_name()):
                                self.names.pop(map_i.get_name())
                            mapset = map_i.get_mapset()
                            # Change map name to given basename.
                            newident = self.basename + "_" + str(count)
                            m = copy.deepcopy(self.m_rename)
                            m.inputs["vect"].value = (map_i.get_name(),newident)
                            m.flags["overwrite"].value = self.overwrite
                            m.run()
                            #m(vect = (map_i.get_name(),newident), \
                            #    overwrite = self.overwrite)
                            map_i.set_id(newident + "@" + mapset)
                            count += 1
                            register_list.append(map_i)
                    else:
                        register_list.append(map_i)

                if len(register_list) > 0:
                    # Open connection to temporal database.
                    dbif, connected = init_dbif(dbif=self.dbif)
                    # Create result space time dataset.
                    resultstds = open_new_space_time_dataset(t[1], self.stdstype, \
                                                                'absolute', t[1], t[1], \
                                                                "temporal vector algebra", dbif=dbif,
                                                                overwrite = self.overwrite)
                    for map_i in register_list:
                        # Check if modules should be executed from command list.
                        if "cmd_list" in dir(map_i):
                            # Get meta data from grass database.
                            map_i.load()
                            if map_i.is_in_db(dbif=dbif) and self.overwrite:
                                # Update map in temporal database.
                                map_i.update_all(dbif=dbif)
                            elif map_i.is_in_db(dbif=dbif) and self.overwrite == False:
                                # Raise error if map exists and no overwrite flag is given.
                                self.msgr.fatal(_("Error vector map %s exist in temporal database. "
                                                  "Use overwrite flag.  : \n%s") \
                                                  %(map_i.get_map_id(), cmd.popen.stderr))
                            else:
                                # Insert map into temporal database.
                                map_i.insert(dbif=dbif)
                        else:
                            #Get metadata from temporal database.
                            map_i.select(dbif=dbif)
                        # Register map in result space time dataset.
                        resultstds.register_map(map_i, dbif=dbif)
                        #count += 1
                        #if count % 10 == 0:
                        #    grass.percent(count, num, 1)
                    resultstds.update_from_registered_maps(dbif=dbif)
                    if connected:
                        dbif.close()
                self.remove_intermediate_vector_maps()
            t[0] = register_list

    def p_overlay_operation(self, t):
        """
        expr : stds AND stds
             | expr AND stds
             | stds AND expr
             | expr AND expr
             | stds OR stds
             | expr OR stds
             | stds OR expr
             | expr OR expr
             | stds XOR stds
             | expr XOR stds
             | stds XOR expr
             | expr XOR expr
             | stds NOT stds
             | expr NOT stds
             | stds NOT expr
             | expr NOT expr
             | stds DISOR stds
             | expr DISOR stds
             | stds DISOR expr
             | expr DISOR expr
        """
        # Check input stds.
        maplistA = self.check_stds(t[1])
        maplistB = self.check_stds(t[3])

        if self.run:
            t[0] = self.create_overlay_operations(maplistA, maplistB, ("EQUAL",), "=", t[2])
        else:
            t[0] = t[1]

    def p_overlay_operation_relation(self, t):
        """
        expr : stds T_OVERLAY_OPERATOR stds
             | expr T_OVERLAY_OPERATOR stds
             | stds T_OVERLAY_OPERATOR expr
             | expr T_OVERLAY_OPERATOR expr
        """
        # Check input stds.
        maplistA = self.check_stds(t[1])
        maplistB = self.check_stds(t[3])
        relations, temporal, function= self.eval_toperator(t[2])

        if self.run:
            t[0] = self.create_overlay_operations(maplistA, maplistB, relations, temporal, function)
        else:
            t[0] = t[1]

    def create_overlay_operations(self, maplistA, maplistB, relations, temporal, function):
        """!Create the spatial overlay operation commad list

           @param maplistA A list of map objects
           @param maplistB A list of map objects
           @param relations The temporal relationships that must be fullfilled as list of strings
                            ("EQUAL", "DURING", ...)
           @param temporal The temporal operator as string "=" or "&", ...
           @param function The spatial overlay operations as string "&", "|", ...
           @return Return the list of maps with overlay commands
        """
        topolist = self.get_temporal_topo_list(maplistA, maplistB, topolist = relations)

        # Select operation name.
        if function == "&":
            opname = "and"
        elif function == "|":
            opname = "or"
        elif function == "^":
            opname = "xor"
        elif function == "~":
            opname = "not"
        elif function == "+":
            opname = "disor"

        if self.run:
            resultlist = []
            for map_i in topolist:
                # Generate an intermediate name for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op=opname,
                                                copy=True)
                # Set first input for overlay module.
                mapainput = map_i.get_id()
                # Loop over temporal related maps and create overlay modules.
                tbrelations = map_i.get_temporal_relations()
                count = 0
                for topo in relations:
                    if topo in tbrelations.keys():
                        for map_j in (tbrelations[topo]):
                            # Create overlayed map extent.
                            returncode = self.overlay_map_extent(map_new, map_j, opname, \
                                                                    temp_op = temporal)
                            # Stop the loop if no temporal or spatial relationship exist.
                            if returncode == 0:
                                break
                            if count == 0:
                                # Set map name.
                                name = map_new.get_id()
                            else:
                                # Generate an intermediate name
                                name = self.generate_map_name()
                                map_new.set_id(name + "@" + mapset)
                            # Set second input for overlay module.
                            mapbinput = map_j.get_id()
                            # Create module command in PyGRASS for v.overlay and v.patch.
                            if opname != "disor":
                                m = copy.deepcopy(self.m_overlay)
                                m.run_ = False
                                m.inputs["operator"].value = opname
                                m.inputs["ainput"].value = str(mapainput)
                                m.inputs["binput"].value = str(mapbinput)
                                m.outputs["output"].value = name
                                m.flags["overwrite"].value = self.overwrite
                            else:
                                patchinput = str(mapainput) + ',' + str(mapbinput)
                                m = copy.deepcopy(self.m_patch)
                                m.run_ = False
                                m.inputs["input"].value = patchinput
                                m.outputs["output"].value = name
                                m.flags["overwrite"].value = self.overwrite
                            # Conditional append of module command.
                            if "cmd_list" in dir(map_new):
                                map_new.cmd_list.append(m)
                            else:
                                map_new.cmd_list = [m]
                            # Set new map name to temporary map name.
                            mapainput = name
                            count += 1
                        if returncode == 0:
                            break
                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

            return resultlist

    def p_buffer_operation(self,t):
        """
        expr : buff_function LPAREN stds COMMA number RPAREN
             | buff_function LPAREN expr COMMA number RPAREN
        """

        if self.run:
            # Check input stds.
            bufflist = self.check_stds(t[3])
            # Create empty result list.
            resultlist = []

            for map_i in bufflist:
                # Generate an intermediate name for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op=None,
                                                copy=True)
                # Change spatial extent based on buffer size.
                map_new.spatial_buffer(float(t[5]))
                # Check buff type.
                if t[1] == "buff_p":
                    buff_type = "point"
                elif t[1] == "buff_l":
                    buff_type = "line"
                elif t[1] == "buff_a":
                    buff_type = "area"
                m = copy.deepcopy(self.m_buffer)
                m.run_ = False
                m.inputs["type"].value = buff_type
                m.inputs["input"].value = str(map_i.get_id())
                m.inputs["distance"].value = float(t[5])
                m.outputs["output"].value = map_new.get_name()
                m.flags["overwrite"].value = self.overwrite

                # Conditional append of module command.
                if "cmd_list" in dir(map_new):
                    map_new.cmd_list.append(m)
                else:
                    map_new.cmd_list = [m]
                resultlist.append(map_new)

            t[0] = resultlist

    def p_buff_function(self, t):
        """buff_function    : BUFF_POINT
                            | BUFF_LINE
                            | BUFF_AREA
                            """
        t[0] = t[1]

    # Handle errors.
    def p_error(self, t):
        raise SyntaxError("syntax error on line %d near '%s' expression '%s'" %
            (t.lineno, t.value, self.expression))

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
