"""!@package grass.temporal

Temporal algebra parser class

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@authors Thomas Leppelt and Soeren Gebbert

@code

    >>> import grass.temporal as tgis
    >>> tgis.init(True)
    >>> p = tgis.TemporalAlgebraLexer()
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
    LexToken(T_COMP_OPERATOR,'{equals,||}',1,25)
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
    LexToken(T_COMP_OPERATOR,'{contain,&&}',1,27)
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
    >>> p = tgis.TemporalAlgebraParser()
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
    >>> expression =  'C = if(td(A) == 5, A, B)'
    >>> p.parse(expression, 'stvds')
    td(A)
    td(A) == 5
    A* =  if condition True  then  A  else  B
    C = A*
    >>> expression =  'C = if(td(A) == 5 || start_date() > "2010-01-01", A, B)'
    >>> p.parse(expression, 'stvds')
    td(A)
    td(A) == 5
    start_date > "2010-01-01"
    True || True
    A* =  if condition True  then  A  else  B
    C = A*

@endcode
"""

try:
    import ply.lex as lex
    import ply.yacc as yacc
except:
    pass

import os
from space_time_datasets import *
from factory import *
from open_stds import *
import copy

##############################################################################

class TemporalAlgebraLexer(object):
    """!Lexical analyzer for the GRASS GIS temporal algebra"""

    # Functions that defines an if condition, temporal buffering and snapping
    conditional_functions = {
        'if'    : 'IF',
        'buff_t': 'BUFF_T',
        'tsnap'  : 'TSNAP',
        'tshift' : 'TSHIFT',
    }

    # Variables with date and time strings
    datetime_functions = {
        'start_time'     : 'START_TIME',     # start time as HH::MM:SS
        'start_date'     : 'START_DATE',     # start date as yyyy-mm-DD
        'start_datetime' : 'START_DATETIME', # start datetime as yyyy-mm-DD HH:MM:SS
        'end_time'       : 'END_TIME',       # end time as HH:MM:SS
        'end_date'       : 'END_DATE',       # end date as yyyy-mm-DD
        'end_datetime'   : 'END_DATETIME',   # end datetime as  yyyy-mm-DD HH:MM:SS
    }

    # Time functions
    time_functions = {
        'td'          : 'TD',            # The size of the current
                                         # sample time interval in days and
                                         # fraction of days for absolute time,
                                         # and in relative units in case of relative time.
        #'start_td'    : 'START_TD',     # The time difference between the start
                                         # time of the sample space time raster
                                         # dataset and the start time of the
                                         # current sample interval or instance.
                                         # The time is measured in days and
                                         # fraction of days for absolute time,
                                         # and in relative units in case of relative time.
        #'end_td'      : 'END_TD',       # The time difference between the
                                         # start time of the sample
                                         # space time raster dataset and the
                                         # end time of the current sample interval.
                                         # The time is measured in days and
                                         # fraction of days for absolute time,
                                         # and in relative units in case of relative time.
                                         # The end_time() will be represented by null() in case of a time instance.
        'start_doy'   : 'START_DOY',     # Day of year (doy) from the start time [1 - 366]
        'start_dow'   : 'START_DOW',     # Day of week (dow) from the start time [1 - 7], the start of the week is Monday == 1
        'start_year'  : 'START_YEAR',    # The year of the start time [0 - 9999]
        'start_month' : 'START_MONTH',   # The month of the start time [1 - 12]
        'start_week'  : 'START_WEEK',    # Week of year of the start time [1 - 54]
        'start_day'   : 'START_DAY',     # Day of month from the start time [1 - 31]
        'start_hour'  : 'START_HOUR',    # The hour of the start time [0 - 23]
        'start_minute': 'START_MINUTE',  # The minute of the start time [0 - 59]
        'start_second': 'START_SECOND',  # The second of the start time [0 - 59]
        'end_doy'     : 'END_DOY',       # Day of year (doy) from the end time [1 - 366]
        'end_dow'     : 'END_DOW',       # Day of week (dow) from the end time [1 - 7], the start of the week is Monday == 1
        'end_year'    : 'END_YEAR',      # The year of the end time [0 - 9999]
        'end_month'   : 'END_MONTH',     # The month of the end time [1 - 12]
        'end_week'    : 'END_WEEK',      # Week of year of the end time [1 - 54]
        'end_day'     : 'END_DAY',       # Day of month from the start time [1 - 31]
        'end_hour'    : 'END_HOUR',      # The hour of the end time [0 - 23]
        'end_minute'  : 'END_MINUTE',    # The minute of the end time [0 - 59]
        'end_second'  : 'END_SECOND',    # The second of the end time [0 - 59]
    }

    # This is the list of token names.
    tokens = (
        'DATETIME',
        'TIME',
        'DATE',
        'INT',
        'FLOAT',
        'LPAREN',
        'RPAREN',
        'COMMA',
        'CEQUALS',
        'EQUALS',
        'UNEQUALS',
        'LOWER',
        'LOWER_EQUALS',
        'GREATER',
        'GREATER_EQUALS',
        'HASH',
        'OR',
        'AND',
        'T_SELECT_OPERATOR',
        'T_HASH_OPERATOR',
        'T_COMP_OPERATOR',
        'T_REL_OPERATOR',
        'T_SELECT',
        'T_NOT_SELECT',
        'NAME',
        'QUOTE',
    )

    # Build the token list
    tokens = tokens + tuple(datetime_functions.values()) \
                    + tuple(time_functions.values()) \
                    + tuple(conditional_functions.values())

    # Regular expression rules for simple tokens
    t_T_SELECT_OPERATOR  = r'\{([a-zA-Z\| ]+[,])?([\|&+=]?[!]?[:])\}'
    t_T_HASH_OPERATOR    = r'\{([a-zA-Z\| ]+[,])?[#]\}'
    t_T_COMP_OPERATOR    = r'\{([a-zA-Z\| ]+[,])?(\|\||&&)\}'
    t_T_REL_OPERATOR     = r'\{([a-zA-Z\| ])+\}'
    t_T_SELECT           = r':'
    t_T_NOT_SELECT       = r'!:'
    t_LPAREN             = r'\('
    t_RPAREN             = r'\)'
    t_COMMA              = r','
    t_CEQUALS            = r'=='
    t_EQUALS             = r'='
    t_UNEQUALS           = r'!='
    t_LOWER              = r'<'
    t_LOWER_EQUALS       = r'<='
    t_GREATER            = r'>'
    t_GREATER_EQUALS     = r'>='
    t_HASH               = r'\#'
    t_OR                 = r'[\|]'
    t_AND                = r'[&]'
    t_QUOTE              = r'[\"\']'

    # These are the things that should be ignored.
    t_ignore = ' \t'

    # Read time string and convert it into a date object
    def t_DATETIME(self, t):
        r'"\d\d\d\d-(0[1-9]|1[012])-(0[1-9]|[12][0-9]|3[01])[ T](0[0-9]|1(0-9)|2[0-4]):(0[0-9]|[1-5][0-9]|60):(0[0-9]|[1-5][0-9]|60)"'
        # t.value = int(t.value)
        return t


    # Read date string and convert it into a date object
    def t_DATE(self, t):
        r'"\d\d\d\d-(0[1-9]|1[012])-(0[1-9]|[12][0-9]|3[01])"'
        # t.value = int(t.value)
        return t

    # Read time string and convert it into a date object
    def t_TIME(self, t):
        r'"(0[0-9]|1[0-9]|2[0-4]):(0[0-9]|[1-5][0-9]|60):(0[0-9]|[1-5][0-9]|60)"'
        # t.value = int(t.value)
        return t

    # Read in a float.  This rule has to be done before the int rule.
    def t_FLOAT(self, t):
        r'-?\d+\.\d*(e-?\d+)?'
        t.value = float(t.value)
        return t

    # Read in an int.
    def t_INT(self, t):
        r'-?\d+'
        t.value = int(t.value)
        return t
    # Read in a list of maps.
    def  t_LIST(self, t):
        r'[\[][.]*[\]]'
        t.value = list(t.value)
        return t

    # Ignore comments.
#    def t_comment(self, t):
#        r'^[#][^\n]*'
#        pass

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
        if t.value in TemporalAlgebraLexer.time_functions.keys():
            t.type = TemporalAlgebraLexer.time_functions.get(t.value)
        elif t.value in TemporalAlgebraLexer.datetime_functions.keys():
            t.type = TemporalAlgebraLexer.datetime_functions.get(t.value)
        elif t.value in TemporalAlgebraLexer.conditional_functions.keys():
            t.type = TemporalAlgebraLexer.conditional_functions.get(t.value)
        else:
            t.type = 'NAME'
        return t

    # Handle errors.
    def t_error(self, t):
        raise SyntaxError("syntax error on line %d near '%s'" %
            (t.lineno, t.value))

    # Build the lexer
    def build(self,**kwargs):
        self.lexer = lex.lex(module=self, optimize=False, debug=False, **kwargs)

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

