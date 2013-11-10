# -*- coding: utf-8 -*-
#   Copyright (C) 2012, Almar Klein
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

""" Module images2ims

Use PIL to create a series of images.

"""

import os

try:
    import numpy as np
except ImportError:
    np = None    

try:
    import PIL
    from PIL import Image    
except ImportError:
    PIL = None


def checkImages(images):
    """ checkImages(images)
    Check numpy images and correct intensity range etc.
    The same for all movie formats.
    """ 
    # Init results
    images2 = []
    
    for im in images:
        if PIL and isinstance(im, PIL.Image.Image):
            # We assume PIL images are allright
            images2.append(im)
        
        elif np and isinstance(im, np.ndarray):
            # Check and convert dtype
            if im.dtype == np.uint8:
                images2.append(im) # Ok
            elif im.dtype in [np.float32, np.float64]:
                theMax = im.max()
                if theMax > 128 and theMax < 300:
                    pass # assume 0:255
                else:
                    im = im.copy()
                    im[im<0] = 0
                    im[im>1] = 1
                    im *= 255
                images2.append( im.astype(np.uint8) )
            else:
                im = im.astype(np.uint8)
                images2.append(im)
            # Check size
            if im.ndim == 2:
                pass # ok
            elif im.ndim == 3:
                if im.shape[2] not in [3,4]:
                    raise ValueError('This array can not represent an image.')
            else:
                raise ValueError('This array can not represent an image.')
        else:
            raise ValueError('Invalid image type: ' + str(type(im)))
    
    # Done
    return images2


def _getFilenameParts(filename):
    if '*' in filename:
        return tuple( filename.split('*',1) )
    else:
        return os.path.splitext(filename)


def _getFilenameWithFormatter(filename, N):
    
    # Determine sequence number formatter
    formatter = '%04i'
    if N < 10:
        formatter = '%i'
    elif N < 100:
        formatter = '%02i'
    elif N < 1000:
        formatter = '%03i'
    
    # Insert sequence number formatter
    part1, part2 = _getFilenameParts(filename)
    return part1 + formatter + part2
    

def _getSequenceNumber(filename, part1, part2):
    # Get string bit
    seq = filename[len(part1):-len(part2)]
    # Get all numeric chars
    seq2 = ''
    for c in seq:
        if c in '0123456789':
            seq2 += c
        else:
            break
    # Make int and return
    return int(seq2)


def writeIms(filename, images):
    """ writeIms(filename, images)
    
    Export movie to a series of image files. If the filenenumber 
    contains an asterix, a sequence number is introduced at its 
    location. Otherwise the sequence number is introduced right 
    before the final dot.
    
    To enable easy creation of a new directory with image files, 
    it is made sure that the full path exists.
    
    Images should be a list consisting of PIL images or numpy arrays. 
    The latter should be between 0 and 255 for integer types, and 
    between 0 and 1 for float types.
    
    """
    
    # Check PIL
    if PIL is None:
        raise RuntimeError("Need PIL to write series of image files.")
    
    # Check images
    images = checkImages(images)
    
    # Get dirname and filename
    filename = os.path.abspath(filename)
    dirname, filename = os.path.split(filename)
    
    # Create dir(s) if we need to
    if not os.path.isdir(dirname):
        os.makedirs(dirname)
    
    # Insert formatter
    filename = _getFilenameWithFormatter(filename, len(images))
    
    # Write
    seq = 0
    for frame in images:
        seq += 1
        # Get filename
        fname = os.path.join(dirname, filename%seq)
        # Write image
        if np and isinstance(frame, np.ndarray):
            frame =  PIL.Image.fromarray(frame)        
        frame.save(fname)



def readIms(filename, asNumpy=True):
    """ readIms(filename, asNumpy=True)
    
    Read images from a series of images in a single directory. Returns a 
    list of numpy arrays, or, if asNumpy is false, a list if PIL images.
    
    """
    
    # Check PIL
    if PIL is None:
        raise RuntimeError("Need PIL to read a series of image files.")
    
    # Check Numpy
    if asNumpy and np is None:
        raise RuntimeError("Need Numpy to return numpy arrays.")
    
    # Get dirname and filename
    filename = os.path.abspath(filename)
    dirname, filename = os.path.split(filename)
    
    # Check dir exists
    if not os.path.isdir(dirname):
        raise IOError('Directory not found: '+str(dirname))
    
    # Get two parts of the filename
    part1, part2 = _getFilenameParts(filename)
    
    # Init images
    images = []
    
    # Get all files in directory
    for fname in os.listdir(dirname):
        if fname.startswith(part1) and fname.endswith(part2):
            # Get sequence number
            nr = _getSequenceNumber(fname, part1, part2)
            # Get Pil image and store copy (to prevent keeping the file)
            im = PIL.Image.open(os.path.join(dirname, fname))
            images.append((im.copy(), nr))
    
    # Sort images 
    images.sort(key=lambda x:x[1])    
    images = [im[0] for im in images]
    
    # Convert to numpy if needed
    if asNumpy:
        images2 = images
        images = []
        for im in images2:
            # Make without palette
            if im.mode == 'P':
                im = im.convert()
            # Make numpy array
            a = np.asarray(im)
            if len(a.shape)==0:
                raise MemoryError("Too little memory to convert PIL image to array")
            # Add
            images.append(a)
    
    # Done
    return images
