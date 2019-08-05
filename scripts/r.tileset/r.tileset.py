#!/usr/bin/env python3

############################################################################
#
# MODULE:       r.tileset
#
# AUTHOR(S):    Cedric Shock
#               Updated for GRASS7 by Martin Landa, 2009
#
# PURPOSE:      To produce tilings of regions in other projections.
#
# COPYRIGHT:    (C) 2006-2009 by Cedric Shoc, Martin Landa, and GRASS development team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

# Bugs:
#  Does not know about meridians in projections. However, unlike the usual
#   hack used to find meridians, this code is perfectly happy with arbitrary
#   rotations and flips
#  The following are planned to be fixed in a future version, if it turns out
#   to be necessary for someone:
#   Does not generate optimal tilings. This means that between an appropriate
#    projection for a region and latitude longitude projection, in the
#    degenerate case, it may create tiles demanding up to twice the necessary
#    information. Requesting data from cylindrical projections near their poles
#    results in divergence. You really don't want to use source data from
#    someone who put it in a cylindrical projection near a pole, do you?
#   Not generating "optimal" tilings has another side effect; the sanity
#    of the destination region will not carry over to generating tiles of
#    realistic aspect ratio. This might be a problem for some WMS servers
#    presenting data in a highly inappropriate projection. Do you really
#    want their data?

#%module
#% description: Produces tilings of the source projection for use in the destination region and projection.
#% keyword: raster
#% keyword: tiling
#%end
#%flag
#% key: g
#% description: Produces shell script output
#%end
#%flag
#% key: w
#% description: Produces web map server query string output
#%end
#%option
#% key: region
#% type: string
#% description: Name of region to use instead of current region for bounds and resolution
#%end
#%option
#% key: sourceproj
#% type: string
#% description: Source projection
#% required : yes
#%end
#%option
#% key: sourcescale
#% type: string
#% description: Conversion factor from units to meters in source projection
#% answer : 1
#%end
#%option
#% key: destproj
#% type: string
#% description: Destination projection, defaults to this location's projection
#% required : no
#%end
#%option
#% key: destscale
#% type: string
#% description: Conversion factor from units to meters in source projection
#% required : no
#%end
#%option
#% key: maxcols
#% type: integer
#% description: Maximum number of columns for a tile in the source projection
#% answer: 1024
#%end
#%option
#% key: maxrows
#% type: integer
#% description: Maximum number of rows for a tile in the source projection
#% answer: 1024
#%end
#%option
#% key: overlap
#% type: integer
#% description: Number of cells tiles should overlap in each direction
#% answer: 0
#%end
#%option G_OPT_F_SEP
#% description: Output field separator
#%end

# Data structures used in this program:
# A bounding box:
# 0 -> left, 1-> bottom, 2->right, 3-> top
# A border:
# An array of points indexed by 0 for "x" and 4 for "y" + by number 0, 1, 2, and 3
# A reprojector [0] is name of source projection, [1] is name of destination
# A projection - [0] is proj.4 text, [1] is scale
from __future__ import print_function

import sys
import tempfile
import math

from grass.script.utils import separator
from grass.script import core as gcore
from grass.script.utils import decode
from grass.exceptions import CalledModuleError


def bboxToPoints(bbox):
    """Make points that are the corners of a bounding box"""
    points = []
    points.append((bbox['w'], bbox['s']))
    points.append((bbox['w'], bbox['n']))
    points.append((bbox['e'], bbox['n']))
    points.append((bbox['e'], bbox['s']))

    return points


def pointsToBbox(points):
    bbox = {}
    min_x = min_y = max_x = max_y = None
    for point in points:
        if not min_x:
            min_x = max_x = point[0]
        if not min_y:
            min_y = max_y = point[1]

        if min_x > point[0]:
            min_x = point[0]
        if max_x < point[0]:
            max_x = point[0]
        if min_y > point[1]:
            min_y = point[1]
        if max_y < point[1]:
            max_y = point[1]

    bbox['n'] = max_y
    bbox['s'] = min_y
    bbox['w'] = min_x
    bbox['e'] = max_x

    return bbox


def project(file, source, dest):
    """Projects point (x, y) using projector"""
    errors = 0
    points = []
    try:
        ret = gcore.read_command('m.proj',
                                 quiet=True,
                                 flags='d',
                                 proj_in=source['proj'],
                                 proj_out=dest['proj'],
                                 sep=';',
                                 input=file)
        ret = decode(ret)
    except CalledModuleError:
        gcore.fatal(cs2cs + ' failed')

    if not ret:
        gcore.fatal(cs2cs + ' failed')

    for line in ret.splitlines():
        if "*" in line:
            errors += 1
        else:
            p_x2, p_y2, p_z2 = list(map(float, line.split(';')))
            points.append((p_x2 / dest['scale'], p_y2 / dest['scale']))

    return points, errors


