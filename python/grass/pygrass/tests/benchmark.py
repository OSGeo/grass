# -*- coding: utf-8 -*-
"""
Created on Sat Jun 16 20:24:56 2012

@author: soeren
"""
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)

import optparse
#import numpy as np
import time
import collections
import copy
import cProfile
import sys
import os
from jinja2 import Template
sys.path.append(os.getcwd())
sys.path.append("%s/.." % (os.getcwd()))

import grass.lib.gis as libgis
import grass.lib.raster as libraster
import grass.script as core
import grass.pygrass
import ctypes


def test__RasterSegment_value_access__if():
    test_a = pygrass.RasterSegment(name="test_a")
    test_a.open(mode="r")

    test_c = pygrass.RasterSegment(name="test_c")
    test_c.open(mode="w", mtype="CELL", overwrite=True)

    buff_a = pygrass.Buffer(test_a.cols, test_a.mtype)

    for row in range(test_a.rows):
        test_a.get_row(row, buff_a)
        for col in range(test_a.cols):
            test_c.put(row, col, buff_a[col] > 50)

    test_a.close()
    test_c.close()

def test__RasterSegment_value_access__add():
    test_a = pygrass.RasterSegment(name="test_a")
    test_a.open(mode="r")

    test_b = pygrass.RasterSegment(name="test_b")
    test_b.open(mode="r")

    test_c = pygrass.RasterSegment(name="test_c")
    test_c.open(mode="w", mtype="DCELL", overwrite=True)

    buff_a = pygrass.Buffer(test_a.cols, test_a.mtype)
    buff_b = pygrass.Buffer(test_b.cols, test_b.mtype)

    for row in range(test_a.rows):
        test_a.get_row(row, buff_a)
        test_b.get_row(row,buff_b)
        for col in range(test_a.cols):
            test_c.put(row, col, buff_a[col] + buff_b[col])

    test_a.close()
    test_b.close()
    test_c.close()

def test__RasterSegment_row_access__if():
    test_a = pygrass.RasterSegment(name="test_a")
    test_a.open(mode="r")

    test_c = pygrass.RasterSegment(name="test_c")
    test_c.open(mode="w", mtype="CELL", overwrite=True)

    buff_a = pygrass.Buffer(test_a.cols, test_a.mtype)

    for row in range(test_a.rows):
        test_a.get_row(row, buff_a)
        test_c.put_row(row, buff_a > 50)

    test_a.close()
    test_c.close()

def test__RasterSegment_row_access__add():
    test_a = pygrass.RasterSegment(name="test_a")
    test_a.open(mode="r")

    test_b = pygrass.RasterSegment(name="test_b")
    test_b.open(mode="r")

    test_c = pygrass.RasterSegment(name="test_c")
    test_c.open(mode="w", mtype="DCELL", overwrite=True)

    buff_a = pygrass.Buffer(test_a.cols, test_a.mtype)
    buff_b = pygrass.Buffer(test_b.cols, test_b.mtype)

    for row in range(test_a.rows):
        test_a.get_row(row, buff_a)
        test_b.get_row(row,buff_b)
        test_c.put_row(row, buff_a + buff_b)

    test_a.close()
    test_b.close()
    test_c.close()

def test__RasterRow_value_access__add():
    test_a = pygrass.RasterRow(name="test_a")
    test_a.open(mode="r")

    test_b = pygrass.RasterRow(name="test_b")
    test_b.open(mode="r")

    test_c = pygrass.RasterRow(name="test_c")
    test_c.open(mode="w", mtype="FCELL", overwrite=True)

    buff_a = pygrass.Buffer(test_a.cols, test_a.mtype)
    buff_b = pygrass.Buffer(test_b.cols, test_b.mtype)
    buff_c = pygrass.Buffer(test_b.cols, test_b.mtype)

    for row in range(test_a.rows):
        test_a.get_row(row, buff_a)
        test_b.get_row(row,buff_b)

        for col in range(test_a.cols):
            buff_c[col] = buff_a[col] + buff_b[col]

        test_c.put_row(buff_c)

    test_a.close()
    test_b.close()
    test_c.close()

def test__RasterRow_value_access__if():
    test_a = pygrass.RasterRow(name="test_a")
    test_a.open(mode="r")

    test_c = pygrass.RasterRow(name="test_c")
    test_c.open(mode="w", mtype="CELL", overwrite=True)

    buff_a = pygrass.Buffer(test_a.cols, test_a.mtype)
    buff_c = pygrass.Buffer(test_a.cols, test_a.mtype)

    for row in range(test_a.rows):
        test_a.get_row(row, buff_a)

        for col in range(test_a.cols):
            buff_c[col] = buff_a[col] > 50

        test_c.put_row(buff_c)

    test_a.close()
    test_c.close()

