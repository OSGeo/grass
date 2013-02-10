#!/usr/bin/env python

############################################################################
#
# MODULE:	    i.pansharpen
#
# AUTHOR(S):    Overall script by Michael Barton (ASU)	
#               Brovey transformation in i.fusion.brovey by Markus Neteler <<neteler at osgeo org>>
#               i.fusion brovey converted to Python by Glynn Clements
#               IHS and PCA transformation added by Michael Barton (ASU)
#               histogram matching algorithm by Michael Barton and Luca Delucchi, Fondazione E. Mach (Italy)
#               Thanks to Markus Metz for help with PCA inversion
#               Thanks to Hamish Bowman for parallel processing algorithm
#
# PURPOSE:	Sharpening of 3 RGB channels using a high-resolution panchromatic channel 
#
# COPYRIGHT:	(C) 2002-2012 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
# REFERENCES:
#   Roller, N.E.G. and Cox, S., 1980. Comparison of Landsat MSS and merged MSS/RBV 
#   data for analysis of natural vegetation. Proc. of the 14th International 
#   Symposium on Remote Sensing of Environment, San Jose, Costa Rica, 23-30 April, pp. 1001-1007.
#
#   Amarsaikhan, D., & Douglas, T. (2004). Data fusion and multisource image classification. 
#   International Journal of Remote Sensing, 25(17), 3529-3539.
#
#   Behnia, P. (2005). Comparison between four methods for data fusion of ETM+ 
#   multispectral and pan images. Geo-spatial Information Science, 8(2), 98-103
#
#   for LANDSAT 5: see Pohl, C 1996 and others
#
#############################################################################

#%Module
#%  description: Image fusion algorithms to sharpen multispectral with high-res panchromatic channels
#%  keywords: imagery
#%  keywords: fusion
#%  keywords: sharpen
#%  keywords: Brovey
#%  keywords: IHS
#%  keywords: PCA
#%  overwrite: yes
#%End
#%option
#%  key: sharpen
#%  description: Choose pan sharpening method
#%  options: brovey,ihs,pca
#%  answer: ihs
#%  required: yes
#%end
#%option G_OPT_R_INPUT
#%  key: ms3
#%  description: Input raster map for red channel
#%end
#%option G_OPT_R_INPUT
#%  key: ms2
#%  description: Input raster map for green channel
#%end
#%option G_OPT_R_INPUT
#%  key: ms1
#%  description: Input raster map for blue channel
#%end
#%  option G_OPT_R_INPUT
#%  key: pan
#%  description: Input raster map for high resolution panchromatic channel
#%end
#%option
#%  key: output_prefix
#%  type: string
#%  description: Prefix for output raster maps
#%  required : yes
#%end
#%flag
#%  key: s
#%  description: Serial processing rather than parallel processing
#%end
#%flag
#%  key: l
#%  description: Rebalance blue channel for landsat maps
#%end

import sys
import os
import numpy as np
import grass.script as grass