def projectPoints(points, source, dest):
    """Projects a list of points"""
    dest_points = []

    input = tempfile.NamedTemporaryFile(mode="wt")
    for point in points:
        input.file.write('%f;%f\n' % (point[0] * source['scale'],
                                      point[1] * source['scale']))
    input.file.flush()

    dest_points, errors = project(input.name, source, dest)

    return dest_points, errors


def sideLengths(points, xmetric, ymetric):
    """Find the length of sides of a set of points from one to the next"""
    ret = []
    for i in range(len(points)):
        x1, y1 = points[i]
        j = i + 1
        if j >= len(points):
            j = 0
        sl_x = (points[j][0] - points[i][0]) * xmetric
        sl_y = (points[j][1] - points[i][1]) * ymetric
        sl_d = math.sqrt(sl_x * sl_x + sl_y * sl_y)
        ret.append(sl_d)

    return {'x': (ret[1], ret[3]), 'y': (ret[0], ret[2])}


def bboxesIntersect(bbox_1, bbox_2):
    """Determine if two bounding boxes intersect"""
    bi_a1 = (bbox_1['w'], bbox_1['s'])
    bi_a2 = (bbox_1['e'], bbox_1['n'])
    bi_b1 = (bbox_2['w'], bbox_2['s'])
    bi_b2 = (bbox_2['e'], bbox_2['n'])
    cin = [False, False]
    for i in (0, 1):
        if (bi_a1[i] <= bi_b1[i] and bi_a2[i] >= bi_b1[i]) or \
           (bi_a1[i] <= bi_b1[i] and bi_a2[i] >= bi_b2[i]) or \
           (bi_b1[i] <= bi_a1[i] and bi_b2[i] >= bi_a1[i]) or \
           (bi_b1[i] <= bi_a1[i] and bi_b2[i] >= bi_a2[i]):
            cin[i] = True

    if cin[0] and cin[1]:
        return True

    return False