class GlobalTemporalVar(object):
    """ This class handles global temporal variable conditional expressions,
        like start_doy() == 3.
        The three parts of the statement are stored separately in
        tfunc (START_DOY), compop (==) and value (3).
        But also boolean values, time differences and relation operators for comparison in
        if-statements can be stored in this class.
    """
    def __init__(self):
        self.tfunc        = None
        self.compop       = None
        self.value        = None
        self.boolean      = None
        self.relationop   = None
        self.topology     = []
        self.td           = None

    def get_type(self):
        if self.tfunc != None and self.compop != None and self.value != None:
            return("global")
        elif self.boolean != None:
            return("boolean")
        elif self.relationop != None and self.topology != []:
            return("operator")
        elif self.td != None:
            return("timediff")

    def get_type_value(self):
        typename = self.get_type()
        valuelist = []
        if typename == "global":
            valuelist = [self.tfunc, self.compop, self.value]
        elif typename == "operator":
            valuelist.append(self.topology)
            valuelist.append(self.relationop)
        elif typename == "boolean":
            valuelist = self.boolean
        elif typename == "timediff":
            valuelist.append(self.td)

        return(valuelist)

    def __str__(self):
        return str(self.tfunc) + str(self.compop) + str(self.value)

###############################################################################

class FatalError(Exception):
    def __init__(self, msg):
        self.value = msg

    def __str__(self):
        return self.value

###############################################################################

