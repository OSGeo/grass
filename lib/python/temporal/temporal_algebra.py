"""@package grass.temporal

Temporal algebra parser class

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Thomas Leppelt and Soeren Gebbert

.. code-block:: python

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
    >>> expression =  "C = test1 {:,equal} test2"
    >>> p.test(expression)
    C = test1 {:,equal} test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_SELECT_OPERATOR,'{:,equal}',1,10)
    LexToken(NAME,'test2',1,20)
    >>> expression =  "C = test1 {!:,equal} test2"
    >>> p.test(expression)
    C = test1 {!:,equal} test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_SELECT_OPERATOR,'{!:,equal}',1,10)
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
    >>> expression =  "C = test1 {#,equal} test2"
    >>> p.test(expression)
    C = test1 {#,equal} test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_HASH_OPERATOR,'{#,equal}',1,10)
    LexToken(NAME,'test2',1,20)
    >>> expression =  "C = test1 {#,equal|during} test2"
    >>> p.test(expression)
    C = test1 {#,equal|during} test2
    LexToken(NAME,'C',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'test1',1,4)
    LexToken(T_HASH_OPERATOR,'{#,equal|during}',1,10)
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
    >>> expression =  'H = tsnap(test2 {:,during} buff_t(test1, "1 days"))'
    >>> p.test(expression)
    H = tsnap(test2 {:,during} buff_t(test1, "1 days"))
    LexToken(NAME,'H',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(TSNAP,'tsnap',1,4)
    LexToken(LPAREN,'(',1,9)
    LexToken(NAME,'test2',1,10)
    LexToken(T_SELECT_OPERATOR,'{:,during}',1,16)
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
    >>> expression =  'H = tshift(test2 {:,during} buff_t(test1, "1 days"), "1 months")'
    >>> p.test(expression)
    H = tshift(test2 {:,during} buff_t(test1, "1 days"), "1 months")
    LexToken(NAME,'H',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(TSHIFT,'tshift',1,4)
    LexToken(LPAREN,'(',1,10)
    LexToken(NAME,'test2',1,11)
    LexToken(T_SELECT_OPERATOR,'{:,during}',1,17)
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
    >>> expression =  'I = if(equals,td(A) > 10 {||,equals} td(B) < 10, A)'
    >>> p.test(expression)
    I = if(equals,td(A) > 10 {||,equals} td(B) < 10, A)
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
    LexToken(T_COMP_OPERATOR,'{||,equals}',1,25)
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
    >>> expression =  'E = if({equals},td(A) >= 4 {&&,contain} td(B) == 2, C : D)'
    >>> p.test(expression)
    E = if({equals},td(A) >= 4 {&&,contain} td(B) == 2, C : D)
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
    LexToken(T_COMP_OPERATOR,'{&&,contain}',1,27)
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
    >>> expression =  'F = if({equals},A {#,equal}, B, C : D)'
    >>> p.test(expression)
    F = if({equals},A {#,equal}, B, C : D)
    LexToken(NAME,'F',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(IF,'if',1,4)
    LexToken(LPAREN,'(',1,6)
    LexToken(T_REL_OPERATOR,'{equals}',1,7)
    LexToken(COMMA,',',1,15)
    LexToken(NAME,'A',1,16)
    LexToken(T_HASH_OPERATOR,'{#,equal}',1,18)
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
    >>> expression =  "D = A {!:} B {:,during} C"
    >>> print(expression)
    D = A {!:} B {:,during} C
    >>> ret = p.parse(expression)
    A* =  A {!:} B
    A** =  A* {:,during} C
    D = A**
    >>> expression =  "D = A {:} B {!:,during} C"
    >>> print(expression)
    D = A {:} B {!:,during} C
    >>> ret = p.parse(expression)
    A* =  A {:} B
    A** =  A* {!:,during} C
    D = A**
    >>> p.run = False
    >>> p.debug = False
    >>> expression =  "C = test1 : test2"
    >>> print(expression)
    C = test1 : test2
    >>> ret = p.parse(expression, 'stvds')
    >>> expression =  'D = buff_t(test1,"10 months")'
    >>> print(expression)
    D = buff_t(test1,"10 months")
    >>> ret = p.parse(expression, 'stvds')
    >>> expression =  'E = test2 {:,during} buff_t(test1,"1 days")'
    >>> print(expression)
    E = test2 {:,during} buff_t(test1,"1 days")
    >>> ret = p.parse(expression, 'stvds')
    >>> expression =  'F = test2 {:,equal} buff_t(test1,"1 days")'
    >>> print(expression)
    F = test2 {:,equal} buff_t(test1,"1 days")
    >>> ret = p.parse(expression, 'stvds')
    >>> p.debug = True
    >>> expression =  'H = tsnap(test2 {:,during} buff_t(test1, "1 days"))'
    >>> ret = p.parse(expression, 'stvds')
    test1* = buff_t( test1 , " 1 days " )
    test2* =  test2 {:,during} test1*
    test2** = tsnap( test2* )
    H = test2**
    >>> expression =  'H = tshift(test2 {:,during} test1, "1 days")'
    >>> ret = p.parse(expression, 'stvds')
    test2* =  test2 {:,during} test1
    test2** = tshift( test2* , " 1 days " )
    H = test2**
    >>> expression =  'H = tshift(H, 3)'
    >>> ret = p.parse(expression, 'stvds')
    H* = tshift( H , 3 )
    H = H*
    >>> expression =  'C = if(td(A) == 2, A)'
    >>> ret = p.parse(expression, 'stvds')
    td(A)
    td(A) == 2
    A* =  if condition None  then  A
    C = A*
    >>> expression =  'C = if(td(A) == 5, A, B)'
    >>> ret = p.parse(expression, 'stvds')
    td(A)
    td(A) == 5
    A* =  if condition None  then  A  else  B
    C = A*
    >>> expression =  'C = if(td(A) == 5 || start_date(A) > "2010-01-01", A, B)'
    >>> ret = p.parse(expression, 'stvds')
    td(A)
    td(A) == 5
    start_date A > "2010-01-01"
    None || None
    A* =  if condition None  then  A  else  B
    C = A*

    >>> p = tgis.TemporalAlgebraLexer()
    >>> p.build()
    >>> p.debug = True
    >>> expression =  "D = strds(A) : stvds(B) : str3ds(C)"
    >>> p.test(expression)
    D = strds(A) : stvds(B) : str3ds(C)
    LexToken(NAME,'D',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(STRDS,'strds',1,4)
    LexToken(LPAREN,'(',1,9)
    LexToken(NAME,'A',1,10)
    LexToken(RPAREN,')',1,11)
    LexToken(T_SELECT,':',1,13)
    LexToken(STVDS,'stvds',1,15)
    LexToken(LPAREN,'(',1,20)
    LexToken(NAME,'B',1,21)
    LexToken(RPAREN,')',1,22)
    LexToken(T_SELECT,':',1,24)
    LexToken(STR3DS,'str3ds',1,26)
    LexToken(LPAREN,'(',1,32)
    LexToken(NAME,'C',1,33)
    LexToken(RPAREN,')',1,34)

    >>> p = tgis.TemporalAlgebraLexer()
    >>> p.build()
    >>> p.debug = True
    >>> expression =  "R = if(A {#,during} stvds(C) == 1, A)"
    >>> p.test(expression)
    R = if(A {#,during} stvds(C) == 1, A)
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(IF,'if',1,4)
    LexToken(LPAREN,'(',1,6)
    LexToken(NAME,'A',1,7)
    LexToken(T_HASH_OPERATOR,'{#,during}',1,9)
    LexToken(STVDS,'stvds',1,20)
    LexToken(LPAREN,'(',1,25)
    LexToken(NAME,'C',1,26)
    LexToken(RPAREN,')',1,27)
    LexToken(CEQUALS,'==',1,29)
    LexToken(INT,1,1,32)
    LexToken(COMMA,',',1,33)
    LexToken(NAME,'A',1,35)
    LexToken(RPAREN,')',1,36)

    >>> p = tgis.TemporalAlgebraLexer()
    >>> p.build()
    >>> p.debug = True
    >>> expression =  "R = if({during}, stvds(C) {#,contains} A == 2, A)"
    >>> p.test(expression)
    R = if({during}, stvds(C) {#,contains} A == 2, A)
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(IF,'if',1,4)
    LexToken(LPAREN,'(',1,6)
    LexToken(T_REL_OPERATOR,'{during}',1,7)
    LexToken(COMMA,',',1,15)
    LexToken(STVDS,'stvds',1,17)
    LexToken(LPAREN,'(',1,22)
    LexToken(NAME,'C',1,23)
    LexToken(RPAREN,')',1,24)
    LexToken(T_HASH_OPERATOR,'{#,contains}',1,26)
    LexToken(NAME,'A',1,39)
    LexToken(CEQUALS,'==',1,41)
    LexToken(INT,2,1,44)
    LexToken(COMMA,',',1,45)
    LexToken(NAME,'A',1,47)
    LexToken(RPAREN,')',1,48)

"""
from __future__ import print_function

try:
    import ply.lex as lex
    import ply.yacc as yacc
except:
    pass

import os
import sys
import copy
from datetime import datetime
import grass.pygrass.modules as pymod
from .core import init_dbif, get_tgis_message_interface, get_current_mapset,\
    SQLDatabaseInterfaceConnection
from .temporal_granularity import compute_common_absolute_time_granularity, \
    compute_common_relative_time_granularity
from .abstract_dataset import AbstractDatasetComparisonKeyStartTime
from .abstract_map_dataset import AbstractMapDataset
from .space_time_datasets import RasterDataset
from .factory import dataset_factory
from .open_stds import open_new_stds, open_old_stds
from .temporal_operator import TemporalOperatorParser
from .spatio_temporal_relationships import SpatioTemporalTopologyBuilder
from .datetime_math import time_delta_to_relative_time, string_to_datetime
from .abstract_space_time_dataset import AbstractSpaceTimeDataset
from .temporal_granularity import compute_absolute_time_granularity