def main():
    # Take into account those extra pixels we'll be a addin'
    max_cols = int(options['maxcols']) - int(options['overlap'])
    max_rows = int(options['maxrows']) - int(options['overlap'])

    if max_cols == 0:
        gcore.fatal(_("It is not possible to set 'maxcols=%s' and "
                      "'overlap=%s'. Please set maxcols>overlap" %
                      (options['maxcols'], options['overlap'])))
    elif max_rows == 0:
        gcore.fatal(_("It is not possible to set 'maxrows=%s' and "
                      "'overlap=%s'. Please set maxrows>overlap" %
                      (options['maxrows'], options['overlap'])))
    # destination projection
    if not options['destproj']:
        dest_proj = gcore.read_command('g.proj',
                                       quiet=True,
                                       flags='jf')
        dest_proj = decode(dest_proj).rstrip('\n')
        if not dest_proj:
            gcore.fatal(_('g.proj failed'))
    else:
        dest_proj = options['destproj']
    gcore.debug("Getting destination projection -> '%s'" % dest_proj)

    # projection scale
    if not options['destscale']:
        ret = gcore.parse_command('g.proj',
                                  quiet=True,
                                  flags='j')
        if not ret:
            gcore.fatal(_('g.proj failed'))

        if '+to_meter' in ret:
            dest_scale = ret['+to_meter'].strip()
        else:
            gcore.warning(
                _("Scale (%s) not found, assuming '1'") %
                '+to_meter')
            dest_scale = '1'
    else:
        dest_scale = options['destscale']
    gcore.debug('Getting destination projection scale -> %s' % dest_scale)

    # set up the projections
    srs_source = {'proj': options['sourceproj'],
                  'scale': float(options['sourcescale'])}
    srs_dest = {'proj': dest_proj, 'scale': float(dest_scale)}

    if options['region']:
        gcore.run_command('g.region',
                          quiet=True,
                          region=options['region'])
    dest_bbox = gcore.region()
    gcore.debug('Getting destination region')

    # output field separator
    fs = separator(options['separator'])

    # project the destination region into the source:
    gcore.verbose('Projecting destination region into source...')
    dest_bbox_points = bboxToPoints(dest_bbox)

    dest_bbox_source_points, errors_dest = projectPoints(dest_bbox_points,
                                                         source=srs_dest,
                                                         dest=srs_source)

    if len(dest_bbox_source_points) == 0:
        gcore.fatal(_("There are no tiles available. Probably the output "
                      "projection system it is not compatible with the "
                      "projection of the current location"))

    source_bbox = pointsToBbox(dest_bbox_source_points)

    gcore.verbose('Projecting source bounding box into destination...')

    source_bbox_points = bboxToPoints(source_bbox)

    source_bbox_dest_points, errors_source = projectPoints(source_bbox_points,
                                                           source=srs_source,
                                                           dest=srs_dest)

    x_metric = 1 / dest_bbox['ewres']
    y_metric = 1 / dest_bbox['nsres']

    gcore.verbose('Computing length of sides of source bounding box...')

    source_bbox_dest_lengths = sideLengths(source_bbox_dest_points,
                                           x_metric, y_metric)

    # Find the skewedness of the two directions.
    # Define it to be greater than one
    # In the direction (x or y) in which the world is least skewed (ie north south in lat long)
    # Divide the world into strips. These strips are as big as possible contrained by max_
    # In the other direction do the same thing.
    # There's some recomputation of the size of the world that's got to come in
    # here somewhere.

    # For now, however, we are going to go ahead and request more data than is necessary.
    # For small regions far from the critical areas of projections this makes very little difference
    # in the amount of data gotten.
    # We can make this efficient for big regions or regions near critical
    # points later.

    bigger = []
    bigger.append(max(source_bbox_dest_lengths['x']))
    bigger.append(max(source_bbox_dest_lengths['y']))
    maxdim = (max_cols, max_rows)

    # Compute the number and size of tiles to use in each direction
    # I'm making fairly even sized tiles
    # They differer from each other in height and width only by one cell
    # I'm going to make the numbers all simpler and add this extra cell to
    # every tile.

    gcore.message(_('Computing tiling...'))
    tiles = [-1, -1]
    tile_base_size = [-1, -1]
    tiles_extra_1 = [-1, -1]
    tile_size = [-1, -1]
    tileset_size = [-1, -1]
    tile_size_overlap = [-1, -1]
    for i in range(len(bigger)):
        # make these into integers.
        # round up
        bigger[i] = int(bigger[i] + 1)
        tiles[i] = int((bigger[i] / maxdim[i]) + 1)
        tile_size[i] = tile_base_size[i] = int(bigger[i] / tiles[i])
        tiles_extra_1[i] = int(bigger[i] % tiles[i])
        # This is adding the extra pixel (remainder) to all of the tiles:
        if tiles_extra_1[i] > 0:
            tile_size[i] = tile_base_size[i] + 1
        tileset_size[i] = int(tile_size[i] * tiles[i])
        # Add overlap to tiles (doesn't effect tileset_size
        tile_size_overlap[i] = tile_size[i] + int(options['overlap'])

    gcore.verbose("There will be %d by %d tiles each %d by %d cells" %
                  (tiles[0], tiles[1], tile_size[0], tile_size[1]))

    ximax = tiles[0]
    yimax = tiles[1]

    min_x = source_bbox['w']
    min_y = source_bbox['s']
    max_x = source_bbox['e']
    max_y = source_bbox['n']
    span_x = (max_x - min_x)
    span_y = (max_y - min_y)

    xi = 0
    tile_bbox = {'w': -1, 's': -1, 'e': -1, 'n': -1}

    if errors_dest > 0:
        gcore.warning(_("During computation %i tiles could not be created" %
                        errors_dest))

    while xi < ximax:
        tile_bbox['w'] = float(
            min_x) + (float(xi) * float(tile_size[0]) / float(tileset_size[0])) * float(span_x)
        tile_bbox['e'] = float(min_x) + (float(xi + 1) * float(tile_size_overlap[0]
                                                               ) / float(tileset_size[0])) * float(span_x)
        yi = 0
        while yi < yimax:
            tile_bbox['s'] = float(
                min_y) + (float(yi) * float(tile_size[1]) / float(tileset_size[1])) * float(span_y)
            tile_bbox['n'] = float(min_y) + (
                float(yi + 1) * float(tile_size_overlap[1]) /
                float(tileset_size[1])) * float(span_y)
            tile_bbox_points = bboxToPoints(tile_bbox)
            tile_dest_bbox_points, errors = projectPoints(tile_bbox_points,
                                                          source=srs_source,
                                                          dest=srs_dest)
            tile_dest_bbox = pointsToBbox(tile_dest_bbox_points)
            if bboxesIntersect(tile_dest_bbox, dest_bbox):
                if flags['w']:
                    print("bbox=%s,%s,%s,%s&width=%s&height=%s" %
                          (tile_bbox['w'], tile_bbox['s'], tile_bbox['e'],
                           tile_bbox['n'], tile_size_overlap[0],
                           tile_size_overlap[1]))
                elif flags['g']:
                    print("w=%s;s=%s;e=%s;n=%s;cols=%s;rows=%s" %
                          (tile_bbox['w'], tile_bbox['s'], tile_bbox['e'],
                           tile_bbox['n'], tile_size_overlap[0],
                           tile_size_overlap[1]))
                else:
                    print("%s%s%s%s%s%s%s%s%s%s%s" %
                          (tile_bbox['w'], fs, tile_bbox['s'], fs,
                           tile_bbox['e'], fs, tile_bbox['n'], fs,
                           tile_size_overlap[0], fs, tile_size_overlap[1]))
            yi += 1
        xi += 1

if __name__ == "__main__":
    cs2cs = 'cs2cs'
    options, flags = gcore.parser()
    sys.exit(main())