def test__RasterRowIO_row_access__add():
    test_a = pygrass.RasterRowIO(name="test_a")
    test_a.open(mode="r")

    test_b = pygrass.RasterRowIO(name="test_b")
    test_b.open(mode="r")

    test_c = pygrass.RasterRowIO(name="test_c")
    test_c.open(mode="w", mtype="FCELL", overwrite=True)

    buff_a = pygrass.Buffer(test_a.cols, test_a.mtype)
    buff_b = pygrass.Buffer(test_b.cols, test_b.mtype)

    for row in range(test_a.rows):
        test_a.get_row(row, buff_a)
        test_b.get_row(row,buff_b)
        test_c.put_row(buff_a + buff_b)

    test_a.close()
    test_b.close()
    test_c.close()

def test__RasterRowIO_row_access__if():
    test_a = pygrass.RasterRowIO(name="test_a")
    test_a.open(mode="r")

    test_c = pygrass.RasterRowIO(name="test_c")
    test_c.open(mode="w", mtype="CELL", overwrite=True)

    buff_a = pygrass.Buffer(test_a.cols, test_a.mtype)

    for row in range(test_a.rows):
        test_a.get_row(row, buff_a)
        test_c.put_row(buff_a > 50)

    test_a.close()
    test_c.close()

def test__RasterRow_row_access__add():
    test_a = pygrass.RasterRow(name="test_a")
    test_a.open(mode="r")

    test_b = pygrass.RasterRow(name="test_b")
    test_b.open(mode="r")

    test_c = pygrass.RasterRow(name="test_c")
    test_c.open(mode="w", mtype="FCELL", overwrite=True)

    buff_a = pygrass.Buffer(test_a.cols, test_a.mtype)
    buff_b = pygrass.Buffer(test_b.cols, test_b.mtype)

    for row in range(test_a.rows):
        test_a.get_row(row, buff_a)
        test_b.get_row(row,buff_b)
        test_c.put_row(buff_a + buff_b)

    test_a.close()
    test_b.close()
    test_c.close()

def test__RasterRow_row_access__if():
    test_a = pygrass.RasterRow(name="test_a")
    test_a.open(mode="r")

    test_c = pygrass.RasterRow(name="test_c")
    test_c.open(mode="w", mtype="CELL", overwrite=True)

    buff_a = pygrass.Buffer(test_a.cols, test_a.mtype)

    for row in range(test_a.rows):
        test_a.get_row(row, buff_a)
        test_c.put_row(buff_a > 50)

    test_a.close()
    test_c.close()

def test__mapcalc__add():
    core.mapcalc("test_c = test_a + test_b", quite=True, overwrite=True)

def test__mapcalc__if():
    core.mapcalc("test_c = if(test_a > 50, 1, 0)", quite=True, overwrite=True)

def mytimer(func, runs=1):
    times = []
    t = 0.0
    for _ in range(runs):
        start = time.time()
        func()
        end = time.time()
        times.append(end - start)
        t = t + end - start

    return t/runs, times



def run_benchmark(resolution_list, runs, testdict, profile):
    regions = []
    for resolution in resolution_list:
        core.use_temp_region()
        core.run_command('g.region', e=50, w=-50, n=50, s=-50, res=resolution, flags='p')

        # Adjust the computational region for this process
        region = libgis.Cell_head()
        libraster.Rast_get_window(ctypes.byref(region))
        region.e = 50
        region.w = -50
        region.n = 50
        region.s = -50
        region.ew_res = resolution
        region.ns_res = resolution

        libgis.G_adjust_Cell_head(ctypes.byref(region), 0, 0)

        libraster.Rast_set_window(ctypes.byref(region))
        libgis.G_set_window(ctypes.byref(region))

        # Create two raster maps with random numbers
        core.mapcalc("test_a = rand(0, 100)", quite=True, overwrite=True)
        core.mapcalc("test_b = rand(0.0, 1.0)", quite=True, overwrite=True)
        result = collections.OrderedDict()
        result['res'] = resolution
        result['cols'] = region.cols
        result['rows'] = region.rows
        result['cells'] = region.rows * region.cols
        result['results'] = copy.deepcopy(testdict)
        for execmode, operation in result['results'].items():
            print(execmode)
            for oper, operdict in operation.items():
                operdict['time'], operdict['times'] = mytimer(operdict['func'],runs)
                if profile:
                    filename = '{0}_{1}_{2}'.format(execmode, oper, profile)
                    cProfile.runctx(operdict['func'].__name__ + '()',
                                    globals(), locals(), filename = filename)
                print(('    {0}: {1: 40.6f}s'.format(oper, operdict['time'])))
                del(operdict['func'])

        regions.append(result)
        core.del_temp_region()

    return regions

