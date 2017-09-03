#!/usr/bin/env python
#
# thumbnails.py: Create thumbnail sample images of the various GRASS color rules
#
#  AUTHOR: Python version by Glynn Clements
#      Earlier Bourne script version by Hamish Bowman,
#      http://grasswiki.osgeo.org/wiki/Talk:Color_tables
#
#   (C) 2009-2013 by the GRASS Development Team
#       This program is free software under the GNU General Public
#       License (>=v2). Read the file COPYING that comes with GRASS
#       for details.
#

import sys
import os
import shutil
import atexit
import string
import array
import grass.script as grass

tmp_img = None
tmp_grad_abs = None
tmp_grad_rel = None

height = 15
width = 85

def cleanup():
    if tmp_img:
        grass.try_remove(tmp_img)
    if tmp_grad_rel:
        grass.run_command('g.remove', flags = 'f', type = 'raster',
                          name = tmp_grad_rel, quiet = True)
    if tmp_grad_abs:
        grass.run_command('g.remove', flags = 'f', type = 'raster',
                          name = tmp_grad_abs, quiet = True)

# def rotate(src, dst):
#     grass.call(["convert", "-rotate", "90", src, dst])

def read_ppm(src):
    fh = open(src, "rb")
    text = fh.read()
    fh.close()
    i = 0
    j = text.find('\n', i)
    if text[i:j] != 'P6':
        raise IOError
    i = j + 1
    j = text.find('\n', i)
    w, h = text[i:j].split()
    if int(w) != width or int(h) != height:
        raise IOError
    i = j + 1
    j = text.find('\n', i)
    maxval = text[i:j]
    if int(maxval) != 255:
        raise IOError
    i = j + 1
    return array.array('B', text[i:])

def write_ppm(dst, data):
    w = height
    h = width
    fh = open(dst, "wb")
    fh.write("P6\n%d %d\n%d\n" % (w, h, 255))
    data.tofile(fh)
    fh.close()

def rotate_ppm(srcd):
    dstd = array.array('B', len(srcd) * '\0')
    for y in xrange(height):
        for x in xrange(width):
            for c in xrange(3):
                dstd[(x * height + (height - 1 - y)) * 3 + c] = srcd[(y * width + x) * 3 + c]
    return dstd

def flip_ppm(srcd):
    dstd = array.array('B', len(srcd) * '\0')
    stride = width * 3
    for y in xrange(height):
        dy = (height - 1 - y)
        dstd[dy * stride:(dy + 1) * stride] = srcd[y * stride:(y + 1) * stride]
    return dstd

def ppmtopng(dst, src):
    if grass.find_program("g.ppmtopng", '--help'):
        grass.run_command('g.ppmtopng', input = src, output = dst, quiet = True)
    elif grass.find_program("pnmtopng"):
        fh = open(dst, 'wb')
        grass.call(["pnmtopng", src], stdout = fh)
        fh.close()
    elif grass.find_program("convert"):
        grass.call(["convert", src, dst])
    else:
        grass.fatal(_("Cannot find g.ppmtopng, pnmtopng or convert"))

# TODO: this code is not used and can be moved to some lib function or
# separate module if useful
def convert_and_rotate(src, dst, flip = False):
    ppm = read_ppm(src)
    if flip:
        ppm = flip_ppm(ppm)
    ppm = rotate_ppm(ppm)
    write_ppm(tmp_img, ppm)
    ppmtopng(dst, tmp_img)

