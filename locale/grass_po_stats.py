#!/usr/bin/env python
#############################################################################
#
# MODULE:       Languages information and statistics (Python)
# AUTHOR(S):    Luca Delucchi <lucadeluge@gmail.com>
#               Pietro Zambelli <peter.zamb@gmail.com>
# PURPOSE:      Create a json file containing languages translations
#               information and statistics.
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
import glob
import codecs

def read_po_files(inputdirpath):
    """Return a dictionary with for each language the list of *.po files"""
    originalpath = os.getcwd()
    os.chdir(inputdirpath)
    languages = {}
    for pofile in glob.glob("*.po"):
        lang = pofile.split('_')[1:]
        # check if are two definitions like pt_br
        if len(lang) == 2:
            lang = ['_'.join(lang)]
        lang = lang[0].split('.')[0]

        # if keys is not in languages add it and the file's name
        if not languages.has_key(lang):
            languages[lang] = [pofile,]
        # add only files name
        else:
            languages[lang].append(pofile)
    os.chdir(originalpath)
    return languages


def read_msgfmt_statistics(msg, lgood, lfuzzy, lbad):
    """Return a dictionary, and the good, fuzzy and bad values from a string"""
    langdict = {}
    # split the output
    out = msg.split(',')
    # TODO maybe check using regex
    # check for each answer
    for o in out:
        o = o.lower().strip()
        # each answer is written into dictionary and
        # the value add to variable for the sum
        if 'untranslated' in o:
            val = int(o.split(' ')[0])
            langdict['bad'] = val
            lbad += val
        elif 'fuzzy' in o:
            val = int(o.split(' ')[0])
            langdict['fuzzy'] = val
            lfuzzy += val
        else:
            val = int(o.split(' ')[0])
            langdict['good'] = val
            lgood += val
    return langdict, lgood, lfuzzy, lbad

def langDefinition(fil):
    f = codecs.open(fil,encoding = 'utf-8', errors='replace', mode = 'r')
    for l in f.readlines():
        if '"Language-Team:' in l:
            lang = l.split(' ')[1:-1]
            break
    f.close()
    if len(lang) == 2:
        return " ".join(lang)
    elif len(lang) == 1:
        return lang[0]
    else:
        return ""

def get_stats(languages, directory):
    """Return a dictionay with the statistic for each language"""
    # output dictionary to transform in json file
    output = {}
    # TO DO TOTALS OF ENGLISH WORD FOR EACH FILE
    # all the total string in english
    output['totals'] = {}
    # all the information about each lang
    output['langs'] = {}
    # for each language
    for lang, pofilelist in languages.iteritems():
        output['langs'][lang] = {}
        output['langs'][lang]['total'] = {}
        output['langs'][lang]['name'] = langDefinition(os.path.join(directory,pofilelist[0]))
        # variables to create sum for each language
        lgood = 0
        lfuzzy = 0
        lbad = 0
        # for each file
        for flang in pofilelist:

            fpref = flang.split('_')[0]
            # run msgfmt for statistics
            # TODO check if it's working on windows
            process = subprocess.Popen(['msgfmt', '--statistics',
                                os.path.join(directory,flang)],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            msg = process.communicate()[1].strip()
            # check if some errors occurs
            if msg.find('error') != -1:
                # TODO CHECK IF grass.warning()
                print "WARNING: file <%s> has some problems: <%s>" % (flang, msg)
                continue
            output['langs'][lang][fpref], lgood, lfuzzy, lbad = \
            read_msgfmt_statistics(msg, lgood, lfuzzy, lbad)
        # write the sum and total of each file
        output['langs'][lang]['total']['good'] = lgood
        output['langs'][lang]['total']['fuzzy'] = lfuzzy
        output['langs'][lang]['total']['bad'] = lbad
        output['langs'][lang]['total']['total'] = lgood + lfuzzy + lbad
    return output


def writejson(stats, outfile):
    # load dictionary into json format
    fjson = json.dumps(stats, sort_keys=True, indent=4)
    # write a string with pretty style
    outjson = os.linesep.join([l.rstrip() for l in  fjson.splitlines()])
    # write out file
    fout = open(outfile,'w')
    fout.write(outjson)
    fout.write(os.linesep)
    fout.close()
    try:
        os.remove("messages.mo")
    except:
	pass

def main(in_dirpath, out_josonpath):
    languages = read_po_files(in_dirpath)
    stats = get_stats(languages, in_dirpath)

    if os.path.exists(out_josonpath):
        os.remove(out_josonpath)
    writejson(stats, out_josonpath)


if __name__ == "__main__":
    directory = 'po/'
    outfile = os.path.join(os.environ['GISBASE'],'translation_status.json')
    sys.exit(main(directory, outfile))
