"""!@package grass.temporal

Temporal raster algebra

(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Thomas Leppelt and Soeren Gebbert

@code

    >>> p = TemporalRasterAlgebraLexer()
    >>> p.build()
    >>> p.debug = True
    >>> expression =  'R = A / B * 20 + C - 2.45'
    >>> p.test(expression)
    R = A / B * 20 + C - 2.45
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(DIV,'/',1,6)
    LexToken(NAME,'B',1,8)
    LexToken(MULT,'*',1,10)
    LexToken(INT,20,1,12)
    LexToken(ADD,'+',1,15)
    LexToken(NAME,'C',1,17)
    LexToken(SUB,'-',1,19)
    LexToken(FLOAT,2.45,1,21)
    >>> expression =  'R = A {equal,|/} B'
    >>> p.test(expression)
    R = A {equal,|/} B
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_ARITH1_OPERATOR,'{equal,|/}',1,6)
    LexToken(NAME,'B',1,17)
    >>> expression =  'R = A {equal,||} B'
    >>> p.test(expression)
    R = A {equal,||} B
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_COMP_OPERATOR,'{equal,||}',1,6)
    LexToken(NAME,'B',1,17)
    >>> expression =  'R = A {equal,&&} B'
    >>> p.test(expression)
    R = A {equal,&&} B
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_COMP_OPERATOR,'{equal,&&}',1,6)
    LexToken(NAME,'B',1,17)
    >>> expression =  'R = A {equal | during,+*} B'
    >>> p.test(expression)
    R = A {equal | during,+*} B
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_ARITH1_OPERATOR,'{equal | during,+*}',1,6)
    LexToken(NAME,'B',1,26)
    >>> expression =  'R = A {equal | during,+:} B'
    >>> p.test(expression)
    R = A {equal | during,+:} B
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(T_SELECT_OPERATOR,'{equal | during,+:}',1,6)
    LexToken(NAME,'B',1,26)
    >>> expression =  'R = abs(A) {equal,+:} exp(B) / sqrt(C) - log(D)'
    >>> p.test(expression)
    R = abs(A) {equal,+:} exp(B) / sqrt(C) - log(D)
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(ABS,'abs',1,4)
    LexToken(LPAREN,'(',1,7)
    LexToken(NAME,'A',1,8)
    LexToken(RPAREN,')',1,9)
    LexToken(T_SELECT_OPERATOR,'{equal,+:}',1,11)
    LexToken(EXP,'exp',1,22)
    LexToken(LPAREN,'(',1,25)
    LexToken(NAME,'B',1,26)
    LexToken(RPAREN,')',1,27)
    LexToken(DIV,'/',1,29)
    LexToken(SQRT,'sqrt',1,31)
    LexToken(LPAREN,'(',1,35)
    LexToken(NAME,'C',1,36)
    LexToken(RPAREN,')',1,37)
    LexToken(SUB,'-',1,39)
    LexToken(LOG,'log',1,41)
    LexToken(LPAREN,'(',1,44)
    LexToken(NAME,'D',1,45)
    LexToken(RPAREN,')',1,46)

@endcode
"""

import grass.pygrass.modules as pymod
from temporal_raster_operator import *
from temporal_algebra import *

##############################################################################

