#!/usr/bin/env python
#############################################################################
#
# MODULE:       Languages informations and statistics (Python)
# AUTHOR(S):    Luca Delucchi <lucadeluge@gmail.com>
# PURPOSE:      Create a json file containing languages translations 
#               informations and statistics.
# COPYRIGHT:    (C) 2012 by the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

import os, sys
import subprocess
import json

def main():
    directory = 'po/'
    # TO DO CHANGE WITH GISBASE
    outdirectory = 'po/'
    outfilename = 'translation_status.json'
    # TODO CHECK IF grass.try_remove CAN WORK
    try:
        os.remove(os.path.join(directory,outfilename))
    except:
        pass
    # dictionary contaning languages and file name
    languages = {}
    # for each po file 
    #TODO MAYBE USE OTHER FUNCTIONS to extract only po file and remove if
    for f in os.listdir(directory):
        if '.po' in f:
            # split filename
            lang = f.split('_')[1:]
            # check if are two definitions like pt_br
            if len(lang) == 2:
                lang = ['_'.join(lang)]
            lang = lang[0].split('.')[0]
            # if keys is not in languages add it and the file's name
            if lang not in languages.keys():
                languages[lang] = [f]
            # add only files name
            else:
                languages[lang].append(f)
    # output dictionary to transform in json file
    output = {}
    # TO DO TOTALS OF ENGLISH WORD FOR EACH FILE
    # all the total string in english 
    output['totals'] = {}
    # all the informations about each lang
    output['langs'] = {}
    # for each language
    for k,v in languages.iteritems():
        output['langs'][k] = {}
        output['langs'][k]['total'] = {}
        # variables to create sum for each language
        lgood = 0
        lfuzzy = 0
        lbad = 0
        # for each file 
        for flang in v:
            fpref = flang.split('_')[0]
            output['langs'][k][fpref] = {}
            # run msgfmt for statistics
            # TODO check if it's working on windows
            p = subprocess.Popen(['msgfmt', '--statistics', 
                                os.path.join(directory,flang)], 
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            out = p.communicate()[1].strip()
            # check if some errors occurs 
            if out.find('error') != -1:
                # TODO CHECK IF grass.warning()
                print "WARNING: file <%s> has some problems" % flang
                continue
            # split the output
            out = out.split(',')
            # TODO maybe check using regex
            # check for each answer
            for o in out:
                o = o.strip()
                # each answer it's written into dictionare and
                # the value add to variable for the sum
                if 'untranslated' in o:
                    val = int(o.split(' ')[0])
                    output['langs'][k][fpref]['bad'] = val
                    lbad += val
                elif 'fuzzy' in o:
                    val = int(o.split(' ')[0])
                    output['langs'][k][fpref]['fuzzy'] = val
                    lfuzzy += val
                else:
                    val = int(o.split(' ')[0])
                    output['langs'][k][fpref]['good'] = val
                    lgood += val
        # write the sum and total of each file
        output['langs'][k]['total']['good'] = lgood
        output['langs'][k]['total']['fuzzy'] = lfuzzy
        output['langs'][k]['total']['bad'] = lbad
        output['langs'][k]['total']['total'] = lgood + lfuzzy + lbad
    # load dictionary into json format    
    fjson = json.dumps(output, sort_keys=True, indent=4)
    # write a string with pretty style
    outjson = '\n'.join([l.rstrip() for l in  fjson.splitlines()])
    # write to file!
    fout = open(os.path.join(directory,outfilename),'w')
    fout.write(outjson)
    fout.close()
    
if __name__ == "__main__":
    sys.exit(main())

