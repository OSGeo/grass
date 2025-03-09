#!/usr/bin/env python3
# Copyright 2008 Marcus D. Hanwell <marcus@cryos.org>
# Minor changes for NUT by Charles Lepple
# Distributed under the terms of the GNU General Public License v2 or later

import re
from textwrap import TextWrapper
import sys
import subprocess

rev_range = ""


# Define the git command and its arguments as a list
git_command = [
    "git",
    "log",
    "--summary",
    "--stat",
    "--no-merges",
    "--date=short",
]

if len(sys.argv) > 1:
    base = sys.argv[1]
    rev_range = "%s..HEAD" % base
    git_command.append(rev_range)

# Execute git log with the desired command line options.
process = subprocess.Popen(git_command, stdout=subprocess.PIPE, encoding="utf8")
fin = process.stdout

# Set up the loop variables in order to locate the blocks we want
authorFound = False
dateFound = False
messageFound = False
filesFound = False
message = ""
messageNL = False
files = ""
prevAuthorLine = ""

wrapper = TextWrapper(initial_indent="\t", subsequent_indent="\t  ")

with open("ChangeLog", "w") as fout:
    # The main part of the loop
    for line in fin:
        # The commit line marks the start of a new commit object.
        if line.startswith("commit"):
            # Start all over again...
            authorFound = False
            dateFound = False
            messageFound = False
            messageNL = False
            message = ""
            filesFound = False
            files = ""
            continue
        # Match the author line and extract the part we want
        # (Don't use startswith to allow Author override inside commit message.)
        elif "Author:" in line:
            authorList = re.split(r": ", line, 1)
            try:
                author = authorList[1]
                author = author[0 : len(author) - 1]
                authorFound = True
            except Exception as e:
                print(f"Could not parse authorList = '{line}'. Error: {e!s}")

        # Match the date line
        elif line.startswith("Date:"):
            dateList = re.split(r":   ", line, 1)
            try:
                date = dateList[1]
                date = date[0 : len(date) - 1]
                dateFound = True
            except Exception as e:
                print(f"Could not parse dateList = '{line}'. Error: {e!s}")
        # The Fossil-IDs, svn-id, ad sign off lines are ignored:
        elif (
            line.startswith(("    Fossil-ID:", "    [[SVN:"))
            or "    git-svn-id:" in line
            or "Signed-off-by" in line
        ):
            continue
        # Extract the actual commit message for this commit
        elif authorFound & dateFound & messageFound is False:
            # Find the commit message if we can
            if len(line) == 1:
                if messageNL:
                    messageFound = True
                else:
                    messageNL = True
            elif len(line) == 4:
                messageFound = True
            elif len(message) == 0:
                message += line.strip()
            else:
                message = message + " " + line.strip()
        # If this line is hit all of the files have been stored for this commit
        elif re.search(r"files? changed", line):
            filesFound = True
            continue
        # Collect the files for this commit. FIXME: Still need to add +/- to files
        elif authorFound & dateFound & messageFound:
            fileList = re.split(r" \| ", line, 2)
            if len(fileList) > 1:
                if len(files) > 0:
                    files = files + ", " + fileList[0].strip()
                else:
                    files = fileList[0].strip()
        # All of the parts of the commit have been found - write out the entry
        if authorFound & dateFound & messageFound & filesFound:
            # First the author line, only outputted if it is the first for that
            # author on this day
            authorLine = date + "  " + author
            if len(prevAuthorLine) == 0:
                fout.write(authorLine + "\n\n")
            elif authorLine == prevAuthorLine:
                pass
            else:
                fout.write("\n" + authorLine + "\n\n")

            # Assemble the actual commit message line(s) and limit the line length
            # to 80 characters.
            commitLine = "* " + files + ": " + message

            # Write out the commit line
            fout.write(wrapper.fill(commitLine) + "\n")

            # Now reset all the variables ready for a new commit block.
            authorFound = False
            dateFound = False
            messageFound = False
            messageNL = False
            message = ""
            filesFound = False
            files = ""
            prevAuthorLine = authorLine

# Close the input and output lines now that we are finished.
fin.close()