from .datetime_math import create_suffix_from_datetime
from .datetime_math import create_time_suffix
from .datetime_math import create_numeric_suffix

if sys.version_info[0] >= 3:
    unicode = str

##############################################################################

class TemporalAlgebraLexer(object):
    """Lexical analyzer for the GRASS GIS temporal algebra"""

    # Functions that defines an if condition, temporal buffering, snapping and
    # selection of maps with temporal extent.
    conditional_functions = {
        'if': 'IF',
        'buff_t': 'BUFF_T',
        'tsnap': 'TSNAP',
        'tshift': 'TSHIFT',
        'tmap': 'TMAP',
        'merge': 'MERGE',
        'strds': 'STRDS',
        'str3ds': 'STR3DS',
        'stvds': 'STVDS',
    }

    # Variables with date and time strings
    datetime_functions = {
        'start_time': 'START_TIME',     # start time as HH::MM:SS
        'start_date': 'START_DATE',     # start date as yyyy-mm-DD
        'start_datetime': 'START_DATETIME', # start datetime as yyyy-mm-DD HH:MM:SS
        'end_time': 'END_TIME',       # end time as HH:MM:SS
        'end_date': 'END_DATE',       # end date as yyyy-mm-DD
        'end_datetime': 'END_DATETIME',   # end datetime as  yyyy-mm-DD HH:MM:SS
    }

    # Time functions
    time_functions = {
        'td': 'TD',            # The size of the current
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
        'start_doy': 'START_DOY',     # Day of year (doy) from the start time [1 - 366]
        'start_dow': 'START_DOW',     # Day of week (dow) from the start time [1 - 7], the start of the week is Monday == 1
        'start_year': 'START_YEAR',    # The year of the start time [0 - 9999]
        'start_month': 'START_MONTH',   # The month of the start time [1 - 12]
        'start_week': 'START_WEEK',    # Week of year of the start time [1 - 54]
        'start_day': 'START_DAY',     # Day of month from the start time [1 - 31]
        'start_hour': 'START_HOUR',    # The hour of the start time [0 - 23]
        'start_minute': 'START_MINUTE',  # The minute of the start time [0 - 59]
        'start_second': 'START_SECOND',  # The second of the start time [0 - 59]
        'end_doy': 'END_DOY',       # Day of year (doy) from the end time [1 - 366]
        'end_dow': 'END_DOW',       # Day of week (dow) from the end time [1 - 7], the start of the week is Monday == 1
        'end_year': 'END_YEAR',      # The year of the end time [0 - 9999]
        'end_month': 'END_MONTH',     # The month of the end time [1 - 12]
        'end_week': 'END_WEEK',      # Week of year of the end time [1 - 54]
        'end_day': 'END_DAY',       # Day of month from the start time [1 - 31]
        'end_hour': 'END_HOUR',      # The hour of the end time [0 - 23]
        'end_minute': 'END_MINUTE',    # The minute of the end time [0 - 59]
        'end_second': 'END_SECOND',    # The second of the end time [0 - 59]
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
    t_T_SELECT_OPERATOR   = r'\{[!]?[:][,]?[a-zA-Z\| ]*([,])?([lrudi]|left|right|union|disjoint|intersect)?\}'
    t_T_HASH_OPERATOR   = r'\{[#][,]?[a-zA-Z\| ]*([,])?([lrudi]|left|right|union|disjoint|intersect)?\}'
    t_T_COMP_OPERATOR   = r'\{(\|\||&&)[,][a-zA-Z\| ]*[,]?[\|&]?([,])?([lrudi]|left|right|union|disjoint|intersect)?\}'
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
    t_ignore = ' \t\n'

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
        r'[a-zA-Z_][a-zA-Z_0-9\@]*'
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
        raise SyntaxError("syntax error on line %d position %i near '%s'" %
                          (t.lineno, t.lexpos, t.value))

    # Build the lexer
    def build(self,**kwargs):
        self.lexer = lex.lex(module=self, optimize=False,
                             nowarn=True, debug=0, **kwargs)

    # Just for testing
    def test(self,data):
        self.name_list = {}
        print(data)
        self.lexer.input(data)
        while True:
            tok = self.lexer.token()
            if not tok:
                break
            print(tok)

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
        if self.tfunc is not None and self.compop is not None and self.value is not None:
            return("global")
        elif self.boolean is not None:
            return("boolean")
        elif self.relationop is not None and self.topology != []:
            return("operator")
        elif self.td is not None:
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
        ('left', 'T_SELECT_OPERATOR', 'T_SELECT', 'T_NOT_SELECT', 'T_HASH_OPERATOR', 'HASH'), # 1
        ('left', 'AND', 'OR', 'T_COMP_OPERATOR'),  # 2
    )

    def __init__(self, pid=None, run=True, debug=False, spatial=False,
                 register_null=False, dry_run=False, nprocs=1, time_suffix=None):
        self.run = run
        self.dry_run = dry_run              # Compute the processes and output but Do not start the processes
        self.process_chain_dict = {}        # This dictionary stores all processes, as well as the maps to register and remove
        self.process_chain_dict["processes"] = []       # The mapcalc and v.patch module calls
        self.process_chain_dict["register"] = []        # Maps that must be registered/updated or inserted in a new STDS
        self.process_chain_dict["remove"] = []          # The g.remove module calls
        self.process_chain_dict["STDS"] = {}            # The STDS that must be created

        self.debug = debug
        self.pid = pid
        # Intermediate vector map names
        self.names = {}
        # Count map names
        self.spatial = spatial
        self.mapset = get_current_mapset()
        self.temporaltype = None
        self.msgr = get_tgis_message_interface()
        self.dbif = SQLDatabaseInterfaceConnection()
        self.dbif.connect()
        self.register_null = register_null
        self.removable_maps = {}
        self.m_mremove = pymod.Module('g.remove')
        self.m_copy = pymod.Module('g.copy')
        self.nprocs = nprocs
        self.use_granularity = False
        self.time_suffix = time_suffix

        # Topology lists
        self.temporal_topology_list = ["EQUAL", "FOLLOWS", "PRECEDES", "OVERLAPS", "OVERLAPPED",
                                       "DURING", "STARTS", "FINISHES", "CONTAINS", "STARTED", "FINISHED"]
        self.spatial_topology_list = ["EQUIVALENT", "COVER", "OVERLAP", "IN", "CONTAIN", "MEET"]

    def __del__(self):
        if self.dbif.connected:
            self.dbif.close()

    def setup_common_granularity(self, expression, stdstype = 'strds', lexer = None):
        """Configure the temporal algebra to use the common granularity of all
             space time datasets in the expression to generate the map lists.

             This function will analyze the expression to detect space time datasets
             and computes the common granularity from all granularities of the input space time datasets.

             This granularity is then be used to generate the map lists. Hence, all
             maps from all STDS will have equidistant temporal extents. The only meaningful
             temporal relation is therefore "equal".

             :param expression: The algebra expression to analyze

             :param lexer: The temporal algebra lexer (select, raster, voxel, vector) that should be used to
                                    parse the expression, default is TemporalAlgebraLexer

             :return: True if successful, False otherwise
        """
        l = lexer
        # Split the expression to ignore the left part
        expressions = expression.split("=")[1:]
        expression = " ".join(expressions)

        # Check if spatio-temporal operators are present in the expression
        if "{" in expression or "}" in expression:
            self.msgr.error(_("Spatio-temporal topological operators are not"
                              " supported in granularity algebra mode"))
            return False

        # detect all STDS
        if l is None:
            l = TemporalAlgebraLexer()
        l.build()
        l.lexer.input(expression)

        name_list = []
        tokens = []

        count = 0
        while True:
            tok = l.lexer.token()
            if not tok:
                break

            # Ignore map layer
            tokens.append(tok.type)
            ignore = False
            if count > 1:
                if tokens[count - 2] == "MAP" or tokens[count - 2] == "TMAP":
                    ignore = True

            if tok.type == "NAME" and ignore is False:
                name_list.append(tok.value)
            count += 1

        grans = []
        start_times = []
        ttypes = {}
        dbif, connected = init_dbif(self.dbif)

        for name in name_list:
            stds = open_old_stds(name, stdstype, dbif)
            # We need valid temporal topology
            if stds.check_temporal_topology() is False:
                self.msgr.error(_("All input space time datasets must have a valid temporal topology."))
                return False

            grans.append(stds.get_granularity())
            start_times.append(stds.get_temporal_extent_as_tuple()[0])
            ttypes[stds.get_temporal_type()] = stds.get_temporal_type()

        # Only one temporal type is allowed
        if len(ttypes) > 1:
            self.msgr.error(_("All input space time datasets must have the same temporal type."))
            return False

        # Compute the common granularity
        if "absolute" in ttypes.keys():
            self.granularity = compute_common_absolute_time_granularity(grans, start_times)
        else:
            self.granularity = compute_common_relative_time_granularity(grans)

        self.use_granularity = True

        return True

    def parse(self, expression, stdstype='strds',
              maptype='rast', mapclass=RasterDataset,
              basename=None, overwrite=False):
        """Parse the algebra expression and run the computation

        :param expression:
        :param stdstype:
        :param maptype:
        :param mapclass:
        :param basename:
        :param overwrite:
        :return: The process chain dictionary is dry-run was enabled, None otherwise
        """
        self.lexer = TemporalAlgebraLexer()
        self.lexer.build()
        self.parser = yacc.yacc(module=self, debug=self.debug, write_tables=False)

        self.overwrite = overwrite
        self.count = 0
        self.stdstype = stdstype
        self.maptype = maptype
        self.mapclass = mapclass
        self.basename = basename
        self.expression = expression
        self.parser.parse(expression)

        return self.process_chain_dict

    def generate_map_name(self):
        """Generate an unique  map name and register it in the objects map list

            The map names are unique between processes. Do not use the
            same object for map name generation in multiple threads.
        """
        self.count += 1
        if self.pid is not None:
            pid = self.pid
        else:
            pid = os.getpid()
        name = "tmp_map_name_%i_%i" % (pid, self.count)
        self.names[name] = name
        return name

    def generate_new_map(self, base_map,
                         bool_op='and',
                         copy=True,
                         rename=True,
                         remove=False):
        """Generate a new map using the spatio-temporal extent of the base map

           :param base_map: This map is used to create the new map
           :param bool_op: The boolean operator specifying the spatial extent
                  operation (intersection, union, disjoint union)
           :param copy: Specifies if the temporal extent of mapB should be
                  copied to mapA
           :param rename: Specifies if the generated map get a random name or get
                  the id from the base map.
            :param remove: Set this True if this map is an intermediate or empty map that should be removed
           :return: Map object
        """
        # Generate an intermediate name for the result map list.
        name = self.generate_map_name()
        # Check for mapset in given stds input.
        mapname = name + "@" + self.mapset
        # Create new map based on the related map list.
        map_new = base_map.get_new_instance(mapname)
        # Set initial map extend of new vector map.
        self.overlay_map_extent(map_new, base_map, bool_op=bool_op, copy=copy)
        if not rename:
            name = base_map.get_id()
            map_new.set_id(name)
        if remove is True:
            self.removable_maps[name] = map_new
        # Make sure to set the uid that is used in several dictionaries
        map_new.uid = name
        return map_new

    def overlay_map_extent(self,
                           mapA,
                           mapB,
                           bool_op=None,
                           temp_op='l',
                           copy=False):
        """Compute the spatio-temporal extent of two topological related maps

           :param mapA: The first map
           :param mapB: The second maps
           :param bool_op: The boolean operator specifying the spatial extent
                  operation (intersection, union, disjoint union)
           :param temp_op: The temporal operator specifying the temporal
                  extent operation (intersection, union, disjoint union, right reference)
                  Left reference is the default temporal extent behaviour.
           :param copy: Specifies if the temporal extent of mapB should be
                  copied to mapA
           :return: 0 if there is no overlay
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
            if "condition_value" in dir(mapB):
                mapA.condition_value = mapB.condition_value
        else:
            # Calculate spatial extent for different overlay operations.
            if bool_op == 'and':
                overlay_ext = mapA.spatial_intersection(mapB)
                if overlay_ext is not None:
                    mapA.set_spatial_extent(overlay_ext)
                else:
                    returncode = 0
            elif bool_op in ['or', 'xor']:
                overlay_ext = mapA.spatial_union(mapB)
                if overlay_ext is not None:
                    mapA.set_spatial_extent(overlay_ext)
                else:
                    returncode = 0
            elif bool_op == 'disor':
                overlay_ext = mapA.spatial_disjoint_union(mapB)
                if overlay_ext is not None:
                    mapA.set_spatial_extent(overlay_ext)
                else:
                    returncode = 0

            # Calculate temporal extent for different temporal operators.
            if temp_op == 'i':
                temp_ext = mapA.temporal_intersection(mapB)
                if temp_ext is not None:
                    mapA.set_temporal_extent(temp_ext)
                else:
                    returncode = 0
            elif temp_op == 'u':
                temp_ext = mapA.temporal_union(mapB)
                if temp_ext is not None:
                    mapA.set_temporal_extent(temp_ext)
                else:
                    returncode = 0
            elif temp_op == 'd':
                temp_ext = mapA.temporal_disjoint_union(mapB)
                if temp_ext is not None:
                    mapA.set_temporal_extent(temp_ext)
                else:
                    returncode = 0
            elif temp_op == 'r':
                temp_ext = mapB.get_temporal_extent()
                if temp_ext is not None:
                    mapA.set_temporal_extent(temp_ext)
                else:
                    returncode = 0
        return(returncode)

    def set_temporal_extent_list(self,
                                 maplist,
                                 topolist=["EQUAL"],
                                 temporal='l' ):
        """ Change temporal extent of map list based on temporal relations to
                other map list and given temporal operator.

            :param maplist: List of map objects for which relations has been build
                                        correctly.
            :param topolist: List of strings of temporal relations.
            :param temporal: The temporal operator specifying the temporal
                                            extent operation (intersection, union, disjoint
                                            union, right reference, left reference).

            :return: Map list with specified temporal extent.
        """
        resultdict = {}
        temporal_topo_list, spatial_topo_list = self._check_topology(topolist=topolist)

        for map_i in maplist:
            # Loop over temporal related maps and create overlay modules.
            tbrelations = map_i.get_temporal_relations()
            # Generate an intermediate map for the result map list.
            map_new = self.generate_new_map(base_map=map_i, bool_op='and',
                                            copy=True, rename=True)
            # Combine temporal and spatial extents of intermediate map with related maps.
            for topo in topolist:
                if topo in tbrelations.keys():
                    for map_j in (tbrelations[topo]):
                        if self._check_spatial_topology_relation(spatial_topo_list, map_i, map_j) is True:
                            if temporal == 'r':
                                # Generate an intermediate map for the result map list.
                                map_new = self.generate_new_map(base_map=map_i, bool_op='and',
                                                                copy=True, rename=True)
                            # Create overlaid map extent.
                            returncode = self.overlay_map_extent(map_new, map_j, 'and',
                                                                 temp_op=temporal)
                            print(map_new.get_id(), map_j.get_id())
                            # Stop the loop if no temporal or spatial relationship exist.
                            if returncode == 0:
                                break
                            # Append map to result map list.
                            elif returncode == 1:
                                # print(map_new.get_id() + " " + str(map_new.get_temporal_extent_as_tuple()))
                                # print(map_new.condition_value)
                                # print(map_new.cmd_list)
                                # resultlist.append(map_new)
                                resultdict[map_new.get_id()] = map_new

                            # Create r.mapcalc expression string for the operation.
                            #cmdstring = self.build_command_string(s_expr_a = map_new,
                            #                                                                s_expr_b = map_j,
                            #                                                                operator = function)
                            # Conditional append of module command.
                            #map_new.cmd_list = cmdstring
                    if returncode == 0:
                        break
            # Append map to result map list.
            #if returncode == 1:
            #    resultlist.append(map_new)
        # Get sorted map objects as values from result dictionoary.
        resultlist = resultdict.values()
        resultlist = sorted(resultlist, key = AbstractDatasetComparisonKeyStartTime)

        return(resultlist)

    ######################### Temporal functions ##############################

    def remove_maps(self):
        """Removes empty or intermediate maps of different type.
        """

        map_names = {}
        map_names["raster"] = []
        map_names["raster3d"] = []
        map_names["vector"] = []

        if self.removable_maps:
            for map in self.removable_maps.values():
                map_names[map.get_type()].append(map.get_name())

        for key in map_names.keys():
            if map_names[key]:
                self.msgr.message(_("Removing un-needed or empty %s maps" % (key)))
                self._remove_maps(map_names[key], key)

    def _remove_maps(self,
                     namelist,
                     map_type):
        """Remove maps of specific type

            :param namelist: List of map names to be removed
            :param map_type: The type of the maps  (raster, raster_3d or vector)
        """
        max = 100
        chunklist = [namelist[i:i + max] for i in range(0, len(namelist), max)]
        for chunk in chunklist:
            stringlist = ",".join(chunk)

            if self.run:
                m = copy.deepcopy(self.m_mremove)
                m.inputs["type"].value = map_type
                m.inputs["name"].value = stringlist
                m.flags["f"].value = True
                # print(m.get_bash())
                self.process_chain_dict["remove"].append(m.get_dict())

                if self.dry_run is False:
                    m.run()

    def check_stds(self,
                   input,
                   clear=False,
                   stds_type=None,
                   check_type=True):
        """ Check if input space time dataset exist in database and return its map list.

        :param input: Name of space time data set as string or list of maps.
        :param clear: Reset the stored conditional values to empty list.
        :param check_type: Check the type of the space time dataset to match the global stds type
        :param stds_type: The type of the space time dataset to be opened, if not provided
                                      then self.stdstype will be used

        :return: List of maps.
        """
        if isinstance(input, unicode) or isinstance(input, str):
            # Check for mapset in given stds input.
            if input.find("@") >= 0:
                id_input = input
            else:
                id_input = input + "@" + self.mapset
            # Create empty spacetime dataset.
            if stds_type:
                stds = dataset_factory(stds_type, id_input)
            else:
                stds = dataset_factory(self.stdstype, id_input)
            # Check for occurrence of space time dataset.
            if stds.is_in_db(dbif=self.dbif) is False:
                raise FatalError(_("Space time %s dataset <%s> not found") %
                    (stds.get_new_map_instance(None).get_type(), id_input))
            else:
                # Select temporal dataset entry from database.
                stds.select(dbif=self.dbif)
                if self.use_granularity:
                    # We create the maplist out of the map array from none-gap objects
                    maplist = []
                    map_array = stds.get_registered_maps_as_objects_by_granularity(gran=self.granularity,
                                                                                   dbif=self.dbif)
                    for entry in map_array:
                        # Ignore gap objects
                        if entry[0].get_id() is not None:
                            maplist.append(entry[0])
                else:
                    maplist = stds.get_registered_maps_as_objects(dbif=self.dbif)
                # Create map_value as empty list item.
                for map_i in maplist:
                    if "map_value" not in dir(map_i):
                        map_i.map_value = []
                    if "condition_value" not in dir(map_i):
                        map_i.condition_value = []
                    # Set and check global temporal type variable and map.
                    if map_i.is_time_absolute() and self.temporaltype is None:
                        self.temporaltype = 'absolute'
                    elif map_i.is_time_relative() and self.temporaltype is None:
                        self.temporaltype = 'relative'
                    elif map_i.is_time_absolute() and self.temporaltype == 'relative':
                        self.msgr.fatal(_("Wrong temporal type of space time dataset <%s> \
                                      <%s> time is required") %
                                     (id_input, self.temporaltype))
                    elif map_i.is_time_relative() and self.temporaltype == 'absolute':
                        self.msgr.fatal(_("Wrong temporal type of space time dataset <%s> \
                                      <%s> time is required") %
                                     (id_input, self.temporaltype))
        elif isinstance(input, self.mapclass):
            # Check if the input is a single map and return it as list with one entry.
            maplist = [input]

        elif isinstance(input, list):
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
        else:
            self.msgr.fatal(_("Wrong type of input " + str(input)))

        # We generate a unique map id that will be used
        # in the topology analysis, since the maplist can
        # contain maps with equal map ids
        for map in maplist:
            map.uid = self.generate_map_name()
            if self.debug:
                print(map.get_name(), map.uid, map.get_temporal_extent_as_tuple())

        return(maplist)

    def _check_spatial_topology_entries(self, spatial_topo_list, spatial_relations):
        """Check the spatial topology entries in the spatial relation list

        Return True if no spatial relation list is provided or if one spatial relation
        was found

        :param spatial_topo_list: The spatial relations that were defined in the expression
        :param spatial_relations: The spatial relations of a single map object

        :return: True if a spatial  topological relation was found, False if not
        """

        # Check spatial topology
        spatial_topo_check = False
        if len(spatial_topo_list) == 0:
            spatial_topo_check = True
        else:
            for spatial_topology in spatial_topo_list:
                if spatial_topology in spatial_relations.keys():
                    spatial_topo_check = True

        if self.debug is True:
            print("Spatial topology list", spatial_topo_list, spatial_topo_check)

        return spatial_topo_check

    def _check_spatial_topology_relation(self, spatial_topo_list, map_a, map_b):
        """Check if map_b has one of the spatial topological relations to map_a that is defined
        in spatial_topo_list

        :param spatial_topo_list:
        :param map_a:
        :param map_b:
        :return:
        """

        # Check spatial topology
        spatial_topo_check = False
        if len(spatial_topo_list) == 0:
            spatial_topo_check = True
        else:
            map_a_sr = map_a.get_spatial_relations()

            for spatial_topology in spatial_topo_list:
                if spatial_topology in map_a_sr.keys():
                    if map_b in map_a_sr[spatial_topology]:
                        spatial_topo_check = True

        if self.debug is True:
            print("Spatial topology list", spatial_topo_list, spatial_topo_check)

        return spatial_topo_check

    def _check_topology(self, topolist):
        """Check the topology definitions of the expression

        :param topolist: List of strings of temporal and spatial relations.

        :return: A tuple of spatial and temporal topology lists (temporal_topo_list, spatial_topo_list)

        :raises: This method will raise a syntax error in case the topology name is unknown
        """
        temporal_topo_list = []
        spatial_topo_list = []
        # Check if given temporal relation are valid.
        for topo in topolist:
            if topo.upper() not in self.temporal_topology_list and topo.upper() not in self.spatial_topology_list:
                raise SyntaxError("Unpermitted topological relation name '" + topo + "'")

            if topo.upper() in self.spatial_topology_list:
                spatial_topo_list.append(topo.upper())

            if topo.upper() in self.temporal_topology_list:
                temporal_topo_list.append(topo.upper())

        return temporal_topo_list, spatial_topo_list

    def build_spatio_temporal_topology_list(self,
                                            maplistA,
                                            maplistB=None,
                                            topolist=["EQUAL"],
                                            assign_val=False,
                                            count_map=False,
                                            compare_bool=False,
                                            compop=None,
                                            aggregate=None):
        """Build spatio-temporal topology for two space time data sets, copy map objects
          for given relation into map list.

          :param maplistA: List of maps.
          :param maplistB: List of maps.
          :param topolist: List of strings of spatio-temporal relations.
          :param assign_val: Boolean for assigning a boolean map value based on
                             the map_values from the compared map list by
                             topological relationships.
          :param count_map: Boolean if the number of topological related maps
                            should be returned.
          :param compare_bool: Boolean for comparing boolean map values based on
                               related map list and comparison operator.
          :param compop: Comparison operator, && or ||.
          :param aggregate: Aggregation operator for relation map list, & or |.

          :return: List of maps from maplistA that fulfil the topological relationships
                   to maplistB specified in topolist.

          .. code-block:: python

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
              ...     mapA.uid = idA
              ...     idB = "b%i@B"%(i)
              ...     mapB = tgis.RasterDataset(idB)
              ...     mapB.uid = idB
              ...     check = mapA.set_relative_time(i, i + 1, "months")
              ...     check = mapB.set_relative_time(i, i + 1, "months")
              ...     mapsA.append(mapA)
              ...     mapsB.append(mapB)
              >>> resultlist = l.build_spatio_temporal_topology_list(mapsA, mapsB, ['EQUAL'])
              >>> for map in resultlist:
              ...     if map.get_equal():
              ...         relations = map.get_equal()
              ...         print("Map %s has equal relation to map %s"%(map.get_name(),
              ...               relations[0].get_name()))
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
              >>> resultlist = l.build_spatio_temporal_topology_list(mapsA, mapsB, ['DURING'])
              >>> print(resultlist)
              []
              >>> # Create two list of maps with equal time stamps
              >>> mapsA = []
              >>> mapsB = []
              >>> for i in range(10):
              ...     idA = "a%i@B"%(i)
              ...     mapA = tgis.RasterDataset(idA)
              ...     mapA.uid = idA
              ...     idB = "b%i@B"%(i)
              ...     mapB = tgis.RasterDataset(idB)
              ...     mapB.uid = idB
              ...     check = mapA.set_relative_time(i, i + 1, "months")
              ...     check = mapB.set_relative_time(i, i + 2, "months")
              ...     mapsA.append(mapA)
              ...     mapsB.append(mapB)
              >>> resultlist = l.build_spatio_temporal_topology_list(mapsA, mapsB, ['starts','during'])
              >>> for map in resultlist:
              ...     if map.get_starts():
              ...         relations = map.get_starts()
              ...         print("Map %s has start relation to map %s"%(map.get_name(),
              ...               relations[0].get_name()))
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
              ...         print("Map %s has during relation to map %s"%(map.get_name(),
              ...               relations[0].get_name()))
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
              ...     mapA.uid = idA
              ...     idB = "b%i@B"%(i)
              ...     mapB = tgis.RasterDataset(idB)
              ...     mapB.uid = idB
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
              ...     mapA.uid = idA
              ...     mapA.map_value = True
              ...     idB = "b%i@B"%(i)
              ...     mapB = tgis.RasterDataset(idB)
              ...     mapB.uid = idB
              ...     mapB.map_value = False
              ...     check = mapA.set_absolute_time(datetime(2000,1,i+1),
              ...             datetime(2000,1,i + 2))
              ...     check = mapB.set_absolute_time(datetime(2000,1,i+6),
              ...             datetime(2000,1,i + 7))
              ...     mapsA.append(mapA)
              ...     mapsB.append(mapB)
              >>> resultlist = l.build_spatio_temporal_topology_list(mapsA, mapsB)
              >>> for map in resultlist:
              ...     print(map.get_id())
              a5@B
              a6@B
              a7@B
              a8@B
              a9@B
              >>> resultlist = l.build_spatio_temporal_topology_list(mapsA, mapsB, ['during'])
              >>> for map in resultlist:
              ...     print(map.get_id())

        """
        # Check the topology definitions and return the list of temporal and spatial
        # topological relations that must be fulfilled
        temporal_topo_list, spatial_topo_list = self._check_topology(topolist=topolist)

        resultdict = {}

        # Create spatio-temporal topology for maplistA to maplistB.
        tb = SpatioTemporalTopologyBuilder()
        if len(spatial_topo_list) > 0:
            # Dictionary with different spatial variables used for topology builder.
            spatialdict = {'strds': '2D', 'stvds': '2D', 'str3ds': '3D'}
            tb.build(maplistA, maplistB, spatial=spatialdict[self.stdstype])
        else:
            tb.build(maplistA, maplistB)
        # Iterate through maps in maplistA and search for relationships given
        # in topolist.
        for map_i in maplistA:
            if assign_val:
                self.assign_bool_value(map_i, temporal_topo_list, spatial_topo_list)
            elif compare_bool:
                self.compare_bool_value(map_i, compop, aggregate, temporal_topo_list, spatial_topo_list)

            temporal_relations = map_i.get_temporal_relations()
            spatial_relations = map_i.get_spatial_relations()

            for temporal_topology in temporal_topo_list:
                if temporal_topology.upper() in temporal_relations.keys():
                    if self._check_spatial_topology_entries(spatial_topo_list, spatial_relations) is True:
                        if count_map:
                            relationmaplist = temporal_relations[temporal_topology.upper()]
                            gvar = GlobalTemporalVar()
                            gvar.td = len(relationmaplist)
                            if "map_value" in dir(map_i):
                                map_i.map_value.append(gvar)
                            else:
                                map_i.map_value = gvar
                        # Use unique identifier, since map names may be equal
                        resultdict[map_i.uid] = map_i
        resultlist = resultdict.values()

        # Sort list of maps chronological.
        resultlist = sorted(resultlist, key=AbstractDatasetComparisonKeyStartTime)

        return(resultlist)

    def assign_bool_value(self,
                          map_i,
                          temporal_topo_list=["EQUAL"],
                          spatial_topo_list=[]):
        """ Function to assign boolean map value based on the map_values from the
        compared map list by topological relationships.

          :param map_i: Map object with temporal extent.
          :param temporal_topo_list: List of strings for given temporal relations.
          :param spatial_topo_list: List of strings for given spatial relations.

          :return: Map object with conditional value that has been assigned by
                        relation maps that fulfil the topological relationships to
                        maplistB specified in temporal_topo_list.
        """

        temporal_relations = map_i.get_temporal_relations()
        condition_value_list = []
        for topo in temporal_topo_list:
            if topo.upper() in temporal_relations.keys():
                relationmaplist = temporal_relations[topo.upper()]
                for relationmap in relationmaplist:
                    if self._check_spatial_topology_relation(spatial_topo_list, map_i, relationmap) is True:
                        for boolean in relationmap.condition_value:
                            if isinstance(boolean, bool):
                                condition_value_list.append(boolean)
                        if self.debug:
                            print("assign_bool_value", str(relationmap.get_temporal_extent_as_tuple())
                                  + str(boolean))
        if all(condition_value_list):
            resultbool = True
        else:
            resultbool = False
        map_i.condition_value = [resultbool]

        return(resultbool)

    def compare_bool_value(self,
                           map_i,
                           compop,
                           aggregate,
                           temporal_topo_list=["EQUAL"],
                           spatial_topo_list=[]):
        """ Function to evaluate two map lists with boolean values by boolean
            comparison operator.

          :param map_i: Map object with temporal extent.
          :param compop: Comparison operator, && or ||.
          :param aggregate: Aggregation operator for relation map list, & or |.
          :param temporal_topo_list: List of strings for given temporal relations.
          :param spatial_topo_list: List of strings for given spatial relations.

          :return: Map object with conditional value that has been evaluated by
                        comparison operators.
        """

        temporal_relations = map_i.get_temporal_relations()

        # Build conditional list with elements from related maps and given relation operator.
        leftbool = map_i.condition_value[0]
        condition_value_list = [leftbool]
        count = 0
        for topo in temporal_topo_list:
            if topo.upper() in temporal_relations.keys():
                relationmaplist = temporal_relations[topo.upper()]
                for relationmap in relationmaplist:
                    if self._check_spatial_topology_relation(spatial_topo_list, map_i, relationmap) is True:
                        if count == 0:
                            condition_value_list.append(compop[0])
                            condition_value_list.append('(')
                        for boolean in relationmap.condition_value:
                            if isinstance(boolean, bool):
                                if count > 0:
                                    condition_value_list.append(aggregate)
                                condition_value_list.append(boolean)
                                count = count + 1

                        if self.debug:
                            print("compare_bool_value", map_i.get_id(), relationmap.get_id())
        if count > 0:
            condition_value_list.append(')')
        # Convert conditional list to concatenated string and evaluate booleans.
        condition_value_str = ''.join(map(str, condition_value_list))
        if self.debug:
            print(condition_value_str)
        resultbool = eval(condition_value_str)
        if self.debug:
            print(resultbool)
        # Add boolean value to result list.
        map_i.condition_value = [resultbool]

        return(resultbool)

    def eval_toperator(self, operator, optype = 'relation'):
        """This function evaluates a string containing temporal operations.

         :param operator: String of temporal operations, e.g. {!=,equal|during,l}.
         :param optype: String to define operator type.

         :return :List of temporal relations (equal, during), the given function
          (!:) and the interval/instances (l).

        .. code-block:: python

             >>> import grass.temporal as tgis
             >>> tgis.init()
             >>> p = tgis.TemporalOperatorParser()
             >>> operator = "{+, during}"
             >>> p.parse(operator, optype = 'raster')
             >>> print((p.relations, p.temporal, p.function))
             (['during'], 'l', '+')

        """
        p = TemporalOperatorParser()
        p.parse(operator, optype)
        p.relations = [rel.upper() for rel in p.relations]

        return(p.relations, p.temporal, p.function, p.aggregate)

    def perform_temporal_selection(self,
                                   maplistA,
                                   maplistB,
                                   topolist=["EQUAL"],
                                   inverse=False,
                                   assign_val=False):
        """This function performs temporal selection operation.

          :param maplistA:   List of maps representing the left side of a temporal
                             expression.
          :param maplistB:   List of maps representing the right side of a temporal
                             expression.
          :param topolist: List of strings of temporal relations.
          :param inverse: Boolean value that specifies if the selection should be
                             inverted.
          :param assign_val: Boolean for assigning a boolean map value based on
                            the map_values from the compared map list by
                            topological relationships.

          :return: List of selected maps from maplistA.

          .. code-block:: python

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
              ...     mapA.uid = idA
              ...     idB = "b%i@B"%(i)
              ...     mapB = tgis.RasterDataset(idB)
              ...     mapB.uid = idB
              ...     check = mapA.set_relative_time(i, i + 1, "months")
              ...     check = mapB.set_relative_time(i + 5, i + 6, "months")
              ...     mapsA.append(mapA)
              ...     mapsB.append(mapB)
              >>> resultlist = l.perform_temporal_selection(mapsA, mapsB, ['EQUAL'],
              ...                                           False)
              >>> for map in resultlist:
              ...     if map.get_equal():
              ...         relations = map.get_equal()
              ...         print("Map %s has equal relation to map %s"%(map.get_name(),
              ...               relations[0].get_name()))
              Map a5 has equal relation to map b0
              Map a6 has equal relation to map b1
              Map a7 has equal relation to map b2
              Map a8 has equal relation to map b3
              Map a9 has equal relation to map b4
              >>> resultlist = l.perform_temporal_selection(mapsA, mapsB, ['EQUAL'],
              ...                                           True)
              >>> for map in resultlist:
              ...     if not map.get_equal():
              ...         print("Map %s has no equal relation to mapset mapsB"%(map.get_name()))
              Map a0 has no equal relation to mapset mapsB
              Map a1 has no equal relation to mapset mapsB
              Map a2 has no equal relation to mapset mapsB
              Map a3 has no equal relation to mapset mapsB
              Map a4 has no equal relation to mapset mapsB

        """
        if not inverse:
            topolist = self.build_spatio_temporal_topology_list(maplistA, maplistB, topolist,
                                                                assign_val = assign_val)
            resultlist = topolist

        else:
            topolist = self.build_spatio_temporal_topology_list(maplistA, maplistB, topolist,
                                                                assign_val = assign_val)
            resultlist = []
            for map_i in maplistA:
                if map_i not in topolist:
                    resultlist.append(map_i)
                    #if assign_val:
                    #   if "condition_value" in dir(map_i):
                    #        map_i.condition_value.append(False)

        # Sort list of maps chronological.
        resultlist = sorted(resultlist, key = AbstractDatasetComparisonKeyStartTime)
        return(resultlist)

    def set_granularity(self,
                        maplistA,
                        maplistB,
                        toperator='l',
                        topolist=["EQUAL"]):
        """This function sets the temporal extends of a list of maps based on
             another map list.

          :param maplistB: List of maps.
          :param maplistB: List of maps.
          :param toperator: String containing the temporal operator: l, r, d, i, u.
          :param topolist: List of topological relations.

          :return: List of maps with the new temporal extends.

          .. code-block:: python

              >>> import grass.temporal as tgis
              >>> tgis.init()
              >>> p = tgis.TemporalAlgebraParser()
              >>> # Create two list of maps with equal time stamps
              >>> mapsA = []
              >>> mapsB = []
              >>> for i in range(10):
              ...     idA = "a%i@B"%(i)
              ...     mapA = tgis.RasterDataset(idA)
              ...     mapA.uid = idA
              ...     idB = "b%i@B"%(i)
              ...     mapB = tgis.RasterDataset(idB)
              ...     mapB.uid = idB
              ...     check = mapA.set_relative_time(i, i + 1, "months")
              ...     check = mapB.set_relative_time(i*2, i*2 + 2, "months")
              ...     mapsA.append(mapA)
              ...     mapsB.append(mapB)
              >>> resultlist = p.set_granularity(mapsA, mapsB, toperator = "u", topolist = ["during"])
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

        """
        topologylist = ["EQUAL", "FOLLOWS", "PRECEDES", "OVERLAPS", "OVERLAPPED",
                        "DURING", "STARTS", "FINISHES", "CONTAINS", "STARTED",
                        "FINISHED"]

        for topo in topolist:
            if topo.upper() not in topologylist:
                raise SyntaxError("Unpermitted temporal relation name '" + topo + "'")

        # Create temporal topology for maplistA to maplistB.
        tb = SpatioTemporalTopologyBuilder()
        # Dictionary with different spatial variables used for topology builder.
        spatialdict = {'strds': '2D', 'stvds': '2D', 'str3ds': '3D'}
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
            map_extent = map_i.get_temporal_extent()
            map_start = map_extent.get_start_time()
            map_end = map_extent.get_end_time()
            unchanged = True
            for topo in topolist:
                if topo.upper() in tbrelations.keys():
                    relationmaplist = tbrelations[topo.upper()]
                    for relationmap in relationmaplist:
                        newextent = None
                        if toperator == "i":
                            newextent = map_i.temporal_intersection(relationmap)
                        elif toperator == "u":
                            newextent = map_i.temporal_union(relationmap)
                        elif toperator == "d":
                            newextent = map_i.temporal_disjoint_union(relationmap)
                        elif toperator == "l":
                            newextent = map_i.get_temporal_extent()
                        elif toperator == "r":
                            newextent = relationmap.get_temporal_extent()
                        if newextent is not None:
                            start = newextent.get_start_time()
                            end = newextent.get_end_time()
                            #print(map_i.get_id() + ' - start: ' + str(start) + ' end: ' + str(end))
                            # Track changes in temporal extents of maps.
                            if map_start != start or map_end != end:
                                unchanged = False
                            if map_i.is_time_absolute():
                                map_i.set_absolute_time(start, end)
                            else:
                                relunit = map_i.get_relative_time_unit()
                                map_i.set_relative_time(int(start), int(end), relunit)
                            resultdict[map_i.get_id()] = map_i
                else:
                    if self.debug:
                        print('Topologic relation: ' + topo.upper() + ' not found.')
                    resultdict[map_i.get_id()] = map_i
            if unchanged is True:
                if self.debug:
                    print('Leave temporal extend of result map: ' + map_i.get_map_id() + ' unchanged.')

        resultlist = resultdict.values()
        # Sort list of maps chronological.
        resultlist = sorted(resultlist, key = AbstractDatasetComparisonKeyStartTime)
        # Get relations to maplistB per map in A.
        # Loop over all relations from list
        # temporal extent = map.temporal_intersection(map)
        # if temporal extend is None = delete map.

        return(resultlist)

    def get_temporal_func_dict(self, map):
        """ This function creates a dictionary containing temporal functions for a
             map dataset with time stamp.

          :param map: Map object with time stamps.

          :return: Dictionary with temporal functions for given input map.

          .. code-block:: python

              >>> import grass.temporal as tgis
              >>> import datetime
              >>> tgis.init()
              >>> l = tgis.TemporalAlgebraParser()
              >>> # Example with one list of maps
              >>> # Create one list of maps with equal time stamps
              >>> for i in range(1):
              ...     idA = "a%i@B"%(i)
              ...     mapA = tgis.RasterDataset(idA)
              ...     mapA.uid = idA
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

        """
        tvardict = {"START_DOY": None, "START_DOW": None, "START_YEAR": None,
            "START_MONTH": None, "START_WEEK": None, "START_DAY": None,
            "START_HOUR": None, "START_MINUTE": None, "START_SECOND": None,
            "END_DOY": None, "END_DOW": None, "END_YEAR": None,
            "END_MONTH": None, "END_WEEK": None, "END_DAY": None,
            "END_HOUR": None, "END_MINUTE": None, "END_SECOND": None,
            "START_DATE": None, "START_DATETIME": None, "START_TIME": None,
            "END_DATE": None, "END_DATETIME": None, "END_TIME": None}

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
            # core.fatal(_("The temporal functions for map <%s> only "
            #              "supported for absolute time." % (str(map.get_id()))))
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
        """ This function evaluates a global variable expression for a map list.
             For example: start_day() > 5 , end_month() == 2.

          :param gvar: Object of type GlobalTemporalVar containing temporal.
          :param maplist: List of map objects.

          :return: List of maps from maplist with added conditional boolean values.
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
            # Check if value has to be transferred to datetime object for comparison.
            if tfunc in ["START_DATE", "END_DATE", "START_TIME", "END_TIME",
                         "START_DATETIME", "END_DATETIME"]:
                timeobj = string_to_datetime(value.replace("\"",""))
                value = timeobj.date()
                boolname = self.eval_datetime_str(tfuncval, comp_op, value)
            else:
                boolname = eval(str(tfuncval) + comp_op + str(value))
            # Add conditional boolean value to the map.
            if "condition_value" in dir(map_i):
                map_i.condition_value.append(boolname)
            else:
                map_i.condition_value = boolname
        return(maplist)

    def eval_map_list(self, maplist,thenlist, topolist=["EQUAL"]):
        """ This function transfers boolean values from temporal expression
             from one map list to another by their topology. These boolean
             values are added to the maps as condition_value.

          :param maplist:  List of map objects containing boolean map values.
          :param thenlist: List of map objects where the boolean values
                          should be added.

          :return: List of maps from thenlist with added conditional boolean values.
        """
        # Get topology of then statement map list in relation to the other maplist
        # and assign boolean values of the maplist to the thenlist.
        containlist = self.perform_temporal_selection(thenlist, maplist,
                                                      assign_val=True,
                                                      topolist=topolist)
        # Inverse selection of maps from thenlist and assigning False values.
        #excludelist = self.perform_temporal_selection(thenlist, maplist,
        #                                              assign_val = True,
        #                                              inverse = True,
        #                                              topolist = topolist)
        # Combining the selection and inverse selection list.
        resultlist = containlist# + excludelist

        return(resultlist)

    def build_condition_list(self, tvarexpr, thenlist, topolist=["EQUAL"]):
        """ This function evaluates temporal variable expressions of a conditional
             expression in two steps.
             At first it combines stepwise the single conditions by their relations with LALR.
             In this process sub condition map lists will be created which will include
             information of the underlying single conditions. Important: The temporal
             relations between conditions are evaluated by implicit aggregation.
             In the second step the aggregated condition map list will be compared with the
             map list of conclusion statements by the given temporal relation.

             The result is written as 'condition_value' attribute to the resulting map objects.
             These attribute consists of boolean expressions and operators which can be
             evaluated with the eval_condition_list function.
             [True,  '||', False, '&&', True]

             For example: td(A) == 1 && start_day() > 5 --> [True || False]
                          (for one map.condition_value in a then map list)

             :param tvarexpr: List of GlobalTemporalVar objects and map lists.
                         The list is constructed by the TemporalAlgebraParser
                         in order of expression evaluation in the parser.

             :param thenlist: Map list object of the conclusion statement.
                         It will be compared and evaluated by the conditions.

             :param topolist: List of temporal relations between the conditions and the
                         conclusions.

             :return: Map list with conditional values for all temporal expressions.

        """

        # Evaluate the temporal variable expression and compute the temporal combination
        # of conditions.

        # Check if the input expression is a valid single global variable.
        if isinstance(tvarexpr, GlobalTemporalVar) and tvarexpr.get_type() == "global":
            # Use method eval_global_var to evaluate expression.
            resultlist = self.eval_global_var(tvarexpr, thenlist)
        # Check if a given list is a list of maps.
        elif all([issubclass(type(ele), AbstractMapDataset) for ele in tvarexpr]):
            # Use method eval_map_list to evaluate map_list in comparison to thenlist.
            resultlist = self.eval_map_list(tvarexpr, thenlist, topolist)
        elif len(tvarexpr) % 2 != 0:
            # Define variables for map list comparisons.
            #self.msgr.fatal("Condition list is not complete. Elements missing")
            for iter in range(len(tvarexpr)):
                expr = tvarexpr[iter]
                operator = tvarexpr[iter + 1]
                relexpr = tvarexpr[iter + 2]
                if all([issubclass(type(ele), list) for ele in [expr, relexpr]]):
                    resultlist = self.build_spatio_temporal_topology_list(expr, relexpr)
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
        """ This function evaluates conditional values of a map list.
             A recursive function is used to evaluate comparison statements
             from left to right in the given conditional list.

             For example::

                  - [True,  '||', False, '&&', True]  -> True
                  - [True,  '||', False, '&&', False] -> False
                  - [True,  '&&', False, '&&', True]  -> False
                  - [False, '||', True,  '||', False] -> True
                  - [False, '&&', True,  '&&', True]  -> False
                  - [True,  '&&', True,  '&&', True]  -> True
                  - [True,  '&&', True]               -> True
                  - [True,  '&&', False]              -> False
                  - [False, '||', True]               -> True

             :param tvarexpr: List of GlobalTemporalVar objects and map lists.
                          The list is constructed by the TemporalAlgebraParser
                          in order of expression evaluation in the parser.

             :return: Map list with conditional values for all temporal expressions.
        """
        def recurse_compare(conditionlist):
            for ele in conditionlist:
                if ele == '||':
                    ele_index = conditionlist.index(ele)
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

        # Loop through map list and evaluate conditional values.
        for map_i in maplist:
            if "condition_value" in dir(map_i):
                # Get condition values from map object.
                conditionlist = map_i.condition_value
                # Evaluate conditions in list with recursive function.
                resultbool = recurse_compare(conditionlist)
                # Set conditional value of map to resulting boolean.
                map_i.condition_value = resultbool
                # Add all maps that fulfill the conditions to result list.
                if resultbool[0]:
                    resultlist.append(map_i)
                    if self.debug:
                        print(map_i.get_map_id() + ' ' + str(map_i.condition_value))
                else:
                    inverselist.append(map_i)
        if inverse:
            return(inverselist)
        else:
            return(resultlist)