def make_gradient(path):
    fh = open(path)
    text = fh.read()
    fh.close()

    lines = text.splitlines()
    records = list()
    for line in lines:
        if line.startswith("#"):
            # skip comments
            continue
        if len(line) == 0:
            # skip empty lines
            continue
        records.append(line.split())
    records = [record for record in records if record[0] != 'nv' and record[0] != 'default']
    relative = False
    absolute = False
    for record in records:
        if record[0].endswith("%"):
            relative = True
            record[0] = record[0].rstrip("%")
        else:
            absolute = True

    if absolute:
        if relative:
            minval = -0.04
            maxval = 0.04
        else:
            minval = float(records[0][0])
            # shift min up for floating point values so that
            # first color in color table is visible
            if '.' in records[0][0]:
                # assumes that 1% of min does not go to the next value
                # and is still represented as float and does not make
                # too much difference in color
                # works better than 1% of the difference to the next value
                minval += abs(minval / 100)
            maxval = float(records[-1][0])
            maxval = min(maxval, 2500000)
        grad = tmp_grad_abs
        # alternatively, only simpler expression would suffice if frames
        # are used to render raster and then the borders
        grass.mapcalc("$grad = if(col() > 2 && col() < ncols() - 1,"
                      " float($min) + (col() - 3) * (float($max) - float($min)) / (ncols() - 2),"
                      " null())",
        	      grad = tmp_grad_abs, min = minval, max = maxval, quiet = True)
    else:
        grad = tmp_grad_rel

    return grad

def make_image(output_dir, table, grad):
    grass.run_command("r.colors", map = grad, color = table, quiet = True)
    grass.run_command("d.rast", map=grad, quiet=True)
    if 1:
        grass.write_command("d.graph", quiet=True, flags='m', stdin="""width 1
        color {outcolor}
        polyline
        {x1} {y1}
        {x2} {y1}
        {x2} {y2}
        {x1} {y2}
        {x1} {y1}
        color {incolor}
        polyline
        {x3} {y3}
        {x4} {y3}
        {x4} {y4}
        {x3} {y4}
        {x3} {y3}
        """.format(x1=1, x2=width, y1=0, y2=height - 1,
                   x3=2, x4=width - 1, y3=1, y4=height - 2,
                   outcolor='white', incolor='black'))
    outfile = os.path.join(output_dir, "colortables", "%s.png" % table)
    shutil.move(tmp_img, outfile)

def main():
    global tmp_img, tmp_grad_abs, tmp_grad_rel

    os.environ['GRASS_OVERWRITE'] = '1'

    color_dir = os.path.join(os.environ['GISBASE'], "etc", "colors")
    output_dir = os.path.join(os.environ['GISBASE'], "docs", "html")

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    pid = os.getpid()
    tmp_grad_abs = "tmp_grad_abs_%d" % pid
    tmp_grad_rel = "tmp_grad_rel_%d" % pid
    tmp_img = grass.tempfile() + ".png"

    os.environ['GRASS_RENDER_WIDTH'] = '%d' % width
    os.environ['GRASS_RENDER_HEIGHT'] = '%d' % height
    os.environ['GRASS_RENDER_FRAME'] = '%f,%f,%f,%f' % (0,height,0,width)
    os.environ['GRASS_RENDER_FILE'] = tmp_img
    os.environ['GRASS_RENDER_TRUECOLOR'] = 'TRUE'
    # for multiple d commands (requires to delete/move image each time)
    os.environ['GRASS_RENDER_FILE_READ'] = 'TRUE'
    os.environ['GRASS_RENDER_FILE_MAPPED'] = 'FALSE'
    os.environ['GRASS_RENDER_TRANSPARENT'] = 'FALSE'
    os.environ['GRASS_RENDER_BACKGROUNDCOLOR'] = 'ffffff'
    os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'
    # for one pixel wide lines
    os.environ['GRASS_RENDER_ANTIALIAS'] = 'none'

    for var in ['GRASS_RENDER_LINE_WIDTH']:
        if var in os.environ:
            del os.environ[var]

    grass.use_temp_region()
    grass.run_command('g.region', s=0, w=0, n=height, e=width,
                      rows=height, cols=width, res=1, flags='a')

    grass.mapcalc("$grad = if(col() > 2 && col() < ncols() - 1,"
                  " float(col()), null())", grad=tmp_grad_rel, quiet=True)
    
    for table in os.listdir(color_dir):
        path = os.path.join(color_dir, table)
        grad = make_gradient(path)
        make_image(output_dir, table, grad)

    grass.mapcalc("$grad = if(col() > 2 && col() < ncols() - 1,"
                  " col(), null())", grad=tmp_grad_abs, quiet=True)
    for table in ['grey.eq', 'grey.log', 'random']:
        make_image(output_dir, table, tmp_grad_abs)

 
if __name__ == "__main__":
    atexit.register(cleanup)
    main()
