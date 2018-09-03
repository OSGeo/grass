"""@package grass.temporal

Temporal raster algebra

(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Thomas Leppelt and Soeren Gebbert

.. code-block:: python

    >>> p = TemporalRasterAlgebraLexer()
    >>> p.build()
    >>> p.debug = True
    >>> expression =  'R = A {+,equal,l} B'
    >>> p.test(expression)
    R = A {+,equal,l} B
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_ARITH2_OPERATOR,'{+,equal,l}',1,6)
    LexToken(NAME,'B',1,18)
    >>> expression =  'R = A {*,equal|during,r} B'
    >>> p.test(expression)
    R = A {*,equal|during,r} B
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_ARITH1_OPERATOR,'{*,equal|during,r}',1,6)
    LexToken(NAME,'B',1,25)
    >>> expression =  'R = A {+,equal|during} B'
    >>> p.test(expression)
    R = A {+,equal|during} B
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_ARITH2_OPERATOR,'{+,equal|during}',1,6)
    LexToken(NAME,'B',1,23)

"""
from __future__ import print_function

try:
    import ply.lex as lex
    import ply.yacc as yacc
except:
    pass

import copy
import grass.pygrass.modules as pymod
from grass.exceptions import FatalError
from .temporal_algebra import TemporalAlgebraLexer, TemporalAlgebraParser, GlobalTemporalVar
from .core import init_dbif
from .abstract_dataset import AbstractDatasetComparisonKeyStartTime
from .factory import dataset_factory
from .open_stds import open_new_stds
from .spatio_temporal_relationships import SpatioTemporalTopologyBuilder
from .space_time_datasets import Raster3DDataset, RasterDataset
from .temporal_granularity import compute_absolute_time_granularity

from .datetime_math import create_suffix_from_datetime
from .datetime_math import create_time_suffix
from .datetime_math import create_numeric_suffix


##############################################################################

class TemporalRasterAlgebraLexer(TemporalAlgebraLexer):
    """Lexical analyzer for the GRASS GIS temporal algebra"""

    def __init__(self):
        TemporalAlgebraLexer.__init__(self)

    # Supported r.mapcalc functions.
    mapcalc_functions = {
        'exp'     : 'EXP',
        'log'     : 'LOG',
        'sqrt'    : 'SQRT',
        'abs'     : 'ABS',
        'cos'     : 'COS',
        'acos'    : 'ACOS',
        'sin'     : 'SIN',
        'asin'    : 'ASIN',
        'tan'     : 'TAN',
        'double'  : 'DOUBLE',
        'float'   : 'FLOATEXP',
        'int'     : 'INTEXP',
        'isnull'  : 'ISNULL',
        'isntnull': 'ISNTNULL',
        'null'    : 'NULL',
        'exist'   : 'EXIST',
    }

    # Functions that defines single maps with time stamp and without temporal extent.
    map_functions = {'map' : 'MAP'}

    # This is the list of token names.
    raster_tokens = (
        'MOD',
        'DIV',
        'MULT',
        'ADD',
        'SUB',
        'T_ARITH1_OPERATOR',
        'T_ARITH2_OPERATOR',
        'L_SPAREN',
        'R_SPAREN',
    )

    # Build the token list
    tokens = TemporalAlgebraLexer.tokens \
                    + raster_tokens \
                    + tuple(mapcalc_functions.values()) \
                    + tuple(map_functions.values())

    # Regular expression rules for simple tokens
    t_MOD                 = r'[\%]'
    t_DIV                 = r'[\/]'
    t_MULT                = r'[\*]'
    t_ADD                 = r'[\+]'
    t_SUB                 = r'[-]'
    t_T_ARITH1_OPERATOR   = r'\{[\%\*\/][,]?[a-zA-Z\| ]*([,])?([lrudi]|left|right|union|disjoint|intersect)?\}'
    t_T_ARITH2_OPERATOR   = r'\{[+-][,]?[a-zA-Z\| ]*([,])?([lrudi]|left|right|union|disjoint|intersect)?\}'
    t_L_SPAREN            = r'\['
    t_R_SPAREN            = r'\]'

    # Parse symbols
    def temporal_symbol(self, t):
        # Check for reserved words
        if t.value in TemporalRasterAlgebraLexer.time_functions.keys():
            t.type = TemporalRasterAlgebraLexer.time_functions.get(t.value)
        elif t.value in TemporalRasterAlgebraLexer.datetime_functions.keys():
            t.type = TemporalRasterAlgebraLexer.datetime_functions.get(t.value)
        elif t.value in TemporalRasterAlgebraLexer.conditional_functions.keys():
            t.type = TemporalRasterAlgebraLexer.conditional_functions.get(t.value)
        elif t.value in TemporalRasterAlgebraLexer.mapcalc_functions.keys():
            t.type = TemporalRasterAlgebraLexer.mapcalc_functions.get(t.value)
        elif t.value in TemporalRasterAlgebraLexer.map_functions.keys():
            t.type = TemporalRasterAlgebraLexer.map_functions.get(t.value)
        else:
            t.type = 'NAME'
        return t

##############################################################################

