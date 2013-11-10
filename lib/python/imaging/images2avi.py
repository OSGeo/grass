#   Copyright (C) 2012, Almar Klein
#   All rights reserved.
#
#   This code is subject to the (new) BSD license:
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the <organization> nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY 
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
# changes of this file GRASS (PNG instead of JPG) by Anna Petrasova 2013 

""" Module images2avi

Uses ffmpeg to read and write AVI files. Requires PIL

I found these sites usefull:
http://www.catswhocode.com/blog/19-ffmpeg-commands-for-all-needs
http://linux.die.net/man/1/ffmpeg

"""

import os, time
import subprocess, shutil
from grass.imaging import images2ims


def _cleanDir(tempDir):
    for i in range(3):
        try:
            shutil.rmtree(tempDir)
        except Exception:
            time.sleep(0.2) # Give OS time to free sources
        else:
            break
    else:
        print("Oops, could not fully clean up temporary files.")


def writeAvi(filename, images, duration=0.1, encoding='mpeg4', 
                                        inputOptions='', outputOptions='' ):
    """ writeAvi(filename, duration=0.1, encoding='mpeg4',
                    inputOptions='', outputOptions='')
    
    Export movie to a AVI file, which is encoded with the given 
    encoding. Hint for Windows users: the 'msmpeg4v2' codec is 
    natively supported on Windows.
    
    Images should be a list consisting of PIL images or numpy arrays. 
    The latter should be between 0 and 255 for integer types, and 
    between 0 and 1 for float types.
    
    Requires the "ffmpeg" application:
      * Most linux users can install using their package manager
      * There is a windows installer on the visvis website
    
    """
    
    # Get fps
    try:
        fps = float(1.0/duration)
    except Exception:
        raise ValueError("Invalid duration parameter for writeAvi.")
    
    # Determine temp dir and create images
    tempDir = os.path.join( os.path.expanduser('~'), '.tempIms')
    images2ims.writeIms( os.path.join(tempDir, 'im*.png'), images)
    
    # Determine formatter
    N = len(images)
    formatter = '%04d'
    if N < 10:
        formatter = '%d'
    elif N < 100:
        formatter = '%02d'
    elif N < 1000:
        formatter = '%03d'
    
    # Compile command to create avi
    command = "ffmpeg -r %i %s " % (int(fps), inputOptions)
    command += "-i im%s.png " % (formatter,)
    command += "-g 1 -vcodec %s %s " % (encoding, outputOptions) 
    command += "output.avi"
    
    # Run ffmpeg
    S = subprocess.Popen(command, shell=True, cwd=tempDir,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    
    # Show what ffmpeg has to say
    outPut = S.stdout.read()
    
    if S.wait():    
        # An error occured, show
        print(outPut)
        print(S.stderr.read())
        # Clean up
        _cleanDir(tempDir)
        raise RuntimeError("Could not write avi.")
    else:
        # Copy avi
        shutil.copy(os.path.join(tempDir, 'output.avi'), filename)
        # Clean up
        _cleanDir(tempDir)


def readAvi(filename, asNumpy=True):
    """ readAvi(filename, asNumpy=True)
    
    Read images from an AVI (or MPG) movie.
    
    Requires the "ffmpeg" application:
      * Most linux users can install using their package manager
      * There is a windows installer on the visvis website
    
    """
    
    # Check whether it exists
    if not os.path.isfile(filename):
        raise IOError('File not found: '+str(filename))
    
    # Determine temp dir, make sure it exists
    tempDir = os.path.join( os.path.expanduser('~'), '.tempIms')
    if not os.path.isdir(tempDir):
        os.makedirs(tempDir)
    
    # Copy movie there
    shutil.copy(filename, os.path.join(tempDir, 'input.avi'))
    
    # Run ffmpeg
    command = "ffmpeg -i input.avi im%d.jpg"
    S = subprocess.Popen(command, shell=True, cwd=tempDir,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    
    # Show what mencodec has to say
    outPut = S.stdout.read()
    
    if S.wait():    
        # An error occured, show
        print(outPut)
        print(S.stderr.read())
        # Clean up
        _cleanDir(tempDir)
        raise RuntimeError("Could not read avi.")
    else:
        # Read images
        images = images2ims.readIms(os.path.join(tempDir, 'im*.jpg'), asNumpy)
        # Clean up
        _cleanDir(tempDir)
    
    # Done
    return images