###########################################################################

    def p_statement_assign(self, t):
        # The expression should always return a list of maps
        # This function starts all the work and is the last one that is called from the parser
        """
        statement : stds EQUALS expr

        """

        if self.run:
            dbif, connected = init_dbif(self.dbif)
            map_type = None
            if isinstance(t[3], list):
                num = len(t[3])
                count = 0
                register_list = []
                if num > 0:

                    # Compute the granularity for suffix creation
                    granularity = None
                    if len(t[3]) > 0 and self.time_suffix == 'gran':
                        map_i = t[3][0]
                        if map_i.is_time_absolute() is True:
                            granularity = compute_absolute_time_granularity(t[3])

                    # compute the size of the numerical suffix
                    num = len(t[3])
                    leadzero = len(str(num))

                    if self.dry_run is False:
                        process_queue = pymod.ParallelModuleQueue(int(self.nprocs))

                    for map_i in t[3]:
                        # Check if the map type and stds type are compatible
                        if count == 0:
                            maps_stds_type = map_i.get_new_stds_instance(None).get_type()
                            map_type = map_i.get_type()
                            if maps_stds_type != self.stdstype:
                                self.msgr.warning(_("The resulting space time dataset type <%(a)s> is "
                                                    "different from the requested type <%(b)s>"
                                                    % ({"a":maps_stds_type, "b":self.stdstype})))
                        else:
                            map_type_2 = map_i.get_type()
                            if map_type != map_type_2:
                                self.msgr.fatal(_("Maps that should be registered in the "
                                                  "resulting space time dataset have different types."))
                        count += 1

                        # Test if temporal extents was been modified by temporal
                        # relation operators (i|r).
                        # If it was modified, then the map will be copied
                        map_a_extent = map_i.get_temporal_extent_as_tuple()
                        map_b = map_i.get_new_instance(map_i.get_id())
                        map_b.select(dbif)
                        map_b_extent = map_b.get_temporal_extent_as_tuple()
                        if map_a_extent != map_b_extent:

                            # Create new map with basename
                            newident = create_numeric_suffix(self.basename, count, "%0" + str(leadzero))

                            if map_i.is_time_absolute() is True and self.time_suffix and \
                                            granularity is not None and self.time_suffix == 'gran':
                                suffix = create_suffix_from_datetime(map_i.temporal_extent.get_start_time(),
                                                                     granularity)
                                newident = "{ba}_{su}".format(ba=self.basename, su=suffix)
                            # If set use the time suffix to create the map name
                            elif map_i.is_time_absolute() is True and self.time_suffix and \
                                            self.time_suffix == 'time':
                                suffix = create_time_suffix(map_i)
                                newident = "{ba}_{su}".format(ba=self.basename, su=suffix)

                            map_result = map_i.get_new_instance(newident + "@" + self.mapset)

                            if map_result.map_exists() and self.overwrite is False:
                                self.msgr.fatal("Error raster maps with basename %s exist. "
                                                "Use --o flag to overwrite existing file" % map_i.get_id())

                            map_result.set_temporal_extent(map_i.get_temporal_extent())
                            map_result.set_spatial_extent(map_i.get_spatial_extent())
                            # Attention we attach a new attribute
                            map_result.is_new = True
                            register_list.append(map_result)

                            # Copy the map
                            m = copy.deepcopy(self.m_copy)
                            m.flags["overwrite"].value = self.overwrite

                            if map_i.get_type() == 'raster':
                                m.inputs["raster"].value = map_i.get_id(), newident
                            elif map_i.get_type() == 'raster3d':
                                m.inputs["raster_3d"].value = map_i.get_id(), newident
                            elif map_i.get_type() == 'vector':
                                m.inputs["vector"].value = map_i.get_id(), newident

                            # Add the process description to the dict
                            self.process_chain_dict["processes"].append(m.get_dict())

                            if self.dry_run is False:
                                process_queue.put(m)
                        else:
                            register_list.append(map_i)

                    # Wait for running processes
                    if self.dry_run is False:
                        process_queue.wait()

                    # Open connection to temporal database.
                    # Create result space time dataset based on the map stds type
                    if self.dry_run is False:
                        resultstds = open_new_stds(t[1],maps_stds_type,
                                                   'absolute', t[1], t[1],
                                                   'mean', self.dbif,
                                                   overwrite=self.overwrite)

                    for map_i in register_list:
                        # Get meta data from grass database.
                        map_i.load()

                        # Put the map into the process dictionary
                        start, end = map_i.get_temporal_extent_as_tuple()
                        self.process_chain_dict["register"].append((map_i.get_name(), str(start), str(end)))

                        if hasattr(map_i, "is_new") is True:
                            # Do not register empty maps if not required
                            # In case of a null map continue, do not register null maps

                            if map_i.get_type() == "raster" or map_i.get_type() == "raster3d":
                                if map_i.metadata.get_min() is None and \
                                   map_i.metadata.get_max() is None:
                                    if not self.register_null:
                                        self.removable_maps[map_i.get_name()] = map_i
                                        continue

                            if map_i.is_in_db(dbif) and self.overwrite:
                                # Update map in temporal database.
                                if self.dry_run is False:
                                    map_i.update_all(dbif)
                            elif map_i.is_in_db(dbif) and self.overwrite is False:
                                # Raise error if map exists and no overwrite flag is given.
                                self.msgr.fatal("Error map %s exist in temporal database. "
                                                "Use overwrite flag." % map_i.get_map_id())
                            else:
                                # Insert map into temporal database.
                                if self.dry_run is False:
                                    map_i.insert(dbif)

                        # Register map in result space time dataset.
                        if self.dry_run is False:
                            success = resultstds.register_map(map_i, dbif)
                            if not success:
                                self.msgr.warning("Unabe to register map layers "
                                                  "in STDS %s" % (t[1]))

                    if self.dry_run is False:
                        resultstds.update_from_registered_maps(dbif)

                    self.process_chain_dict["STDS"]["name"] = t[1]
                    self.process_chain_dict["STDS"]["stdstype"] = self.stdstype
                    self.process_chain_dict["STDS"]["temporal_type"] = 'absolute'

                elif num == 0:
                    self.msgr.warning("Empty result space time dataset. "
                                      "No map has been registered in %s" % (t[1]))
                    # Open connection to temporal database.
                    # Create result space time dataset.
                    if self.dry_run is False:
                        resultstds = open_new_stds(t[1], self.stdstype,
                                                   'absolute', t[1], t[1],
                                                   'mean', dbif,
                                                   overwrite=self.overwrite)

            if connected:
                dbif.close()
            t[0] = t[3]
        else:
            t[0] = t[3]

        if self.debug:
            print(t[1], "=", t[3])

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

    def p_expr_strds_function(self, t):
        # Explicitly specify a space time raster dataset
        # R = A : strds(B)
        """
        expr : STRDS LPAREN stds RPAREN
        """
        if self.run:
            t[0] = self.check_stds(t[3], stds_type = "strds", check_type=False)
        else:
            t[0] = t[3]
            if self.debug:
                print("Opening STRDS: ", t[0])

    def p_expr_str3ds_function(self, t):
        # Explicitly specify a space time raster dataset
        # R = A : str3ds(B)
        """
        expr : STR3DS LPAREN stds RPAREN
        """
        if self.run:
            t[0] = self.check_stds(t[3], stds_type = "str3ds", check_type=False)
        else:
            t[0] = t[3]
            if self.debug:
                print("Opening STR3DS: ", t[0])

    def p_expr_stvds_function(self, t):
        # Explicitly specify a space time vector dataset
        # R = A : stvds(B)
        """
        expr : STVDS LPAREN stds RPAREN
        """
        if self.run:
            print(t[3])
            t[0] = self.check_stds(t[3], stds_type = "stvds", check_type=False)
        else:
            t[0] = t[3]
            if self.debug:
                print("Opening STVDS: ", t[0])

    def p_expr_tmap_function(self, t):
        # Add a single map.
        # Only the spatial extent of the map is evaluated.
        # Temporal extent is not existing.
        # Examples:
        #    R = tmap(A)
        """
        expr : TMAP LPAREN stds RPAREN
        """
        if self.run:
            # Check input map.
            input = t[3]
            if not isinstance(input, list):
                # Check for mapset in given stds input.
                if input.find("@") >= 0:
                    id_input = input
                else:
                    id_input = input + "@" + self.mapset
                # Create empty map dataset.
                map_i = dataset_factory(self.maptype, id_input)
                # Check for occurrence of space time dataset.
                if map_i.map_exists() is False:
                    raise FatalError(_("%s map <%s> not found in GRASS spatial database") %
                                      (map_i.get_type(), id_input))
                else:
                    # Select dataset entry from database.
                    map_i.select(dbif=self.dbif)
            else:
                raise FatalError(_("Wrong map type. TMAP only supports single "
                                   "maps that are registered in the temporal GRASS database"))
            # Return map object.
            t[0] = [map_i]
        else:
            t[0] = "tmap(", t[3], ")"

        if self.debug:
            print("tmap(", t[3], ")")

    def p_expr_tmerge_function(self, t):
        # Merge two maplists of same STDS type into a result map list.
        # Only possible for same data types!
        # Examples:
        #    R = merge(A, B)
        """
        expr : MERGE LPAREN stds COMMA stds RPAREN
             | MERGE LPAREN expr COMMA stds RPAREN
             | MERGE LPAREN stds COMMA expr RPAREN
             | MERGE LPAREN expr COMMA expr RPAREN
        """
        if self.run:
            # Check input map.
            maplistA   = self.check_stds(t[3])
            maplistB   = self.check_stds(t[5])

            # Check empty lists.
            if len(maplistA) == 0 and len(maplistB) == 0:
                self.msgr.warning(_("Merging empty map lists"))
                resultlist = maplistA + maplistB
            elif len(maplistA) == 0:
                self.msgr.message(_("First Map list is empty, can't merge it. Return only last map list"))
                resultlist = maplistB
            elif len(maplistB) == 0:
                self.msgr.message(_("Second Map list is empty, can't merge it. Return only first map list"))
                resultlist = maplistA
            else:
                # Check for identical data types in map lists.
                typeA = maplistA[0].metadata.get_datatype()
                typeB = maplistB[0].metadata.get_datatype()

                if typeA != typeB:
                    raise FatalError(_("Space time datasets to merge must have the same temporal type"))

                resultlist = maplistA + maplistB

            # Return map list.
            t[0] = resultlist
        else:
            t[0] = "merge(", t[3], ",", t[5], ")"

        if self.debug:
            print("merge(", t[3], ",", t[5], ")")

    def p_t_hash(self,t):
        """
        t_hash_var : stds HASH stds
                   | expr HASH stds
                   | stds HASH expr
                   | expr HASH expr
        """

        if self.run:
            maplistA   = self.check_stds(t[1])
            maplistB   = self.check_stds(t[3])
            resultlist = self.build_spatio_temporal_topology_list(maplistA,
                                                                  maplistB,
                                                                  count_map=True)
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
            topolist   = self.eval_toperator(t[2], optype='hash')[0]
            resultlist = self.build_spatio_temporal_topology_list(maplistA,
                                                                  maplistB,
                                                                  topolist,
                                                                  count_map=True)
            t[0] = resultlist

    def p_t_hash_paren(self, t):
        """
        t_hash_var : LPAREN t_hash_var RPAREN
        """
        t[0] = t[2]

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
                    if end is not None:
                        td = time_delta_to_relative_time(end - start)
                else:
                    start, end, unit = map_i.get_relative_time()
                    if end is not None:
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
            print("td(" + str(t[3]) + ")")


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

    def p_t_var_expr_td_hash(self, t):
        # Examples:
        #    A # B == 2
        #    td(A) < 31
        """
        t_var_expr : t_td_var   comp_op number
                   | t_hash_var comp_op number
        """
        if self.run:
            maplist = self.check_stds(t[1])
            comp_op = t[2]
            value  = str(t[3])
            for map_i in maplist:
                # Evaluate time diferences and hash operator statements for each map.
                try:
                    td = map_i.map_value[0].td
                    boolname = eval(str(td) + comp_op + value)
                    # Add conditional boolean value to the map.
                    if "condition_value" in dir(map_i):
                        map_i.condition_value.append(boolname)
                    else:
                        map_i.condition_value = boolname
                except:
                    self.msgr.fatal("Error: the given expression does not contain a correct time difference object.")

            t[0] = maplist

        if self.debug:
            print(t[1], t[2], t[3])

    def p_t_var_expr_number(self, t):
        # Examples:
        #    start_month(A) > 2
        #    start_day(B) < 14
        #    start_day(B) < start_month(A)
        """
        t_var_expr : t_var LPAREN stds RPAREN comp_op number
                   | t_var LPAREN expr RPAREN comp_op number
        """
        # TODO:  Implement comparison operator for map lists.
        #| t_var LPAREN stds RPAREN comp_op t_var LPAREN stds RPAREN
        #| t_var LPAREN stds RPAREN comp_op t_var LPAREN expr RPAREN
        #| t_var LPAREN expr RPAREN comp_op t_var LPAREN expr RPAREN
        #| t_var LPAREN expr RPAREN comp_op t_var LPAREN stds RPAREN
        # TODO:  Implement statement in backward direction:
        # number comp_op t_var LPAREN stds RPAREN
        if self.run:
            maplist = self.check_stds(t[3])
            gvar = GlobalTemporalVar()
            gvar.tfunc  = t[1]
            gvar.compop = t[5]
            gvar.value  = t[6]
            # Evaluate temporal variable for given maplist.
            resultlist = self.eval_global_var(gvar, maplist)
            t[0] = resultlist

        if self.debug:
            print(t[1], t[3], t[5], t[6])

    def p_t_var_expr_time(self, t):
        # Examples:
        #   start_time(A) == "12:30:00"
        #   start_date(B) <= "2001-01-01"
        #   start_datetime(C) > "2001-01-01 12:30:00"
        # TODO:  Implement statement in backward direction:
        # TIME comp_op START_TIME LPAREN stds RPAREN
        """
        t_var_expr : START_TIME     LPAREN stds RPAREN comp_op TIME
                   | START_DATE     LPAREN stds RPAREN comp_op DATE
                   | START_DATETIME LPAREN stds RPAREN comp_op DATETIME
                   | END_TIME       LPAREN stds RPAREN comp_op TIME
                   | END_DATE       LPAREN stds RPAREN comp_op DATE
                   | END_DATETIME   LPAREN stds RPAREN comp_op DATETIME
                   | START_TIME     LPAREN expr RPAREN comp_op TIME
                   | START_DATE     LPAREN expr RPAREN comp_op DATE
                   | START_DATETIME LPAREN expr RPAREN comp_op DATETIME
                   | END_TIME       LPAREN expr RPAREN comp_op TIME
                   | END_DATE       LPAREN expr RPAREN comp_op DATE
                   | END_DATETIME   LPAREN expr RPAREN comp_op DATETIME
        """
        if self.run:
            # Check input maplist.
            maplist = self.check_stds(t[3])
            # Build global temporal variable.
            gvar = GlobalTemporalVar()
            gvar.tfunc  = t[1]
            gvar.compop = t[5]
            gvar.value  = t[6]
            # Evaluate temporal variable for given maplist.
            resultlist = self.eval_global_var(gvar, maplist)

            t[0] = resultlist

        if self.debug:
            print(t[1], t[3], t[5], t[6])

    def p_t_var_expr_comp(self, t):
        """
        t_var_expr : t_var_expr AND AND t_var_expr
                   | t_var_expr OR OR t_var_expr
        """
        if self.run:
            # Check input maplists and operators.
            tvarexprA  = t[1]
            tvarexprB  = t[4]
            relations = ["EQUAL"]
            temporal = "l"
            function = t[2] + t[3]
            aggregate = t[2]
            # Build conditional values based on topological relationships.
            complist = self.build_spatio_temporal_topology_list(tvarexprA, tvarexprB, topolist=relations,
                                                                compare_bool=True, compop=function[0],
                                                                aggregate=aggregate)
            # Set temporal extent based on topological relationships.
            resultlist = self.set_temporal_extent_list(complist, topolist = relations,
                                                       temporal=temporal)

            t[0] = resultlist

        if self.debug:
            print(t[1], t[2] + t[3], t[4])

    def p_t_var_expr_comp_op(self, t):
        """
        t_var_expr : t_var_expr T_COMP_OPERATOR t_var_expr
        """
        if self.run:
            tvarexprA  = t[1]
            tvarexprB  = t[3]
            # Evaluate temporal comparison operator.
            relations, temporal, function, aggregate = self.eval_toperator(t[2], optype='boolean')
            # Build conditional values based on topological relationships.
            complist = self.build_spatio_temporal_topology_list(tvarexprA, tvarexprB, topolist=relations,
                                                                compare_bool=True, compop=function[0], aggregate=aggregate)
            # Set temporal extent based on topological relationships.
            resultlist = self.set_temporal_extent_list(complist, topolist=relations,
                                                       temporal=temporal)

            t[0] = resultlist

        if self.debug:
            print(t[1], t[2], t[3])

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
            print(str(t[1]), "* = ", t[1], t[2], t[3])

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
                                                         inverse=True)
            # Return map list.
            t[0] = selectlist
        else:
            t[0] = t[1] + "*"

        if self.debug:
            print(t[1] + "* = ", t[1], t[2], t[3])

    def p_expr_t_select_operator(self, t):
        # Temporal equal selection
        # The temporal topology relation equals is implicit
        # Examples:
        #    A {!:} B  # Select the part of A that is temporally unequal to B
        #    A { :} B  # Select the part of A that is temporally equal B
        #    A {!:, equals} B          # Select the part of A that is temporally unequal to B
        #    A {!:, during} B          # Select the part of A that is temporally not during B
        #    A {:, overlaps} B         # Select the part of A that temporally overlaps B
        #    A {:, overlaps|equals} B  # Select the part of A that temporally overlaps or equals B
        """
        expr : stds T_SELECT_OPERATOR stds
             | expr T_SELECT_OPERATOR stds
             | stds T_SELECT_OPERATOR expr
             | expr T_SELECT_OPERATOR expr
        """
        if self.run:
            # Check input stds.
            maplistA = self.check_stds(t[1])
            maplistB = self.check_stds(t[3])
            # Evaluate temporal operator.
            operators  = self.eval_toperator(t[2], optype='select')
            # Check for negative selection.
            if operators[2] == "!:":
                negation = True
            else:
                negation = False
            # Perform selection.
            selectlist = self.perform_temporal_selection(maplistA, maplistB,
                                                         topolist=operators[0],
                                                         inverse=negation)
            selectlist = self.set_granularity(selectlist, maplistB, operators[1],
                                              operators[0])
            # Return map list.
            t[0] = selectlist
        else:
            t[0] = t[1] + "*"

        if self.debug:
            print(t[1] + "* = ", t[1], t[2], t[3])


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
            print(str(t[5]) + "* = ", "if condition", str(t[3]), ' then ', str(t[5]))

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
            topolist     = self.eval_toperator(t[3], optype='relation')[0]
            thencond     = self.build_condition_list(tvarexpr, thenlist, topolist)
            thenresult   = self.eval_condition_list(thencond)
            # Clear the map and conditional values of the map list.
            resultlist   = self.check_stds(thenresult, clear = True)
            # Return resulting map list.
            t[0] = resultlist
        else:
            t[0] = t[7] + "*"

        if self.debug:
            print("result* = ", "if ", str(t[3]), "condition", str(t[5]), " then ", str(t[7]))

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
            combilist    = thenresult + elseresult
            resultlist   = sorted(combilist, key = AbstractDatasetComparisonKeyStartTime)
            # Clear the map and conditional values of the map list.
            resultlist   = self.check_stds(resultlist, clear = True)
            # Return resulting map list.
            t[0] = resultlist
        else:
            t[0] = t[5] + "*"

        if self.debug:
            print(str(t[5]) + "* = ", "if condition", str(t[3]), " then ", str(t[5]), ' else ', str(t[7]))

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
            topolist     = self.eval_toperator(t[3], optype='relation')[0]
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
                print(str(t[7]), "* = ", "if condition", str(t[5]), " then ", str(t[7]), ' else ', str(t[9]))
            else:
                print(str(t[9]), "* = ", "if condition", str(t[5]), " then ", str(t[7]), ' else ', str(t[9]))

    def p_expr_t_buff(self, t):
        # Examples
        # buff_t(A : B, "10 minutes")  # Select the part of A that is temporally
        #                                equal to B and create a buffer of 10 minutes around
        """
        expr : BUFF_T LPAREN stds COMMA QUOTE  number NAME QUOTE RPAREN
             | BUFF_T LPAREN expr COMMA QUOTE  number NAME QUOTE RPAREN
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
                print(str(t[3]) + "* = buff_t(", str(t[3]), ",", '"', str(t[6]), str(t[7]), '"', ")")
            elif len(t) == 7:
                print(str(t[3]) + "* = buff_t(", str(t[3]), ",", str(t[5]), ")")

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
            print(str(t[3]) + "* = tsnap(", str(t[3]), ")")

    def p_expr_t_shift(self, t):
        # Examples
        # tshift(A : B, "10 minutes")  # Shift the selection from A temporally
        #                                by 10 minutes.
        """
        expr : TSHIFT LPAREN stds COMMA QUOTE  number NAME QUOTE RPAREN
             | TSHIFT LPAREN expr COMMA QUOTE  number NAME QUOTE RPAREN
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
            shiftlist = AbstractSpaceTimeDataset.shift_map_list(maplist,
                                                                increment)
            t[0] = shiftlist
        else:
            t[0] = t[3] + "*"

        if self.debug:
            if len(t) == 10:
                print(str(t[3]) + "* = tshift(", str(t[3]), ",", '"', str(t[6]), str(t[7]), '"', ")")
            elif len(t) == 7:
                print(str(t[3]) + "* = tshift(", str(t[3]), ",", str(t[5]), ")")

    # Handle errors.
    def p_error(self, t):
        if t:
            raise SyntaxError("syntax error on line %d, position %i token %s near '%s' expression '%s'" %
                              (t.lineno, t.lexpos, t.type, t.value, self.expression))
        else:
            raise SyntaxError("Unexpected syntax error")

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
