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
    >>> expression =  'R = A[0,1,0] / B[0,0,1] * 20 + C[0,1,1] - 2.45'
    >>> p.test(expression)
    R = A[0,1,0] / B[0,0,1] * 20 + C[0,1,1] - 2.45
    LexToken(NAME,'R',1,0)
    LexToken(EQUALS,'=',1,2)
    LexToken(NAME,'A',1,4)
    LexToken(L_SPAREN,'[',1,5)
    LexToken(INT,0,1,6)
    LexToken(COMMA,',',1,7)
    LexToken(INT,1,1,8)
    LexToken(COMMA,',',1,9)
    LexToken(INT,0,1,10)
    LexToken(R_SPAREN,']',1,11)
    LexToken(DIV,'/',1,13)
    LexToken(NAME,'B',1,15)
    LexToken(L_SPAREN,'[',1,16)
    LexToken(INT,0,1,17)
    LexToken(COMMA,',',1,18)
    LexToken(INT,0,1,19)
    LexToken(COMMA,',',1,20)
    LexToken(INT,1,1,21)
    LexToken(R_SPAREN,']',1,22)
    LexToken(MULT,'*',1,24)
    LexToken(INT,20,1,26)
    LexToken(ADD,'+',1,29)
    LexToken(NAME,'C',1,31)
    LexToken(L_SPAREN,'[',1,32)
    LexToken(INT,0,1,33)
    LexToken(COMMA,',',1,34)
    LexToken(INT,1,1,35)
    LexToken(COMMA,',',1,36)
    LexToken(INT,1,1,37)
    LexToken(R_SPAREN,']',1,38)
    LexToken(SUB,'-',1,40)
    LexToken(FLOAT,2.45,1,42)

@endcode
"""

from temporal_raster_base_algebra import *

###############################################################################

class TemporalRasterAlgebraParser(TemporalRasterBaseAlgebraParser):
    """The temporal raster algebra class"""

    def __init__(self, pid=None, run=False, debug=True, spatial = False, nprocs = 1, register_null = False):
        TemporalRasterBaseAlgebraParser.__init__(self, pid, run, debug, spatial, nprocs, register_null)

        self.m_mapcalc = pymod.Module('r.mapcalc')
        self.m_mremove = pymod.Module('g.mremove')

    def parse(self, expression, basename = None, overwrite=False):
        self.lexer = TemporalRasterAlgebraLexer()
        self.lexer.build()
        self.parser = yacc.yacc(module=self, debug=self.debug)

        self.overwrite = overwrite
        self.count = 0
        self.stdstype = "strds"
        self.basename = basename
        self.expression = expression
        self.parser.parse(expression)

    def remove_empty_maps(self):
        """! Removes the intermediate vector maps.
        """
        if self.empty_maps:
            self.msgr.message(_("Removing empty raster maps"))
            namelist = self.empty_maps.values()
            max = 100
            chunklist = [namelist[i:i + max] for i in range(0, len(namelist), max)]
            for chunk in chunklist:
                stringlist = ",".join(chunk)

                if self.run:
                    m = copy.deepcopy(self.m_mremove)
                    m.inputs["rast"].value = stringlist
                    m.flags["f"].value = True
                    m.run()

    ######################### Temporal functions ##############################

    def p_statement_assign(self, t):
        # The expression should always return a list of maps.
        """
        statement : stds EQUALS expr
        """
        TemporalRasterBaseAlgebraParser.p_statement_assign(self, t)

    def p_ts_neighbour_operation(self, t):
        # Examples:
        # A[1,0]
        # B[-2]
        # C[-2,1,3]
        """
        expr : stds L_SPAREN number COMMA number R_SPAREN
             | stds L_SPAREN number R_SPAREN
             | stds L_SPAREN number COMMA number COMMA number R_SPAREN
        """
        # Check input stds.
        maplist = self.check_stds(t[1])
        row_neigbour = None
        col_neigbour = None
        if len(t) == 5:
            t_neighbour = t[3]
        elif len(t) == 7:
            t_neighbour = 0
            row_neigbour = t[3]
            col_neigbour = t[5]
        elif len(t) == 9:
            t_neighbour = t[7]
            row_neigbour = t[3]
            col_neigbour = t[5]
        if self.run:
            resultlist = []
            max_index = len(maplist)
            for map_i in maplist:
                # Get map index and temporal extent.
                map_index = maplist.index(map_i)
                new_index = map_index + t_neighbour
                if new_index < max_index and new_index >= 0:
                    map_i_t_extent = map_i.get_temporal_extent()
                    # Get neighbouring map and set temporal extent.
                    map_n = maplist[new_index]
                    # Generate an intermediate map for the result map list.
                    map_new = self.generate_new_map(map_n, bool_op = 'and', copy = True)
                    map_new.set_temporal_extent(map_i_t_extent)
                    # Create r.mapcalc expression string for the operation.
                    if "cmd_list" in dir(map_new) and len(t) == 5:
                        cmdstring = "%s" %(map_new.cmd_list)
                    elif "cmd_list" not in dir(map_new) and len(t) == 5:
                        cmdstring = "%s" %(map_n.get_id())
                    elif "cmd_list" in dir(map_new) and len(t) in (7, 9):
                        cmdstring = "%s[%s,%s]" %(map_new.cmd_list, row_neigbour, col_neigbour)
                    elif "cmd_list" not in dir(map_new) and len(t) in (7, 9):
                        cmdstring = "%s[%s,%s]" %(map_n.get_id(), row_neigbour, col_neigbour)
                    # Set new command list for map.
                    map_new.cmd_list = cmdstring
                    # Append map with updated command list to result list.
                    resultlist.append(map_new)

            t[0] = resultlist

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()