class TemporalRasterAlgebraLexer(TemporalAlgebraLexer):
    """!Lexical analyzer for the GRASS GIS temporal algebra"""

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
                    + tuple(mapcalc_functions.values())

    # Regular expression rules for simple tokens
    t_MOD                 = r'[\%]'
    t_DIV                 = r'[\/]'
    t_MULT                = r'[\*]'
    t_ADD                 = r'[\+]'
    t_SUB                 = r'[-]'
    t_T_ARITH1_OPERATOR   = r'\{([a-zA-Z\| ]+[,])?([\|&+=]?[\%\*\/])\}'
    t_T_ARITH2_OPERATOR   = r'\{([a-zA-Z\| ]+[,])?([\|&+=]?[+-])\}'
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
        ('left', 'ADD', 'SUB', 'T_ARITH2_OPERATOR'), #2
        ('left', 'AND', 'OR', 'T_COMP_OPERATOR', 'MOD', 'DIV', 'MULT',
         'T_ARITH1_OPERATOR'))

    def __init__(self, pid=None, run = True, debug = False, spatial = False, \
                  nprocs = 1, register_null = False):
        TemporalAlgebraParser.__init__(self, pid, run, debug, spatial)
        self.nprocs = nprocs
        self.empty_maps = {}
        self.register_null = register_null

    def check_null(self, t):
        try:
            int(t)
            return t
        except ValueError:
            return "null()"

    ######################### Temporal functions ##############################

    def eval_toperator(self, operator, comparison = False):
        """!This function evaluates a string containing temporal operations.

         @param operator String of temporal operations, e.g. {equal|during,=!:}.

         @return List of temporal relations (equal, during), the given function
          (!:) and the interval/instances (=).
        @code
         >>> init(True)
         >>> p = TemporalRasterBaseAlgebraParser()
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

        @endcode

        """
        p = TemporalRasterOperatorParser()
        p.parse(operator, comparison)
        p.relations = [rel.upper() for rel in p.relations]

        return(p.relations, p.temporal, p.function)

    ###########################################################################

    def p_statement_assign(self, t):
        # The expression should always return a list of maps.
        """
        statement : stds EQUALS expr

        """
        if self.run:
            # Create the process queue for parallel mapcalc processing
            process_queue = pymod.ParallelModuleQueue(int(self.nprocs))
            if isinstance(t[3], list):
                num = len(t[3])
                count = 0
                returncode = 0
                register_list = []
                for i in range(num):
                    # Check if resultmap names exist in GRASS database.
                    rastername = self.basename + "_" + str(i) + "@" + self.mapset
                    rastermap = RasterDataset(rastername)
                    if rastermap.map_exists() and self.overwrite == False:
                        self.msgr.fatal("Error raster maps with basename %s exist. Use --o flag to overwrite existing file" \
                                            %(rastername))
                map_test_list = []
                for map_i in t[3]:
                    newident = self.basename + "_" + str(count)
                    if "cmd_list" in dir(map_i):
                        print(newident + ' = ' + map_i.cmd_list)
                        # Build r.mapcalc module and execute expression.
                        # Change map name to given basename.
                        # Create deepcopy of r.mapcalc module.

                        map_test = map_i.get_new_instance(newident + "@" + self.mapset)
                        map_test.set_temporal_extent(map_i.get_temporal_extent())
                        map_test.set_spatial_extent(map_i.get_spatial_extent())
                        map_test_list.append(map_test)

                        m = copy.deepcopy(self.m_mapcalc)
                        m_expression = newident + "=" + map_i.cmd_list
                        m.inputs["expression"].value = str(m_expression)
                        m.flags["overwrite"].value = self.overwrite
                        process_queue.put(m)
                    else:
                        map_i.set_id(newident + "@" + self.mapset)
                        map_test_list.append(map_i)
                    count  += 1

                process_queue.wait()

                for map_i in map_test_list:
                    if not map_test.map_exists():
                        self.msgr.error(_("Error computing map <%s>"%(map_i.get_id()) ))
                    else:
                        register_list.append(map_i)

                # Open connection to temporal database.
                dbif, connect = init_dbif(self.dbif)
                # Create result space time dataset.
                resultstds = open_new_space_time_dataset(t[1], self.stdstype, \
                                                         'absolute', t[1], t[1], \
                                                         'mean', self.dbif, \
                                                         overwrite = self.overwrite)
                for map_i in register_list:
                    # Get meta data from grass database.
                    map_i.load()
                    # Do not register empty maps if not required

                    # In case of a null map continue, do not register null maps
                    if map_i.metadata.get_min() is None and \
                       map_i.metadata.get_max() is None:
                        if not self.register_null:
                            self.empty_maps[map_i.get_name()] = map_i.get_name()
                            continue

                    if map_i.is_in_db(dbif) and self.overwrite:
                        # Update map in temporal database.
                        map_i.update_all(dbif)
                    elif map_i.is_in_db(dbif) and self.overwrite == False:
                        # Raise error if map exists and no overwrite flag is given.
                        self.msgr.fatal("Error vector map %s exist in temporal database. Use overwrite flag.  : \n%s" \
                                            %(map_i.get_map_id(), cmd.popen.stderr))
                    else:
                        # Insert map into temporal database.
                        map_i.insert(dbif)
                    # Register map in result space time dataset.
                    success = resultstds.register_map(map_i, dbif)
                resultstds.update_from_registered_maps(dbif)
                dbif.close()
                t[0] = register_list

                self.remove_empty_maps()

    def p_arith1_operation(self, t):
        """
        expr : stds MOD stds
             | expr MOD stds
             | stds MOD expr
             | expr MOD expr
             | stds DIV stds
             | expr DIV stds
             | stds DIV expr
             | expr DIV expr
             | stds MULT stds
             | expr MULT stds
             | stds MULT expr
             | expr MULT expr
             | stds MOD t_td_var
             | expr MOD t_td_var
             | stds DIV t_td_var
             | expr DIV t_td_var
             | stds MULT t_td_var
             | expr MULT t_td_var
        """
        # Check input stds.
        maplistA = self.check_stds(t[1])
        maplistB = self.check_stds(t[3])
        topolist = self.get_temporal_topo_list(maplistA, maplistB)

        if self.run:
            resultlist = []
            for map_i in topolist:
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapainput = map_i.get_id()
                # Loop over temporal related maps and create overlay modules.
                tbrelations = map_i.get_temporal_relations()
                count = 0
                for map_j in (tbrelations['EQUAL']):
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                            temp_op = '=')
                    # Stop the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        break
                    if count == 0:
                        # Set map name.
                        name = map_new.get_id()
                    else:
                        # Generate an intermediate map
                        name = self.generate_map_name()

                    if "map_value" in dir(map_j) and len(map_j.map_value) > 0 and map_j.map_value[0].get_type() == "timediff":
                        mapbinput = map_j.map_value[0].get_type_value()[0]
                    else:
                    # Set first input for overlay module.
                        mapbinput = map_j.get_id()

                    # Create r.mapcalc expression string for the operation.
                    if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                        cmdstring = "(%s %s %s)" %(map_new.cmd_list, t[2], mapbinput)
                    elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                        cmdstring = "(%s %s %s)" %(mapainput, t[2], map_j.cmd_list)
                    elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                        cmdstring = "(%s %s %s)" %(map_new.cmd_list, t[2], map_j.cmd_list)
                    else:
                        cmdstring = "(%s %s %s)" %(mapainput, t[2], mapbinput)
                    # Conditional append of module command.
                    map_new.cmd_list = cmdstring
                    # Set new map name to temporary map name.
                    mapainput = name
                    count += 1
                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_arith1_operation_numeric1(self, t):
        """
        expr : stds MOD number
             | expr MOD number
             | stds DIV number
             | expr DIV number
             | stds MULT number
             | expr MULT number
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
                print map.cmd_list


    def p_arith1_operation_numeric2(self, t):
        """
        expr : number MOD stds
             | number MOD expr
             | number DIV stds
             | number DIV expr
             | number MULT stds
             | number MULT expr
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
                print map.cmd_list


    def p_arith2_operation(self, t):
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
        topolist = self.get_temporal_topo_list(maplistA, maplistB)

        if self.run:
            resultlist = []
            for map_i in topolist:
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapainput = map_i.get_id()
                # Loop over temporal related maps and create overlay modules.
                tbrelations = map_i.get_temporal_relations()
                count = 0
                for map_j in (tbrelations['EQUAL']):
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                            temp_op = '=')
                    # Stop the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        break
                    if count == 0:
                        # Set map name.
                        name = map_new.get_id()
                    else:
                        # Generate an intermediate map
                        name = self.generate_map_name()

                    if "map_value" in dir(map_j) and len(map_j.map_value) > 0 and map_j.map_value[0].get_type() == "timediff":
                        mapbinput = map_j.map_value[0].get_type_value()[0]
                    else:
                    # Set first input for overlay module.
                        mapbinput = map_j.get_id()

                    # Create r.mapcalc expression string for the operation.
                    if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                        cmdstring = "(%s %s %s)" %(map_new.cmd_list, t[2], mapbinput)
                    elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                        cmdstring = "(%s %s %s)" %(mapainput, t[2], map_j.cmd_list)
                    elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                        cmdstring = "(%s %s %s)" %(map_new.cmd_list, t[2], map_j.cmd_list)
                    else:
                        cmdstring = "(%s %s %s)" %(mapainput, t[2], mapbinput)
                    # Conditional append of module command.
                    map_new.cmd_list = cmdstring
                    # Set new map name to temporary map name.
                    mapainput = name
                    count += 1

                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_arith2_operation_numeric1(self, t):
        """
        expr : stds ADD number
             | expr ADD number
             | stds SUB number
             | expr SUB number
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
                print map.cmd_list

    def p_arith2_operation_numeric2(self, t):
        """
        expr : number ADD stds
             | number ADD expr
             | number SUB stds
             | number SUB expr
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
                print map.cmd_list

    def p_arith1_operation_relation(self, t):
        """
        expr : stds T_ARITH1_OPERATOR stds
             | expr T_ARITH1_OPERATOR stds
             | stds T_ARITH1_OPERATOR expr
             | expr T_ARITH1_OPERATOR expr
             | stds T_ARITH1_OPERATOR t_td_var
             | expr T_ARITH1_OPERATOR t_td_var
        """
        # Check input stds.
        maplistA = self.check_stds(t[1])
        maplistB = self.check_stds(t[3])
        relations, temporal, function= self.eval_toperator(t[2])
        topolist = self.get_temporal_topo_list(maplistA, maplistB, topolist = relations)

        if self.run:
            resultlist = []
            for map_i in topolist:
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapainput = map_i.get_id()
                # Loop over temporal related maps and create overlay modules.
                tbrelations = map_i.get_temporal_relations()
                count = 0
                for topo in relations:
                    if topo in tbrelations.keys():
                        for map_j in (tbrelations[topo]):
                            # Create overlayed map extent.
                            returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                                    temp_op = temporal)
                            print(returncode)
                            # Stop the loop if no temporal or spatial relationship exist.
                            if returncode == 0:
                                break
                            if count == 0:
                                # Set map name.
                                name = map_new.get_id()
                            else:
                                # Generate an intermediate map
                                name = self.generate_map_name()
                                map_new.set_id(name + "@" + mapset)
                            # Set second input for overlay module.

                            if "map_value" in dir(map_j) and len(map_j.map_value) > 0 and map_j.map_value[0].get_type() == "timediff":
                                mapbinput = map_j.map_value[0].get_type_value()[0]
                            else:
                            # Set first input for overlay module.
                                mapbinput = map_j.get_id()

                            # Create r.mapcalc expression string for the operation.
                            if "cmd_list" in dir(map_new):
                                cmdstring = "(%s %s %s)" %(map_new.cmd_list, function, mapbinput)
                                print('with cmd in a: ' + map_j.get_id())
                            elif "cmd_list" in dir(map_j):
                                cmdstring = "(%s %s %s)" %(mapainput, function, map_j.cmd_list)
                                print('with cmd in b')
                            elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                                cmdstring = "(%s %s %s)" %(map_new.cmd_list, function, map_j.cmd_list)
                                print('with cmd in b')
                            else:
                                cmdstring = "(%s %s %s)" %(mapainput, function, mapbinput)
                            print(cmdstring)
                            # Conditional append of module command.
                            map_new.cmd_list = cmdstring
                            # Set new map name to temporary map name.
                            mapainput = name
                            count += 1
                        if returncode == 0:
                            break
                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_arith2_operation_relation(self, t):
        """
        expr : stds T_ARITH2_OPERATOR stds
             | expr T_ARITH2_OPERATOR stds
             | stds T_ARITH2_OPERATOR expr
             | expr T_ARITH2_OPERATOR expr
             | stds T_ARITH2_OPERATOR t_td_var
             | expr T_ARITH2_OPERATOR t_td_var
        """
        # Check input stds.
        maplistA = self.check_stds(t[1])
        maplistB = self.check_stds(t[3])
        relations, temporal, function= self.eval_toperator(t[2])
        topolist = self.get_temporal_topo_list(maplistA, maplistB, topolist = relations)

        if self.run:
            resultlist = []
            for map_i in topolist:
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapainput = map_i.get_id()
                # Loop over temporal related maps and create overlay modules.
                tbrelations = map_i.get_temporal_relations()
                count = 0
                for topo in relations:
                    if topo in tbrelations.keys():
                        for map_j in (tbrelations[topo]):
                            # Create overlayed map extent.
                            returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                                    temp_op = temporal)
                            # Stop the loop if no temporal or spatial relationship exist.
                            if returncode == 0:
                                break
                            if count == 0:
                                # Set map name.
                                name = map_new.get_id()
                            else:
                                # Generate an intermediate map
                                name = self.generate_map_name()
                                map_new.set_id(name + "@" + self.mapset)
                            # Set second input for overlay module.

                            if "map_value" in dir(map_j) and len(map_j.map_value) > 0 and map_j.map_value[0].get_type() == "timediff":
                                mapbinput = map_j.map_value[0].get_type_value()[0]
                            else:
                            # Set first input for overlay module.
                                mapbinput = map_j.get_id()

                            # Create r.mapcalc expression string for the operation.
                            if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                                cmdstring = "(%s %s %s)" %(map_new.cmd_list, function, mapbinput)
                            elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                                cmdstring = "(%s %s %s)" %(mapainput, function, map_j.cmd_list)
                            elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                                cmdstring = "(%s %s %s)" %(map_new.cmd_list, function, map_j.cmd_list)
                            else:
                                cmdstring = "(%s %s %s)" %(mapainput, function, mapbinput)
                            # Conditional append of module command.
                            map_new.cmd_list = cmdstring
                            # Set new map name to temporary map name.
                            mapainput = name
                            count += 1
                        if returncode == 0:
                            break
                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

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
            for map in resultlist:
                print map.cmd_list


    def p_mapcalc_operation(self, t):
        # Examples:
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
                print map.cmd_list


    def p_s_var_expr(self, t):
        # Examples:
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
                print map.cmd_list

    def p_s_var_expr_1(self, t):
        # Examples:
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
                print map.cmd_list

    def p_s_var_expr2(self, t):
        # Examples:
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
                print map.cmd_list

    def p_s_var_expr3(self, t):
        # Examples:
        #   A <= 2 || B == 10
        #   A < 3 && A > 1
        """
        s_var_expr : s_var_expr AND AND s_var_expr
                   | s_var_expr OR OR s_var_expr
        """
        # Check input stds.
        maplistA = self.check_stds(t[1])
        maplistB = self.check_stds(t[4])
        topolist = self.get_temporal_topo_list(maplistA, maplistB)

        if self.run:
            resultlist = []
            for map_i in topolist:
                # Loop over temporal related maps and create overlay modules.
                tbrelations = map_i.get_temporal_relations()
                count = 0
                for map_j in (tbrelations['EQUAL']):
                    # Generate an intermediate map for the result map list.
                    map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                    # Set first input for overlay module.
                    mapainput = map_i.get_id()
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                            temp_op = '=')
                    # Stop the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        break
                    if count == 0:
                        # Set map name.
                        name = map_new.get_id()
                    else:
                        # Generate an intermediate map
                        name = self.generate_map_name()

                    # Set first input for overlay module.
                    mapbinput = map_j.get_id()
                    # Create r.mapcalc expression string for the operation.
                    if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                        cmdstring = "%s %s %s" %(map_new.cmd_list, t[2] + t[3], mapbinput)
                    elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                        cmdstring = "%s %s %s" %(mapainput, t[2] + t[3], map_j.cmd_list)
                    elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                        cmdstring = "%s %s %s" %(map_new.cmd_list, t[2] + t[3], map_j.cmd_list)
                    else:
                        cmdstring = "%s %s %s" %(mapainput, t[2] + t[3], mapbinput)
                    # Conditional append of module command.
                    map_new.cmd_list = cmdstring
                    # Set new map name to temporary map name.
                    #mapainput = name
                    count += 1
                    # Append map to result map list.
                    if returncode == 1:
                        resultlist.append(map_new)
            t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_s_var_expr4(self, t):
        # Examples:
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
                print map.cmd_list

    def p_s_expr_condition_if(self, t):
        # Examples:
        #   if(s_var_expr, B)
        #   if(A == 1, B)
        """
        expr : IF LPAREN s_var_expr COMMA stds RPAREN
             | IF LPAREN s_var_expr COMMA expr RPAREN
        """

        ifmaplist = self.check_stds(t[3])
        thenmaplist = self.check_stds(t[5])
        topolist = self.get_temporal_topo_list(ifmaplist, thenmaplist)
        resultlist = []
        for map_i in topolist:
            #print(map_i.get_id())
            # Loop over temporal related maps and create overlay modules.
            tbrelations = map_i.get_temporal_relations()
            count = 0
            for map_j in (tbrelations['EQUAL']):
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapainput = map_i.get_id()
                # Create overlayed map extent.
                returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                        temp_op = '=')
                # Stop the loop if no temporal or spatial relationship exist.
                if returncode == 0:
                    break
                if count == 0:
                    # Set map name.
                    name = map_new.get_id()
                else:
                    # Generate an intermediate map
                    name = self.generate_map_name()

                # Set first input for overlay module.
                mapbinput = map_j.get_id()
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                    cmdstring = "if(%s,%s)" %(map_new.cmd_list, mapbinput)
                elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                    cmdstring = "if(%s,%s)" %(mapainput, map_j.cmd_list)
                elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                    cmdstring = "if(%s,%s)" %(map_new.cmd_list, map_j.cmd_list)
                else:
                    cmdstring = "if(%s,%s)" %(mapainput, mapbinput)
                # Conditional append of module command.
                map_new.cmd_list = cmdstring
                # Set new map name to temporary map name.
                #mapainput = name
                count += 1
                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_s_numeric_condition_if(self, t):
        # Examples:
        #   if(s_var_expr, 1)
        #   if(A == 5, 10)
        """
        expr : IF LPAREN s_var_expr COMMA number RPAREN
             | IF LPAREN s_var_expr COMMA NULL LPAREN RPAREN RPAREN
        """
        ifmaplist = self.check_stds(t[3])
        resultlist = []
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 7:
            numinput = t[5]
        elif len(t) == 9:
            numinput = self.check_null(t[5])

        # Iterate over condition map list.
        for map_i in ifmaplist:
            mapinput = map_i.get_id()
            # Create r.mapcalc expression string for the operation.
            if "cmd_list" in dir(map_i):
                cmdstring = "if(%s,%s)" %(map_i.cmd_list, numinput)
            else:
                cmdstring = "if(%s,%s)" %(mapinput, numinput)
            # Conditional append of module command.
            map_i.cmd_list = cmdstring
            # Append map to result map list.
            resultlist.append(map_i)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_s_expr_condition_if_relation(self, t):
        # Examples:
        #   if({equal||during}, s_var_expr, A)
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA expr RPAREN
        """
        relations, temporal, function= self.eval_toperator(t[3])
        ifmaplist = self.check_stds(t[5])
        thenmaplist = self.check_stds(t[7])
        topolist = self.get_temporal_topo_list(ifmaplist, thenmaplist,
                    topolist = relations)
        resultlist = []
        for map_i in topolist:
            #print(map_i.get_id())
            # Loop over temporal related maps.
            tbrelations = map_i.get_temporal_relations()
            count = 0
            for topo in relations:
                if topo in tbrelations.keys():
                    for map_j in (tbrelations[topo]):
                        # Generate an intermediate map for the result map list.
                        map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                        # Set first input for overlay module.
                        mapainput = map_i.get_id()
                        # Create overlayed map extent.
                        returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                                temp_op = '=')
                        # Stop the loop if no temporal or spatial relationship exist.
                        if returncode == 0:
                            break
                        if count == 0:
                            # Set map name.
                            name = map_new.get_id()
                        else:
                            # Generate an intermediate map
                            name = self.generate_map_name()

                        # Set first input for overlay module.
                        mapbinput = map_j.get_id()
                        #print(mapbinput)
                        #mapbinput = mapbinput.split('@')[0]
                        # Create r.mapcalc expression string for the operation.
                        if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                            cmdstring = "if(%s,%s)" %(map_new.cmd_list, mapbinput)
                        elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                            cmdstring = "if(%s,%s)" %(mapainput, map_j.cmd_list)
                        elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                            cmdstring = "if(%s,%s)" %(map_new.cmd_list, map_j.cmd_list)
                        else:
                            cmdstring = "if(%s,%s)" %(mapainput, mapbinput)
                        # Conditional append of module command.
                        map_new.cmd_list = cmdstring
                        # Set new map name to temporary map name.
                        #mapainput = name
                        count += 1
                        # Append map to result map list.
                        if returncode == 1:
                            resultlist.append(map_new)
        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_s_expr_condition_elif(self, t):
        # Examples:
        #   if(s_var_expr, A, B)
        """
        expr : IF LPAREN s_var_expr COMMA stds COMMA stds RPAREN
             | IF LPAREN s_var_expr COMMA stds COMMA expr RPAREN
             | IF LPAREN s_var_expr COMMA expr COMMA stds RPAREN
             | IF LPAREN s_var_expr COMMA expr COMMA expr RPAREN
        """
        ifmaplist = self.check_stds(t[3])
        thenmaplist = self.check_stds(t[5])
        elsemaplist = self.check_stds(t[7])
        resultlist = []
        thendict = {}
        elsedict = {}
        # Get topologies for the appropriate conclusion term.
        thentopolist = self.get_temporal_topo_list(ifmaplist, thenmaplist)
        # Fill dictionaries with related maps for both conclusion terms.
        for map_i in thentopolist:
            thenrelations = map_i.get_temporal_relations()
            relationmaps = thenrelations['EQUAL']
            thendict[map_i.get_id()] = relationmaps
        # Get topologies for the alternative conclusion term.
        elsetopolist = self.get_temporal_topo_list(ifmaplist, elsemaplist)
        for map_i in elsetopolist:
            elserelations = map_i.get_temporal_relations()
            relationmaps = elserelations['EQUAL']
            elsedict[map_i.get_id()] = relationmaps
        # Loop through conditional map list.
        for map_i in ifmaplist:
            if map_i.get_id() in thendict.keys():
                thenlist = thendict[map_i.get_id()]
            else:
                thenlist = []
            if map_i.get_id() in elsedict.keys():
                elselist = elsedict[map_i.get_id()]
            else:
                elselist = []
            # Set iteration amount to maximal or minimum number of related
            # conclusion maps, depending on null map creation flag.
            if self.null:
                iternum = max(len(thenlist), len(elselist))
            else:
                iternum = min(len(thenlist), len(elselist))
            # Calculate difference in conclusion lengths.
            iterthen = iternum - len(thenlist)
            iterelse = iternum - len(elselist)
            # Extend null maps to the list to get conclusions with same length.
            if iterthen != 0:
                for i in range(iterthen):
                    thenlist.extend(['null()'])
            if iterelse != 0:
                for i in range(iterelse):
                    elselist.extend(['null()'])

            # Combine the conclusions in a paired list.
            conclusionlist = zip(thenlist, elselist)
            for i in range(iternum):
                conclusionmaps = conclusionlist[i]
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapifinput = map_i.get_id()
                # Get conclusion maps.
                map_then = conclusionmaps[0]
                map_else = conclusionmaps[1]

                # Check if conclusions are map objects.
                if map_then != 'null()':
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_then, 'and', \
                                                            temp_op = '=')
                    maptheninput = map_then.get_id()
                    # Continue the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        continue
                else:
                    maptheninput = 'null()'
                # Check if conclusions are map objects.
                if map_else != 'null()':
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_else, 'and', \
                                                            temp_op = '=')
                    mapelseinput = map_else.get_id()
                    # Continue the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        continue
                else:
                    mapelseinput = 'null()'

                #if map_then != 'null()' and map_else != 'null()':
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, maptheninput,
                    mapelseinput)
                elif "cmd_list" in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, map_then.cmd_list,
                    mapelseinput)
                elif "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, maptheninput,
                    map_else.cmd_list)
                elif "cmd_list" in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, map_then.cmd_list,
                    map_else.cmd_list)
                elif "cmd_list" not in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, map_then.cmd_list,
                    mapelseinput)
                elif "cmd_list" not in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, maptheninput,
                    map_else.cmd_list)
                elif "cmd_list" not in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, map_then.cmd_list,
                    map_else.cmd_list)
                else:
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, maptheninput,
                    mapelseinput)
                # Conditional append of module command.
                map_new.cmd_list = cmdstring
                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_s_numeric_condition_elif(self, t):
        # Examples:
        #   if(s_var_expr, 1, 2)
        #   if(A == 5, 10, 0)
        """
        expr : IF LPAREN s_var_expr COMMA number COMMA number RPAREN
             | IF LPAREN s_var_expr COMMA NULL LPAREN RPAREN COMMA number RPAREN
             | IF LPAREN s_var_expr COMMA number COMMA NULL LPAREN RPAREN RPAREN
             | IF LPAREN s_var_expr COMMA NULL LPAREN RPAREN COMMA NULL LPAREN RPAREN RPAREN
        """
        ifmaplist = self.check_stds(t[3])
        resultlist = []
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
        # Iterate over condition map list.
        for map_i in ifmaplist:
            mapinput = map_i.get_id()
            # Create r.mapcalc expression string for the operation.
            if "cmd_list" in dir(map_i):
                cmdstring = "if(%s, %s, %s)" %(map_i.cmd_list, numthen, numelse)
            else:
                cmdstring = "if(%s, %s, %s)" %(mapinput, numthen, numelse)
            # Conditional append of module command.
            map_i.cmd_list = cmdstring
            # Append map to result map list.
            resultlist.append(map_i)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_s_numeric_expr_condition_elif(self, t):
        # Examples:
        #   if(s_var_expr, 1, A)
        #   if(A == 5 && C > 5, A, null())
        """
        expr : IF LPAREN s_var_expr COMMA number COMMA stds RPAREN
             | IF LPAREN s_var_expr COMMA NULL LPAREN RPAREN COMMA stds RPAREN
             | IF LPAREN s_var_expr COMMA number COMMA expr RPAREN
             | IF LPAREN s_var_expr COMMA NULL LPAREN RPAREN COMMA expr RPAREN
             | IF LPAREN s_var_expr COMMA stds COMMA number RPAREN
             | IF LPAREN s_var_expr COMMA stds COMMA NULL LPAREN RPAREN RPAREN
             | IF LPAREN s_var_expr COMMA expr COMMA number RPAREN
             | IF LPAREN s_var_expr COMMA expr COMMA NULL LPAREN RPAREN RPAREN
        """
        ifmaplist = self.check_stds(t[3])
        resultlist = []
        thenmaplist = []
        numthen = ''
        elsemaplist = []
        numelse = ''
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 9:
            try:
                thenmaplist = self.check_stds(t[5])
            except:
                numthen = self.check_null(t[5])
            try:
                elsemaplist = self.check_stds(t[7])
            except:
                numelse = self.check_null(t[7])
        elif len(t) == 11:
            try:
                thenmaplist = self.check_stds(t[5])
            except:
                numthen = self.check_null(t[5])
            try:
                elsemaplist = self.check_stds(t[9])
            except:
                numelse = self.check_null(t[7])

        if thenmaplist != []:
            topolist = self.get_temporal_topo_list(ifmaplist, thenmaplist)
        elif elsemaplist != []:
            topolist = self.get_temporal_topo_list(ifmaplist, elsemaplist)
        if numthen !=  '':
            numinput = numthen
        elif numelse !=  '':
            numinput = numelse

        # Iterate over condition map lists with temporal relations.
        for map_i in topolist:
            # Loop over temporal related maps and create overlay modules.
            tbrelations = map_i.get_temporal_relations()
            count = 0
            for map_j in (tbrelations['EQUAL']):
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapainput = map_i.get_id()
                # Create overlayed map extent.
                returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                        temp_op = '=')
                # Stop the loop if no temporal or spatial relationship exist.
                if returncode == 0:
                    break
                if count == 0:
                    # Set map name.
                    name = map_new.get_id()
                else:
                    # Generate an intermediate map
                    name = self.generate_map_name()

                # Set first input for overlay module.
                mapbinput = map_j.get_id()
                # Create r.mapcalc expression string for the operation.
                if thenmaplist != []:
                    if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                        cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, mapbinput, \
                                                      numinput)
                    elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                        cmdstring = "if(%s,%s,%s)" %(mapainput, map_j.cmd_list, \
                                                      numinput)
                    elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                        cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, map_j.cmd_list, \
                                                      numinput)
                    else:
                        cmdstring = "if(%s,%s,%s)" %(mapainput, mapbinput, numinput)
                if elsemaplist != []:
                    if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                        cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, numinput, \
                                                      mapbinput)
                    elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                        cmdstring = "if(%s,%s,%s)" %(mapainput, numinput, \
                                                      map_j.cmd_list)
                    elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                        cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, numinput, \
                                                      map_j.cmd_list)
                    else:
                        cmdstring = "if(%s,%s,%s)" %(mapainput, numinput, mapbinput)
                # Conditional append of module command.
                map_new.cmd_list = cmdstring
                # Set new map name to temporary map name.
                #mapainput = name
                count += 1
                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_s_numeric_expr_condition_elif_relation(self, t):
        # Examples:
        #   if({during},s_var_expr, 1, A)
        #   if({during}, A == 5, A, null())
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA number COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA NULL LPAREN RPAREN COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA number COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA NULL LPAREN RPAREN COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA stds COMMA number RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA stds COMMA NULL LPAREN RPAREN RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA expr COMMA number RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA expr COMMA NULL LPAREN RPAREN RPAREN
        """
        relations, temporal, function= self.eval_toperator(t[3])
        ifmaplist = self.check_stds(t[5])
        resultlist = []
        thenmaplist = []
        numthen = ''
        elsemaplist = []
        numelse = ''
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 11:
            try:
                thenmaplist = self.check_stds(t[7])
            except:
                numthen = self.check_null(t[7])
            try:
                elsemaplist = self.check_stds(t[9])
            except:
                numelse = self.check_null(t[9])
        elif len(t) == 13:
            try:
                thenmaplist = self.check_stds(t[7])
            except:
                numthen = self.check_null(t[7])
            try:
                elsemaplist = self.check_stds(t[11])
            except:
                numelse = self.check_null(t[9])

        if thenmaplist != []:
            topolist = self.get_temporal_topo_list(ifmaplist, thenmaplist, \
                                                    topolist = relations)
        elif elsemaplist != []:
            topolist = self.get_temporal_topo_list(ifmaplist, elsemaplist, \
                                                    topolist = relations)
        if numthen !=  '':
            numinput = numthen
        elif numelse !=  '':
            numinput = numelse

        # Iterate over condition map lists with temporal relations.
        for map_i in topolist:
            # Loop over temporal related maps and create overlay modules.
            tbrelations = map_i.get_temporal_relations()
            count = 0
            for topo in relations:
                if topo in tbrelations.keys():
                    for map_j in (tbrelations[topo]):
                        # Generate an intermediate map for the result map list.
                        map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                        # Set first input for overlay module.
                        mapainput = map_i.get_id()
                        # Create overlayed map extent.
                        returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                                temp_op = '=')
                        # Stop the loop if no temporal or spatial relationship exist.
                        if returncode == 0:
                            break
                        if count == 0:
                            # Set map name.
                            name = map_new.get_id()
                        else:
                            # Generate an intermediate map
                            name = self.generate_map_name()

                        # Set first input for overlay module.
                        mapbinput = map_j.get_id()
                        # Create r.mapcalc expression string for the operation.
                        if thenmaplist != []:
                            if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                                cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, mapbinput, \
                                                              numinput)
                            elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                                cmdstring = "if(%s,%s,%s)" %(mapainput, map_j.cmd_list, \
                                                              numinput)
                            elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                                cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, map_j.cmd_list, \
                                                              numinput)
                            else:
                                cmdstring = "if(%s,%s,%s)" %(mapainput, mapbinput, numinput)
                        if elsemaplist != []:
                            if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                                cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, numinput, \
                                                              mapbinput)
                            elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                                cmdstring = "if(%s,%s,%s)" %(mapainput, numinput, \
                                                              map_j.cmd_list)
                            elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                                cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, numinput, \
                                                              map_j.cmd_list)
                            else:
                                cmdstring = "if(%s,%s,%s)" %(mapainput, numinput, mapbinput)
                        # Conditional append of module command.
                        map_new.cmd_list = cmdstring
                        # Set new map name to temporary map name.
                        #mapainput = name
                        count += 1
                        # Append map to result map list.
                        if returncode == 1:
                            resultlist.append(map_new)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list


    def p_s_expr_condition_elif_relation(self, t):
        # Examples:
        #   if({equal||during}, s_var_expr, A, B)
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA stds COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA stds COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA expr COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA s_var_expr COMMA expr COMMA expr RPAREN
        """
        relations, temporal, function= self.eval_toperator(t[3])
        ifmaplist = self.check_stds(t[5])
        thenmaplist = self.check_stds(t[7])
        elsemaplist = self.check_stds(t[9])
        resultlist = []
        thendict = {}
        elsedict = {}
        # Get topologies for the appropriate conclusion term.
        thentopolist = self.get_temporal_topo_list(ifmaplist, thenmaplist, \
                                                    topolist = relations)
        # Fill dictionaries with related maps for both conclusion terms.
        for map_i in thentopolist:
            thenrelations = map_i.get_temporal_relations()
            relationmaps = []
            for topo in relations:
                if topo in thenrelations.keys():
                    relationmaps = relationmaps + thenrelations[topo]
            thendict[map_i.get_id()] = relationmaps
        # Get topologies for the alternative conclusion term.
        elsetopolist = self.get_temporal_topo_list(ifmaplist, elsemaplist, \
                                                    topolist = relations)
        for map_i in elsetopolist:
            elserelations = map_i.get_temporal_relations()
            relationmaps = []
            for topo in relations:
                if topo in elserelations.keys():
                    relationmaps = relationmaps + elserelations[topo]
            elsedict[map_i.get_id()] = relationmaps
        # Loop trough conditional map list.
        for map_i in ifmaplist:
            if map_i.get_id() in thendict.keys():
                thenlist = thendict[map_i.get_id()]
            else:
                thenlist = []
            if map_i.get_id() in elsedict.keys():
                elselist = elsedict[map_i.get_id()]
            else:
                elselist = []
            # Set iteration amount to maximal or minimum number of related
            # conclusion maps, depending on null map creation flag.
            if self.null:
                iternum = max(len(thenlist), len(elselist))
            else:
                iternum = min(len(thenlist), len(elselist))
            # Calculate difference in conclusion lengths.
            iterthen = iternum - len(thenlist)
            iterelse = iternum - len(elselist)
            # Extend null maps to the list to get conclusions with same length.
            if iterthen != 0:
                for i in range(iterthen):
                    thenlist.extend(['null()'])
            if iterelse != 0:
                for i in range(iterelse):
                    elselist.extend(['null()'])

            # Combine the conclusions in a paired list.
            conclusionlist = zip(thenlist, elselist)
            for i in range(iternum):
                conclusionmaps = conclusionlist[i]
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapifinput = map_i.get_id()
                # Get conclusion maps.
                map_then = conclusionmaps[0]
                map_else = conclusionmaps[1]

                # Check if conclusions are map objects.
                if map_then != 'null()':
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_then, 'and', \
                                                            temp_op = '=')
                    maptheninput = map_then.get_id()
                    # Continue the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        continue
                else:
                    maptheninput = 'null()'
                # Check if conclusions are map objects.
                if map_else != 'null()':
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_else, 'and', \
                                                            temp_op = '=')
                    mapelseinput = map_else.get_id()
                    # Continue the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        continue
                else:
                    mapelseinput = 'null()'

                #if map_then != 'null()' and map_else != 'null()':
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, maptheninput,
                    mapelseinput)
                elif "cmd_list" in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, map_then.cmd_list,
                    mapelseinput)
                elif "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, maptheninput,
                    map_else.cmd_list)
                elif "cmd_list" in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, map_then.cmd_list,
                    map_else.cmd_list)
                elif "cmd_list" not in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, map_then.cmd_list,
                    mapelseinput)
                elif "cmd_list" not in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, maptheninput,
                    map_else.cmd_list)
                elif "cmd_list" not in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, map_then.cmd_list,
                    map_else.cmd_list)
                else:
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, maptheninput,
                    mapelseinput)
                # Conditional append of module command.
                map_new.cmd_list = cmdstring
                # Append map to result map list.
                if returncode == 1:
                    resultlist.append(map_new)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list

    def p_ts_var_expr1(self, t):
        # Combination of spatial and temporal conditional expressions.
        # Examples:
        #   A <= 2 || start_date <= 2013-01-01
        #   end_date > 2013-01-15 && A > 10
        #  IMPORTANT: Only the intersection of map lists in conditionals are
        #  exported.
        """
        ts_var_expr : s_var_expr AND AND t_var_expr
                    | t_var_expr AND AND s_var_expr
                    | t_var_expr OR OR s_var_expr
                    | s_var_expr OR OR t_var_expr
                    | ts_var_expr AND AND s_var_expr
                    | ts_var_expr AND AND t_var_expr
                    | ts_var_expr OR OR s_var_expr
                    | ts_var_expr OR OR t_var_expr
                    | s_var_expr AND AND ts_var_expr
                    | t_var_expr AND AND ts_var_expr
                    | s_var_expr OR OR ts_var_expr
                    | t_var_expr OR OR ts_var_expr
        """
        # Check whether inputs are map lists or global temporal variables and
        # store each in separate lists.
        ts_var_dict = {"temporal" : [], "spatial" : []}
        temporal_list = []
        spatial_list = []
        operator = t[2] + t[3]
        temporalop = GlobalTemporalVar()
        temporalop.relationop = operator
        temporalop.topology.append("EQUAL")

        if isinstance(t[1], dict) and "spatial" in t[1]:
            temporal_list = temporal_list + t[1]["temporal"]
            spatial_list.append(t[1]["spatial"])
        elif isinstance(t[1], list):
            if all([isinstance(i, ta.GlobalTemporalVar) for i in t[1]]):
                temporal_list = temporal_list + t[1]
            else:
                tsexprA = self.check_stds(t[1])
                spatial_list.append(tsexprA)
        elif isinstance(t[1], ta.GlobalTemporalVar):
            temporal_list.append(t[1])
        if temporal_list != [] and \
            isinstance(t[4], ta.GlobalTemporalVar):
            temporal_list.append(temporalop)
        if temporal_list != [] and \
            isinstance(t[4], list) and \
            all([isinstance(i, ta.GlobalTemporalVar) for i in t[4]]):
            temporal_list.append(temporalop)
        if isinstance(t[4], dict) and "spatial" in t[4]:
            temporal_list = temporal_list + t[4]["temporal"]
            spatial_list.append(t[4]["spatial"])
        elif isinstance(t[4], list):
            if all([isinstance(i, ta.GlobalTemporalVar) for i in t[4]]):
                temporal_list = temporal_list + t[4]
            else:
                tsexprB = self.check_stds(t[4])
                spatial_list.append(tsexprB)
        elif isinstance(t[4], ta.GlobalTemporalVar):
            temporal_list.append(t[4])

        ts_var_dict["temporal"] = temporal_list
        # Condition for two map lists in spatio temporal expression.
        if len(spatial_list) == 2:
            # Build topology for both map lists in spatio temporal expression.
            topolist = self.get_temporal_topo_list(spatial_list[0], spatial_list[1])
            resultlist = []
            for map_i in topolist:
                # Loop over temporal related maps and create overlay modules.
                tbrelations = map_i.get_temporal_relations()
                count = 0
                for map_j in (tbrelations['EQUAL']):
                    # Generate an intermediate map for the result map list.
                    map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                    # Set first input for overlay module.
                    mapainput = map_i.get_id()
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                            temp_op = '=')
                    # Stop the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        break
                    if count == 0:
                        # Set map name.
                        name = map_new.get_id()
                    else:
                        # Generate an intermediate map
                        name = self.generate_map_name()

                    # Set first input for overlay module.
                    mapbinput = map_j.get_id()
                    # Create r.mapcalc expression string for the operation.
                    if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                        cmdstring = "%s %s %s" %(map_new.cmd_list, operator, mapbinput)
                    elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                        cmdstring = "%s %s %s" %(mapainput, operator, map_j.cmd_list)
                    elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                        cmdstring = "%s %s %s" %(map_new.cmd_list, operator, map_j.cmd_list)
                    else:
                        cmdstring = "%s %s %s" %(mapainput, operator, mapbinput)
                    # Conditional append of module command.
                    map_new.cmd_list = cmdstring
                    # Set new map name to temporary map name.
                    #mapainput = name
                    count += 1
                    # Append map to result map list.
                    if returncode == 1:
                      resultlist.append(map_new)
                    # Return dictionary with spatial map list of temporal
                    # intersected maps and temporal expression in list form.
                    ts_var_dict["spatial"] = resultlist
        # Condition for only one map list in spatio temporal expression.
        elif len(spatial_list) == 1:
            ts_var_dict["spatial"] = spatial_list[0]

        t[0] = ts_var_dict

    def p_ts_expr_condition_if(self, t):
        # Examples:
        #   if(ts_var_expr, A)
        #   if(start_year == 2013 || B != 5, A)
        """
        expr : IF LPAREN ts_var_expr COMMA stds RPAREN
             | IF LPAREN ts_var_expr COMMA expr RPAREN
        """
        ts_var_dict = t[3]
        spatialcond = ts_var_dict["spatial"]
        # Extract spatial map list from condition.
        ifmaplist = self.check_stds(spatialcond)
        thenmaplist = self.check_stds(t[5])
        topolist = self.get_temporal_topo_list(ifmaplist, thenmaplist)
        resultlist = []
        resultspatial = []
        for map_i in topolist:
            #print(map_i.get_id())
            # Loop over temporal related maps and create overlay modules.
            tbrelations = map_i.get_temporal_relations()
            count = 0
            for map_j in (tbrelations['EQUAL']):
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapainput = map_i.get_id()
                # Create overlayed map extent.
                returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                        temp_op = '=')
                # Stop the loop if no temporal or spatial relationship exist.
                if returncode == 0:
                    break
                if count == 0:
                    # Set map name.
                    name = map_new.get_id()
                else:
                    # Generate an intermediate map
                    name = self.generate_map_name()

                # Set first input for overlay module.
                mapbinput = map_j.get_id()
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                    cmdstring = "if(%s,%s)" %(map_new.cmd_list, mapbinput)
                elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                    cmdstring = "if(%s,%s)" %(mapainput, map_j.cmd_list)
                elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                    cmdstring = "if(%s,%s)" %(map_new.cmd_list, map_j.cmd_list)
                else:
                    cmdstring = "if(%s,%s)" %(mapainput, mapbinput)
                # Conditional append of module command.
                map_new.cmd_list = cmdstring
                # Set new map name to temporary map name.
                #mapainput = name
                count += 1
                # Append map to result map list.
                if returncode == 1:
                    resultspatial.append(map_new)
        # Evaluate temporal statements in spatio temporal condition.

        #if len(ts_var_dict["temporal"]) == 1:
            #temporalcond = ts_var_dict["temporal"][0]
        #else:
        temporalcond = ts_var_dict["temporal"]
        resultspatial = self.check_stds(resultspatial)
        thencond = self.build_condition_list(temporalcond, resultspatial)
        thenresult = self.eval_condition_list(thencond)
        # Clear the map list.
        resultlist = self.check_stds(thenresult, clear = True)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list


    def p_ts_expr_condition_if_relation(self, t):
        # Examples:
        #   if({equal||during}, ts_var_expr, A)
        #   if({starts||during}, B > 2 || end_month() == 4, A)
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA expr RPAREN
        """
        relations, temporal, function= self.eval_toperator(t[3])
        ts_var_dict = t[5]
        spatialcond = ts_var_dict["spatial"]
        # Extract spatial map list from condition.
        ifmaplist = self.check_stds(spatialcond)
        thenmaplist = self.check_stds(t[7])
        topolist = self.get_temporal_topo_list(ifmaplist, thenmaplist,
                                                topolist = relations)
        resultspatial = []
        for map_i in topolist:
            #print(map_i.get_id())
            # Loop over temporal related maps and create overlay modules.
            tbrelations = map_i.get_temporal_relations()
            count = 0
            for topo in relations:
                if topo in tbrelations.keys():
                    for map_j in (tbrelations[topo]):
                        # Generate an intermediate map for the result map list.
                        map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                        # Set first input for overlay module.
                        mapainput = map_i.get_id()
                        # Create overlayed map extent.
                        returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                                temp_op = '=')
                        # Stop the loop if no temporal or spatial relationship exist.
                        if returncode == 0:
                            break
                        if count == 0:
                            # Set map name.
                            name = map_new.get_id()
                        else:
                            # Generate an intermediate map
                            name = self.generate_map_name()

                        # Set first input for overlay module.
                        mapbinput = map_j.get_id()
                        # Create r.mapcalc expression string for the operation.
                        if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                            cmdstring = "if(%s,%s)" %(map_new.cmd_list, mapbinput)
                        elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                            cmdstring = "if(%s,%s)" %(mapainput, map_j.cmd_list)
                        elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                            cmdstring = "if(%s,%s)" %(map_new.cmd_list, map_j.cmd_list)
                        else:
                            cmdstring = "if(%s,%s)" %(mapainput, mapbinput)
                        # Conditional append of module command.
                        map_new.cmd_list = cmdstring
                        # Set new map name to temporary map name.
                        #mapainput = name
                        count += 1
                        # Append map to result map list.
                        if returncode == 1:
                            resultspatial.append(map_new)
        # Evaluate temporal statements in spatio temporal condition.

        #if len(ts_var_dict["temporal"]) == 1:
            #temporalcond = ts_var_dict["temporal"][0]
        #else:
        temporalcond = ts_var_dict["temporal"]
        resultspatial = self.check_stds(resultspatial)
        thencond = self.build_condition_list(temporalcond, resultspatial, relations)
        thenresult = self.eval_condition_list(thencond)
        # Clear the map list.
        resultlist = self.check_stds(thenresult, clear = True)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list


    def p_ts_numeric_condition_if(self, t):
        # Examples:
        #   if(ts_var_expr, 1)
        #   if(A == 5 && start_day() > 5, 10)
        """
        expr : IF LPAREN ts_var_expr COMMA number RPAREN
             | IF LPAREN ts_var_expr COMMA NULL LPAREN RPAREN RPAREN
        """
        ts_var_dict = t[3]
        spatialcond = ts_var_dict["spatial"]
        ifmaplist = self.check_stds(spatialcond)
        resultspatial = []
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 7:
            numinput = t[5]
        elif len(t) == 9:
            numinput = t[5] + t[6] + t[7]
        # Iterate over condition map list.
        for map_i in ifmaplist:
            mapinput = map_i.get_id()
            # Create r.mapcalc expression string for the operation.
            if "cmd_list" in dir(map_i):
                cmdstring = "if(%s,%s)" %(map_i.cmd_list, numinput)
            else:
                cmdstring = "if(%s,%s)" %(mapinput, numinput)
            # Conditional append of module command.
            map_i.cmd_list = cmdstring
            # Append map to result map list.
            resultspatial.append(map_i)

        temporalcond = ts_var_dict["temporal"]
        resultspatial = self.check_stds(resultspatial)
        thencond = self.build_condition_list(temporalcond, resultspatial)
        thenresult = self.eval_condition_list(thencond)
        # Clear the map list.
        resultlist = self.check_stds(thenresult, clear = True)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list


    def p_ts_expr_condition_elif(self, t):
        # Examples:
        #   if(s_var_expr, A, B)
        #   if(start_day() < 20 && B > 2, A, B)
        """
        expr : IF LPAREN ts_var_expr COMMA stds COMMA stds RPAREN
             | IF LPAREN ts_var_expr COMMA stds COMMA expr RPAREN
             | IF LPAREN ts_var_expr COMMA expr COMMA stds RPAREN
             | IF LPAREN ts_var_expr COMMA expr COMMA expr RPAREN
        """
        ts_var_dict = t[3]
        spatialcond = ts_var_dict["spatial"]
        ifmaplist = self.check_stds(spatialcond)
        thenmaplist = self.check_stds(t[5])
        elsemaplist = self.check_stds(t[7])
        resultspatial = []
        thendict = {}
        elsedict = {}
        # Get topologies for the appropriate conclusion term.
        thentopolist = self.get_temporal_topo_list(ifmaplist, thenmaplist)
        # Fill dictionaries with related maps for both conclusion terms.
        for map_i in thentopolist:
            thenrelations = map_i.get_temporal_relations()
            relationmaps = thenrelations['EQUAL']
            thendict[map_i.get_id()] = relationmaps
        # Get topologies for the alternative conclusion term.
        elsetopolist = self.get_temporal_topo_list(ifmaplist, elsemaplist)
        for map_i in elsetopolist:
            elserelations = map_i.get_temporal_relations()
            relationmaps = elserelations['EQUAL']
            elsedict[map_i.get_id()] = relationmaps
        # Loop through conditional map list.
        for map_i in ifmaplist:
            if map_i.get_id() in thendict.keys():
                thenlist = thendict[map_i.get_id()]
            else:
                thenlist = []
            if map_i.get_id() in elsedict.keys():
                elselist = elsedict[map_i.get_id()]
            else:
                elselist = []
            # Set iteration amount to maximal or minimum number of related
            # conclusion maps, depending on null map creation flag.
            if self.null:
                iternum = max(len(thenlist), len(elselist))
            else:
                iternum = min(len(thenlist), len(elselist))
            # Calculate difference in conclusion lengths.
            iterthen = iternum - len(thenlist)
            iterelse = iternum - len(elselist)
            # Extend null maps to the list to get conclusions with same length.
            if iterthen != 0:
                for i in range(iterthen):
                    thenlist.extend(['null()'])
            if iterelse != 0:
                for i in range(iterelse):
                    elselist.extend(['null()'])
            # Combine the conclusions in a paired list.
            conclusionlist = zip(thenlist, elselist)
            for i in range(iternum):
                conclusionmaps = conclusionlist[i]
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapifinput = map_i.get_id()
                # Get conclusion maps.
                map_then = conclusionmaps[0]
                map_else = conclusionmaps[1]

                # Check if conclusions are map objects.
                if map_then != 'null()':
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_then, 'and', \
                                                            temp_op = '=')
                    maptheninput = map_then.get_id()
                    # Continue the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        continue
                else:
                    maptheninput = 'null()'
                # Check if conclusions are map objects.
                if map_else != 'null()':
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_else, 'and', \
                                                            temp_op = '=')
                    mapelseinput = map_else.get_id()
                    # Continue the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        continue
                else:
                    mapelseinput = 'null()'

                #if map_then != 'null()' and map_else != 'null()':
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, maptheninput,
                    mapelseinput)
                elif "cmd_list" in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, map_then.cmd_list,
                    mapelseinput)
                elif "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, maptheninput,
                    map_else.cmd_list)
                elif "cmd_list" in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, map_then.cmd_list,
                    map_else.cmd_list)
                elif "cmd_list" not in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, map_then.cmd_list,
                    mapelseinput)
                elif "cmd_list" not in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, maptheninput,
                    map_else.cmd_list)
                elif "cmd_list" not in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, map_then.cmd_list,
                    map_else.cmd_list)
                else:
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, maptheninput,
                    mapelseinput)
                # Conditional append of module command.
                map_new.cmd_list = cmdstring
                # Append map to result map list.
                if returncode == 1:
                    resultspatial.append(map_new)

        temporalcond = ts_var_dict["temporal"]
        resultspatial = self.check_stds(resultspatial)
        thencond = self.build_condition_list(temporalcond, resultspatial)
        thenresult = self.eval_condition_list(thencond)
        #elseresult = self.eval_condition_list(thencond, inverse = True)
        # Combine and sort else and then statement to result map list.
        #combilist = thenresult + elseresult
        #resultlist = sorted(combilist, key = AbstractDatasetComparisonKeyStartTime)
        # Clear the map list.
        #resultlist = self.check_stds(resultlist, clear = True)
        resultlist = self.check_stds(thenresult, clear = True)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list


    def p_ts_expr_condition_elif_relation(self, t):
        # Examples:
        #   if({equal||during}, s_var_expr, A, B)
        #   if({contains}, start_day() == 3 || C != 2, A, B)
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA stds COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA stds COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA expr COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA expr COMMA expr RPAREN
        """
        relations, temporal, function= self.eval_toperator(t[3])
        ts_var_dict = t[5]
        spatialcond = ts_var_dict["spatial"]
        # Extract spatial map list from condition.
        ifmaplist = self.check_stds(spatialcond)
        thenmaplist = self.check_stds(t[7])
        elsemaplist = self.check_stds(t[9])
        resultspatial = []
        thendict = {}
        elsedict = {}
        # Get topologies for the appropriate conclusion term.
        thentopolist = self.get_temporal_topo_list(ifmaplist, thenmaplist, \
                                                    topolist = relations)
        # Fill dictionaries with related maps for both conclusion terms.
        for map_i in thentopolist:
            thenrelations = map_i.get_temporal_relations()
            relationmaps = []
            for topo in relations:
                if topo in thenrelations.keys():
                    relationmaps = relationmaps + thenrelations[topo]
            thendict[map_i.get_id()] = relationmaps
        # Get topologies for the alternative conclusion term.
        elsetopolist = self.get_temporal_topo_list(ifmaplist, elsemaplist, \
                                                    topolist = relations)
        for map_i in elsetopolist:
            elserelations = map_i.get_temporal_relations()
            relationmaps = []
            for topo in relations:
                if topo in elserelations.keys():
                    relationmaps = relationmaps + elserelations[topo]
            elsedict[map_i.get_id()] = relationmaps
        # Loop trough conditional map list.
        for map_i in ifmaplist:
            if map_i.get_id() in thendict.keys():
                thenlist = thendict[map_i.get_id()]
            else:
                thenlist = []
            if map_i.get_id() in elsedict.keys():
                elselist = elsedict[map_i.get_id()]
            else:
                elselist = []
            # Set iteration amount to maximal or minimum number of related
            # conclusion maps, depending on null map creation flag.
            if self.null:
                iternum = max(len(thenlist), len(elselist))
            else:
                iternum = min(len(thenlist), len(elselist))
            # Calculate difference in conclusion lengths.
            iterthen = iternum - len(thenlist)
            iterelse = iternum - len(elselist)
            # Extend null maps to the list to get conclusions with same length.
            if iterthen != 0:
                for i in range(iterthen):
                    thenlist.extend(['null()'])
            if iterelse != 0:
                for i in range(iterelse):
                    elselist.extend(['null()'])

            # Combine the conclusions in a paired list.
            conclusionlist = zip(thenlist, elselist)
            for i in range(iternum):
                conclusionmaps = conclusionlist[i]
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapifinput = map_i.get_id()
                # Get conclusion maps.
                map_then = conclusionmaps[0]
                map_else = conclusionmaps[1]

                # Check if conclusions are map objects.
                if map_then != 'null()':
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_then, 'and', \
                                                            temp_op = '=')
                    maptheninput = map_then.get_id()
                    # Continue the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        continue
                else:
                    maptheninput = 'null()'
                # Check if conclusions are map objects.
                if map_else != 'null()':
                    # Create overlayed map extent.
                    returncode = self.overlay_map_extent(map_new, map_else, 'and', \
                                                            temp_op = '=')
                    mapelseinput = map_else.get_id()
                    # Continue the loop if no temporal or spatial relationship exist.
                    if returncode == 0:
                        continue
                else:
                    mapelseinput = 'null()'

                #if map_then != 'null()' and map_else != 'null()':
                # Create r.mapcalc expression string for the operation.
                if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, maptheninput,
                    mapelseinput)
                elif "cmd_list" in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, map_then.cmd_list,
                    mapelseinput)
                elif "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, maptheninput,
                    map_else.cmd_list)
                elif "cmd_list" in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(map_new.cmd_list, map_then.cmd_list,
                    map_else.cmd_list)
                elif "cmd_list" not in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" not in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, map_then.cmd_list,
                    mapelseinput)
                elif "cmd_list" not in dir(map_new) and "cmd_list" not in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, maptheninput,
                    map_else.cmd_list)
                elif "cmd_list" not in dir(map_new) and "cmd_list" in dir(map_then) \
                    and "cmd_list" in dir(map_else):
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, map_then.cmd_list,
                    map_else.cmd_list)
                else:
                    cmdstring = "if(%s, %s, %s)" %(mapifinput, maptheninput,
                    mapelseinput)
                # Conditional append of module command.
                map_new.cmd_list = cmdstring
                # Append map to result map list.
                if returncode == 1:
                    resultspatial.append(map_new)

        temporalcond = ts_var_dict["temporal"]
        resultspatial = self.check_stds(resultspatial)
        thencond = self.build_condition_list(temporalcond, resultspatial, relations)
        thenresult = self.eval_condition_list(thencond)
        #elseresult = self.eval_condition_list(thencond, inverse = True)
        # Combine and sort else and then statement to result map list.
        #combilist = thenresult + elseresult
        #resultlist = sorted(combilist, key = AbstractDatasetComparisonKeyStartTime)
        # Clear the map list.
        resultlist = self.check_stds(thenresult, clear = True)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list


    def p_ts_numeric_condition_elif(self, t):
        # Examples:
        #   if(ts_var_expr, 1, 2)
        #   if(A == 1 || end_year == 2013, 10, null())
        """
        expr : IF LPAREN ts_var_expr COMMA number COMMA number RPAREN
             | IF LPAREN ts_var_expr COMMA NULL LPAREN RPAREN COMMA number RPAREN
             | IF LPAREN ts_var_expr COMMA number COMMA NULL LPAREN RPAREN RPAREN
             | IF LPAREN ts_var_expr COMMA NULL LPAREN RPAREN COMMA NULL LPAREN RPAREN RPAREN
        """
        ts_var_dict = t[3]
        spatialcond = ts_var_dict["spatial"]
        # Extract spatial map list from condition.
        ifmaplist = self.check_stds(spatialcond)
        resultspatial = []
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 9:
            numthen = t[5]
            numelse = t[7]
        elif len(t) == 11:
            numthen = self.check_null(t[5])
            numelse = self.check_null(t[7])
        elif len(t) == 13:
            numthen = self.check_null(t[5])
            numelse = self.check_null(t[9])

        # Iterate over condition map list.
        for map_i in ifmaplist:
            mapinput = map_i.get_id()
            # Create r.mapcalc expression string for the operation.
            if "cmd_list" in dir(map_i):
                cmdstring = "if(%s, %s, %s)" %(map_i.cmd_list, numthen, numelse)
            else:
                cmdstring = "if(%s, %s, %s)" %(mapinput, numthen, numelse)
            # Conditional append of module command.
            map_i.cmd_list = cmdstring
            # Append map to result map list.
            resultspatial.append(map_i)

        temporalcond = ts_var_dict["temporal"]
        resultspatial = self.check_stds(resultspatial)
        thencond = self.build_condition_list(temporalcond, resultspatial)
        thenresult = self.eval_condition_list(thencond)
        #elseresult = self.eval_condition_list(thencond, inverse = True)
        # Combine and sort else and then statement to result map list.
        #combilist = thenresult + elseresult
        #resultlist = sorted(combilist, key = AbstractDatasetComparisonKeyStartTime)
        # Clear the map list.
        #resultlist = self.check_stds(resultlist, clear = True)
        resultlist = self.check_stds(thenresult, clear = True)

        t[0] = resultlist

    def p_ts_numeric_expr_condition_elif(self, t):
        # Examples:
        #   if(ts_var_expr, 1, A)
        #   if(A == 5 && start_day() > 5, A, null())
        """
        expr : IF LPAREN ts_var_expr COMMA number COMMA stds RPAREN
             | IF LPAREN ts_var_expr COMMA NULL LPAREN RPAREN COMMA stds RPAREN
             | IF LPAREN ts_var_expr COMMA number COMMA expr RPAREN
             | IF LPAREN ts_var_expr COMMA NULL LPAREN RPAREN COMMA expr RPAREN
             | IF LPAREN ts_var_expr COMMA stds COMMA number RPAREN
             | IF LPAREN ts_var_expr COMMA stds COMMA NULL LPAREN RPAREN RPAREN
             | IF LPAREN ts_var_expr COMMA expr COMMA number RPAREN
             | IF LPAREN ts_var_expr COMMA expr COMMA NULL LPAREN RPAREN RPAREN
        """
        ts_var_dict = t[3]
        spatialcond = ts_var_dict["spatial"]
        ifmaplist = self.check_stds(spatialcond)
        resultspatial = []
        thenmaplist = []
        numthen = ''
        elsemaplist = []
        numelse = ''
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 9:
            try:
                thenmaplist = self.check_stds(t[5])
            except:
                numthen = t[5]
            try:
                elsemaplist = self.check_stds(t[7])
            except:
                numelse = t[7]
        elif len(t) == 11:
            try:
                thenmaplist = self.check_stds(t[5])
            except:
                numthen = t[5] + t[6] + t[7]
            try:
                elsemaplist = self.check_stds(t[9])
            except:
                numelse = t[7] + t[8] + t[9]
        if thenmaplist != []:
            topolist = self.get_temporal_topo_list(ifmaplist, thenmaplist)
        elif elsemaplist != []:
            topolist = self.get_temporal_topo_list(ifmaplist, elsemaplist)
        if numthen !=  '':
            numinput = numthen
        elif numelse !=  '':
            numinput = numelse

        # Iterate over condition map lists with temporal relations.
        for map_i in topolist:
            # Loop over temporal related maps and create overlay modules.
            tbrelations = map_i.get_temporal_relations()
            count = 0
            for map_j in (tbrelations['EQUAL']):
                # Generate an intermediate map for the result map list.
                map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                # Set first input for overlay module.
                mapainput = map_i.get_id()
                # Create overlayed map extent.
                returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                        temp_op = '=')
                # Stop the loop if no temporal or spatial relationship exist.
                if returncode == 0:
                    break
                if count == 0:
                    # Set map name.
                    name = map_new.get_id()
                else:
                    # Generate an intermediate map
                    name = self.generate_map_name()

                # Set first input for overlay module.
                mapbinput = map_j.get_id()
                # Create r.mapcalc expression string for the operation.
                if thenmaplist != []:
                    if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                        cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, mapbinput, \
                                                      numinput)
                    elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                        cmdstring = "if(%s,%s,%s)" %(mapainput, map_j.cmd_list, \
                                                      numinput)
                    elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                        cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, map_j.cmd_list, \
                                                      numinput)
                    else:
                        cmdstring = "if(%s,%s,%s)" %(mapainput, mapbinput, numinput)
                if elsemaplist != []:
                    if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                        cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, numinput, \
                                                      mapbinput)
                    elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                        cmdstring = "if(%s,%s,%s)" %(mapainput, numinput, \
                                                      map_j.cmd_list)
                    elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                        cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, numinput, \
                                                      map_j.cmd_list)
                    else:
                        cmdstring = "if(%s,%s,%s)" %(mapainput, numinput, mapbinput)
                # Conditional append of module command.
                map_new.cmd_list = cmdstring
                # Set new map name to temporary map name.
                #mapainput = name
                count += 1
                # Append map to result map list.
                if returncode == 1:
                    resultspatial.append(map_new)

        temporalcond = ts_var_dict["temporal"]
        resultspatial = self.check_stds(resultspatial)
        thencond = self.build_condition_list(temporalcond, resultspatial)
        thenresult = self.eval_condition_list(thencond)
        # Clear the map list.
        resultlist = self.check_stds(thenresult, clear = True)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list


    def p_ts_numeric_expr_condition_elif_relation(self, t):
        # Examples:
        #   if({during},ts_var_expr, 1, A)
        #   if({during}, A == 5 && start_day() > 5, A, null())
        """
        expr : IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA number COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA NULL LPAREN RPAREN COMMA stds RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA number COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA NULL LPAREN RPAREN COMMA expr RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA stds COMMA number RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA stds COMMA NULL LPAREN RPAREN RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA expr COMMA number RPAREN
             | IF LPAREN T_REL_OPERATOR COMMA ts_var_expr COMMA expr COMMA NULL LPAREN RPAREN RPAREN
        """
        relations, temporal, function= self.eval_toperator(t[3])
        ts_var_dict = t[5]
        spatialcond = ts_var_dict["spatial"]
        ifmaplist = self.check_stds(spatialcond)
        resultspatial = []
        thenmaplist = []
        numthen = ''
        elsemaplist = []
        numelse = ''
        # Select input for r.mapcalc expression based on length of PLY object.
        if len(t) == 11:
            try:
                thenmaplist = self.check_stds(t[7])
            except:
                numthen = t[7]
            try:
                elsemaplist = self.check_stds(t[9])
            except:
                numelse = t[9]
        elif len(t) == 13:
            try:
                thenmaplist = self.check_stds(t[7])
            except:
                numthen = t[9] + t[10] + t[11]
            try:
                elsemaplist = self.check_stds(t[11])
            except:
                numelse = t[9] + t[10] + t[11]
        if thenmaplist != []:
            topolist = self.get_temporal_topo_list(ifmaplist, thenmaplist, \
                                                    topolist = relations)
        elif elsemaplist != []:
            topolist = self.get_temporal_topo_list(ifmaplist, elsemaplist, \
                                                    topolist = relations)
        if numthen !=  '':
            numinput = numthen
        elif numelse !=  '':
            numinput = numelse

        # Iterate over condition map lists with temporal relations.
        for map_i in topolist:
            # Loop over temporal related maps and create overlay modules.
            tbrelations = map_i.get_temporal_relations()
            count = 0
            for topo in relations:
                if topo in tbrelations.keys():
                    for map_j in (tbrelations[topo]):
                        # Generate an intermediate map for the result map list.
                        map_new = self.generate_new_map(base_map=map_i, bool_op = 'and', copy = True)
                        # Set first input for overlay module.
                        mapainput = map_i.get_id()
                        # Create overlayed map extent.
                        returncode = self.overlay_map_extent(map_new, map_j, 'and', \
                                                                temp_op = '=')
                        # Stop the loop if no temporal or spatial relationship exist.
                        if returncode == 0:
                            break
                        if count == 0:
                            # Set map name.
                            name = map_new.get_id()
                        else:
                            # Generate an intermediate map
                            name = self.generate_map_name()

                        # Set first input for overlay module.
                        mapbinput = map_j.get_id()
                        # Create r.mapcalc expression string for the operation.
                        if thenmaplist != []:
                            if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                                cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, mapbinput, \
                                                              numinput)
                            elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                                cmdstring = "if(%s,%s,%s)" %(mapainput, map_j.cmd_list, \
                                                              numinput)
                            elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                                cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, map_j.cmd_list, \
                                                              numinput)
                            else:
                                cmdstring = "if(%s,%s,%s)" %(mapainput, mapbinput, numinput)
                        if elsemaplist != []:
                            if "cmd_list" in dir(map_new) and "cmd_list" not in dir(map_j):
                                cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, numinput, \
                                                              mapbinput)
                            elif "cmd_list" in dir(map_j) and "cmd_list" not in dir(map_new):
                                cmdstring = "if(%s,%s,%s)" %(mapainput, numinput, \
                                                              map_j.cmd_list)
                            elif "cmd_list" in dir(map_j) and "cmd_list" in dir(map_new):
                                cmdstring = "if(%s,%s,%s)" %(map_new.cmd_list, numinput, \
                                                              map_j.cmd_list)
                            else:
                                cmdstring = "if(%s,%s,%s)" %(mapainput, numinput, mapbinput)
                        # Conditional append of module command.
                        map_new.cmd_list = cmdstring
                        # Set new map name to temporary map name.
                        #mapainput = name
                        count += 1
                        # Append map to result map list.
                        if returncode == 1:
                            resultspatial.append(map_new)

        temporalcond = ts_var_dict["temporal"]
        resultspatial = self.check_stds(resultspatial)
        thencond = self.build_condition_list(temporalcond, resultspatial)
        thenresult = self.eval_condition_list(thencond)
        # Clear the map list.
        resultlist = self.check_stds(thenresult, clear = True)

        t[0] = resultlist

        if self.debug:
            for map in resultlist:
                print map.cmd_list


###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
