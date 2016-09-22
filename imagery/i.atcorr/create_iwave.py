#!/usr/bin/env python
"""
Created on Sat Mar 27 11:35:32 2010

Program to interpolate filter function to correct 
step. Should be 2.5 nm
Then output filter function in a format similar to
what is needed in the Iwave.cpp file

Needs numpy and scipy

@author: daniel victoria, 2010
contact: daniel {dot} victoria {at} gmail {dot} com

usage() explains how this is supposed to work
Basically it needs a .csv file with spectral response for each
band in a column. First column has to be wavelength (nm)
First line (and only first) is a header with Wl, followed by band names
file name is used for sensor name

Updated by: Anne Ghisla, 2010
Bug fix (9/12/2010) by Daniel:
   1) function interpolate_band was not generating the spectral response for the last value in the filter function. Fixed
   2) function pretty_print was not printing the 8th value of every line, cutting the filter function short.
 
"""
import os
import sys
import numpy as np
from scipy import interpolate

def usage():
    """How to use this..."""
    print "create_iwave.py <csv file>"
    print
    print "Generates filter function template for iwave.cpp from csv file. Note:"
    print "- csv file must have wl response for each band in each column"
    print "- first line must be a header with wl followed by band names"
    print "- all following lines will be the data."
    print "If spectral response is null, leave field empty in csv file. Example:"
    print
    print "WL(nm),band 1,band 2,band 3,band 4"
    print "455,0.93,,,"
    print "485,0.94,0.00,,"
    print "545,0.00,0.87,0.00,"
    print
    print "This script will interpolate the filter functions to 2.5 nm steps"
    print "and output a cpp template file in the IWave format to be added to iwave.cpp"

def read_input(csvfile):
    """
    Function to read input file
    return number of bands and array of values for each band
    should be a .csv file with the values
    of the filter function for each band in the sensor
    one column for band
    first line must have a header with sensor band names
    first column is wavelength
    values are those of the discrete band filter functions
    """
    infile = open(csvfile, 'r')

    # get number of bands and band names
    bands = infile.readline().split(',')
    bands.remove(bands[0])
    bands[-1] = bands[-1].strip()
    print " > Number of bands found: %d" % len(bands)
    infile.close()

    # create converter dictionary for import
    # fix nodata or \n
    conv = {}
    for b in range(len(bands)):
        conv[b+1] = lambda s: float(s or -99)

    values = np.loadtxt(csvfile, delimiter=',', skiprows=1, converters = conv)

    return (bands, values)

def interpolate_band(values, step = 2.5):
    """
    Receive wavelength and response for one band
    interpolate at 2.5 nm steps
    return interpolated filter func
    and min, max wl values
    values must be numpy array with 2 columns
    """
    # These 2 lines select the subarray 
    # remove nodata (-99) lines in values array
    # where response is nodata?

    w = values[:,1] >= 0
    response = values[w]

    wavelengths = response[:,0]  # 1st column of input array
    spectral_response = response[:,1]  # 2nd column
    assert len(wavelengths) == len(spectral_response), "Number of wavelength slots and spectral responses are not equal!"

    # interpolating
    f = interpolate.interp1d(wavelengths, spectral_response)
    start = wavelengths[0]

    # considering last entry for np.arange
    input_step = wavelengths[-1] - wavelengths[-2]

    # if wavelength step in input data is 1 nm
    # print "  > Wavelength step in input data is", input_step, "nm"
    if input_step == 1:
        addendum = 0
        # print "    i No need to modify the stop value for the np.arrange function."

    # what if input step != 1 and input step != 2.5?
    else:
        addendum = step
        # print "  ! Wavelength step in input data is nor 1 nm, neither 2.5 nm.",
        # print "Internally setting the \"stop\" value for the np.arrange()",
        # print "function so as to not exceed the end value", wavelengths[-1],
        # print "of the open half ended range."

    stop = wavelengths[-1] + addendum

    filter_f = f(np.arange(start, stop, step))
   
    # how many spectral responses?
    expected = np.ceil((stop - start) / step)
    assert len(filter_f) == expected, "Number of interpolated spectral responses not equal to expected number of interpolations" 

    # convert limits from nanometers to micrometers
    lowerlimit = wavelengths[0]/1000
    upperlimit = wavelengths[-1]/1000

    return(filter_f, (lowerlimit, upperlimit))

def plot_filter(values):
    """Plot wl response values and interpolated
    filter function. This is just for checking...
    value is a 2 column numpy array
    function has to be used inside Spyder python environment
    """
    filter_f, limits = interpolate_band(values)
    
    # removing nodata
    w = values[:,1] >= 0
    response = values[w]
    
    plot(response[:,0],response[:,1], 'ro')
    plot(arange(limits[0], limits[1], 2.5), filter_f)
    
    return

