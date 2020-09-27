"""@package grass.temporal

Temporal vector algebra

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Thomas Leppelt and Soeren Gebbert

.. code-block:: python

    >>> import grass.temporal as tgis
    >>> tgis.init(True)
    >>> p = tgis.TemporalVectorAlgebraLexer()
    >>> p.build()
    >>> p.debug = True
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

"""
from __future__ import print_function

try:
    import ply.lex as lex
    import ply.yacc as yacc
except:
    pass

import grass.pygrass.modules as pygrass

import copy
from .temporal_algebra import TemporalAlgebraLexer, TemporalAlgebraParser, GlobalTemporalVar
from .core import init_dbif, get_current_mapset
from .abstract_dataset import AbstractDatasetComparisonKeyStartTime
from .open_stds import open_new_stds
from .spatio_temporal_relationships import SpatioTemporalTopologyBuilder
from .space_time_datasets import VectorDataset


##############################################################################

class TemporalVectorAlgebraLexer(TemporalAlgebraLexer):
    """Lexical analyzer for the GRASS GIS temporal vector algebra"""

    def __init__(self):
        TemporalAlgebraLexer.__init__(self)

    # Buffer functions from v.buffer
    vector_buff_functions = {
       'buff_p': 'BUFF_POINT',
       'buff_l': 'BUFF_LINE',
       'buff_a': 'BUFF_AREA',
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
    #t_T_OVERLAY_OPERATOR = r'\{([a-zA-Z\|]+[,])?([\|&+=]?[\|&+=\^\~])\}'
    t_T_OVERLAY_OPERATOR = r'\{[\|&+\^\~][,]?[a-zA-Z\| ]*([,])?([lrudi]|left|right|union|disjoint|intersect)?\}'

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
        ('left', 'T_SELECT_OPERATOR', 'T_SELECT', 'T_NOT_SELECT', 'T_HASH_OPERATOR', 'HASH'), # 1
        ('left', 'AND', 'OR', 'T_COMP_OPERATOR', 'T_OVERLAY_OPERATOR', 'DISOR', \
          'NOT', 'XOR'),  # 2
    )

    def __init__(self, pid=None, run=False, debug=True, spatial = False):
        TemporalAlgebraParser.__init__(self, pid, run, debug, spatial)

        self.m_overlay = pygrass.Module('v.overlay', quiet=True, run_=False)
        self.m_rename = pygrass.Module('g.rename', quiet=True, run_=False)
        self.m_patch = pygrass.Module('v.patch', quiet=True, run_=False)
        self.m_mremove = pygrass.Module('g.remove', quiet=True, run_=False)
        self.m_buffer = pygrass.Module('v.buffer', quiet=True, run_=False)

    def parse(self, expression, basename = None, overwrite = False):
        # Check for space time dataset type definitions from temporal algebra
        l = TemporalVectorAlgebraLexer()
        l.build()
        l.lexer.input(expression)

        while True:
            tok = l.lexer.token()
            if not tok:
                break

            if tok.type == "STVDS" or tok.type == "STRDS" or tok.type == "STR3DS":
                raise SyntaxError("Syntax error near '%s'" % (tok.type))

        self.lexer = TemporalVectorAlgebraLexer()
        self.lexer.build()
        self.parser = yacc.yacc(module=self, debug=self.debug, write_tables=False)

        self.overwrite = overwrite
        self.count = 0
        self.stdstype = "stvds"
        self.maptype = "vector"
        self.mapclass = VectorDataset
        self.basename = basename
        self.expression = expression
        self.parser.parse(expression)

    ######################### Temporal functions ##############################

    def build_spatio_temporal_topology_list(self, maplistA, maplistB = None, topolist = ["EQUAL"],
                                            assign_val = False, count_map = False, compare_bool = False,
                                            compare_cmd = False, compop = None, aggregate = None,
                                            new = False, convert = False, overlay_cmd = False):
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
          :param overlay_cmd: Boolean for aggregate overlay operators implicitly
                        in command list values based on related map lists.

          :return: List of maps from maplistA that fulfil the topological relationships
                  to maplistB specified in topolist.
        """
        topologylist = ["EQUAL", "FOLLOWS", "PRECEDES", "OVERLAPS", "OVERLAPPED",
                        "DURING", "STARTS", "FINISHES", "CONTAINS", "STARTED",
                        "FINISHED"]
        complementdict = {"EQUAL": "EQUAL", "FOLLOWS": "PRECEDES",
                          "PRECEDES": "FOLLOWS", "OVERLAPS": "OVERLAPPED",
                          "OVERLAPPED": "OVERLAPS", "DURING": "CONTAINS",
                          "CONTAINS": "DURING", "STARTS": "STARTED",
                          "STARTED": "STARTS", "FINISHES": "FINISHED",
                          "FINISHED": "FINISHES"}
        resultdict = {}
        # Check if given temporal relation are valid.
        for topo in topolist:
            if topo.upper() not in topologylist:
                raise SyntaxError("Unpermitted temporal relation name '" + topo + "'")

        # Create temporal topology for maplistA to maplistB.
        tb = SpatioTemporalTopologyBuilder()
        # Dictionary with different spatial variables used for topology builder.
        spatialdict = {'strds': '2D', 'stvds': '2D', 'str3ds': '3D'}
        # Build spatial temporal topology
        if self.spatial:
            tb.build(maplistA, maplistB, spatial = spatialdict[self.stdstype])
        else:
            tb.build(maplistA, maplistB)
        # Iterate through maps in maplistA and search for relationships given
        # in topolist.
        for map_i in maplistA:
            tbrelations = map_i.get_temporal_relations()
            # Check for boolean parameters for further calculations.
            if assign_val:
                self.assign_bool_value(map_i, tbrelations, topolist)
            elif compare_bool:
                self.compare_bool_value(map_i, tbrelations, compop, aggregate, topolist)
            elif compare_cmd:
                self.compare_cmd_value(map_i, tbrelations, compop, aggregate, topolist, convert)
            elif overlay_cmd:
                self.overlay_cmd_value(map_i, tbrelations, compop, topolist)

            for topo in topolist:
                if topo.upper() in tbrelations.keys():
                    if count_map:
                        relationmaplist = tbrelations[topo.upper()]
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
        resultlist = sorted(resultlist, key = AbstractDatasetComparisonKeyStartTime)

        return(resultlist)

    def overlay_cmd_value(self, map_i, tbrelations, function, topolist = ["EQUAL"]):
        """ Function to evaluate two map lists by given overlay operator.

          :param map_i: Map object with temporal extent.
          :param tbrelations: List of temporal relation to map_i.
          :param topolist: List of strings for given temporal relations.
          :param function: Overlay operator, &|+^~.

          :return: Map object with command list with  operators that has been
                        evaluated by implicit aggregration.
        """
        # Build comandlist list with elements from related maps and given relation operator.
        resultlist = []
        # Define overlay operation dictionary.
        overlaydict = {"&":"and", "|":"or", "^":"xor", "~":"not", "+":"disor"}
        operator = overlaydict[function]
        # Set first input for overlay module.
        mapainput = map_i.get_id()
        # Append command list of given map to result command list.
        if "cmd_list" in dir(map_i):
            resultlist = resultlist + map_i.cmd_list
        for topo in topolist:
            if topo.upper() in tbrelations.keys():
                relationmaplist = tbrelations[topo.upper()]
                for relationmap in relationmaplist:
                    # Append command list of given map to result command list.
                    if "cmd_list" in dir(relationmap):
                        resultlist = resultlist + relationmap.cmd_list
                    # Generate an intermediate name
                    name = self.generate_map_name()
                    # Put it into the removalbe map list
                    self.removable_maps[name] = VectorDataset(name + "@%s" % (self.mapset))
                    map_i.set_id(name + "@" + self.mapset)
                    # Set second input for overlay module.
                    mapbinput = relationmap.get_id()
                    # Create module command in PyGRASS for v.overlay and v.patch.
                    if operator != "disor":
                        m = copy.deepcopy(self.m_overlay)
                        m.run_ = False
                        m.inputs["operator"].value = operator
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
                    resultlist.append(m)
                    # Set new map name to temporary map name.
                    mapainput = name
        # Add command list to result map.
        map_i.cmd_list = resultlist

        return(resultlist)

    def set_temporal_extent_list(self, maplist, topolist = ["EQUAL"], temporal = 'l' ):
        """ Change temporal extent of map list based on temporal relations to
                other map list and given temporal operator.

            :param maplist: List of map objects for which relations has been build
                                        correctely.
            :param topolist: List of strings of temporal relations.
            :param temporal: The temporal operator specifying the temporal
                                            extent operation (intersection, union, disjoint
                                            union, right reference, left reference).

            :return: Map list with specified temporal extent.
        """
        resultdict = {}

        for map_i in maplist:
            # Loop over temporal related maps and create overlay modules.
            tbrelations = map_i.get_temporal_relations()
            # Generate an intermediate map for the result map list.
            map_new = self.generate_new_map(base_map=map_i, bool_op = 'and',
                                                                        copy = True, rename = False,
                                                                        remove = True)
            # Combine temporal and spatial extents of intermediate map with related maps.
            for topo in topolist:
                if topo in tbrelations.keys():
                    for map_j in (tbrelations[topo]):
                        if temporal == 'r':
                            # Generate an intermediate map for the result map list.
                            map_new = self.generate_new_map(base_map=map_i, bool_op = 'and',
                                                                                        copy = True, rename = False,
                                                                                        remove = True)
                        # Create overlaid map extent.
                        returncode = self.overlay_map_extent(map_new, map_j, 'and',
                                                                temp_op = temporal)
                        # Stop the loop if no temporal or spatial relationship exist.
                        if returncode == 0:
                            break
                        # Append map to result map list.
                        elif returncode == 1:
                            # resultlist.append(map_new)
                            resultdict[map_new.get_id()] = map_new
                    if returncode == 0:
                        break
            # Append map to result map list.
            #if returncode == 1:
            #    resultlist.append(map_new)
        # Get sorted map objects as values from result dictionoary.
        resultlist = resultdict.values()
        resultlist = sorted(resultlist, key = AbstractDatasetComparisonKeyStartTime)

        return(resultlist)

    ###########################################################################

    def p_statement_assign(self, t):
        # The expression should always return a list of maps.
        """
        statement : stds EQUALS expr

        """
        # Execute the command lists
        if self.run:
            # Open connection to temporal database.
            dbif, connected = init_dbif(dbif=self.dbif)
            if isinstance(t[3], list):
                num = len(t[3])
                count = 0
                returncode = 0
                register_list = []
                leadzero = len(str(num))
                for i in range(num):
                    # Check if resultmap names exist in GRASS database.
                    vectorname = self.basename + "_" + str(i).zfill(leadzero)
                    vectormap = VectorDataset(vectorname + "@" + get_current_mapset())
                    if vectormap.map_exists() and self.overwrite is False:
                        self.msgr.fatal(_("Error vector maps with basename %s exist. "
                                          "Use --o flag to overwrite existing file")
                                          % (vectorname))
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
                                self.msgr.fatal(_("Error starting %s : \n%s")
                                                  % (cmd.get_bash(),
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
                            if map_i.get_name() in self.removable_maps:
                                self.removable_maps.pop(map_i.get_name())
                            mapset = map_i.get_mapset()
                            # Change map name to given basename.
                            newident = self.basename + "_" + str(count).zfill(leadzero)
                            m = copy.deepcopy(self.m_rename)
                            m.inputs["vector"].value = (map_i.get_name(),newident)
                            m.flags["overwrite"].value = self.overwrite
                            m.run()
                            map_i.set_id(newident + "@" + mapset)
                            count += 1
                            register_list.append(map_i)
                    else:
                        # Test if temporal extents have been changed by temporal
                        # relation operators (i|r). This is a code copy from temporal_algebra.py
                        map_i_extent = map_i.get_temporal_extent_as_tuple()
                        map_test = map_i.get_new_instance(map_i.get_id())
                        map_test.select(dbif)
                        map_test_extent = map_test.get_temporal_extent_as_tuple()
                        if map_test_extent != map_i_extent:
                            # Create new map with basename
                            newident = self.basename + "_" + str(count).zfill(leadzero)
                            map_result = map_i.get_new_instance(newident + "@" + self.mapset)

                            if map_test.map_exists() and self.overwrite is False:
                                self.msgr.fatal("Error raster maps with basename %s exist. "
                                                "Use --o flag to overwrite existing file"
                                                % (mapname))

                            map_result.set_temporal_extent(map_i.get_temporal_extent())
                            map_result.set_spatial_extent(map_i.get_spatial_extent())
                            # Attention we attach a new attribute
                            map_result.is_new = True
                            count += 1
                            register_list.append(map_result)

                            # Copy the map
                            m = copy.deepcopy(self.m_copy)
                            m.inputs["vector"].value = map_i.get_id(), newident
                            m.flags["overwrite"].value = self.overwrite
                            m.run()
                        else:
                            register_list.append(map_i)

                if len(register_list) > 0:
                    # Create result space time dataset.
                    resultstds = open_new_stds(t[1], self.stdstype,
                                               'absolute', t[1], t[1],
                                               "temporal vector algebra", self.dbif,
                                               overwrite=self.overwrite)
                    for map_i in register_list:
                        # Check if modules should be executed from command list.
                        if hasattr(map_i, "cmd_list") or hasattr(map_i, "is_new"):
                            # Get meta data from grass database.
                            map_i.load()
                            if map_i.is_in_db(dbif=dbif) and self.overwrite:
                                # Update map in temporal database.
                                map_i.update_all(dbif=dbif)
                            elif map_i.is_in_db(dbif=dbif) and self.overwrite is False:
                                # Raise error if map exists and no overwrite flag is given.
                                self.msgr.fatal(_("Error vector map %s exist in temporal database. "
                                                  "Use overwrite flag.  : \n%s")
                                                  % (map_i.get_map_id(), cmd.popen.stderr))
                            else:
                                # Insert map into temporal database.
                                map_i.insert(dbif=dbif)
                        else:
                            # Map is original from an input STVDS
                            map_i.load()
                        # Register map in result space time dataset.
                        print(map_i.get_temporal_extent_as_tuple())
                        success = resultstds.register_map(map_i, dbif=dbif)
                    resultstds.update_from_registered_maps(dbif)

            # Remove intermediate maps
            self.remove_maps()
            if connected:
                dbif.close()
            t[0] = t[3]

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
        if self.run:
            # Check input stds and operator.
            maplistA = self.check_stds(t[1])
            maplistB = self.check_stds(t[3])
            relations = ["EQUAL"]
            temporal = 'l'
            function = t[2]
            # Build command list for related maps.
            complist = self.build_spatio_temporal_topology_list(maplistA, maplistB, topolist = relations,
                                                                compop = function, overlay_cmd = True)
            # Set temporal extent based on topological relationships.
            resultlist = self.set_temporal_extent_list(complist, topolist = relations,
                                temporal = temporal)

            t[0] = resultlist
        if self.debug:
            str(t[1]) + t[2] + str(t[3])

    def p_overlay_operation_relation(self, t):
        """
        expr : stds T_OVERLAY_OPERATOR stds
             | expr T_OVERLAY_OPERATOR stds
             | stds T_OVERLAY_OPERATOR expr
             | expr T_OVERLAY_OPERATOR expr
        """
        if self.run:
            # Check input stds and operator.
            maplistA = self.check_stds(t[1])
            maplistB = self.check_stds(t[3])
            relations, temporal, function, aggregate = self.eval_toperator(t[2], optype = 'overlay')
            # Build command list for related maps.
            complist = self.build_spatio_temporal_topology_list(maplistA, maplistB, topolist = relations,
                                                                compop = function, overlay_cmd = True)
            # Set temporal extent based on topological relationships.
            resultlist = self.set_temporal_extent_list(complist, topolist = relations,
                                temporal = temporal)

            t[0] = resultlist
        if self.debug:
            str(t[1]) + t[2] + str(t[3])

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
                                                copy=True, remove = True)
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