def main():
    sharpen   = options['sharpen'] # sharpening algorithm
    ms1       = options['ms1'] # blue channel
    ms2       = options['ms2'] # green channel
    ms3       = options['ms3'] # red channel
    pan       = options['pan'] # high res pan channel
    out       = options['output_prefix'] # prefix for output RGB maps
    bladjust  = flags['l'] # adjust blue channel
    sproc     = flags['s'] # serial processing
    
    outb = grass.core.find_file('%s_blue' % out)
    outg = grass.core.find_file('%s_green' % out)
    outr = grass.core.find_file('%s_red' % out)
        
    if (outb['name'] != '' or outg['name'] != '' or outr['name'] != '') and \
        (not grass.overwrite() and not flags['o']):
        grass.warning(_('Maps with selected output prefix names already exist. \
                        Delete them or use overwrite flag'))
        return
    
    pid = str(os.getpid())

    #get PAN resolution:
    kv = grass.raster_info(map = pan)
    nsres = kv['nsres']
    ewres = kv['ewres']
    panres = (nsres + ewres) / 2
    
    # clone current region
    grass.use_temp_region()

    grass.run_command('g.region', res = panres, align = pan)
    
    grass.message('\n ')
    grass.message(_("Performing pan sharpening with hi res pan image: %f" % panres))

    if sharpen == "brovey":
        grass.message(_("Using Brovey algorithm"))

        #pan/intensity histogram matching using linear regression
        outname = 'tmp%s_pan1' % pid
        panmatch1 = matchhist(pan, ms1, outname)

        outname = 'tmp%s_pan2' % pid
        panmatch2 = matchhist(pan, ms2, outname)

        outname = 'tmp%s_pan3' % pid
        panmatch3 = matchhist(pan, ms3, outname)
        
        outr = '%s_red' % out
        outg = '%s_green' % out
        outb = '%s_blue' % out

        #calculate brovey transformation
        grass.message('\n ')
        grass.message(_("Calculating Brovey transformation..."))
        
        if sproc:
            # serial processing
            e = '''eval(k = "$ms1" + "$ms2" + "$ms3")
                "$outr" = 1.0 * "$ms3" * "$panmatch3" / k
                "$outg" = 1.0 * "$ms2" * "$panmatch2" / k
                "$outb" = 1.0 * "$ms1" * "$panmatch1" / k'''
            grass.mapcalc(e, outr=outr, outg=outg, outb=outb, 
                          panmatch1=panmatch1, panmatch2=panmatch2, panmatch3=panmatch3, 
                          ms1=ms1, ms2=ms2, ms3=ms3, overwrite=True)
        else:
            # parallel processing
            pb = grass.mapcalc_start('%s_blue = (1.0 * %s * %s) / (%s + %s + %s)' %
                                (out, ms1, panmatch1, ms1, ms2, ms3),
                                overwrite=True)   
            pg = grass.mapcalc_start('%s_green = (1.0 * %s * %s) / (%s + %s + %s)' %
                                (out, ms2, panmatch2, ms1, ms2, ms3),
                                overwrite=True)   
            pr = grass.mapcalc_start('%s_red = (1.0 * %s * %s) / (%s + %s + %s)' %
                                (out, ms3, panmatch3, ms1, ms2, ms3),
                                overwrite=True)   
               
            pb.wait()
            pg.wait()
            pr.wait()
            

        # Cleanup
        grass.run_command('g.remove', quiet=True, rast='%s,%s,%s' % (panmatch1, panmatch2, panmatch3))

    elif sharpen == "ihs":
        grass.message(_("Using IHS<->RGB algorithm"))
        #transform RGB channels into IHS color space
        grass.message('\n ')
        grass.message(_("Transforming to IHS color space..."))
        grass.run_command('i.rgb.his', overwrite=True,
                          red_input=ms3, 
                          green_input=ms2, 
                          blue_input=ms1, 
                          hue_output="tmp%s_hue" % pid, 
                          intensity_output="tmp%s_int" % pid, 
                          saturation_output="tmp%s_sat" % pid)
                
        #pan/intensity histogram matching using linear regression
        target = "tmp%s_int" % pid
        outname = "tmp%s_pan_int" % pid
        panmatch = matchhist(pan, target, outname)
        
        #substitute pan for intensity channel and transform back to RGB color space
        grass.message('\n ')
        grass.message(_("Transforming back to RGB color space and sharpening..."))
        grass.run_command('i.his.rgb', overwrite=True, 
                          hue_input="tmp%s_hue" % pid, 
                          intensity_input="%s" % panmatch, 
                          saturation_input="tmp%s_sat" % pid,
                          red_output="%s_red" % out, 
                          green_output="%s_green" % out, 
                          blue_output="%s_blue" % out)

        # Cleanup
        grass.run_command('g.remove', quiet=True, rast=panmatch)
        
    elif sharpen == "pca":
        grass.message(_("Using PCA/inverse PCA algorithm"))
        grass.message('\n ')
        grass.message(_("Creating PCA images and calculating eigenvectors..."))

        #initial PCA with RGB channels
        pca_out = grass.read_command('i.pca', quiet=True, rescale='0,0', input='%s,%s,%s' % (ms1, ms2, ms3), output_prefix='tmp%s.pca' % pid)
        b1evect = []
        b2evect = []
        b3evect = []
        for l in pca_out.replace('(',',').replace(')',',').splitlines():
            b1evect.append(float(l.split(',')[1]))
            b2evect.append(float(l.split(',')[2]))
            b3evect.append(float(l.split(',')[3]))
            
        #inverse PCA with hi res pan channel substituted for principal component 1
        pca1 = 'tmp%s.pca.1' % pid
        pca2 = 'tmp%s.pca.2' % pid
        pca3 = 'tmp%s.pca.3' % pid
        b1evect1 = b1evect[0]
        b1evect2 = b1evect[1]
        b1evect3 = b1evect[2]
        b2evect1 = b2evect[0]
        b2evect2 = b2evect[1]
        b2evect3 = b2evect[2]
        b3evect1 = b3evect[0]
        b3evect2 = b3evect[1]
        b3evect3 = b3evect[2]

        outname = 'tmp%s_pan' % pid
        panmatch = matchhist(pan, ms1, outname)
        
        grass.message('\n ')
        grass.message(_("Performing inverse PCA ..."))
                
        stats1 = grass.parse_command("r.univar", map=ms1, flags='g', 
                                           parse=(grass.parse_key_val, { 'sep' : '=' }))
        stats2 = grass.parse_command("r.univar", map=ms2, flags='g', 
                                           parse=(grass.parse_key_val, { 'sep' : '=' }))
        stats3 = grass.parse_command("r.univar", map=ms3, flags='g', 
                                           parse=(grass.parse_key_val, { 'sep' : '=' }))
            
        b1mean = float(stats1['mean'])
        b2mean = float(stats2['mean'])
        b3mean = float(stats3['mean'])


        if sproc: 
            # serial processing
            e = '''eval(k = "$ms1" + "$ms2" + "$ms3")
                "$outr" = 1.0 * "$ms3" * "$panmatch3" / k
                "$outg" = 1.0 * "$ms2" * "$panmatch2" / k
                "$outb" = 1.0* "$ms1" * "$panmatch1" / k'''

            outr = '%s_red' % out
            outg = '%s_green' % out
            outb = '%s_blue' % out

            cmd1 = "$outb = (1.0 * $panmatch * $b1evect1) + ($pca2 * $b2evect1) + ($pca3 * $b3evect1) + $b1mean"
            cmd2 = "$outg = (1.0 * $panmatch * $b1evect2) + ($pca2 * $b2evect1) + ($pca3 * $b3evect2) + $b2mean"
            cmd3 = "$outr = (1.0 * $panmatch * $b1evect3) + ($pca2 * $b2evect3) + ($pca3 * $b3evect3) + $b3mean"
            
            cmd =  '\n'.join([cmd1, cmd2, cmd3])
            
            grass.mapcalc(cmd, outb=outb, outg=outg, outr=outr,
                          panmatch=panmatch, pca2=pca2, pca3=pca3, 
                          b1evect1=b1evect1, b2evect1=b2evect1, b3evect1=b3evect1, 
                          b1evect2=b1evect2, b2evect2=b2evect2, b3evect2=b3evect2, 
                          b1evect3=b1evect3, b2evect3=b2evect3, b3evect3=b3evect3, 
                          b1mean=b1mean, b2mean=b2mean, b3mean=b3mean, 
                          overwrite=True)
        else:
            # parallel processing
            pb = grass.mapcalc_start('%s_blue = (%s * %f) + (%s * %f) + (%s * %f) + %f' 
                          % (out, panmatch, b1evect1, pca2, b2evect1, pca3, b3evect1, b1mean), 
                          overwrite=True)
            
            pg = grass.mapcalc_start('%s_green = (%s * %f) + (%s * %f) + (%s * %f) + %f' 
                          % (out, panmatch, b1evect2, pca2, b2evect2, pca3, b3evect2, b2mean), 
                          overwrite=True)
            
            pr = grass.mapcalc_start('%s_red = (%s * %f) + (%s * %f) + (%s * %f) + %f' 
                          % (out, panmatch, b1evect3, pca2, b2evect3, pca3, b3evect3, b3mean), 
                          overwrite=True)
            
            pr.wait()
            pg.wait()
            pb.wait()

        
        # Cleanup
        grass.run_command('g.mremove', flags='f', quiet=True, rast='tmp%s*,%s' % (pid,panmatch))
        
    #Could add other sharpening algorithms here, e.g. wavelet transformation

    grass.message('\n ')
    grass.message(_("Assigning grey equalized color tables to output images..."))
    #equalized grey scales give best contrast
    for ch in ['red', 'green', 'blue']:
        grass.run_command('r.colors', quiet=True, map = "%s_%s" % (out, ch), flags="e", col = 'grey')

    #Landsat too blue-ish because panchromatic band less sensitive to blue light, 
    #  so output blue channed can be modified
    if bladjust:
        grass.message('\n ')
        grass.message(_("Adjusting blue channel color table..."))
        rules = grass.tempfile()
        colors = open(rules, 'w')
        colors.write('5 0 0 0\n20 200 200 200\n40 230 230 230\n67 255 255 255 \n')
        colors.close()

        grass.run_command('r.colors', map="%s_blue" % out, rules=rules)
        os.remove(rules)

    #output notice
    grass.message('\n ')
    grass.message(_("The following pan-sharpened output maps have been generated:"))
    for ch in ['red', 'green', 'blue']:
        grass.message(_("%s_%s") % (out, ch))

    grass.message('\n ')
    grass.message(_("To visualize output, run: g.region -p rast=%s.red" % out))
    grass.message(_("d.rgb r=%s_red g=%s_green b=%s_blue" % (out, out, out)))
    grass.message('\n ')
    grass.message(_("If desired, combine channels into a single RGB map with 'r.composite'."))
    grass.message(_("Channel colors can be rebalanced using i.landsat.rgb."))

    # write cmd history:
    for ch in ['red', 'green', 'blue']:
        grass.raster_history("%s_%s" % (out, ch))

    # Cleanup        
    grass.run_command('g.mremove', flags="f", rast="tmp%s*" % pid, quiet=True)

        
