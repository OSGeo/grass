#!/usr/bin/env python

import sys
import os
import atexit
import string
import array
import grass.script as grass

tmp_img = None
tmp_grad_abs = None
tmp_grad_rel = None

height = 85
width = 15

def cleanup():
    if tmp_img:
        grass.try_remove(tmp_img)
    if tmp_grad_rel:
        grass.run_command('g.remove', rast = tmp_grad_rel, quiet = True)
    if tmp_grad_abs:
        grass.run_command('g.remove', rast = tmp_grad_abs, quiet = True)

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
    if grass.find_program("g.ppmtopng", ["help"]):
        grass.run_command('g.ppmtopng', input = src, output = dst, quiet = True)
    elif grass.find_program("pnmtopng"):
        fh = open(dst, 'wb')
        grass.call(["pnmtopng", src], stdout = fh)
        fh.close()
    elif grass.find_program("convert"):
        grass.call(["convert", src, dst])
    else:
        grass.fatal(_("Cannot find g.ppmtopng, pnmtopng or convert"))

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
        records.append(line.split())
    records = [record for record in records if record[0] != 'nv']
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
            maxval = float(records[-1][0])
            maxval = min(maxval, 2500000)
        grad = tmp_grad_abs
        grass.mapcalc("$grad = if(row()==1, float($min), float($max))",
        	      grad = tmp_grad_abs, min = minval, max = maxval, quiet = True)
    else:
        grad = tmp_grad_rel

    return grad

def make_image(output_dir, table, grad, discrete = False):
    if discrete:
        lines, cols = height, 1
    else:
        lines, cols = None, None
    grass.run_command("r.colors", map = grad, color = table, quiet = True)
    grass.run_command("d.colortable", flags = 'n', map = grad, lines = lines, cols = cols, quiet = True)
    outfile = os.path.join(output_dir, "Colortable_%s.png" % table)
    convert_and_rotate(tmp_img, outfile, discrete)

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
    tmp_img = grass.tempfile() + ".ppm"

    os.environ['GRASS_WIDTH'] = '%d' % width
    os.environ['GRASS_HEIGHT'] = '%d' % height
    os.environ['GRASS_FRAME'] = '%f,%f,%f,%f' % (0,height,0,width)
    os.environ['GRASS_PNGFILE'] = tmp_img
    os.environ['GRASS_TRUECOLOR'] = 'TRUE'
    os.environ['GRASS_PNG_READ'] = 'FALSE'
    os.environ['GRASS_PNG_MAPPED'] = 'FALSE'
    os.environ['GRASS_TRANSPARENT'] = 'FALSE'
    os.environ['GRASS_BACKGROUNDCOLOR'] = 'ffffff'
    os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'

    for var in ['GRASS_LINE_WIDTH', 'GRASS_ANTIALIAS']:
        if var in os.environ:
            del os.environ[var]

    grass.use_temp_region()
    grass.run_command('g.region', rows = 100, cols = 100)

    grass.mapcalc("$grad = row()/1.0", grad = tmp_grad_rel, quiet = True)
    
    for table in os.listdir(color_dir):
        path = os.path.join(color_dir, table)
        grad = make_gradient(path)
        make_image(output_dir, table, grad)
    
    grass.mapcalc("$grad = row()", grad = tmp_grad_abs, quiet = True)
    for table in ['grey.eq', 'grey.log', 'random']:
        make_image(output_dir, table, tmp_grad_abs, True)
 
if __name__ == "__main__":
    atexit.register(cleanup)
    main()
