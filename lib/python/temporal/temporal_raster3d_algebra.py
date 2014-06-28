"""!@package grass.temporal

Temporal raster algebra

(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Thomas Leppelt and Soeren Gebbert

"""

import grass.pygrass.modules as pygrass
from temporal_raster_base_algebra import *

###############################################################################

class TemporalRaster3DAlgebraParser(TemporalRasterBaseAlgebraParser):
    """The temporal raster algebra class"""

    def __init__(self, pid=None, run=False, debug=True, spatial = False, nprocs = 1, register_null = False):
        TemporalRasterBaseAlgebraParser.__init__(self, pid, run, debug, spatial, nprocs, register_null)

        self.m_mapcalc = pymod.Module('r3.mapcalc')
        self.m_mremove = pymod.Module('g.mremove')

    def parse(self, expression, basename = None, overwrite=False):
        self.lexer = TemporalRasterAlgebraLexer()
        self.lexer.build()
        self.parser = yacc.yacc(module=self, debug=self.debug)

        self.overwrite = overwrite
        self.count = 0
        self.stdstype = "str3ds"
        self.basename = basename
        self.expression = expression
        self.parser.parse(expression)

    def remove_empty_maps(self):
        """! Removes the intermediate vector maps.
        """
        if self.empty_maps:
            self.msgr.message(_("Removing empty 3D raster maps"))
            namelist = self.empty_maps.values()
            max = 100
            chunklist = [namelist[i:i + max] for i in range(0, len(namelist), max)]
            for chunk in chunklist:
                stringlist = ",".join(chunk)

                if self.run:
                    m = copy.deepcopy(self.m_mremove)
                    m.inputs["type"].value = "rast3d"
                    m.inputs["pattern"].value = stringlist
                    m.flags["f"].value = True
                    m.run()

    ######################### Temporal functions ##############################

    def p_statement_assign(self, t):
        # The expression should always return a list of maps.
        """
        statement : stds EQUALS expr
        """
        TemporalRasterBaseAlgebraParser.p_statement_assign(self, t)

    def p_ts_neighbor_operation(self, t):
        # Examples:
        # A[1,0,-1]
        # B[-2]
        # C[1,-2,1,3]
        """
        expr : stds L_SPAREN number COMMA number COMMA number R_SPAREN
             | stds L_SPAREN number R_SPAREN
             | stds L_SPAREN number COMMA number COMMA number COMMA number R_SPAREN
        """
        # Check input stds.
        maplist = self.check_stds(t[1])
        t_neighbor = 0
        row_neighbor = None
        col_neighbor = None
        depth_neighbor = None
        if len(t) == 5:
            t_neighbor = t[3]
        elif len(t) == 9:
            row_neighbor = t[3]
            col_neighbor = t[5]
            depth_neighbor = t[7]
        elif len(t) == 11:
            t_neighbor = t[9]
            row_neighbor = t[3]
            col_neighbor = t[5]
            depth_neighbor = t[7]
        if self.run:
            resultlist = []
            max_index = len(maplist)
            for map_i in maplist:
                # Get map index and temporal extent.
                map_index = maplist.index(map_i)
                new_index = map_index + t_neighbor
                if new_index < max_index and new_index >= 0:
                    map_i_t_extent = map_i.get_temporal_extent()
                    # Get neighboring map and set temporal extent.
                    map_n = maplist[new_index]
                    # Generate an intermediate map for the result map list.
                    map_new = self.generate_new_map(map_n, bool_op = 'and', copy = True)
                    map_new.set_temporal_extent(map_i_t_extent)
                    # Create r.mapcalc expression string for the operation.
                    if "cmd_list" in dir(map_new) and len(t) == 5:
                        cmdstring = "%s" %(map_new.cmd_list)
                    elif "cmd_list" not in dir(map_new) and len(t) == 5:
                        cmdstring = "%s" %(map_n.get_id())
                    elif "cmd_list" in dir(map_new) and len(t) in (9,11):
                        cmdstring = "%s[%s,%s,%s]" %(map_new.cmd_list, row_neighbor, col_neighbor, depth_neighbor)
                    elif "cmd_list" not in dir(map_new) and len(t) in (9,11):
                        cmdstring = "%s[%s,%s,%s]" %(map_n.get_id(), row_neighbor, col_neighbor, depth_neighbor)
                    # Set new command list for map.
                    map_new.cmd_list = cmdstring
                    # Append map with updated command list to result list.
                    resultlist.append(map_new)

            t[0] = resultlist

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()