def pretty_print(filter_f):
    """
    Create pretty string out of filter function
    8 values per line, with spaces, commas and all the rest
    """
    pstring = ''
    for i in range(len(filter_f)+1):
        if i%8 is 0:
            if i is not 0:
                value_wo_leading_zero = ('%.4f' % (filter_f[i-1])).lstrip('0')
                pstring += value_wo_leading_zero+', '
            if i is not 1: 
                # trim the trailing whitespace at the end of line
                pstring = pstring.rstrip()
            pstring += "\n\t\t"
        else:
            value_wo_leading_zero = ('%.4f' % (filter_f[i-1])).lstrip('0')
            pstring += value_wo_leading_zero+', '
    # trim starting \n and trailing , 
    pstring = pstring.lstrip("\n").rstrip(", ")
    return pstring

def write_cpp(bands, values, sensor, folder):
    """
    from bands, values and sensor name
    create output file in cpp style
    needs other functions: interpolate_bands, pretty_print
    """
    
    # getting necessary data
    # single or multiple bands?
    if len(bands) == 1:
        filter_f, limits = interpolate_band(values)
    else:
        filter_f = []
        limits = []
        for b in range(len(bands)):
            fi, li = interpolate_band(values[:,[0,b+1]])
            filter_f.append(fi)
            limits.append(li)
    
    # writing...
    outfile = open(os.path.join(folder, sensor+"_cpp_template.txt"), 'w')
    outfile.write('/* Following filter function created using create_iwave.py */\n\n')
    
    if len(bands) == 1:
        outfile.write('void IWave::%s()\n{\n\n' % (sensor.lower()))
    else:
        outfile.write('void IWave::%s(int iwa)\n{\n\n' % (sensor.lower()))
        
    # single band case
    if len(bands) == 1:
        outfile.write('    /* %s of %s */\n' % (bands[0], sensor))
        outfile.write('    static const float sr[%i] = {' % (len(filter_f)))
        filter_text = pretty_print(filter_f)
        outfile.write(filter_text)
        
        # calculate wl slot for band start
        # slots range from 250 to 4000 at 2.5 increments (total 1500)
        s_start = int((limits[0]*1000 - 250)/2.5)
        
        outfile.write('\n')
        outfile.write('    ffu.wlinf = %.4ff;\n' % (limits[0]))
        outfile.write('    ffu.wlsup = %.4ff;\n' % (limits[1]))
        outfile.write('    int i = 0;\n')
        outfile.write('    for(i = 0; i < %i; i++)\tffu.s[i] = 0;\n' % (s_start))
        outfile.write('    for(i = 0; i < %i; i++)\tffu.s[%i+i] = sr[i];\n' % (len(filter_f), s_start))
        outfile.write('    for(i = %i; i < 1501; i++)\tffu.s[i] = 0;\n' % (s_start + len(filter_f)))
        outfile.write('}\n')
        
    else: # more than 1 band
        # writing bands
        for b in range(len(bands)):
            outfile.write('    /* %s of %s */\n' % (bands[b], sensor))
            outfile.write('    static const float sr%i[%i] = {\n' % (b+1,len(filter_f[b])))
            filter_text = pretty_print(filter_f[b])
            outfile.write(filter_text+'\n    };\n\t\n')
        
        # writing band limits
        for b in range(len(bands)):
            inf = ", ".join(["%.3f" % i[0] for i in limits])
            sup = ", ".join(["%.3f" % i[1] for i in limits])
        
        outfile.write('    static const float wli[%i] = {%s};\n' % (len(bands), inf))
        outfile.write('    static const float wls[%i] = {%s};\n' % (len(bands), sup))
        
        outfile.write('\n')
        outfile.write('    ffu.wlinf = (float)wli[iwa-1];\n')
        outfile.write('    ffu.wlsup = (float)wls[iwa-1];\n\n')
        
        outfile.write('    int i;\n')
        outfile.write('    for(i = 0; i < 1501; i++) ffu.s[i] = 0;\n\n')
        
        outfile.write('    switch(iwa)\n    {\n')
        
        # now start of case part...
        for b in range(len(bands)):
            s_start = int((limits[b][0]*1000 - 250)/2.5)
            outfile.write('    case %i: for(i = 0; i < %i; i++)  ffu.s[%i+i] = sr%i[i];\n' % ((b+1), len(filter_f[b]), s_start, (b+1)))
            outfile.write('        break;\n')
        outfile.write('    }\n}\n')
        
    return

def main():
    """ control function """
    
    inputfile = sys.argv[1]
    
    # getting sensor name from full csv file name
    sensor = os.path.splitext(os.path.basename(inputfile))[0]
    
    print
    print " > Getting sensor name from csv file: %s" % (sensor)
    
    # getting data from file
    bands, values = read_input(inputfile)
    
    # writing file in same folder of input file
    write_cpp(bands, values, sensor, os.path.dirname(inputfile))
    
    print " > Filter functions exported to %s" % ("sensors_csv/"+sensor+"_cpp_template.txt")
    print " > Please check this file for possible errors before inserting the code into file iwave.cpp"
    print " > Don't forget to add the necessary data to the files iwave.h, geomcond.h, geomcond.cpp, and to i.atcorr.html"
    print
    
    return

if __name__ == '__main__':
    if len(sys.argv) == 1:
        usage()
        sys.exit()
    else:
        main()