def matchhist(original, target, matched):
    #pan/intensity histogram matching using numpy arrays
    grass.message('\n ')
    grass.message(_("Histogram matching..."))

    # input images
    original = original.split('@')[0]
    target = target.split('@')[0]
    images = [original, target]
            
    # create a dictionary to hold arrays for each image            
    arrays = {}
    
    for i in images:
        # calculate number of cells for each grey value for for each image
        stats_out = grass.pipe_command('r.stats', flags='cin', input= i, sep=':')
        stats =  stats_out.communicate()[0].split('\n')[:-1]
        stats_dict = dict( s.split(':', 1) for s in stats)
        total_cells = 0 # total non-null cells
        for j in stats_dict:
            stats_dict[j] = int(stats_dict[j])
            if j != '*':
                total_cells += stats_dict[j]        
                
        # Make a 2x256 structured array for each image with a 
        #   cumulative distribution function (CDF) for each grey value.
        #   Grey value is the integer (i4) and cdf is float (f4).

        arrays[i] = np.zeros((256,),dtype=('i4,f4'))
        cum_cells = 0 # cumulative total of cells for sum of current and all lower grey values
        
        for n in range(0,256):
            if str(n) in stats_dict:
                num_cells = stats_dict[str(n)]
            else:
                num_cells = 0
                
            cum_cells += num_cells  
            
            # cdf is the the number of cells at or below a given grey value
            #   divided by the total number of cells              
            cdf = float(cum_cells) / float(total_cells)
            
            # insert values into array
            arrays[i][n] = (n, cdf) 
    
    # open file for reclass rules
    outfile = open(grass.tempfile(), 'w')
    new_grey = 0
        
    for i in arrays[original]:
        # for each grey value and corresponding cdf value in original, find the  
        #   cdf value in target that is closest to the target cdf value 
        difference_list = []
        for j in arrays[target]:
            # make a list of the difference between each original cdf value and
            #   the target cdf value
            difference_list.append(abs(i[1] - j[1]))
            
        # get the smallest difference in the list
        min_difference = min(difference_list)

        for j in arrays[target]:
            # find the grey value in target that correspondes to the cdf 
            #   closest to the original cdf
            if j[1] == i[1] + min_difference or j[1] == i[1] - min_difference:
                # build a reclass rules file from the original grey value and
                #   corresponding grey value from target
                out_line = "%d = %d\n" % (i[0], j[0])
                outfile.write(out_line)     
                break
                         
    outfile.close() 
    
    # create reclass of target from reclass rules file
    result = grass.core.find_file(matched, element = 'cell')
    if result['fullname']:
        grass.run_command('g.remove', quiet=True, rast=matched)
        grass.run_command('r.reclass', input=original, out=matched, rules=outfile.name)
    else:
        grass.run_command('r.reclass', input=original, out=matched, rules=outfile.name)

    # Cleanup
    # remove the rules file
    grass.try_remove(outfile.name)  

    # return reclass of target with histogram that matches original
    return matched

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
