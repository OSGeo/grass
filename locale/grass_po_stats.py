#!/usr/bin/env python3
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

from __future__ import annotations

import glob
import json
import os
import subprocess
import sys
from pathlib import Path


def read_po_files(inputdirpath):
    """Return a dictionary with for each language the list of *.po files"""
    originalpath = Path.cwd()
    os.chdir(inputdirpath)
    languages = {}
    for pofile in sorted(glob.glob("*.po")):
        lang = pofile.split("_")[1:]
        # check if are two definitions like pt_br
        if len(lang) == 2:
            lang = ["_".join(lang)]
        lang = lang[0].split(".")[0]

        # if keys is not in languages add it and the file's name
        if lang not in languages:
            languages[lang] = [pofile]
        # add only files name
        else:
            languages[lang].append(pofile)
    os.chdir(originalpath)
    return languages


def read_msgfmt_statistics(msg, lgood, lfuzzy, lbad):
    """Return a dictionary, and the good, fuzzy and bad values from a string"""
    langdict = {}
    # split the output
    out = msg.split(b",")
    # TODO maybe check using regex
    # check for each answer
    for o in out:
        o = o.lower().strip()
        # each answer is written into dictionary and
        # the value add to variable for the sum
        if b"untranslated" in o:
            val = int(o.split(b" ")[0])
            langdict["bad"] = val
            lbad += val
        elif b"fuzzy" in o:
            val = int(o.split(b" ")[0])
            langdict["fuzzy"] = val
            lfuzzy += val
        else:
            val = int(o.split(b" ")[0])
            langdict["good"] = val
            lgood += val
    return langdict, lgood, lfuzzy, lbad


def langDefinition(fil: str) -> str:
    lang: str | list[str] = ""
    with open(fil, encoding="utf-8", errors="replace") as f:
        for line in f:
            if '"Language-Team:' in line:
                lang = line.split(" ")[1:-1]
                break

    if len(lang) == 2:
        return " ".join(lang)
    if len(lang) == 1:
        return lang[0]
    return ""


def get_stats(languages, directory):
    """Return a dictionary with the statistic for each language"""
    # output dictionary to transform in json file
    output = {}
    # TO DO TOTALS OF ENGLISH WORD FOR EACH FILE
    # all the total string in english
    output["totals"] = {}
    # all the information about each lang
    output["langs"] = {}
    # for each language
    for lang, pofilelist in languages.items():
        output["langs"][lang] = {}
        output["langs"][lang]["total"] = {}
        output["langs"][lang]["name"] = langDefinition(
            os.path.join(directory, pofilelist[0])
        )
        # variables to create sum for each language
        lgood = 0
        lfuzzy = 0
        lbad = 0
        # for each file
        for flang in pofilelist:
            fpref = flang.split("_")[0]
            # run msgfmt for statistics
            # TODO check if it's working on windows
            process = subprocess.Popen(
                ["msgfmt", "--statistics", os.path.join(directory, flang)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            msg = process.communicate()[1].strip()
            # check if some errors occurs
            if msg.find(b"error") != -1:
                # TODO CHECK IF grass.warning()
                print("WARNING: file <%s> has some problems: <%s>" % (flang, msg))
                continue
            output["langs"][lang][fpref], lgood, lfuzzy, lbad = read_msgfmt_statistics(
                msg, lgood, lfuzzy, lbad
            )
        # write the sum and total of each file
        output["langs"][lang]["total"]["good"] = lgood
        output["langs"][lang]["total"]["fuzzy"] = lfuzzy
        output["langs"][lang]["total"]["bad"] = lbad
        output["langs"][lang]["total"]["total"] = lgood + lfuzzy + lbad
    return output


def writejson(stats, outfile):
    # load dictionary into json format
    fjson = json.dumps(stats, sort_keys=True, indent=4)
    # write a string with pretty style
    outjson = os.linesep.join([line.rstrip() for line in fjson.splitlines()])
    # write out file
    with open(outfile, "w", encoding="utf-8") as fout:
        fout.write(outjson)
        fout.write(os.linesep)
    try:
        os.remove("messages.mo")
    except OSError:
        pass


def main(in_dirpath, out_josonpath):
    languages = read_po_files(in_dirpath)
    stats = get_stats(languages, in_dirpath)

    if Path(out_josonpath).exists():
        os.remove(out_josonpath)
    writejson(stats, out_josonpath)


if __name__ == "__main__":
    directory = "po/"
    outfile = os.path.join(os.environ["GISBASE"], "translation_status.json")
    sys.exit(main(directory, outfile))