class TemporalAlgebraParser(object):
    """The temporal algebra class"""

    # Get the tokens from the lexer class
    tokens = TemporalAlgebraLexer.tokens

    # Setting equal precedence level for select and hash operations.
    precedence = (
        ('left', 'T_SELECT_OPERATOR', 'T_SELECT', 'T_NOT_SELECT'), # 1
        ('left', 'AND', 'OR', 'T_COMP_OPERATOR'), #2
        )

    def __init__(self, pid=None, run = True, debug = False, spatial = False, null = False):
        self.run = run
        self.debug = debug
        self.pid = pid
        # Intermediate vector map names
        self.names = {}
        # Count map names
        self.spatial = spatial
        self.null = null
        self.mapset = get_current_mapset()
        self.temporaltype = None
        self.msgr = get_tgis_message_interface()
        self.dbif = SQLDatabaseInterfaceConnection()
        self.dbif.connect()

    def __del__(self):
        if self.dbif.connected:
            self.dbif.close()

    def parse(self, expression, stdstype = 'strds', basename = None, overwrite=False):
        self.lexer = TemporalAlgebraLexer()
        self.lexer.build()
        self.parser = yacc.yacc(module=self, debug=self.debug)

        self.overwrite = overwrite
        self.count = 0
        self.stdstype = stdstype
        self.basename = basename
        self.expression = expression
        self.parser.parse(expression)

    def generate_map_name(self):
        """!Generate an unique intermediate vector map name
            and register it in the objects map list for later removement.

            The vector map names are unique between processes. Do not use the
            same object for map name generation in multiple threads.
        """
        self.count += 1
        if self.pid != None:
            pid = self.pid
        else:
            pid = os.getpid()
        name = "tmp_map_name_%i_%i"%(pid, self.count)
        self.names[name] = name
        return name

    def generate_new_map(self, base_map, bool_op = 'and', copy = True):
        """!Generate a new map using the spatio-temporal extent of the base map

           @param base_map This map is used to create the new map
        """
        # Generate an intermediate name for the result map list.
        name = self.generate_map_name()
        # Check for mapset in given stds input.
        mapname = name + "@" + self.mapset
        # Create new map based on the related map list.
        map_new = base_map.get_new_instance(mapname)
        # Set initial map extend of new vector map.
        self.overlay_map_extent(map_new, base_map, bool_op = bool_op, copy = copy)

        return map_new

    def overlay_map_extent(self, mapA, mapB, bool_op = None, temp_op = '=',
                            copy = False):
        """!Compute the spatio-temporal extent of two topological related maps

           @param mapA The first map
           @param mapB The second maps
           @param bool_op The boolean operator specifying the spatial extent
                  operation (intersection, union, disjoint union)
           @param temp_op The temporal operator specifying the temporal
                  extent operation (intersection, union, disjoint union)
           @param copy Specifies if the temporal extent of mapB should be
                  copied to mapA
           @return 0 if there is no overlay
        """
        returncode = 1
        if copy:
            map_extent_temporal = mapB.get_temporal_extent()
            map_extent_spatial = mapB.get_spatial_extent()
            # Set initial map extend of new vector map.
            mapA.set_spatial_extent(map_extent_spatial)
            mapA.set_temporal_extent(map_extent_temporal)
            if "cmd_list" in dir(mapB):
                mapA.cmd_list = mapB.cmd_list
        else:
            # Calculate spatial extent for different overlay operations.
            if bool_op == 'and':
                overlay_ext = mapA.spatial_intersection(mapB)
                if overlay_ext != None:
                    mapA.set_spatial_extent(overlay_ext)
                else:
                    returncode = 0
            elif bool_op in ['or', 'xor']:
                overlay_ext = mapA.spatial_union(mapB)
                if overlay_ext != None:
                    mapA.set_spatial_extent(overlay_ext)
                else:
                    returncode = 0
            elif bool_op == 'disor':
                overlay_ext = mapA.spatial_disjoint_union(mapB)
                if overlay_ext != None:
                    mapA.set_spatial_extent(overlay_ext)
                else:
                    returncode = 0

            # Calculate temporal extent for different temporal operators.
            if temp_op == '&':
                temp_ext = mapA.temporal_intersection(mapB)
                if temp_ext != None:
                    mapA.set_temporal_extent(temp_ext)
                else:
                    returncode = 0
            elif temp_op == '|':
                temp_ext = mapA.temporal_union(mapB)
                if temp_ext != None:
                    mapA.set_temporal_extent(temp_ext)
                else:
                    returncode = 0
            elif temp_op == '+':
                temp_ext = mapA.temporal_disjoint_union(mapB)
                if temp_ext != None:
                    mapA.set_temporal_extent(temp_ext)
                else:
                    returncode = 0

        return(returncode)

    ######################### Temporal functions ##############################

    def check_stds(self, input, clear = False):
        """! Check if input space time dataset exist in database and return its map list.

            @param input Name of space time data set as string or list of maps.
            @param clear Reset the stored conditional values to empty list.

            @return List of maps.

        """
        if not isinstance(input, list):
            # Check for mapset in given stds input.
            if input.find("@") >= 0:
                id_input = input
            else:
                id_input = input + "@" + self.mapset
            # Create empty spacetime dataset.
            stds = dataset_factory(self.stdstype, id_input)
            # Check for occurence of space time dataset.
            if stds.is_in_db(dbif=self.dbif) == False:
                raise FatalError(_("Space time %s dataset <%s> not found") %
                    (stds.get_new_map_instance(None).get_type(), id_input))
            else:
                # Select temporal dataset entry from database.
                stds.select(dbif=self.dbif)
                maplist = stds.get_registered_maps_as_objects(dbif=self.dbif)
                # Create map_value as empty list item.
                for map_i in maplist:
                    if "map_value" not in dir(map_i):
                        map_i.map_value = []
                    if "condition_value" not in dir(map_i):
                        map_i.condition_value = []
                    # Set and check global temporal type variable and map.
                    if map_i.is_time_absolute() and self.temporaltype == None:
                        self.temporaltype = 'absolute'
                    elif map_i.is_time_relative() and self.temporaltype == None:
                        self.temporaltype = 'relative'
                    elif map_i.is_time_absolute() and self.temporaltype == 'relative':
                        self.msgr.fatal(_("Wrong temporal type of space time dataset <%s> \
                                      <%s> time is required") %
                                     (id_input, self.temporaltype))
                    elif map_i.is_time_relative() and self.temporaltype == 'absolute':
                        self.msgr.fatal(_("Wrong temporal type of space time dataset <%s> \
                                      <%s> time is required") %
                                     (id_input, self.temporaltype))

        else:
            maplist = input
            # Create map_value as empty list item.
            for map_i in maplist:
                if "map_value" not in dir(map_i):
                    map_i.map_value = []
                elif clear:
                    map_i.map_value = []
                if "condition_value" not in dir(map_i):
                    map_i.condition_value = []
                elif clear:
                    map_i.condition_value = []

        return(maplist)

    def get_temporal_topo_list(self, maplistA, maplistB = None, topolist = ["EQUAL"],
                               assign_val = False, count_map = False):
        """!Build temporal topology for two space time data sets, copy map objects
          for given relation into map list.
          @param maplistA List of maps.
          @param maplistB List of maps.
          @param topolist List of strings of temporal relations.
          @param assign_val Boolean for assigning a boolean map value based on
                            the map_values from the compared map list by
                            topological relationships.
          @param count_map Boolean if the number of topological related maps
                           should be returned.
          @return List of maps from maplistA that fulfil the topological relationships
                  to maplistB specified in topolist.

          @code


          # Example with two lists of maps
          >>> import grass.temporal as tgis
          >>> tgis.init(True)
          >>> l = tgis.TemporalAlgebraParser()
          >>> # Create two list of maps with equal time stamps
          >>> mapsA = []
          >>> mapsB = []
          >>> for i in range(10):
          ...     idA = "a%i@B"%(i)
          ...     mapA = tgis.RasterDataset(idA)
          ...     idB = "b%i@B"%(i)
          ...     mapB = tgis.RasterDataset(idB)
          ...     check = mapA.set_relative_time(i, i + 1, "months")
          ...     check = mapB.set_relative_time(i, i + 1, "months")
          ...     mapsA.append(mapA)
          ...     mapsB.append(mapB)
          >>> resultlist = l.get_temporal_topo_list(mapsA, mapsB, ['EQUAL'])
          >>> for map in resultlist:
          ...     if map.get_equal():
          ...         relations = map.get_equal()
          ...         print "Map %s has equal relation to map %s"%(map.get_name(),
          ...               relations[0].get_name())
          Map a0 has equal relation to map b0
          Map a1 has equal relation to map b1
          Map a2 has equal relation to map b2
          Map a3 has equal relation to map b3
          Map a4 has equal relation to map b4
          Map a5 has equal relation to map b5
          Map a6 has equal relation to map b6
          Map a7 has equal relation to map b7
          Map a8 has equal relation to map b8
          Map a9 has equal relation to map b9
          >>> resultlist = l.get_temporal_topo_list(mapsA, mapsB, ['DURING'])
          >>> print(resultlist)
          []
          >>> # Create two list of maps with equal time stamps
          >>> mapsA = []
          >>> mapsB = []
          >>> for i in range(10):
          ...     idA = "a%i@B"%(i)
          ...     mapA = tgis.RasterDataset(idA)
          ...     idB = "b%i@B"%(i)
          ...     mapB = tgis.RasterDataset(idB)
          ...     check = mapA.set_relative_time(i, i + 1, "months")
          ...     check = mapB.set_relative_time(i, i + 2, "months")
          ...     mapsA.append(mapA)
          ...     mapsB.append(mapB)
          >>> resultlist = l.get_temporal_topo_list(mapsA, mapsB, ['starts','during'])
          >>> for map in resultlist:
          ...     if map.get_starts():
          ...         relations = map.get_starts()
          ...         print "Map %s has start relation to map %s"%(map.get_name(),
          ...               relations[0].get_name())
          Map a0 has start relation to map b0
          Map a1 has start relation to map b1
          Map a2 has start relation to map b2
          Map a3 has start relation to map b3
          Map a4 has start relation to map b4
          Map a5 has start relation to map b5
          Map a6 has start relation to map b6
          Map a7 has start relation to map b7
          Map a8 has start relation to map b8
          Map a9 has start relation to map b9
          >>> for map in resultlist:
          ...     if map.get_during():
          ...         relations = map.get_during()
          ...         print "Map %s has during relation to map %s"%(map.get_name(),
          ...               relations[0].get_name())
          Map a0 has during relation to map b0
          Map a1 has during relation to map b0
          Map a2 has during relation to map b1
          Map a3 has during relation to map b2
          Map a4 has during relation to map b3
          Map a5 has during relation to map b4
          Map a6 has during relation to map b5
          Map a7 has during relation to map b6
          Map a8 has during relation to map b7
          Map a9 has during relation to map b8
          >>> # Create two list of maps with equal time stamps and map_value method.
          >>> mapsA = []
          >>> mapsB = []
          >>> for i in range(10):
          ...     idA = "a%i@B"%(i)
          ...     mapA = tgis.RasterDataset(idA)
          ...     idB = "b%i@B"%(i)
          ...     mapB = tgis.RasterDataset(idB)
          ...     check = mapA.set_relative_time(i, i + 1, "months")
          ...     check = mapB.set_relative_time(i, i + 1, "months")
          ...     mapB.map_value = True
          ...     mapsA.append(mapA)
          ...     mapsB.append(mapB)
          >>> # Create two list of maps with equal time stamps
          >>> mapsA = []
          >>> mapsB = []
          >>> for i in range(10):
          ...     idA = "a%i@B"%(i)
          ...     mapA = tgis.RasterDataset(idA)
          ...     mapA.map_value = True
          ...     idB = "b%i@B"%(i)
          ...     mapB = tgis.RasterDataset(idB)
          ...     mapB.map_value = False
          ...     check = mapA.set_absolute_time(datetime(2000,1,i+1),
          ...             datetime(2000,1,i + 2))
          ...     check = mapB.set_absolute_time(datetime(2000,1,i+6),
          ...             datetime(2000,1,i + 7))
          ...     mapsA.append(mapA)
          ...     mapsB.append(mapB)
          >>> resultlist = l.get_temporal_topo_list(mapsA, mapsB)
          >>> for map in resultlist:
          ...     print(map.get_id())
          a5@B
          a6@B
          a7@B
          a8@B
          a9@B
          >>> resultlist = l.get_temporal_topo_list(mapsA, mapsB, ['during'])
          >>> for map in resultlist:
          ...     print(map.get_id())

          @endcode
        """
        topologylist = ["EQUAL", "FOLLOWS", "PRECEDES", "OVERLAPS", "OVERLAPPED", \
                        "DURING", "STARTS", "FINISHES", "CONTAINS", "STARTED", \
                        "FINISHED"]
        complementdict = {"EQUAL": "EQUAL", "FOLLOWS" : "PRECEDES",
                          "PRECEDES" : "FOLLOWS", "OVERLAPS" : "OVERLAPPED",
                          "OVERLAPPED" : "OVERLAPS", "DURING" : "CONTAINS",
                          "CONTAINS" : "DURING", "STARTS" : "STARTED",
                          "STARTED" : "STARTS", "FINISHES" : "FINISHED",
                          "FINISHED" : "FINISHES"}
        resultdict = {}
        # Check if given temporal relation are valid.
        for topo in topolist:
          if topo.upper() not in topologylist:
              raise SyntaxError("Unpermitted temporal relation name '" + topo + "'")

        # Create temporal topology for maplistA to maplistB.
        tb = SpatioTemporalTopologyBuilder()
        # Dictionary with different spatial variables used for topology builder.
        spatialdict = {'strds' : '2D', 'stvds' : '2D', 'str3ds' : '3D'}
        # Build spatial temporal topology
        if self.spatial:
            tb.build(maplistA, maplistB, spatial = spatialdict[self.stdstype])
        else:
            tb.build(maplistA, maplistB)
        # Iterate through maps in maplistA and search for relationships given
        # in topolist.
        # TODO: Better implementation with less nesting
        for map_i in maplistA:
            tbrelations = map_i.get_temporal_relations()
            for topo in topolist:
                if topo.upper() in tbrelations.keys():
                    if assign_val:
                        mapvaluelist = []
                        if complementdict[topo.upper()] in tbrelations:
                            relationmaplist = tbrelations[complementdict[topo.upper()]]
                            for relationmap in relationmaplist:
                                if "map_value" in dir(relationmap):
                                    for element in relationmap.map_value:
                                        if isinstance(element, GlobalTemporalVar):
                                            if element.get_type() == "boolean":
                                                mapvaluelist.append(element.boolean)
                        if all(mapvaluelist):
                            resultbool = True
                        else:
                            resultbool = False
                        if "condition_value" in dir(map_i):
                            if isinstance(map_i.condition_value, list):
                                map_i.condition_value.append(resultbool)
                    if count_map:
                        relationmaplist = tbrelations[topo.upper()]
                        gvar = GlobalTemporalVar()
                        gvar.td = len(relationmaplist)
                        if "map_value" in dir(map_i):
                            map_i.map_value.append(gvar)
                        else:
                            map_i.map_value = gvar
                    resultdict[map_i.get_id()] = map_i
        resultlist = resultdict.values()

        # Sort list of maps chronological.
        resultlist = sorted(resultlist, key = AbstractDatasetComparisonKeyStartTime)

        return(resultlist)

    def eval_toperator(self, operator):
        """!This function evaluates a string containing temporal operations.

          @param operator String of temporal operations, e.g. {equal|during,=!:}.

          @return List of temporal relations (equal, during), the given function
           (!:) and the interval/instances (=).

          @code
          >>> import grass.temporal as tgis
          >>> tgis.init()
          >>> p = tgis.TemporalAlgebraParser()
          >>> operator = "{equal,:}"
          >>> p.eval_toperator(operator)
          (['equal'], '=', ':')
          >>> operator = "{equal|during,:}"
          >>> p.eval_toperator(operator)
          (['equal', 'during'], '=', ':')
          >>> operator = "{equal,!:}"
          >>> p.eval_toperator(operator)
          (['equal'], '=', '!:')
          >>> operator = "{equal|during,!:}"
          >>> p.eval_toperator(operator)
          (['equal', 'during'], '=', '!:')
          >>> operator = "{equal|during,=!:}"
          >>> p.eval_toperator(operator)
          (['equal', 'during'], '=', '!:')
          >>> operator = "{equal|during|starts,#}"
          >>> p.eval_toperator(operator)
          (['equal', 'during', 'starts'], '=', '#')
          >>> operator = "{!:}"
          >>> p.eval_toperator(operator)
          (['equal'], '=', '!:')
          >>> operator = "{=:}"
          >>> p.eval_toperator(operator)
          (['equal'], '=', ':')
          >>> operator = "{#}"
          >>> p.eval_toperator(operator)
          (['equal'], '=', '#')
          >>> operator = "{equal|during}"
          >>> p.eval_toperator(operator)
          (['equal', 'during'], '=', '')
          >>> operator = "{equal}"
          >>> p.eval_toperator(operator)
          (['equal'], '=', '')
          >>> operator = "{equal,||}"
          >>> p.eval_toperator(operator)
          (['equal'], '=', '||')
          >>> operator = "{equal|during,&&}"
          >>> p.eval_toperator(operator)
          (['equal', 'during'], '=', '&&')

          @endcode

        """
        topologylist = ["EQUAL", "FOLLOWS", "PRECEDES", "OVERLAPS", "OVERLAPPED", \
                        "DURING", "STARTS", "FINISHES", "CONTAINS", "STARTED", \
                        "FINISHED"]
        functionlist = [":", "!:", "#"]
        intervallist = ["=", "|", "&", "+"]
        comparelist  = ["||", "&&"]
        relations = []
        interval = '='
        function = ''
        op = operator.strip('{}')
        oplist = op.split(',')
        if len(oplist) > 1:
            relationlist =  oplist[0].split('|')
            for relation in relationlist:
                if relation.upper() in topologylist and relation not in relations:
                    relations.append(relation)
                else:
                    raise SyntaxError("invalid syntax")
            opright = oplist[1]
            if opright in comparelist:
                function = opright
            elif opright[0] in intervallist:
                interval = opright[0]
                if opright[1:] in functionlist:
                    function = opright[1:]
                else:
                    raise SyntaxError("invalid syntax")
            elif opright in functionlist:
                function = opright
            else:
                raise SyntaxError("invalid syntax")
        elif all([rel.upper() in topologylist for rel in oplist[0].split('|')]):
            relations = oplist[0].split('|')
        else:
            relations = ['equal']
            opstr = str(oplist[0])
            if opstr[0] in intervallist:
                interval = opstr[0]
                if opstr[1:] in functionlist:
                    function = opstr[1:]
                else:
                    raise SyntaxError("invalid syntax")
            elif opstr in functionlist:
                function = opstr
            #else:
                #raise SyntaxError("invalid syntax")

        return(relations, interval, function)

    def perform_temporal_selection(self, maplistA, maplistB, topolist = ["EQUAL"],
                                   inverse = False, assign_val = False):
        """!This function performs temporal selection operation.

          @param maplistA   List of maps representing the left side of a temporal
                             expression.
          @param maplistB   List of maps representing the right side of a temporal
                             expression.
          @param topolist   List of strings of temporal relations.
          @param inverse    Boolean value that specifies if the selection should be
                             inverted.
          @param assign_val Boolean for assigning a boolean map value based on
                            the map_values from the compared map list by
                            topological relationships.

          @return List of selected maps from maplistA.

          @code

          >>> import grass.temporal as tgis
          >>> tgis.init()
          >>> l = tgis.TemporalAlgebraParser()
          >>> # Example with two lists of maps
          >>> # Create two list of maps with equal time stamps
          >>> mapsA = []
          >>> mapsB = []
          >>> for i in range(10):
          ...     idA = "a%i@B"%(i)
          ...     mapA = tgis.RasterDataset(idA)
          ...     idB = "b%i@B"%(i)
          ...     mapB = tgis.RasterDataset(idB)
          ...     check = mapA.set_relative_time(i, i + 1, "months")
          ...     check = mapB.set_relative_time(i + 5, i + 6, "months")
          ...     mapsA.append(mapA)
          ...     mapsB.append(mapB)
          >>> resultlist = l.perform_temporal_selection(mapsA, mapsB, ['EQUAL'],
          ...                                           False)
          >>> for map in resultlist:
          ...     if map.get_equal():
          ...         relations = map.get_equal()
          ...         print "Map %s has equal relation to map %s"%(map.get_name(),
          ...               relations[0].get_name())
          Map a5 has equal relation to map b0
          Map a6 has equal relation to map b1
          Map a7 has equal relation to map b2
          Map a8 has equal relation to map b3
          Map a9 has equal relation to map b4
          >>> resultlist = l.perform_temporal_selection(mapsA, mapsB, ['EQUAL'],
          ...                                           True)
          >>> for map in resultlist:
          ...     if not map.get_equal():
          ...         print "Map %s has no equal relation to mapset mapsB"%(map.get_name())
          Map a0 has no equal relation to mapset mapsB
          Map a1 has no equal relation to mapset mapsB
          Map a2 has no equal relation to mapset mapsB
          Map a3 has no equal relation to mapset mapsB
          Map a4 has no equal relation to mapset mapsB

          @endcode
        """
        if not inverse:
            topolist = self.get_temporal_topo_list(maplistA, maplistB, topolist,
                                                    assign_val = assign_val)
            resultlist = topolist

        else:
            topolist = self.get_temporal_topo_list(maplistA, maplistB, topolist,
                                                    assign_val = False)
            resultlist = []

            for map_i in maplistA:
                if map_i not in topolist:
                    resultlist.append(map_i)
                    if assign_val:
                        if "condition_value" in dir(map_i):
                            map_i.condition_value.append(False)

        # Sort list of maps chronological.
        resultlist = sorted(resultlist, key = AbstractDatasetComparisonKeyStartTime)

        return(resultlist)

    def set_granularity(self, maplistA, maplistB, toperator = '=', topolist = ["EQUAL"]):
        """!This function sets the temporal extends of a list of maps based on
             another map list.

          @param maplistB List of maps.
          @param maplistB List of maps.
          @param toperator String containing the temporal operator: =, +, &, |.
          @param topolist List of topological relations.

          @return List of maps with the new temporal extends.

          @code
          >>> import grass.temporal as tgis
          >>> tgis.init()
          >>> p = tgis.TemporalAlgebraParser()
          >>> # Create two list of maps with equal time stamps
          >>> mapsA = []
          >>> mapsB = []
          >>> for i in range(10):
          ...     idA = "a%i@B"%(i)
          ...     mapA = tgis.RasterDataset(idA)
          ...     idB = "b%i@B"%(i)
          ...     mapB = tgis.RasterDataset(idB)
          ...     check = mapA.set_relative_time(i, i + 1, "months")
          ...     check = mapB.set_relative_time(i*2, i*2 + 2, "months")
          ...     mapsA.append(mapA)
          ...     mapsB.append(mapB)
          >>> resultlist = p.set_granularity(mapsA, mapsB, toperator = "|", topolist = ["during"])
          >>> for map in resultlist:
          ...     start,end,unit = map.get_relative_time()
          ...     print(map.get_id() + ' - start: ' + str(start) + ' end: ' + str(end))
          a1@B - start: 0 end: 2
          a0@B - start: 0 end: 2
          a3@B - start: 2 end: 4
          a2@B - start: 2 end: 4
          a5@B - start: 4 end: 6
          a4@B - start: 4 end: 6
          a7@B - start: 6 end: 8
          a6@B - start: 6 end: 8
          a9@B - start: 8 end: 10
          a8@B - start: 8 end: 10

          @endcode
        """
        topologylist = ["EQUAL", "FOLLOWS", "PRECEDES", "OVERLAPS", "OVERLAPPED", \
                        "DURING", "STARTS", "FINISHES", "CONTAINS", "STARTED", \
                        "FINISHED"]

        for topo in topolist:
          if topo.upper() not in topologylist:
              raise SyntaxError("Unpermitted temporal relation name '" + topo + "'")

        # Create temporal topology for maplistA to maplistB.
        tb = SpatioTemporalTopologyBuilder()
        # Dictionary with different spatial variables used for topology builder.
        spatialdict = {'strds' : '2D', 'stvds' : '2D', 'str3ds' : '3D'}
        # Build spatial temporal topology for maplistB to maplistB.
        if self.spatial:
            tb.build(maplistA, maplistB, spatial = spatialdict[self.stdstype])
        else:
            tb.build(maplistA, maplistB)
        resultdict = {}
        # Iterate through maps in maplistA and search for relationships given
        # in topolist.
        for map_i in maplistA:
            tbrelations = map_i.get_temporal_relations()
            for topo in topolist:
                if topo.upper() in tbrelations.keys():
                    relationmaplist = tbrelations[topo.upper()]
                    for relationmap in relationmaplist:
                        newextend = None
                        if toperator == "&":
                            newextend = map_i.temporal_intersection(relationmap)
                        elif toperator == "|":
                            newextend = map_i.temporal_union(relationmap)
                        elif toperator == "+":
                            newextend = map_i.temporal_disjoint_union(relationmap)
                        elif toperator == "=":
                            resultdict[map_i.get_id()] = map_i

                        if newextend != None:
                            start = newextend.get_start_time()
                            end = newextend.get_end_time()
                            #print(map_i.get_id() + ' - start: ' + str(start) + ' end: ' + str(end))
                            if map_i.is_time_absolute():
                                map_i.set_absolute_time(start, end)
                            else:
                                relunit = map_i.get_relative_time_unit()
                                map_i.set_relative_time(int(start), int(end), relunit)
                            resultdict[map_i.get_id()] = map_i

        resultlist = resultdict.values()
        # Sort list of maps chronological.
        resultlist = sorted(resultlist, key = AbstractDatasetComparisonKeyStartTime)
        # Get relations to maplistB per map in A.
        # Loop over all relations from list
        # temporal extent = map.temporal_intersection(map)
        # if temporal extend is None = delete map.

        return(resultlist)

    def get_temporal_func_dict(self, map):
        """! This function creates a dictionary containing temporal functions for a
             map dataset with time stamp.

          @param map Map object with time stamps.

          @return Dictionary with temporal functions for given input map.

          @code
          >>> import grass.temporal as tgis
          >>> import datetime
          >>> tgis.init()
          >>> l = tgis.TemporalAlgebraParser()
          >>> # Example with one list of maps
          >>> # Create one list of maps with equal time stamps
          >>> for i in range(1):
          ...     idA = "a%i@B"%(i)
          ...     mapA = tgis.RasterDataset(idA)
          ...     check = mapA.set_absolute_time(datetime.datetime(2000,1,1),
          ...             datetime.datetime(2000,10,1))
          ...     tfuncdict = l.get_temporal_func_dict(mapA)
          >>> print(tfuncdict["START_YEAR"])
          2000
          >>> print(tfuncdict["START_TIME"])
          00:00:00
          >>> print(tfuncdict["START_DATE"])
          2000-01-01
          >>> print(tfuncdict["START_DATETIME"])
          2000-01-01 00:00:00

          @endcode

        """
        tvardict = {"START_DOY" : None, "START_DOW" : None, "START_YEAR" : None,
            "START_MONTH" : None, "START_WEEK" : None, "START_DAY" : None,
            "START_HOUR" : None, "START_MINUTE" : None, "START_SECOND" : None,
            "END_DOY" : None, "END_DOW" : None, "END_YEAR" : None,
            "END_MONTH" : None, "END_WEEK" : None, "END_DAY" : None,
            "END_HOUR" : None, "END_MINUTE" : None, "END_SECOND" : None,
            "START_DATE" : None, "START_DATETIME" : None, "START_TIME" : None,
            "END_DATE" : None, "END_DATETIME" : None, "END_TIME" : None}

        # Compute temporal function only for maps with absolute time reference.
        if map.is_time_absolute:
            # Get datetime of map.
            start, end = map.get_absolute_time()
            # Compute DOY via time deltas.
            yearstart = datetime(start.year, 1, 1)
            yearend = datetime(end.year, 1, 1)
            deltastart = start - yearstart
            deltaend = end - yearend

            # Evaluate datetime objects and fill in into dict.
            tvardict["START_DOY"]      = deltastart.days + 1
            tvardict["START_DOW"]      = start.isoweekday()
            tvardict["START_YEAR"]     = start.year
            tvardict["START_MONTH"]    = start.month
            tvardict["START_WEEK"]     = start.isocalendar()[1]
            tvardict["START_DAY"]      = start.day
            tvardict["START_HOUR"]     = start.hour
            tvardict["START_MINUTE"]   = start.minute
            tvardict["START_SECOND"]   = start.second
            tvardict["END_DOY"]        = deltaend.days + 1
            tvardict["END_DOW"]        = end.isoweekday()
            tvardict["END_YEAR"]       = end.year
            tvardict["END_MONTH"]      = end.month
            tvardict["END_WEEK"]       = end.isocalendar()[1]
            tvardict["END_DAY"]        = end.day
            tvardict["END_HOUR"]       = end.hour
            tvardict["END_MINUTE"]     = end.minute
            tvardict["END_SECOND"]     = end.second
            tvardict["START_DATE"]     = start.date()
            tvardict["START_DATETIME"] = start
            tvardict["START_TIME"]     = start.time()
            tvardict["END_DATE"]       = end.date()
            tvardict["END_DATETIME"]   = end
            tvardict["END_TIME"]       = end.time()

        if not map.is_time_absolute:
            tvardict["START_DATE"]     = start.date()
            tvardict["START_DATETIME"] = start
            tvardict["START_TIME"]     = start.time()
            tvardict["END_DATE"]       = end.date()
            tvardict["END_DATETIME"]   = end
            tvardict["END_TIME"]       = end.time()
            #core.fatal(_("The temporal functions for map <%s> only supported for absolute"\
                #          "time." % (str(map.get_id()))))
        return(tvardict)

    def eval_datetime_str(self, tfuncval, comp, value):
        # Evaluate date object comparison expression.
        if comp == "<":
            boolname = eval(str(tfuncval < value))
        elif comp == ">":
            boolname = eval(str(tfuncval > value))
        elif comp == "==":
            boolname = eval(str(tfuncval == value))
        elif comp == "<=":
            boolname = eval(str(tfuncval <= value))
        elif comp == ">=":
            boolname = eval(str(tfuncval >= value))
        elif comp == "!=":
            boolname = eval(str(tfuncval != value))

        return(boolname)

    def eval_global_var(self, gvar, maplist):
        """! This function evaluates a global variable expression for a map list.
             For example: start_day() > 5 , end_month() == 2.

          @param gvar    Object of type GlobalTemporalVar containing temporal.
          @param maplist List of map objects.

          @return List of maps from maplist with added conditional boolean values.
        """
        boollist = []
        # Loop over maps of input map list.
        for map_i in maplist:
            # Get dictionary with temporal variables for the map.
            tfuncdict = self.get_temporal_func_dict(map_i)
            # Get value from global variable.
            value = gvar.value
            # Get comparison operator from global variable, like <, >, <=, >=, ==, !=
            comp_op = gvar.compop
            # Get temporal function name for global variable.
            tfunc = gvar.tfunc.upper()
            # Get value for function name from dictionary.
            tfuncval = tfuncdict[tfunc]
            # Check if value has to be transfered to datetime object for comparison.
            if tfunc in ["START_DATE", "END_DATE"]:
                timeobj = datetime.strptime(value.replace("\"",""), '%Y-%m-%d')
                value = timeobj.date()
                boolname = self.eval_datetime_str(tfuncval, comp_op, value)
            elif tfunc in ["START_TIME", "END_TIME"]:
                timeobj = datetime.strptime(value.replace("\"",""), '%H:%M:%S')
                value = timeobj.time()
                boolname = self.eval_datetime_str(tfuncval, comp_op, value)
            elif tfunc in ["START_DATETIME", "END_DATETIME"]:
                timeobj = datetime.strptime(value.replace("\"",""), '%Y-%m-%d %H:%M:%S')
                value = timeobj
                boolname = self.eval_datetime_str(tfuncval, comp_op, value)
            else:
                boolname = eval(str(tfuncval) + comp_op + str(value))
            # Add conditional boolean value to the map.
            if "condition_value" in dir(map_i):
                map_i.condition_value.append(boolname)
            else:
                map_i.condition_value = boolname

        return(maplist)

    def eval_map_list(self, maplist ,thenlist, topolist = ["EQUAL"]):
        """! This function transfers boolean values from temporal expression
             from one map list to another by their topology. These boolean
             values are added to the maps as condition_value.

          @param maplist  List of map objects containing boolean map values.
          @param thenlist List of map objects where the boolean values
                          should be added.

          @return List of maps from thenlist with added conditional boolean values.
        """
        # Get topology of then statement map list in relation to the other maplist
        # and assign boolean values of the maplist to the thenlist.
        containlist = self.perform_temporal_selection(thenlist, maplist,
                                                        assign_val = True,
                                                        topolist = topolist)
        # Inverse selection of maps from thenlist and assigning False values.
        excludelist = self.perform_temporal_selection(thenlist, maplist,
                                                        assign_val = True,
                                                        inverse = True,
                                                        topolist = topolist)
        # Combining the selection and inverse selection list.
        resultlist = containlist + excludelist

        return(resultlist)

    def build_condition_list(self, tvarexpr, thenlist, topolist = ["EQUAL"]):
        """! This function evaluates temporal variable expressions of a conditional
             expression related to the map list of the then statement.
             Global variables or map lists with booleans are compared to the topology
             of the conclusion map list and a conditional list will be appended to
             every map. It contain the boolean expressions from these comparisons
             and optional operators to combine several temporal expressions, like
             "&&" or "||".

             For example: td(A) == 1 && start_day() > 5 --> [True || False]
                          (for one map.condition_value in a then map list)

          @param tvarexpr List of GlobalTemporalVar objects and map lists.
                          The list is constructed by the TemporalAlgebraParser
                          in order of expression evaluation in the parser.

          @param thenlist Map list object of the conclusion statement.
                          It will be compared and evaluated by the conditions.

          @return Map list with conditional values for all temporal expressions.

          @code
          >>> import grass.temporal as tgis
          >>> tgis.init()
          >>> p = tgis.TemporalAlgebraParser()
          >>> # Example with two lists of maps
          >>> # Create two list of maps with equal time stamps
          >>> mapsA = []
          >>> mapsB = []
          >>> for i in range(10):
          ...     idA = "a%i@B"%(i)
          ...     mapA = tgis.RasterDataset(idA)
          ...     idB = "b%i@B"%(i)
          ...     mapB = tgis.RasterDataset(idB)
          ...     check = mapA.set_absolute_time(datetime(2000,1,i + 1),
          ...             datetime(2000,1,i + 2))
          ...     check = mapB.set_absolute_time(datetime(2000,1,i + 6),
          ...             datetime(2000,1,i + 7))
          ...     mapsA.append(mapA)
          ...     mapsB.append(mapB)
          >>> mapsA = p.check_stds(mapsA)
          >>> mapsB = p.check_stds(mapsB)
          >>> # Create global expression object.
          >>> gvarA = tgis.GlobalTemporalVar()
          >>> gvarA.tfunc = "start_day"
          >>> gvarA.compop = ">"
          >>> gvarA.value = 5
          >>> gvarB = tgis.GlobalTemporalVar()
          >>> gvarB.tfunc = "start_day"
          >>> gvarB.compop = "<="
          >>> gvarB.value = 8
          >>> gvarOP = tgis.GlobalTemporalVar()
          >>> gvarOP.relationop = "&&"
          >>> gvarOP.topology.append("EQUAL")
          >>> tvarexpr = gvarA
          >>> result = p.build_condition_list(tvarexpr, mapsA)
          >>> for map_i in result:
          ...     print(map_i.get_map_id() + ' ' + str(map_i.condition_value))
          a0@B [False]
          a1@B [False]
          a2@B [False]
          a3@B [False]
          a4@B [False]
          a5@B [True]
          a6@B [True]
          a7@B [True]
          a8@B [True]
          a9@B [True]
          >>> tvarexpr = [gvarA, gvarOP, gvarB]
          >>> result = p.build_condition_list(tvarexpr, mapsB)
          >>> for map_i in result:
          ...     print(map_i.get_map_id() + ' ' + str(map_i.condition_value))
          b0@B [True, ['EQUAL'], '&&', True]
          b1@B [True, ['EQUAL'], '&&', True]
          b2@B [True, ['EQUAL'], '&&', True]
          b3@B [True, ['EQUAL'], '&&', False]
          b4@B [True, ['EQUAL'], '&&', False]
          b5@B [True, ['EQUAL'], '&&', False]
          b6@B [True, ['EQUAL'], '&&', False]
          b7@B [True, ['EQUAL'], '&&', False]
          b8@B [True, ['EQUAL'], '&&', False]
          b9@B [True, ['EQUAL'], '&&', False]

          @endcode
        """

        # Check if the input expression is a valid single global variable.
        if not isinstance(tvarexpr, list):
            if isinstance(tvarexpr, GlobalTemporalVar):
                if tvarexpr.get_type() == "global":
                    # Use method eval_global_var to evaluate expression.
                    resultlist = self.eval_global_var(tvarexpr, thenlist)
        else:
            # Check if a given list is a list of maps.
            if all([issubclass(type(ele), AbstractMapDataset) for ele in tvarexpr]):
                # Use method eval_map_list to evaluate map_list in comparison to thenlist.
                resultlist = self.eval_map_list(tvarexpr, thenlist, topolist)

            # Loop through the list, search for map lists or global variables.
            for expr in tvarexpr:
                if isinstance(expr, list):
                    if all([issubclass(type(ele), AbstractMapDataset) for ele in expr]):
                        # Use method eval_map_list to evaluate map_list
                        resultlist = self.eval_map_list(expr, thenlist, topolist)
                    else:
                        # Recursive function call to look into nested list elements.
                        self.build_condition_list(expr, thenlist)

                elif isinstance(expr, GlobalTemporalVar):
                    # Use according functions for different global variable types.
                    if expr.get_type() == "operator":
                        if all(["condition_value" in dir(map_i) for map_i in thenlist]):
                            # Add operator string to the condition list.
                            [map_i.condition_value.extend(expr.get_type_value()) for map_i in thenlist]
                    if expr.get_type() == "global":
                        # Use method eval_global_var to evaluate expression.
                        resultlist = self.eval_global_var(expr, thenlist)

        # Sort resulting list of maps chronological.
        resultlist = sorted(resultlist, key = AbstractDatasetComparisonKeyStartTime)

        return(resultlist)

    def eval_condition_list(self, maplist, inverse = False):
        """! This function evaluates conditional values of a map list.
             A recursive function is used to evaluate comparison statements
             from left to right in the given conditional list.

            For example: [True,  '||', False, '&&', True]  -> True
                          [True,  '||', False, '&&', False] -> False
                          [True,  '&&', False, '&&', True]  -> False
                          [False, '||', True,  '||', False] -> True
                          [False, '&&', True,  '&&', True]  -> False
                          [True,  '&&', True,  '&&', True]  -> True
                          [True,  '&&', True]               -> True
                          [True,  '&&', False]              -> False
                          [False, '||', True]               -> True

          @param tvarexpr List of GlobalTemporalVar objects and map lists.
                          The list is constructed by the TemporalAlgebraParser
                          in order of expression evaluation in the parser.

          @return Map list with conditional values for all temporal expressions.

        @code
            >>> import grass.temporal as tgis
            >>> tgis.init()
            >>> p = tgis.TemporalAlgebraParser()
            >>> # Example with two lists of maps
            >>> # Create two list of maps with equal time stamps
            >>> mapsA = []
            >>> mapsB = []
            >>> for i in range(10):
            ...     idA = "a%i@B"%(i)
            ...     mapA = tgis.RasterDataset(idA)
            ...     idB = "b%i@B"%(i)
            ...     mapB = tgis.RasterDataset(idB)
            ...     check = mapA.set_absolute_time(datetime(2000,1,i + 1),
            ...             datetime(2000,1,i + 2))
            ...     check = mapB.set_absolute_time(datetime(2000,1,i + 6),
            ...             datetime(2000,1,i + 7))
            ...     mapsA.append(mapA)
            ...     mapsB.append(mapB)
            >>> mapsA = p.check_stds(mapsA)
            >>> mapsB = p.check_stds(mapsB)
            >>> # Create global expression object.
            >>> gvarA = tgis.GlobalTemporalVar()
            >>> gvarA.tfunc = "start_day"
            >>> gvarA.compop = ">"
            >>> gvarA.value = 5
            >>> gvarB = tgis.GlobalTemporalVar()
            >>> gvarB.tfunc = "start_day"
            >>> gvarB.compop = "<="
            >>> gvarB.value = 8
            >>> gvarOP = tgis.GlobalTemporalVar()
            >>> gvarOP.relationop = "&&"
            >>> gvarOP.topology.append("EQUAL")
            >>> tvarexpr = [mapsA, gvarOP,gvarA]

          @endcode
        """
        def recurse_compare(conditionlist):
            for ele in conditionlist:
                if ele == '||':
                    ele_index = conditionlist.index(ele)
                    topolist = conditionlist.pop(ele_index -1)
                    right = conditionlist.pop(ele_index)
                    left  = conditionlist.pop(ele_index - 2)
                    if any([left, right]):
                        result = True
                    else:
                        result = False
                    conditionlist[ele_index - 2] = result
                    recurse_compare(conditionlist)
                if ele == '&&':
                    ele_index = conditionlist.index(ele)
                    topolist = conditionlist.pop(ele_index -1)
                    right = conditionlist.pop(ele_index)
                    left  = conditionlist.pop(ele_index - 2)
                    if all([left, right]):
                        result = True
                    else:
                        result = False
                    conditionlist[ele_index - 2] = result
                    recurse_compare(conditionlist)
            resultlist = conditionlist

            return(resultlist)

        resultlist  = []
        inverselist = []
        for map_i in maplist:
            if "condition_value" in dir(map_i):
                # Get condition values from map object.
                conditionlist = map_i.condition_value
                #print(map_i.get_map_id() + ' ' + str(map_i.condition_value))
                # Evaluate conditions in list with recursive function.
                resultbool = recurse_compare(conditionlist)
                # Set conditional value of map to resulting boolean.
                map_i.condition_value = resultbool
                # Add all maps that fulfill the conditions to result list.
                if resultbool[0]:
                    resultlist.append(map_i)
                else:
                    inverselist.append(map_i)
                #print(map_i.get_map_id() + ' ' + str(map_i.condition_value))
        if inverse:
            return(inverselist)
        else:
            return(resultlist)

    ###########################################################################

    def p_statement_assign(self, t):
        # The expression should always return a list of maps.
        """
        statement : stds EQUALS expr

        """
        if self.run:
            resultstds = open_new_space_time_dataset(t[1], self.stdstype, \
                                                        self.temporaltype, "", "", \
                                                        'mean', dbif=self.dbif, \
                                                        overwrite = self.overwrite)
            if isinstance(t[3], list):
                num = len(t[3])
                count = 0
                if num > 0:
                    dbif, connected = init_dbif(None)
                    for map in t[3]:
                        map.select(dbif=dbif)
                        #map.update()
                        resultstds.register_map(map, dbif=dbif)
                        count += 1
                        if count % 10 == 0:
                            self.msgr.percent(count, num, 1)

                    resultstds.update_from_registered_maps(dbif=dbif)
                    if connected:
                        dbif.close()
            t[0] = t[3]

        else:
            t[0] = t[3]

        if self.debug:
            print t[1], "=", t[3]

    def p_stds_1(self, t):
        # Definition of a space time dataset
        """
        stds : NAME
        """
        t[0] = t[1]

    def p_paren_expr(self, t):
        """ expr : LPAREN expr RPAREN"""
        t[0] = t[2]

    def p_number(self,t):
        """number : INT
                  | FLOAT
        """
        t[0] = t[1]

    def p_t_hash(self,t):
        """
        t_hash_var : stds HASH stds
                   | stds HASH expr
                   | expr HASH stds
                   | expr HASH expr
        """

        if self.run:
            maplistA   = self.check_stds(t[1])
            maplistB   = self.check_stds(t[3])
            resultlist = self.get_temporal_topo_list(maplistA, maplistB,
                                                        count_map = True)

            t[0] = resultlist

    def p_t_hash2(self,t):
        """
        t_hash_var : stds T_HASH_OPERATOR stds
                   | stds T_HASH_OPERATOR expr
                   | expr T_HASH_OPERATOR stds
                   | expr T_HASH_OPERATOR expr
        """

        if self.run:
            maplistA   = self.check_stds(t[1])
            maplistB   = self.check_stds(t[3])
            topolist   = self.eval_toperator(t[2])[0]
            resultlist = self.get_temporal_topo_list(maplistA, maplistB, topolist,
                                                     count_map = True)

            t[0] = resultlist

    def p_t_td_var(self, t):
        """
        t_td_var : TD LPAREN stds RPAREN
                 | TD LPAREN expr RPAREN
        """
        if self.run:
            maplist = self.check_stds(t[3])
            for map_i in maplist:
                if map_i.is_time_absolute:
                    start, end = map_i.get_absolute_time()
                    if end != None:
                        td = time_delta_to_relative_time(end - start)
                else:
                    start, end, unit = current.get_relative_time()
                    if end != None:
                        td = end - start
                if "map_value" in dir(map_i):
                    gvar = GlobalTemporalVar()
                    gvar.td = td
                    map_i.map_value.append(gvar)
                else:
                    map_i.map_value = gvar

            t[0] = maplist
        else:
            t[0] = "td(" + str(t[3]) + ")"

        if self.debug:
            print "td(" + str(t[3]) + ")"


    def p_t_time_var(self, t):
        # Temporal variables that return a double or integer value
        """
        t_var : START_DOY
              | START_DOW
              | START_YEAR
              | START_MONTH
              | START_WEEK
              | START_DAY
              | START_HOUR
              | START_MINUTE
              | START_SECOND
              | END_DOY
              | END_DOW
              | END_YEAR
              | END_MONTH
              | END_WEEK
              | END_DAY
              | END_HOUR
              | END_MINUTE
              | END_SECOND
        """

        t[0] = t[1]

    def p_compare_op(self, t):
        # Compare operators that are supported for temporal expressions
        """
        comp_op : CEQUALS
                | UNEQUALS
                | LOWER
                | LOWER_EQUALS
                | GREATER
                | GREATER_EQUALS
        """
        t[0] = t[1]

    def p_t_var_expr(self, t):
        # Examples:
        #    start_month() > 2
        #    start_day() < 14
        #    start_day() < start_month()
        #    td() < 31
        """
        t_var_expr : t_var LPAREN RPAREN comp_op number
                   | t_var LPAREN RPAREN comp_op t_var
                   | t_td_var comp_op number
                   | t_td_var comp_op t_var
                   | t_hash_var comp_op number
                   | t_hash_var comp_op t_var
        """
        if self.run:
            if len(t) == 4:
                maplist = self.check_stds(t[1])
                comp_op = t[2]
                for map_i in maplist:
                    for obj in map_i.map_value:
                        if isinstance(obj, GlobalTemporalVar):
                            td = obj.td
                            boolnum = eval(str(td) + comp_op + str(t[3]))
                            gvar = GlobalTemporalVar()
                            gvar.boolean = boolnum
                            if obj.get_type() == "timediff":
                                index = map_i.map_value.index(obj)
                                map_i.map_value[index] = gvar

                t[0] = maplist

            if len(t) == 6:
                if isinstance(t[1], GlobalTemporalVar):
                    pass
                gvar = GlobalTemporalVar()
                gvar.tfunc  = t[1]
                gvar.compop = t[4]
                gvar.value  = t[5]
                t[0] = gvar
        else:
            t[0] = True

        if self.debug:
            if len(t) == 6:
                print t[1], t[4], t[5]
            if len(t) == 4:
                print t[1], t[2], t[3]

    def p_t_var_expr_time1(self, t):
        # Examples:
        #   start_time() == "12:30:00"
        #   start_date() <= "2001-01-01"
        #   start_datetime() > "2001-01-01 12:30:00"
        """
        t_var_expr : START_TIME LPAREN RPAREN comp_op TIME
                   | START_DATE LPAREN RPAREN comp_op DATE
                   | START_DATETIME LPAREN RPAREN comp_op DATETIME
                   | END_TIME LPAREN RPAREN comp_op TIME
                   | END_DATE LPAREN RPAREN comp_op DATE
                   | END_DATETIME LPAREN RPAREN comp_op DATETIME
        """
        if self.run:
            gvar = GlobalTemporalVar()
            gvar.tfunc  = t[1]
            gvar.compop = t[4]
            gvar.value  = t[5]
            t[0] = gvar
        else:
            t[0] = True

        if self.debug:
            print t[1], t[4], t[5]

    def p_t_var_expr_time2(self, t):
        """
        t_var_expr : TIME     comp_op START_TIME LPAREN RPAREN
                   | DATE     comp_op START_DATE LPAREN RPAREN
                   | DATETIME comp_op START_DATETIME LPAREN RPAREN
                   | TIME     comp_op END_TIME LPAREN RPAREN
                   | DATE     comp_op END_DATE LPAREN RPAREN
                   | DATETIME comp_op END_DATETIME LPAREN RPAREN
        """
        if self.run:
            reverseop = {"<" : ">", ">" : "<", "<=" : ">=", ">=" : "<=",
                          "==" : "==", "!=" : "!="}
            gvar = GlobalTemporalVar()
            gvar.tfunc  = t[3]
            gvar.compop = reverseop[t[2]]
            gvar.value  = t[1]
            t[0] = gvar
        else:
            t[0] = True

        if self.debug:
            print(t[4])
            print t[1], t[4], t[5]

    def p_t_var_expr_comp(self, t):
        """
        t_var_expr : t_var_expr AND AND t_var_expr
                   | t_var_expr OR OR t_var_expr
        """
        if self.run:
            tvarexprA  = t[1]
            tvarexprB  = t[4]
            operator   = GlobalTemporalVar()
            operator.relationop = t[2] + t[3]
            operator.topology.append("EQUAL")
            resultlist = []
            resultlist.append(tvarexprA)
            resultlist.append(operator)
            resultlist.append(tvarexprB)

            t[0] = resultlist
        else:
            t[0] = True

        if self.debug:
            print t[1], t[2] + t[3], t[4]

    def p_t_var_expr_comp_op(self, t):
        """
        t_var_expr : t_var_expr T_COMP_OPERATOR t_var_expr
        """
        if self.run:
            tvarexprA  = t[1]
            tvarexprB  = t[3]
            operator   = GlobalTemporalVar()
            toperator  = self.eval_toperator(t[2])[0]
            relationop = toperator[2]
            relations  = toperator[0]
            operator.relationop = relationop
            operator.topology.extend(relations)
            resultlist = []
            resultlist.append(tvarexprA)
            resultlist.append(operator)
            resultlist.append(tvarexprB)

            t[0] = resultlist
        else:
            t[0] = True

        if self.debug:
            print t[1], t[2], t[3]

    def p_expr_t_select(self, t):
        # Temporal equal selection
        # The temporal topology relation equals is implicit
        # Examples:
        #    A : B  # Select the part of A that is temporally equal B
        """
        expr : stds T_SELECT stds
             | expr T_SELECT stds
             | stds T_SELECT expr
             | expr T_SELECT expr
        """
        if self.run:
            # Setup database connection.

            # Check input stds.
            maplistA     = self.check_stds(t[1])
            maplistB     = self.check_stds(t[3])
            # Perform selection.
            selectlist = self.perform_temporal_selection(maplistA, maplistB)
            # Return map list.
            t[0] = selectlist
        else:
            t[0] = t[1] + "*"

        if self.debug:
            print t[1] + "* = ", t[1], t[2], t[3]

    def p_expr_t_not_select(self, t):
        # Temporal equal selection
        # The temporal topology relation equals is implicit
        # Examples:
        #    A !: B  # Select the part of A that is temporally unequal to B
        """
        expr : stds T_NOT_SELECT stds
             | expr T_NOT_SELECT stds
             | stds T_NOT_SELECT expr
             | expr T_NOT_SELECT expr
        """
        if self.run:
            # Check input stds.
            maplistA     = self.check_stds(t[1])
            maplistB     = self.check_stds(t[3])
            # Perform negative selection.
            selectlist = self.perform_temporal_selection(maplistA, maplistB,
                                                         inverse = True)
            # Return map list.
            t[0] = selectlist
        else:
            t[0] = t[1] + "*"

        if self.debug:
            print t[1] + "* = ", t[1], t[2], t[3]


    def p_expr_t_select_operator(self, t):
        # Temporal equal selection
        # The temporal topology relation equals is implicit
        # Examples:
        #    A {!:} B  # Select the part of A that is temporally unequal to B
        #    A  {:} B  # Select the part of A that is temporally equal B
        #    A {equals, !:} B    # Select the part of A that is temporally unequal to B
        #    A {during, !:} B    # Select the part of A that is temporally not during B
        #    A {overlaps, :} B   # Select the part of A that temporally overlaps B
        #    A {overlaps|equals, :} B  # Select the part of A that temporally overlaps or equals B
        """
        expr : stds T_SELECT_OPERATOR stds
             | expr T_SELECT_OPERATOR stds
             | stds T_SELECT_OPERATOR expr
             | expr T_SELECT_OPERATOR expr
        """
        if self.run:
            # Check input stds.
            maplistA     = self.check_stds(t[1])
            maplistB     = self.check_stds(t[3])
            # Evaluate temporal operator.
            operators  = self.eval_toperator(t[2])
            # Check for negative selection.
            if operators[2] == "!:":
                negation = True
            else:
                negation = False
            # Perform selection.
            selectlist = self.perform_temporal_selection(maplistA, maplistB,
                         topolist = operators[0], inverse = negation)
            selectlist = self.set_granularity(selectlist, maplistB, operators[1],
                                              operators[0])

            # Return map list.
            t[0] = selectlist
        else:
            t[0] = t[1] + "*"

        if self.debug:
            print t[1] + "* = ", t[1], t[2], t[3]


    def p_expr_condition_if(self, t):
        # Examples
        # if( start_date() < "2005-06-01", A:B)
        """
        expr : IF LPAREN t_var_expr COMMA stds RPAREN
             | IF LPAREN t_var_expr COMMA expr RPAREN
        """
        if self.run:
            # Get stds/map list of then statement.
            thenlist     = self.check_stds(t[5])
            # Get temporal conditional statement.
            tvarexpr     = t[3]
            thencond     = self.build_condition_list(tvarexpr, thenlist)
            thenresult   = self.eval_condition_list(thencond)
            # Clear the map and conditional values of the map list.
            resultlist   = self.check_stds(thenresult, clear = True)
            # Return resulting map list.
            t[0] = resultlist
        else:
            t[0] = t[5] + "*"

        if self.debug:
            print str(t[5]) + "* = ", "if condition", str(t[3]), ' then ', str(t[5])

    def p_expr_condition_if_relation(self, t):
        # Examples
        # if({equal} start_date() < "2005-06-01", A:B)
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA t_var_expr COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA t_var_expr COMMA expr RPAREN
        """
        if self.run:
            # Get stds/map list of then statement.
            thenlist     = self.check_stds(t[7])
            # Get temporal conditional statement.
            tvarexpr     = t[5]
            topolist     = self.eval_toperator(t[3])[0]
            thencond     = self.build_condition_list(tvarexpr, thenlist, topolist)
            thenresult   = self.eval_condition_list(thencond)
            # Clear the map and conditional values of the map list.
            resultlist   = self.check_stds(thenresult, clear = True)
            # Return resulting map list.
            t[0] = resultlist
        else:
            t[0] = t[7] + "*"

        if self.debug:
            print "result* = ", "if ", str(t[3]),  "condition", str(t[5]), " then ", str(t[7])

    def p_expr_condition_elif(self, t):
        # Examples
        # if( start_date() < "2005-06-01", if(start_time() < "12:30:00", A:B), A!:B)
        """
        expr : IF LPAREN t_var_expr COMMA stds COMMA stds RPAREN
             | IF LPAREN t_var_expr COMMA stds COMMA expr RPAREN
             | IF LPAREN t_var_expr COMMA expr COMMA stds RPAREN
             | IF LPAREN t_var_expr COMMA expr COMMA expr RPAREN
        """
        if self.run:
            # Get stds/map list of then statement.
            thenlist     = self.check_stds(t[5])
            elselist     = self.check_stds(t[7])
            # Get temporal conditional statement for then and else expressions.
            tvarexpr     = t[3]
            thencond     = self.build_condition_list(tvarexpr, thenlist)
            thenresult   = self.eval_condition_list(thencond)
            elsecond     = self.build_condition_list(tvarexpr, elselist)
            elseresult   = self.eval_condition_list(elsecond, inverse = True)

            # Combine and sort else and then statement to result map list.
            combilist = thenresult + elseresult
            resultlist = sorted(combilist, key = AbstractDatasetComparisonKeyStartTime)
            # Clear the map and conditional values of the map list.
            resultlist   = self.check_stds(resultlist, clear = True)
            # Return resulting map list.
            t[0] = resultlist
        else:
            t[0] = t[5] + "*"

        if self.debug:
            print str(t[5]) + "* = ", "if condition", str(t[3]), " then ", str(t[5]), ' else ', str(t[7])

    def p_expr_condition_elif_relation(self, t):
        # Examples
        # if({equal}, start_date() < "2005-06-01", if(start_time() < "12:30:00", A:B), A!:B)
        # The then and else statement using the same topological relationships.
        # Feature request: Independent relationships for then and else to conditions.
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA t_var_expr COMMA stds COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA t_var_expr COMMA stds COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA t_var_expr COMMA expr COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA t_var_expr COMMA expr COMMA expr RPAREN
        """
        if self.run:
            # Get stds/map list of then statement.
            thenlist     = self.check_stds(t[7])
            elselist     = self.check_stds(t[9])
            # Get temporal conditional statement.
            tvarexpr     = t[5]
            topolist     = self.eval_toperator(t[3])[0]
            thencond     = self.build_condition_list(tvarexpr, thenlist, topolist)
            thenresult   = self.eval_condition_list(thencond)
            elsecond     = self.build_condition_list(tvarexpr, elselist, topolist)
            elseresult   = self.eval_condition_list(elsecond, inverse = True)

            # Combine and sort else and then statement to result map list.
            combilist = thenresult + elseresult
            resultlist = sorted(combilist, key = AbstractDatasetComparisonKeyStartTime)
            # Clear the map and conditional values of the map list.
            resultlist   = self.check_stds(resultlist, clear = True)
            # Return resulting map list.
            t[0] = resultlist
        else:
            if t[5]:
                t[0] = str(t[7])
            else:
                t[0] = str(t[9])

        if self.debug:
            if t[5]:
                print str(t[7]), "* = ", "if condition", str(t[5]), " then ", str(t[7]), ' else ', str(t[9])
            else:
                print str(t[9]), "* = ", "if condition", str(t[5]), " then ", str(t[7]), ' else ', str(t[9])

    def p_expr_t_buff(self, t):
        # Examples
        # buff_t(A : B, "10 minutes")  # Select the part of A that is temporally
        #                                equal to B and create a buffer of 10 minutes around
        """
        expr : BUFF_T LPAREN stds COMMA QUOTE number NAME QUOTE RPAREN
             | BUFF_T LPAREN expr COMMA QUOTE number NAME QUOTE RPAREN
             | BUFF_T LPAREN stds COMMA number RPAREN
             | BUFF_T LPAREN expr COMMA number RPAREN
        """
        if self.run:
            # Check input stds.
            bufflist     = self.check_stds(t[3])
            for map in bufflist:
                # Get increment format.
                if len(t) == 10:
                    increment = str(t[6]) + " " + t[7]
                elif len(t) == 7:
                    increment = str(t[5])
                # Perform buffering.
                map.temporal_buffer(increment)
            t[0] = bufflist
        else:
            t[0] = t[3] + "*"

        if self.debug:
            if len(t) == 10:
                print str(t[3]) + "* = buff_t(", str(t[3]), "," , '"', str(t[6]), str(t[7]), '"', ")"
            elif len(t) == 7:
                print str(t[3]) + "* = buff_t(", str(t[3]), ",", str(t[5]), ")"

    def p_expr_t_snap(self, t):
        # Examples
        # tsnap(A : B)  # Snap the maps of A temporally.
        """
        expr : TSNAP LPAREN stds RPAREN
             | TSNAP LPAREN expr RPAREN
        """
        if self.run:
            # Check input stds.
            maplist     = self.check_stds(t[3])
            # Perform snapping.
            snaplist = AbstractSpaceTimeDataset.snap_map_list(maplist)
            t[0] = snaplist
        else:
            t[0] = t[3] + "*"

        if self.debug:
            print str(t[3]) + "* = tsnap(", str(t[3]), ")"

    def p_expr_t_shift(self, t):
        # Examples
        # tshift(A : B, "10 minutes")  # Shift the selection from A temporally
        #                                by 10 minutes.
        """
        expr : TSHIFT LPAREN stds COMMA QUOTE number NAME QUOTE RPAREN
             | TSHIFT LPAREN expr COMMA QUOTE number NAME QUOTE RPAREN
             | TSHIFT LPAREN stds COMMA number RPAREN
             | TSHIFT LPAREN expr COMMA number RPAREN
        """
        if self.run:
            # Check input stds.
            maplist     = self.check_stds(t[3])
            # Get increment format.
            if len(t) == 10:
                increment = str(t[6]) + " " + t[7]
            elif len(t) == 7:
                increment = str(t[5])
            # Perform shifting.
            shiftlist = AbstractSpaceTimeDataset.shift_map_list(maplist, increment)
            t[0] = shiftlist
        else:
            t[0] = t[3] + "*"

        if self.debug:
            if len(t) == 10:
                print str(t[3]) + "* = tshift(", str(t[3]), "," , '"', str(t[6]), str(t[7]), '"', ")"
            elif len(t) == 7:
                print str(t[3]) + "* = tshift(", str(t[3]), ",", str(t[5]), ")"

    # Handle errors.
    def p_error(self, t):
        if t:
            raise SyntaxError("syntax error on line %d, token %s near '%s' expression '%s'" %
                             (t.lineno, t.type, t.value, self.expression))
        else:
            raise SyntaxError("Unexpected syntax error")

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
