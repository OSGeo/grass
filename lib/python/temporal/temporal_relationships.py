"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

Usage:

@code
import grass.temporal as tgis

tgis.print_temporal_relations(maps)
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from abstract_dataset import *
from datetime_math import *


def print_temporal_relations(maps1, maps2):
    """!Print the temporal relation matrix of the temporal ordered map lists maps1 and maps2
       to stdout.
	
	@param maps1: a ordered by start_time list of map objects
	@param maps2: a ordered by start_time list of map objects
	@param dbif: The database interface to be used
    """
    
    identical = False
    use_id = True
    
    if maps1 == maps2:
	identical = True
	use_id = False

    for i in range(len(maps1)):
	if identical == True:
	    start = i + 1
	else:
	    start = 0
	for j in range(start, len(maps2)):
	    relation = maps1[j].temporal_relation(maps2[i])

	    if use_id == False:
		print maps2[j].base.get_name(), relation, maps1[i].base.get_name()
	    else:
		print maps2[j].base.get_id(), relation, maps1[i].base.get_id()

	    # Break if the last map follows
	    if relation == "follows":
		if j < len(maps1) - 1:
		    relation = maps1[j + 1].temporal_relation(maps2[i])
		    if relation == "after":
			break
	    # Break if the the next map is after
	    if relation == "after":
		break

def get_temporal_relation_matrix(maps1, maps2):
    """!Return the temporal relation matrix of all registered maps as list of lists

	The map list must be ordered by start time

	The temporal relation matrix includes the temporal relations between
	all registered maps. Returned is a nested dict representing 
	a sparse (upper right side in case maps1 == maps2) relationship matrix.
	
	@param maps: a ordered by start_time list of map objects
	@param dbif: The database interface to be used
    """

    matrix = {}
    identical = False
    
    if maps1 == maps2:
	identical = True

    for i in range(len(maps1)):
	if identical == True:
	    start = i + 1
	else:
	    start = 0
	    
	row = {}
	    
	for j in range(start, len(maps2)):
	    relation = maps1[j].temporal_relation(maps2[i])

	    row[maps2[j].base.get_id()] = relation 

	    # Break if the last map follows
	    if relation == "follows":
		if j < len(maps1) - 1:
		    relation = maps1[j + 1].temporal_relation(maps2[i])
		    if relation == "after":
			break
	    # Break if the the next map is after
	    if relation == "after":
		break

	matrix[maps1[i].base.get_id()] = row

    return matrix

def count_temporal_relations(maps1, maps2):
    """!Count the temporal relations between the registered maps.

	The map lists must be ordered by start time. Temporal relations are counted 
	by analyzing the sparse (upper right side in case maps1 == maps2) temporal relationships matrix.

	@param maps: A sorted (start_time) list of abstract_dataset objects
	@param dbif: The database interface to be used
	@return A dictionary with counted temporal relationships
    """
    
    tcount = {}
    identical = False
    
    if maps1 == maps2:
	identical = True

    for i in range(len(maps1)):
	if identical == True:
	    start = i + 1
	else:
	    start = 0
	for j in range(start, len(maps2)):
	    relation = maps1[j].temporal_relation(maps2[i])

	    if tcount.has_key(relation):
		tcount[relation] = tcount[relation] + 1
	    else:
		tcount[relation] = 1

	    # Break if the last map follows
	    if relation == "follows":
		if j < len(maps1) - 1:
		    relation = maps1[j + 1].temporal_relation(maps2[i])
		    if relation == "after":
			break
	    # Break if the the next map is after
	    if relation == "after":
		break  

    return tcount