def get_testlist(loc):
    testlist = [test for test in list(loc.keys()) if 'test' in test[:5]]
    testlist.sort()
    return testlist

def get_testdict(testlist):
    testdict = collections.OrderedDict()
    for testfunc in testlist:
        #import pdb; pdb.set_trace()
        dummy, execmode, operation = testfunc.split('__')
        if execmode in list(testdict.keys()):
            testdict[execmode][operation] = collections.OrderedDict()
            testdict[execmode][operation]['func'] = loc[testfunc]
        else:
            testdict[execmode] = collections.OrderedDict()
            testdict[execmode][operation] = collections.OrderedDict()
            testdict[execmode][operation]['func'] = loc[testfunc]
    return testdict

def print_test(testdict):
    for execmode, operation in testdict.items():
        print(execmode)
        for oper, operdict in operation.items():
            print('    ', oper)
            for key, value in operdict.items():
                print('        ', key)

TXT = """
{% for region in regions %}
{{ '#'*60 }}
### Benchmark cols = {{ region.cols }} rows = {{ region.rows}} cells = {{ region.cells }}
{{ '#'*60 }}

    # equation: c = a + b
    {% for execmode, operation in region.results.iteritems() %}
        {{ "%-30s - %5s % 12.6fs"|format(execmode, 'add', operation.add.time) }}
    {%- endfor %}

    # equation: c = if a > 50 then 1 else 0
    {% for execmode, operation in region.results.iteritems() %}
        {{ "%-30s - %5s % 12.6fs"|format(execmode, 'if', operation.if.time) }}
    {%- endfor %}
{%- endfor %}
"""


CSV = """Class; Mode; Operation;

"""

RST = """
"""
#>>> txt = Template(TxT)
#>>> txt.render(name='John Doe')


def get_txt(results):
    txt = Template(TXT)
    return txt.render(regions = results)


#classes for required options
strREQUIRED = 'required'

class OptionWithDefault(optparse.Option):
    ATTRS = optparse.Option.ATTRS + [strREQUIRED]

    def __init__(self, *opts, **attrs):
        if attrs.get(strREQUIRED, False):
            attrs['help'] = '(Required) ' + attrs.get('help', "")
        optparse.Option.__init__(self, *opts, **attrs)

class OptionParser(optparse.OptionParser):
    def __init__(self, **kwargs):
        kwargs['option_class'] = OptionWithDefault
        optparse.OptionParser.__init__(self, **kwargs)

    def check_values(self, values, args):
        for option in self.option_list:
            if hasattr(option, strREQUIRED) and option.required:
                if not getattr(values, option.dest):
                    self.error("option {} is required".format(str(option)))
        return optparse.OptionParser.check_values(self, values, args)


def main(testdict):
    """Main function"""
    #usage
    usage = "usage: %prog [options] raster_map"
    parser = OptionParser(usage=usage)
    # ntime
    parser.add_option("-n", "--ntimes", dest="ntime",default=5, type="int",
                      help="Number of run for each test.")
    # res
    parser.add_option("-r", "--resolution", action="store", type="string",
                      dest="res", default = '1,0.25',
                      help="Resolution list separate by comma.")
    # fmt
    parser.add_option("-f", "--fmt", action="store", type="string",
                      dest="fmt", default = 'txt',
                      help="Choose the output format: 'txt', 'csv', 'rst'.")

    # output
    parser.add_option("-o", "--output", action="store", type="string",
                      dest="output", help="The output filename.")

    # store
    parser.add_option("-s", "--store", action="store", type="string",
                      dest="store", help="The filename of pickle obj.")

    # profile
    parser.add_option("-p", "--profile", action="store", type="string",
                      dest="profile", help="The filename of the profile results.")

    #return options and argument
    options, args = parser.parse_args()
    res = [float(r) for r in options.res.split(',')]
    #res = [1, 0.25, 0.1, 0.05]

    results = run_benchmark(res, options.ntime, testdict, options.profile)

    if options.store:
        import pickle
        output = open(options.store, 'wb')
        pickle.dump(results, output)
        output.close()
    #import pdb; pdb.set_trace()
    print(get_txt(results))


#add options
if __name__ == "__main__":
    #import pdb; pdb.set_trace()
    loc = locals()
    testlist = get_testlist(loc)
    testdict = get_testdict(testlist)
    #print_test(testdict)



    #import pdb; pdb.set_trace()

    main(testdict)