class TemporalRasterBaseAlgebraParser(TemporalAlgebraParser):
    """The temporal algebra class"""

    # Get the tokens from the lexer class
    tokens = TemporalRasterAlgebraLexer.tokens

    # Setting equal precedence level for select and hash operations.
    precedence = (
        ('left', 'T_SELECT_OPERATOR', 'T_SELECT', 'T_NOT_SELECT'), # 1
        ('left', 'ADD', 'SUB', 'T_ARITH2_OPERATOR',  'T_HASH_OPERATOR',  'HASH'), #2
        ('left', 'AND', 'OR', 'T_COMP_OPERATOR', 'MOD', 'DIV', 'MULT',
         'T_ARITH1_OPERATOR'))

    def __init__(self, pid=None, run=True,
                 debug=False, spatial=False,
                 register_null=False,
                 dry_run=False, nprocs=1,
                 time_suffix=None):

        TemporalAlgebraParser.__init__(self,
                                       pid=pid,
                                       run=run,
                                       debug=debug,
                                       spatial=spatial,
                                       register_null=register_null,
                                       dry_run=dry_run,
                                       nprocs=nprocs,
                                       time_suffix=time_suffix)

    def check_null(self, t):
        try:
            int(t)
            return t
        except ValueError:
            return "null()"

    ######################### Temporal functions ##############################
    def build_spatio_temporal_topology_list(self, maplistA, maplistB=None, topolist=["EQUAL"],
                                            assign_val=False, count_map=False, compare_bool=False,
                                            compare_cmd=False, compop=None, aggregate=None,
                                            new=False, convert=False, operator_cmd=False):
        """Build temporal topology for two space time data sets, copy map objects
        for given relation into map list.

        :param maplistA: List of maps.
        :param maplistB: List of maps.
        :param topolist: List of strings of temporal relations.
        :param assign_val: Boolean for assigning a boolean map value based on
                        the map_values from the compared map list by
                        topological relationships.
        :param count_map: Boolean if the number of topological related maps
                       should be returned.
        :param compare_bool: Boolean for comparing boolean map values based on
                        related map list and compariosn operator.
        :param compare_cmd: Boolean for comparing command list values based on
                        related map list and compariosn operator.
        :param compop: Comparison operator, && or ||.
        :param aggregate: Aggregation operator for relation map list, & or |.
        :param new: Boolean if new temporary maps should be created.
        :param convert: Boolean if conditional values should be converted to
                    r.mapcalc command strings.
        :param operator_cmd: Boolean for aggregate arithmetic operators implicitly
                    in command list values based on related map lists.

        :return: List of maps from maplistA that fulfil the topological relationships
              to maplistB specified in topolist.

        >>> # Create two list of maps with equal time stamps
        >>> from datetime import datetime
        >>> import grass.temporal as tgis
        >>> tgis.init(True)
        >>> l = tgis.TemporalAlgebraParser()
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

        """
        print(topolist, assign_val, count_map, compare_bool, compare_cmd,
              compop, aggregate, new, convert, operator_cmd)

        # Check the topology definitions and return the list of temporal and spatial
        # topological relations that must be fulfilled
        temporal_topo_list, spatial_topo_list = self._check_topology(topolist=topolist)

        resultdict = {}
        # Create temporal topology for maplistA to maplistB.
        tb = SpatioTemporalTopologyBuilder()
        # Build spatio-temporal topology
        if len(spatial_topo_list) > 0:
            # Dictionary with different spatial variables used for topology builder.
            spatialdict = {'strds' : '2D', 'stvds' : '2D', 'str3ds' : '3D'}
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
            elif compare_cmd:
                self.compare_cmd_value(map_i, compop, aggregate, temporal_topo_list, spatial_topo_list, convert)
            elif operator_cmd:
                self.operator_cmd_value(map_i,  compop, temporal_topo_list, spatial_topo_list)

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
                        # map_i.print_info()

        resultlist = resultdict.values()

        # Sort list of maps chronological.
        resultlist = sorted(resultlist, key=AbstractDatasetComparisonKeyStartTime)

        return(resultlist)

    def build_command_string(self, map_i, relmap, operator = None, cmd_type = None):
        """This function build the r.mapcalc command string for conditionals,
        spatial variable combinations and boolean comparisons.

        For Example: 'if(a1 == 1, b1, c2)' or 'exist(a1) && sin(b1)'

        :param map_i: map object with temporal extent and built relations.
        :param relmap: map object with defined temporal relation to map_i.
        :param operator: String representing operator between two spatial variables
                        (&&,||,+,-,*,/).
        :param cmd_type: map object with defined temporal relation to map_i:
                        condition, conclusion or operator.

        :return: the resulting command string for conditionals or spatial variable
            combinations
        """
        def sub_cmdstring(map_i):
            """This function search for command string in a map object and
            return substitute string (contained commandstring or map name)"""
            if "cmd_list" in dir(map_i):
                map_sub = map_i.cmd_list
            elif "map_value" in dir(map_i) and len(map_i.map_value) > 0 and map_i.map_value[0].get_type() == "timediff":
                map_sub = map_i.map_value[0].get_type_value()[0]
            else:
                try:
                    map_sub = map_i.get_id()
                except:
                    map_sub = map_i
            return(map_sub)

        # Check  for type of operation, conditional or spatial variable combination
        # and Create r.mapcalc expression string for the operation.
        cmdstring = ""
        if cmd_type == 'condition':
            conditionsub = sub_cmdstring(map_i)
            conclusionsub = sub_cmdstring(relmap)
            cmdstring = "if(%s, %s)" %(conditionsub, conclusionsub)
        elif cmd_type == 'conclusion':
            thensub = sub_cmdstring(map_i)
            elsesub = sub_cmdstring(relmap)
            cmdstring = "%s, %s" %(thensub, elsesub)
        elif cmd_type == 'operator':
            leftsub = sub_cmdstring(map_i)
            rightsub = sub_cmdstring(relmap)
            if operator == None:
                self.msgr.fatal("Error: Can't build command string for map %s, operator is missing"
                    %(map_i.get_map_id()))
            cmdstring = "(%s %s %s)" %(leftsub, operator, rightsub)
        return(cmdstring)

    def compare_cmd_value(self, map_i, compop, aggregate,
                          temporal_topo_list = ["EQUAL"],
                          spatial_topo_list = [], convert = False):
        """ Function to evaluate two map lists with boolean values by boolean
        comparison operator.

        R = A && B

        R = if(A < 1 && B > 1, A, B)

        R = if(A < 1 {&&,equal|equivalent} B > 1, A, B)

        Extended temporal algebra version with command
        list builder for temporal raster algebra.

        :param map_i: Map object with temporal extent.
        :param temporal_relations: List of temporal relation to map_i.
        :param temporal_topo_list: List of strings for given temporal relations.
        :param compop: Comparison operator, && or ||.
        :param aggregate: Aggregation operator for relation map list, & or |.
        :param convert: Boolean if conditional values should be converted to
                        r.mapcalc command strings.

        :return: Map object with conditional value that has been evaluated by
                    comparison operators.
        """
        # Build command list list with elements from related maps and given relation operator.
        if convert and "condition_value" in dir(map_i):
            if map_i.condition_value != []:
                cmdstring = str(int(map_i.condition_value[0]))
                map_i.cmd_list = cmdstring
        if "cmd_list" in dir(map_i):
            leftcmd = map_i.cmd_list
            cmd_value_list = [leftcmd]
        count = 0

        temporal_relations = map_i.get_temporal_relations()

        for topo in temporal_topo_list:
            if topo.upper() in temporal_relations.keys():
                relationmaplist = temporal_relations[topo.upper()]
                if count == 0 and "cmd_list" in dir(map_i):
                    cmd_value_list.append(compop)
                    cmd_value_list.append('(')
                for relationmap in relationmaplist:
                    if self._check_spatial_topology_relation(spatial_topo_list, map_i, relationmap) is True:
                        if convert and "condition_value" in dir(relationmap):
                            if relationmap.condition_value != []:
                                cmdstring = str(int(relationmap.condition_value[0]))
                                relationmap.cmd_list = cmdstring
                        if "cmd_list" in dir(relationmap):
                            if count > 0:
                                cmd_value_list.append(aggregate + aggregate)
                            cmd_value_list.append(relationmap.cmd_list)
                            count = count + 1
                        if self.debug:
                            print("compare_cmd_value", map_i.get_id(),
                                  relationmap.get_id(), relationmap.cmd_list)
        if count > 0:
            cmd_value_list.append(')')
            cmd_value_str = ''.join(map(str, cmd_value_list))
            # Add command list to result map.
            map_i.cmd_list = cmd_value_str

            print(cmd_value_str)

            return(cmd_value_str)

    def operator_cmd_value(self,  map_i, operator,
                          temporal_topo_list = ["EQUAL"],
                          spatial_topo_list = []):
        """ Function to evaluate two map lists by given arithmetic operator.

        :param map_i: Map object with temporal extent.
        :param operator: Arithmetic operator, +-*/%.
        :param temporal_topo_list: List of strings for given temporal relations.
        :param spatial_topo_list: List of strings for given spatial relations.

        :return: Map object with command list with operators that has been
                    evaluated by implicit aggregation.
        """

        temporal_relations = map_i.get_temporal_relations()
        spatial_relations = map_i.get_spatial_relations()

        # Build comandlist list with elements from related maps and given relation operator.
        leftcmd = map_i
        cmdstring = ""
        for topo in temporal_topo_list:
            if topo.upper() in temporal_relations.keys():
                relationmaplist = temporal_relations[topo.upper()]
                for relationmap in relationmaplist:
                    if self._check_spatial_topology_relation(spatial_topo_list, map_i, relationmap) is True:
                        # Create r.mapcalc expression string for the operation.
                        cmdstring = self.build_command_string(leftcmd,
                                                              relationmap,
                                                              operator=operator,
                                                              cmd_type="operator")
                        leftcmd = cmdstring

                        if self.debug:
                            print("operator_cmd_value", map_i.get_id(), operator, relationmap.get_id())
        # Add command list to result map.
        map_i.cmd_list = cmdstring

        print("map command string", cmdstring)
        return(cmdstring)

    def set_temporal_extent_list(self, maplist, topolist=["EQUAL"], temporal='l' ,
                                 cmd_bool=False, cmd_type=None,  operator=None):
        """ Change temporal extent of map list based on temporal relations to
        other map list and given temporal operator.

        :param maplist: List of map objects for which relations has been build
                        correctly.
        :param topolist: List of strings of temporal relations.
        :param temporal: The temporal operator specifying the temporal
                         extent operation (intersection, union, disjoint
                         union, right reference, left reference).
        :param cmd_bool: Boolean if command string should be merged for related maps.
        :param cmd_type: map object with defined temporal relation to map_i:
                        condition, conclusion or operator.
        :param operator: String defining the type of operator.

        :return: Map list with specified temporal extent and optional command string.
        """
        resultdict = {}
        temporal_topo_list, spatial_topo_list = self._check_topology(topolist=topolist)

        for map_i in maplist:
            # Loop over temporal related maps and create overlay modules.
            tbrelations = map_i.get_temporal_relations()
            # Generate an intermediate map for the result map list.
            map_new = self.generate_new_map(base_map=map_i,
                                            bool_op='and',
                                            copy=True,
                                            rename=True)

            # Combine temporal and spatial extents of intermediate map with related maps.
            for topo in topolist:
                if topo in tbrelations.keys():
                    for map_j in (tbrelations[topo]):
                        if self._check_spatial_topology_relation(spatial_topo_list, map_i, map_j) is True:
                            if temporal == 'r':
                                # Generate an intermediate map for the result map list.
                                map_new = self.generate_new_map(base_map=map_i,
                                                                bool_op='and',
                                                                copy=True,
                                                                rename=True)
                            # Create overlaid map extent.
                            returncode = self.overlay_map_extent(map_new, map_j,
                                                                 'and',
                                                                 temp_op=temporal)

                            # Stop the loop if no temporal or spatial relationship exist.
                            if returncode == 0:
                                break
                            # Append map to result map list.
                            elif returncode == 1:
                                # print(map_new.cmd_list)
                                # resultlist.append(map_new)
                                if cmd_bool:
                                    # Create r.mapcalc expression string for the operation.
                                    cmdstring = self.build_command_string(map_i,
                                                                          map_j,
                                                                          operator=operator,
                                                                          cmd_type=cmd_type)
                                    # Conditional append of module command.
                                    map_new.cmd_list = cmdstring
                                # Write map object to result dictionary.
                                resultdict[map_new.uid] = map_new
                    if returncode == 0:
                        break
            # Append map to result map list.
            #if returncode == 1:
            #    resultlist.append(map_new)
        # Get sorted map objects as values from result dictionoary.
        resultlist = resultdict.values()
        resultlist = sorted(resultlist, key = AbstractDatasetComparisonKeyStartTime)

        return(resultlist)

    def build_condition_cmd_list(self, iflist, thenlist,  elselist=None,
                                 condition_topolist=["EQUAL"],
                                 conclusion_topolist=["EQUAL"],
                                 temporal='l', null=False):
        """This function build the r.mapcalc command strings for spatial conditionals.
        For Example: 'if(a1 == 1, b1, c2)'

        :param iflist: Map list with temporal extents and command list.
        :param thenlist: Map list with temporal extents and command list or numeric string.
        :param elselist: Map list with temporal extents and command list or numeric string.
        :param condition_topolist: List of strings for given temporal relations between
                                   conditions and conclusions.
        :param conclusion_topolist: List of strings for given temporal relations between
                                    conditions (then and else).
        :param temporal: The temporal operator specifying the temporal
                         extent operation (intersection, union, disjoint
                         union, right reference, left reference).
        :param null: Boolean if null map support should be activated.

        :return: map list with resulting command string for given condition type.
        """
        resultlist = []
        # First merge conclusion command maplists or strings.
        # Check if alternative conclusion map list is given.
        if all([isinstance(thenlist, list), isinstance(elselist, list)]):
            # Build conclusion command map list.
            conclusiontopolist = self.build_spatio_temporal_topology_list(thenlist, elselist,
                                                                          conclusion_topolist)
            conclusionlist = self.set_temporal_extent_list(conclusiontopolist,
                                                           topolist=conclusion_topolist,
                                                           temporal=temporal ,
                                                           cmd_bool=True,
                                                           cmd_type="conclusion")
        # Check if any conclusion is a numeric statements.
        elif any([isinstance(thenlist, str), isinstance(elselist, str)]):
            conclusionlist = []
            # Check if only alternative conclusion is a numeric statements.
            if all([isinstance(thenlist, list), isinstance(elselist, str)]):
                listinput = thenlist
                numinput = elselist
                for map_i in listinput:
                    # Create r.mapcalc expression string for the operation.
                    cmdstring = self.build_command_string(map_i,
                                                          numinput,
                                                          cmd_type='conclusion')
                    # Conditional append of module command.
                    map_i.cmd_list = cmdstring
                    # Append map to result map list.
                    conclusionlist.append(map_i)
            # Check if only direct conclusion is a numeric statements.
            elif all([isinstance(thenlist, str), isinstance(elselist, list)]):
                listinput = elselist
                numinput =  thenlist
                for map_i in listinput:
                    # Create r.mapcalc expression string for the operation.
                    cmdstring = self.build_command_string(numinput,
                                                          map_i,
                                                          cmd_type='conclusion')
                    # Conditional append of module command.
                    map_i.cmd_list = cmdstring
                    # Append map to result map list.
                    conclusionlist.append(map_i)
            elif all([isinstance(thenlist, str), isinstance(elselist, str)]):
                conclusionlist = thenlist + ',' + elselist
        else:
            # The direct conclusion is used.
            conclusionlist = thenlist
        # Use the conclusion map or string to merge it with the condition and
        # return maplist.
        if isinstance(conclusionlist,  str):
            resultlist = []
            for map_i in iflist:
                # Create r.mapcalc expression string for the operation.
                cmdstring = self.build_command_string(map_i,
                                                      conclusionlist,
                                                      cmd_type='condition')
                # Conditional append of module command.
                map_i.cmd_list = cmdstring
                # Append map to result map list.
                resultlist.append(map_i)
            return(resultlist)
        elif isinstance(conclusionlist,  list):
            # Build result command map list between conditions and conclusions.
            print("build_condition_cmd_list", condition_topolist)
            conditiontopolist = self.build_spatio_temporal_topology_list(iflist,
                                                                         conclusionlist,
                                                                         topolist=condition_topolist)
            resultlist = self.set_temporal_extent_list(conditiontopolist,
                                                       topolist=condition_topolist,
                                                       temporal='r',
                                                       cmd_bool=True,
                                                       cmd_type="condition")
            return(resultlist)

    ###########################################################################

    def p_statement_assign(self, t):
        # This function executes the processing of raster/raster3d algebra
        # that was build based on the expression
        """
        statement : stds EQUALS expr
        """
        if self.run:
            # Create the process queue for parallel mapcalc processing
            if self.dry_run is False:
                process_queue = pymod.ParallelModuleQueue(int(self.nprocs))

            if isinstance(t[3], list):

                granularity = None
                if len(t[3]) > 0 and self.time_suffix == 'gran':
                    map_i = t[3][0]
                    if map_i.is_time_absolute() is True:
                        granularity = compute_absolute_time_granularity(t[3])

                # The first loop is to check if the raster maps exists in the database
                # Compute the size of the numerical suffix
                num = len(t[3])
                register_list = []
                leadzero = len(str(num))
                for i in range(num):
                    map_i = t[3][i]

                    # Create new map with basename
                    newident = create_numeric_suffix(self.basename, i, "%0" + str(leadzero))

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

                    # Check if resultmap names exist in GRASS database.
                    newident = newident + "@" + self.mapset

                    if self.stdstype == "strds":
                        new_map = RasterDataset(newident)
                    else:
                        new_map = Raster3DDataset(newident)
                    if new_map.map_exists() and self.overwrite is False:
                        self.msgr.fatal("Error maps with basename %s exist. "
                                        "Use --o flag to overwrite existing file"%newident)

                # The second loop creates the resulting raster maps
                count = 0
                map_test_list = []
                for map_i in t[3]:

                    # Create new map with basename
                    newident = create_numeric_suffix(self.basename, count,
                                                     "%0" + str(leadzero))

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

                    if "cmd_list" in dir(map_i):
                        # Build r.mapcalc module and execute expression.
                        # Change map name to given basename.
                        # Create deepcopy of r.mapcalc module.

                        new_map = map_i.get_new_instance(newident + "@" + self.mapset)
                        new_map.set_temporal_extent(map_i.get_temporal_extent())
                        new_map.set_spatial_extent(map_i.get_spatial_extent())
                        map_test_list.append(new_map)

                        m = copy.deepcopy(self.m_mapcalc)
                        m_expression = newident + "=" + map_i.cmd_list
                        m.inputs["expression"].value = str(m_expression)
                        m.flags["overwrite"].value = self.overwrite
                        if self.debug:
                            print(m.get_bash())
                        self.process_chain_dict["processes"].append(m.get_dict())

                        if self.dry_run is False:
                            process_queue.put(m)

                    elif map_i.map_exists():
                        # Copy map if it exists b = a
                        new_map = map_i.get_new_instance(newident + "@" + self.mapset)
                        new_map.set_temporal_extent(map_i.get_temporal_extent())
                        new_map.set_spatial_extent(map_i.get_spatial_extent())
                        map_test_list.append(new_map)

                        m = copy.deepcopy(self.m_mapcalc)
                        m_expression = newident + "=" + map_i.get_map_id()
                        m.inputs["expression"].value = str(m_expression)
                        m.flags["overwrite"].value = self.overwrite
                        if self.debug:
                            print(m.get_bash())
                        self.process_chain_dict["processes"].append(m.get_dict())

                        if self.dry_run is False:
                            process_queue.put(m)

                    else:
                        self.msgr.error(_("Error computing map <%s>"%map_i.get_id()))
                    count += 1

                if self.dry_run is False:
                    process_queue.wait()

                for map_i in map_test_list:
                    register_list.append(map_i)

                # Open connection to temporal database.
                dbif, connect = init_dbif(self.dbif)

                # Create result space time dataset.
                if self.dry_run is False:
                    resultstds = open_new_stds(t[1], self.stdstype,
                                               'absolute', t[1], t[1],
                                               'mean', self.dbif,
                                               overwrite = self.overwrite)
                for map_i in register_list:

                    # Put the map into the process dictionary
                    start, end = map_i.get_temporal_extent_as_tuple()
                    self.process_chain_dict["register"].append((map_i.get_name(),
                                                                str(start),
                                                                str(end)))

                    if self.dry_run is False:
                        # Get meta data from grass database.
                        map_i.load()
                        # Do not register empty maps if not required
                        # In case of a null map continue, do not register null maps
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
                        self.msgr.fatal("Error raster map %s exist in temporal database. "
                                        "Use overwrite flag."%map_i.get_map_id())
                    else:
                        # Insert map into temporal database.
                        if self.dry_run is False:
                            map_i.insert(dbif)
                    # Register map in result space time dataset.
                    if self.dry_run is False:
                        success = resultstds.register_map(map_i, dbif)

                if self.dry_run is False:
                    resultstds.update_from_registered_maps(dbif)

                self.process_chain_dict["STDS"]["name"] = t[1]
                self.process_chain_dict["STDS"]["stdstype"] = self.stdstype
                self.process_chain_dict["STDS"]["temporal_type"] = 'absolute'

                dbif.close()
                t[0] = register_list
                # Remove intermediate maps
                self.remove_maps()

    def p_expr_spmap_function(self, t):
        # Add a single map.
        # Only the spatial extent of the map is evaluated.
        # Temporal extent is not existing.
        # Examples:
        #    R = map(A)
        """
        mapexpr : MAP LPAREN stds RPAREN
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
                if map_i.map_exists() == False:
                    raise FatalError(_("%s map <%s> not found in GRASS spatial database") %
                        (map_i.get_type(), id_input))
                else:
                    # Select dataset entry from database.
                    map_i.select(dbif=self.dbif)
                    # Create command list for map object.
                    cmdstring = "(%s)" %(map_i.get_map_id())
                    map_i.cmd_list = cmdstring
            # Return map object.
            t[0] = cmdstring
        else:
            t[0] = "map(" + t[3] + ")"

        if self.debug:
            print("map(" + t[3] + ")")

    def p_arith1_operation(self, t):
        # A % B
        # A / B
        # A * B
        # A % td(B)
        # A * td(B)
        # A / td(B)
        """
        expr : stds MOD  stds
             | expr MOD  stds
             | stds MOD  expr
             | expr MOD  expr
             | stds DIV  stds
             | expr DIV  stds
             | stds DIV  expr
             | expr DIV  expr
             | stds MULT stds
             | expr MULT stds
             | stds MULT expr
             | expr MULT expr
             | stds MOD  t_td_var
             | expr MOD  t_td_var
             | stds DIV  t_td_var
             | expr DIV  t_td_var
             | stds MULT t_td_var
             | expr MULT t_td_var
        """
        # Check input stds.
        maplistA = self.check_stds(t[1])
        maplistB = self.check_stds(t[3])

        topolist = self.build_spatio_temporal_topology_list(maplistA, maplistB)

        if self.run:
            resultlist = []
            for map_i in topolist:
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i,
                                                bool_op='and',
                                                copy=True)
                # Loop over temporal related maps and create overlay modules.
                tbrelations = map_i.get_temporal_relations()
                count = 0
                for map_j in (tbrelations['EQUAL']):
                    # Create overlaid map extent.
                    returncode = self.overlay_map_extent(map_new, map_j,
                                                         'and',
                                                         temp_op='l')
                    # Stop the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        break
                    if count == 0:
                        # Set map name.
                        name = map_new.get_id()
                    else:
                        # Generate an intermediate map
                        name = self.generate_map_name()

                    # Create r.mapcalc expression string for the operation.
                    cmdstring = self.build_command_string(map_i, map_j,
                                                          operator=t[2],
                                                          cmd_type="operator")
                    # Conditional append of module command.
                    map_new.cmd_list = cmdstring
                    count += 1
                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_arith1_operation_numeric1(self, t):
        # A % 1
        # A / 4
        # A * 5
        # A % map(b1)
        # A * map(b2)
        # A / map(b3)
        """
        expr : stds MOD  number
             | expr MOD  number
             | stds DIV  number
             | expr DIV  number
             | stds MULT number
             | expr MULT number
             | stds MOD  numberstr
             | expr MOD  numberstr
             | stds DIV  numberstr
             | expr DIV  numberstr
             | stds MULT numberstr
             | expr MULT numberstr
             | stds MOD  mapexpr
             | expr MOD  mapexpr
             | stds DIV  mapexpr
             | expr DIV  mapexpr
             | stds MULT mapexpr
             | expr MULT mapexpr
        """
        # Check input stds.
        maplist = self.check_stds(t[1])

        if self.run:
            resultlist = []
            for map_i in maplist:
                mapinput = map_i.get_id()
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_i):
                    cmdstring = "(%s %s %s)" %(map_i.cmd_list, t[2], t[3])
                else:
                    cmdstring = "(%s %s %s)" %(mapinput, t[2], t[3])
                # Conditional append of module command.
                map_i.cmd_list = cmdstring
                # Append map to result map list.
                resultlist.append(map_i)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)


    def p_arith1_operation_numeric2(self, t):
        # 1 % A
        # 4 / A
        # 5 * A
        # map(b1) % A
        # map(b4) / A
        # map(b5) * A
        """
        expr : number    MOD  stds
             | number    MOD  expr
             | number    DIV  stds
             | number    DIV  expr
             | number    MULT stds
             | number    MULT expr
             | numberstr MOD  stds
             | numberstr MOD  expr
             | numberstr DIV  stds
             | numberstr DIV  expr
             | numberstr MULT stds
             | numberstr MULT expr
             | mapexpr   MOD  stds
             | mapexpr   MOD  expr
             | mapexpr   DIV  stds
             | mapexpr   DIV  expr
             | mapexpr   MULT stds
             | mapexpr   MULT expr
        """
        # Check input stds.
        maplist = self.check_stds(t[3])

        if self.run:
            resultlist = []
            for map_i in maplist:
                mapinput = map_i.get_id()
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_i):
                    cmdstring = "(%s %s %s)" %(t[1], t[2], map_i.cmd_list)
                else:
                    cmdstring = "(%s %s %s)" %(t[1], t[2], mapinput)
                # Conditional append of module command.
                map_i.cmd_list = cmdstring
                # Append map to result map list.
                resultlist.append(map_i)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)


    def p_arith2_operation(self, t):
        # A + B
        # A - B
        # A + td(B)
        # A - td(B)
        """
        expr : stds ADD stds
             | expr ADD stds
             | stds ADD expr
             | expr ADD expr
             | stds SUB stds
             | expr SUB stds
             | stds SUB expr
             | expr SUB expr
             | stds ADD t_td_var
             | expr ADD t_td_var
             | expr SUB t_td_var
             | stds SUB t_td_var

        """
        # Check input stds.
        maplistA = self.check_stds(t[1])
        maplistB = self.check_stds(t[3])
        topolist = self.build_spatio_temporal_topology_list(maplistA, maplistB)

        if self.run:
            resultlist = []
            for map_i in topolist:
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i,
                                                bool_op='and',
                                                copy=True)

                # Loop over temporal related maps and create overlay modules.
                tbrelations = map_i.get_temporal_relations()
                count = 0
                for map_j in (tbrelations['EQUAL']):
                    # Create overlaid map extent.
                    returncode = self.overlay_map_extent(map_new,
                                                         map_j,
                                                         'and',
                                                         temp_op='l')
                    # Stop the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        break
                    if count == 0:
                        # Set map name.
                        name = map_new.get_id()
                    else:
                        # Generate an intermediate map
                        name = self.generate_map_name()

                    # Create r.mapcalc expression string for the operation.
                    cmdstring = self.build_command_string(map_i,
                                                          map_j,
                                                          operator=t[2],
                                                          cmd_type="operator")
                    # Conditional append of module command.
                    map_new.cmd_list = cmdstring
                    count += 1

                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_arith2_operation_numeric1(self, t):
        # A + 2
        # A - 3
        # A + map(b4)
        # A - map(b5)
        """
        expr : stds ADD number
             | expr ADD number
             | stds SUB number
             | expr SUB number
             | stds ADD numberstr
             | expr ADD numberstr
             | stds SUB numberstr
             | expr SUB numberstr
             | stds ADD mapexpr
             | expr ADD mapexpr
             | stds SUB mapexpr
             | expr SUB mapexpr
        """
        # Check input stds.
        maplist = self.check_stds(t[1])

        if self.run:
            resultlist = []
            for map_i in maplist:
                mapinput = map_i.get_id()
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_i):
                    cmdstring = "(%s %s %s)" %(map_i.cmd_list, t[2], t[3])
                else:
                    cmdstring = "(%s %s %s)" %(mapinput, t[2], t[3])
                # Conditional append of module command.
                map_i.cmd_list = cmdstring
                # Append map to result map list.
                resultlist.append(map_i)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_arith2_operation_numeric2(self, t):
        # 2 + A
        # 3 - A
        # map(b2) + A
        # map(b3) - A
        """
        expr : number    ADD stds
             | number    ADD expr
             | number    SUB stds
             | number    SUB expr
             | numberstr ADD stds
             | numberstr ADD expr
             | numberstr SUB stds
             | numberstr SUB expr
             | mapexpr   ADD stds
             | mapexpr   ADD expr
             | mapexpr   SUB stds
             | mapexpr   SUB expr
        """
        # Check input stds.
        maplist = self.check_stds(t[3])

        if self.run:
            resultlist = []
            for map_i in maplist:
                mapinput = map_i.get_id()
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_i):
                    cmdstring = "(%s %s %s)" %(t[1], t[2], map_i.cmd_list)
                else:
                    cmdstring = "(%s %s %s)" %(t[1], t[2], mapinput)
                # Conditional append of module command.
                map_i.cmd_list = cmdstring
                # Append map to result map list.
                resultlist.append(map_i)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_arith1_operation_relation(self, t):
        # A {*, equal, l} B
        # A {*, equal, l} td(B)
        # A {*, equal, l} B {/, during, r} C
        # A {*, equal, l} B {/, equal, l}  C {/, during, r} D
        """
        expr : stds T_ARITH1_OPERATOR stds
             | expr T_ARITH1_OPERATOR stds
             | stds T_ARITH1_OPERATOR expr
             | expr T_ARITH1_OPERATOR expr
             | stds T_ARITH1_OPERATOR t_td_var
             | expr T_ARITH1_OPERATOR t_td_var
        """
        if self.run:
            # Check input stds.
            maplistA = self.check_stds(t[1])
            maplistB = self.check_stds(t[3])
            relations, temporal, function, aggregate = self.eval_toperator(t[2], optype='raster')
            # Build conditional values based on topological relationships.
            complist = self.build_spatio_temporal_topology_list(maplistA,
                                                                maplistB,
                                                                topolist=relations,
                                                                operator_cmd=True,
                                                                compop=function)
            # Set temporal extent based on topological relationships.
            resultlist = self.set_temporal_extent_list(complist,
                                                       topolist=relations,
                                                       temporal=temporal)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_arith2_operation_relation(self, t):
        # A {+, equal, l} B
        # A {+, equal, l} td(b)
        # A {+, equal, l} B {-, during, r} C
        # A {+, equal, l} B {+, equal, l}  C {-, during, r} D
        """
        expr : stds T_ARITH2_OPERATOR stds
             | expr T_ARITH2_OPERATOR stds
             | stds T_ARITH2_OPERATOR expr
             | expr T_ARITH2_OPERATOR expr
             | stds T_ARITH2_OPERATOR t_td_var
             | expr T_ARITH2_OPERATOR t_td_var
        """
        if self.run:
            # Check input stds.
            maplistA = self.check_stds(t[1])
            maplistB = self.check_stds(t[3])
            relations, temporal, function, aggregate = self.eval_toperator(t[2], optype='raster')
            # Build conditional values based on topological relationships.
            complist = self.build_spatio_temporal_topology_list(maplistA,
                                                                maplistB,
                                                                topolist=relations,
                                                                operator_cmd=True,
                                                                compop=function)
            # Set temporal extent based on topological relationships.
            resultlist = self.set_temporal_extent_list(complist,
                                                       topolist=relations,
                                                       temporal=temporal)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_arith_operation_numeric_string(self, t):
        # 1 + 1
        # 1 - 1
        # 1 * 1
        # 1 / 1
        # 1 % 1
        """
        numberstr : number ADD number
                  | number SUB number
                  | number DIV number
                  | number MULT number
                  | number MOD number
        """
        numstring = "(%s %s %s)" %(t[1], t[2], t[3])

        t[0] = numstring

        if self.debug:
            print(numstring)

    def p_mapcalc_function(self, t):
        # Supported mapcalc functions.
        """
        mapcalc_arith : ABS
                      | LOG
                      | SQRT
                      | EXP
                      | COS
                      | ACOS
                      | SIN
                      | ASIN
                      | TAN
                      | DOUBLE
                      | FLOATEXP
                      | INTEXP
        """
        t[0] = t[1]

        if self.debug:
            print(t[1])


    def p_mapcalc_operation1(self, t):
        # sin(A)
        # log(B)
        """
        expr : mapcalc_arith LPAREN stds RPAREN
             | mapcalc_arith LPAREN expr RPAREN
        """
        # Check input stds.
        maplist = self.check_stds(t[3])

        if self.run:
            resultlist = []
            for map_i in maplist:
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_i):
                    cmdstring = "%s(%s)" %(t[1].lower(), map_i.cmd_list)
                else:
                    cmdstring = "%s(%s)" %(t[1].lower(), map_i.get_id())
                # Set new command list for map.
                map_i.cmd_list = cmdstring
                # Append map with updated command list to result list.
                resultlist.append(map_i)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_mapexpr_operation(self, t):
        # sin(map(a))
        """
        mapexpr : mapcalc_arith LPAREN mapexpr RPAREN
        """
        # Check input stds.
        mapstring = t[3]

        if self.run:
            cmdstring = "%s(%s)" %(t[1].lower(), mapstring)

            t[0] = cmdstring

        if self.debug:
            print(mapstring)

    def p_s_var_expr_1(self, t):
        #   isnull(A)
        """
        s_var_expr : ISNULL LPAREN stds RPAREN
                   | ISNULL LPAREN expr RPAREN
        """
        # Check input stds.
        maplist = self.check_stds(t[3])

        if self.run:
            resultlist = []
            for map_i in maplist:
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_i):
                    cmdstring = "%s(%s)" %(t[1].lower(), map_i.cmd_list)
                else:
                    cmdstring = "%s(%s)" %(t[1].lower(), map_i.get_id())
                # Set new command list for map.
                map_i.cmd_list = cmdstring
                # Append map with updated command list to result list.
                resultlist.append(map_i)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_var_expr_2(self, t):
        #   isntnull(A)
        """
        s_var_expr : ISNTNULL LPAREN stds RPAREN
                   | ISNTNULL LPAREN expr RPAREN
        """
        # Check input stds.
        maplist = self.check_stds(t[3])

        if self.run:
            resultlist = []
            for map_i in maplist:
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_i):
                    cmdstring = "!isnull(%s)" %(map_i.cmd_list)
                else:
                    cmdstring = "!isnull(%s)" %(map_i.get_id())
                # Set new command list for map.
                map_i.cmd_list = cmdstring
                # Append map with updated command list to result list.
                resultlist.append(map_i)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_var_expr_3(self, t):
        #   A <= 2
        """
        s_var_expr : stds comp_op number
                   | expr comp_op number
        """
        # Check input stds.
        maplist = self.check_stds(t[1])

        if self.run:
            resultlist = []
            for map_i in maplist:
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_i):
                    cmdstring = "%s %s %s" %(map_i.cmd_list, t[2], t[3])
                else:
                    cmdstring = "%s %s %s" %(map_i.get_id(), t[2], t[3])
                # Set new command list for map.
                map_i.cmd_list = cmdstring
                # Append map with updated command list to result list.
                resultlist.append(map_i)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_var_expr_4(self, t):
        #   exist(B)
        """
        s_var_expr : EXIST LPAREN stds RPAREN
                   | EXIST LPAREN expr RPAREN
        """
        # Check input stds.
        maplist = self.check_stds(t[3])

        if self.run:
            resultlist = []
            for map_i in maplist:
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_i):
                    cmdstring = "%s" %(map_i.cmd_list)
                else:
                    cmdstring = "%s" %(map_i.get_id())
                # Set new command list for map.
                map_i.cmd_list = cmdstring
                # Append map with updated command list to result list.
                resultlist.append(map_i)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_var_expr_comp(self, t):
        #   A <= 2 || B == 10
        #   A < 3 && A > 1
        """
        s_var_expr : s_var_expr AND AND s_var_expr
                   | s_var_expr OR  OR  s_var_expr
        """
        if self.run:
            # Check input stds.
            s_var_exprA = self.check_stds(t[1])
            s_var_exprB = self.check_stds(t[4])
            relations = ["EQUAL"]
            temporal = "l"
            function = t[2] + t[3]
            aggregate = t[2]
            # Build conditional values based on topological relationships.
            complist = self.build_spatio_temporal_topology_list(s_var_exprA,
                                                                s_var_exprB,
                                                                topolist=relations,
                                                                compare_cmd=True,
                                                                compop=function,
                                                                aggregate=aggregate)
            # Set temporal extent based on topological relationships.
            resultlist = self.set_temporal_extent_list(complist,
                                                       topolist=relations,
                                                       temporal=temporal)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_var_expr_comp_op(self, t):
        #   A <= 2 {||} B == 10
        #   A < 3 {&&, equal} A > 1
        """
        s_var_expr : s_var_expr T_COMP_OPERATOR s_var_expr
        """
        if self.run:
            # Check input stds.
            s_var_exprA = self.check_stds(t[1])
            s_var_exprB = self.check_stds(t[3])
            # Evaluate temporal comparison operator.
            relations, temporal, function, aggregate = self.eval_toperator(t[2], optype='boolean')
            # Build conditional values based on topological relationships.
            complist = self.build_spatio_temporal_topology_list(s_var_exprA,
                                                                s_var_exprB,
                                                                topolist=relations,
                                                                compare_cmd=True,
                                                                compop=function,
                                                                aggregate=aggregate)
            # Set temporal extent based on topological relationships.
            resultlist = self.set_temporal_extent_list(complist,
                                                       topolist=relations,
                                                       temporal=temporal)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_expr_condition_if(self, t):
        #   if(s_var_expr, B)
        #   if(A == 1, B)
        """
        expr : IF LPAREN s_var_expr  COMMA stds RPAREN
             | IF LPAREN s_var_expr  COMMA expr RPAREN
             | IF LPAREN ts_var_expr COMMA stds RPAREN
             | IF LPAREN ts_var_expr COMMA expr RPAREN
        """
        ifmaplist = self.check_stds(t[3])
        thenmaplist = self.check_stds(t[5])
        resultlist = self.build_condition_cmd_list(ifmaplist,
                                                   thenmaplist,
                                                   elselist=None,
                                                   condition_topolist=["EQUAL"],
                                                   conclusion_topolist=["EQUAL"],
                                                   temporal='r',
                                                   null=False)
        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_numeric_condition_if(self, t):
        #   if(s_var_expr, 1)
        #   if(A == 5, 10)
        """
        expr : IF LPAREN s_var_expr  COMMA number RPAREN
             | IF LPAREN s_var_expr  COMMA NULL   LPAREN RPAREN RPAREN
             | IF LPAREN ts_var_expr COMMA number RPAREN
             | IF LPAREN ts_var_expr COMMA NULL   LPAREN RPAREN RPAREN
        """
        ifmaplist = self.check_stds(t[3])
        resultlist = []
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 7:
            numinput = str(t[5])
        elif len(t) == 9:
            numinput = str(t[5] + t[6] + t[7])
        # Iterate over condition map list.
        for map_i in ifmaplist:
            # Create r.mapcalc expression string for the operation.
            cmdstring = self.build_command_string(map_i, numinput,
                                                  cmd_type='condition')
            # Conditional append of module command.
            map_i.cmd_list = cmdstring
            # Append map to result map list.
            resultlist.append(map_i)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_expr_condition_if_relation(self, t):
        #   if({equal||during}, s_var_expr, A)
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA expr RPAREN
        """
        relations, temporal, function,  aggregation = self.eval_toperator(t[3],
                                                                          optype='relation')
        ifmaplist = self.check_stds(t[5])
        thenmaplist = self.check_stds(t[7])
        resultlist = self.build_condition_cmd_list(ifmaplist,
                                                   thenmaplist,
                                                   elselist=None,
                                                   condition_topolist=relations,
                                                   conclusion_topolist=["EQUAL"],
                                                   temporal='r',
                                                   null=False)
        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_expr_condition_elif(self, t):
        #   if(s_var_expr, A, B)
        """
        expr : IF LPAREN s_var_expr  COMMA stds COMMA stds RPAREN
             | IF LPAREN s_var_expr  COMMA stds COMMA expr RPAREN
             | IF LPAREN s_var_expr  COMMA expr COMMA stds RPAREN
             | IF LPAREN s_var_expr  COMMA expr COMMA expr RPAREN
             | IF LPAREN ts_var_expr COMMA stds COMMA stds RPAREN
             | IF LPAREN ts_var_expr COMMA stds COMMA expr RPAREN
             | IF LPAREN ts_var_expr COMMA expr COMMA stds RPAREN
             | IF LPAREN ts_var_expr COMMA expr COMMA expr RPAREN
        """
        # Check map list inputs.
        ifmaplist = self.check_stds(t[3])
        thenmaplist = self.check_stds(t[5])
        elsemaplist = self.check_stds(t[7])
        # Create conditional command map list.
        resultlist = self.build_condition_cmd_list(ifmaplist,
                                                   thenmaplist,
                                                   elselist=elsemaplist,
                                                   condition_topolist=["EQUAL"],
                                                   conclusion_topolist=["EQUAL"],
                                                   temporal='r',
                                                   null=False)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_numeric_condition_elif(self, t):
        #   if(s_var_expr, 1, 2)
        #   if(A == 5, 10, 0)
        """
        expr : IF LPAREN s_var_expr  COMMA number COMMA  number RPAREN
             | IF LPAREN s_var_expr  COMMA NULL   LPAREN RPAREN COMMA  number RPAREN
             | IF LPAREN s_var_expr  COMMA number COMMA  NULL   LPAREN RPAREN RPAREN
             | IF LPAREN s_var_expr  COMMA NULL   LPAREN RPAREN COMMA  NULL   LPAREN RPAREN RPAREN
             | IF LPAREN ts_var_expr COMMA number COMMA  number RPAREN
             | IF LPAREN ts_var_expr COMMA NULL   LPAREN RPAREN COMMA  number RPAREN
             | IF LPAREN ts_var_expr COMMA number COMMA  NULL   LPAREN RPAREN RPAREN
             | IF LPAREN ts_var_expr COMMA NULL   LPAREN RPAREN COMMA  NULL   LPAREN RPAREN RPAREN
        """
        ifmaplist = self.check_stds(t[3])
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 9:
            numthen = t[5]
            numelse = t[7]
        elif len(t) == 11 and t[6] == '(':
            numthen = t[5] + t[6] + t[7]
            numelse = t[9]
        elif len(t) == 11 and t[6] == ',':
            numthen = t[5]
            numelse = t[7] + t[8] + t[9]
        elif len(t) == 13:
            numthen = t[5] + t[6] + t[7]
            numelse = t[9] + t[10] + t[11]
        numthen = str(numthen)
        numelse = str(numelse)
        print(numthen + " " +numelse )
        # Create conditional command map list.
        resultlist = self.build_condition_cmd_list(ifmaplist,
                                                   numthen,
                                                   numelse,
                                                   condition_topolist=["EQUAL"],
                                                   conclusion_topolist=["EQUAL"],
                                                   temporal='r',
                                                   null=False)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_numeric_expr_condition_elif(self, t):
        #   if(s_var_expr, 1, A)
        #   if(A == 5 && C > 5, A, null())
        """
        expr : IF LPAREN s_var_expr  COMMA number COMMA  stds   RPAREN
             | IF LPAREN s_var_expr  COMMA NULL   LPAREN RPAREN COMMA  stds   RPAREN
             | IF LPAREN s_var_expr  COMMA number COMMA  expr   RPAREN
             | IF LPAREN s_var_expr  COMMA NULL   LPAREN RPAREN COMMA  expr   RPAREN
             | IF LPAREN s_var_expr  COMMA stds   COMMA  number RPAREN
             | IF LPAREN s_var_expr  COMMA stds   COMMA  NULL   LPAREN RPAREN RPAREN
             | IF LPAREN s_var_expr  COMMA expr   COMMA  number RPAREN
             | IF LPAREN s_var_expr  COMMA expr   COMMA  NULL   LPAREN RPAREN RPAREN
             | IF LPAREN ts_var_expr COMMA number COMMA  stds   RPAREN
             | IF LPAREN ts_var_expr COMMA NULL   LPAREN RPAREN COMMA  stds   RPAREN
             | IF LPAREN ts_var_expr COMMA number COMMA  expr   RPAREN
             | IF LPAREN ts_var_expr COMMA NULL   LPAREN RPAREN COMMA  expr   RPAREN
             | IF LPAREN ts_var_expr COMMA stds   COMMA  number RPAREN
             | IF LPAREN ts_var_expr COMMA stds   COMMA  NULL   LPAREN RPAREN RPAREN
             | IF LPAREN ts_var_expr COMMA expr   COMMA  number RPAREN
             | IF LPAREN ts_var_expr COMMA expr   COMMA  NULL   LPAREN RPAREN RPAREN
        """
        ifmaplist = self.check_stds(t[3])
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 9:
            if isinstance(t[5],  int):
                theninput = str(t[5])
                elseinput = self.check_stds(t[7])
            elif isinstance(t[7],  int):
                theninput = self.check_stds(t[5])
                elseinput = str(t[7])
        elif len(t) == 11:
            if t[5] == 'null':
                theninput = str(t[5] + t[6] + t[7])
                elseinput = self.check_stds(t[9])
            elif t[7] == 'null':
                theninput = self.check_stds(t[5])
                elseinput = str(t[7] + t[8] + t[9])

        # Create conditional command map list.
        resultlist = self.build_condition_cmd_list(ifmaplist,
                                                   theninput,
                                                   elseinput,
                                                   condition_topolist=["EQUAL"],
                                                   conclusion_topolist=["EQUAL"],
                                                   temporal='r',
                                                   null=False)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_numeric_expr_condition_elif_relation(self, t):
        #   if({during},s_var_expr, 1, A)
        #   if({during}, A == 5, A, null())
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA number COMMA  stds   RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA NULL   LPAREN RPAREN COMMA  stds   RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA number COMMA  expr   RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA NULL   LPAREN RPAREN COMMA  expr   RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA stds   COMMA  number RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA stds   COMMA  NULL   LPAREN RPAREN RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA expr   COMMA  number RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA expr   COMMA  NULL   LPAREN RPAREN RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA number COMMA  stds   RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA NULL   LPAREN RPAREN COMMA  stds   RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA number COMMA  expr   RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA NULL   LPAREN RPAREN COMMA  expr   RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA stds   COMMA  number RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA stds   COMMA  NULL   LPAREN RPAREN RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA expr   COMMA  number RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA expr   COMMA  NULL   LPAREN RPAREN RPAREN
        """
        relations, temporal, function,  aggregation = self.eval_toperator(t[3], optype='relation')
        ifmaplist = self.check_stds(t[5])
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 11:
            if isinstance(t[7],  int):
                theninput = str(t[7])
                elseinput = self.check_stds(t[9])
            elif isinstance(t[9],  int):
                theninput = self.check_stds(t[7])
                elseinput = str(t[9])
        elif len(t) == 13:
            if t[7] == 'null':
                theninput = str(t[7] + t[8] + t[9])
                elseinput = self.check_stds(t[11])
            elif t[9] == 'null':
                theninput = self.check_stds(t[7])
                elseinput = str(t[9] + t[10] + t[11])

        # Create conditional command map list.
        resultlist = self.build_condition_cmd_list(ifmaplist,
                                                   theninput,
                                                   elseinput,
                                                   condition_topolist=relations,
                                                   conclusion_topolist=["EQUAL"],
                                                   temporal='r',
                                                   null=False)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_s_expr_condition_elif_relation(self, t):
        #   if({equal||during}, s_var_expr, A, B)
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA stds COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA stds COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA expr COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr  COMMA expr COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA stds COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA stds COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA expr COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA expr COMMA expr RPAREN
        """
        relations, temporal, function, aggregation = self.eval_toperator(t[3], optype='relation')
        ifmaplist = self.check_stds(t[5])
        thenmaplist = self.check_stds(t[7])
        elsemaplist = self.check_stds(t[9])

        # Create conditional command map list.
        resultlist = self.build_condition_cmd_list(ifmaplist,
                                                   thenmaplist,
                                                   elsemaplist,
                                                   condition_topolist=relations,
                                                   conclusion_topolist=["EQUAL"],
                                                   temporal='r',
                                                   null=False)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

    def p_ts_var_expr1(self, t):
        # Combination of spatial and temporal conditional expressions.
        # Examples:
        #   A <= 2 || start_date <= 2013-01-01
        #   end_date > 2013-01-15 && A > 10
        #  IMPORTANT: Only the intersection of map lists in conditionals are
        #  exported.
        """
        ts_var_expr : s_var_expr  AND AND t_var_expr
                    | t_var_expr  AND AND s_var_expr
                    | t_var_expr  OR  OR  s_var_expr
                    | s_var_expr  OR  OR  t_var_expr
                    | ts_var_expr AND AND s_var_expr
                    | ts_var_expr AND AND t_var_expr
                    | ts_var_expr OR  OR  s_var_expr
                    | ts_var_expr OR  OR  t_var_expr
                    | s_var_expr  AND AND ts_var_expr
                    | t_var_expr  AND AND ts_var_expr
                    | s_var_expr  OR  OR  ts_var_expr
                    | t_var_expr  OR  OR  ts_var_expr
        """
        if self.run:
            # Check input stds.
            s_var_exprA = self.check_stds(t[1])
            s_var_exprB = self.check_stds(t[4])
            relations = ["EQUAL"]
            temporal = "l"
            function = t[2] + t[3]
            aggregate = t[2]
            # Build conditional values based on topological relationships.
            complist = self.build_spatio_temporal_topology_list(s_var_exprA,
                                                                s_var_exprB,
                                                                topolist=relations,
                                                                compare_cmd=True,
                                                                compop=function,
                                                                aggregate=aggregate,
                                                                convert=True)
            # Set temporal extent based on topological relationships.
            resultlist = self.set_temporal_extent_list(complist,
                                                       topolist=relations,
                                                       temporal=temporal)

        t[0] = resultlist

    def p_hash_operation(self, t):
        # Calculate the number of maps within an interval of another map from a
        # second space time dataset.
        # A # B
        # A {equal,r#} B
        """
        expr : t_hash_var
        """
        # Check input stds.
        maplist = self.check_stds(t[1])

        if self.run:
            resultlist = []
            for map_i in maplist:
                for obj in map_i.map_value:
                    if isinstance(obj, GlobalTemporalVar):
                        n_maps = obj.td
                mapinput = map_i.get_id()
                # Create r.mapcalc expression string for the operation.
                cmdstring = "(%s)" %(n_maps)
                 # Append module command.
                map_i.cmd_list = cmdstring
                # Append map to result map list.
                resultlist.append(map_i)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print(map.cmd_list)

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
