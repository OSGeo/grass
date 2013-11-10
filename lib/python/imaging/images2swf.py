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

""" Module images2swf

Provides a function (writeSwf) to store a series of PIL images or numpy 
arrays in an SWF movie, that can be played on a wide range of OS's. 

This module came into being because I wanted to store a series of images
in a movie that can be viewed by other people, and which I can embed in 
flash presentations. For writing AVI or MPEG you really need a c/c++ 
library, and allthough the filesize is then very small, the quality is 
sometimes not adequate. Besides I'd like to be independant of yet another 
package. I tried writing animated gif using PIL (which is widely available), 
but the quality is so poor because it only allows for 256 different colors.
[EDIT: thanks to Ant1, now the quality of animated gif isn't so bad!]
I also looked into MNG and APNG, two standards similar to the PNG stanard.
Both standards promise exactly what I need. However, hardly any application
can read those formats, and I cannot import them in flash. 

Therefore I decided to check out the swf file format, which is very well
documented. This is the result: a pure python module to create an SWF file
that shows a series of images. The images are stored using the DEFLATE
algorithm (same as PNG and ZIP and which is included in the standard Python
distribution). As this compression algorithm is much more effective than 
that used in GIF images, we obtain better quality (24 bit colors + alpha
channel) while still producesing smaller files (a test showed ~75%).
Although SWF also allows for JPEG compression, doing so would probably 
require a third party library (because encoding JPEG is much harder).

This module requires Python 2.x and numpy.

sources and tools:
- SWF on wikipedia
- Adobes "SWF File Format Specification" version 10
  (http://www.adobe.com/devnet/swf/pdf/swf_file_format_spec_v10.pdf)
- swftools (swfdump in specific) for debugging
- iwisoft swf2avi can be used to convert swf to avi/mpg/flv with really
  good quality, while file size is reduced with factors 20-100.
  A good program in my opinion. The free version has the limitation
  of a watermark in the upper left corner. 


"""

import os, sys, time
import zlib

try:
    import numpy as np
except ImportError:
    np = None 

try: 
    import PIL.Image
except ImportError:
    PIL = None


# True if we are running on Python 3.
# Code taken from six.py by Benjamin Peterson (MIT licensed)
import types
PY3 = sys.version_info[0] == 3
if PY3:
    string_types = str,
    integer_types = int,
    class_types = type,
    text_type = str
    binary_type = bytes
else:
    string_types = basestring,
    integer_types = (int, long)
    class_types = (type, types.ClassType)
    text_type = unicode
    binary_type = str


# todo: use imageio/FreeImage to support reading JPEG images from SWF?


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


## Base functions and classes


class BitArray:
    """ Dynamic array of bits that automatically resizes
    with factors of two. 
    Append bits using .Append() or += 
    You can reverse bits using .Reverse()
    """
    
    def __init__(self, initvalue=None):
        self.data = np.zeros((16,), dtype=np.uint8)
        self._len = 0
        if initvalue is not None:
            self.Append(initvalue)
    
    def __len__(self):
        return self._len #self.data.shape[0]
    
    def __repr__(self):
        return self.data[:self._len].tostring().decode('ascii')
    
    def _checkSize(self):
        # check length... grow if necessary
        arraylen = self.data.shape[0]
        if self._len >= arraylen:
            tmp = np.zeros((arraylen*2,), dtype=np.uint8)
            tmp[:self._len] = self.data[:self._len]
            self.data = tmp
    
    def __add__(self, value):
        self.Append(value)
        return self
    
    def Append(self, bits):
        
        # check input
        if isinstance(bits, BitArray):
            bits = str(bits)
        if isinstance(bits, int):
            bits = str(bits)
        if not isinstance(bits, string_types):
            raise ValueError("Append bits as strings or integers!")
        
        # add bits
        for bit in bits:
            self.data[self._len] = ord(bit)
            self._len += 1
            self._checkSize()
    
    def Reverse(self):
        """ In-place reverse. """
        tmp = self.data[:self._len].copy()
        self.data[:self._len] = np.flipud(tmp)
    
    def ToBytes(self):
        """ Convert to bytes. If necessary,
        zeros are padded to the end (right side).
        """
        bits = str(self)
        
        # determine number of bytes
        nbytes = 0
        while nbytes*8 < len(bits):
            nbytes +=1
        # pad
        bits = bits.ljust(nbytes*8, '0')
        
        # go from bits to bytes
        bb = binary_type()
        for i in range(nbytes):
            tmp = int( bits[i*8:(i+1)*8], 2)
            bb += intToUint8(tmp)
        
        # done
        return bb


if PY3:
    def intToUint32(i):
        return int(i).to_bytes(4,'little')
    def intToUint16(i):
        return int(i).to_bytes(2,'little')
    def intToUint8(i):
        return int(i).to_bytes(1,'little')
else:
    def intToUint32(i):
        number = int(i)
        n1, n2, n3, n4 = 1, 256, 256*256, 256*256*256
        b4, number = number // n4, number % n4
        b3, number = number // n3, number % n3
        b2, number = number // n2, number % n2
        b1 = number
        return chr(b1) + chr(b2) + chr(b3) + chr(b4)
    def intToUint16(i):
        i = int(i)
        # devide in two parts (bytes)    
        i1 = i % 256
        i2 = int( i//256)
        # make string (little endian)
        return chr(i1) + chr(i2)
    def intToUint8(i):
        return chr(int(i))


def intToBits(i,n=None):
    """ convert int to a string of bits (0's and 1's in a string), 
    pad to n elements. Convert back using int(ss,2). """
    ii = i
    
    # make bits    
    bb = BitArray()
    while ii > 0:
        bb += str(ii % 2)
        ii = ii >> 1
    bb.Reverse()
    
    # justify
    if n is not None:
        if len(bb) > n:
            raise ValueError("intToBits fail: len larger than padlength.")
        bb = str(bb).rjust(n,'0')
    
    # done
    return BitArray(bb)

def bitsToInt(bb, n=8):
    # Init
    value = ''
    
    # Get value in bits
    for i in range(len(bb)):
        b = bb[i:i+1]
        tmp = bin(ord(b))[2:]
        #value += tmp.rjust(8,'0')
        value = tmp.rjust(8,'0') + value
    
    # Make decimal
    return( int(value[:n], 2) )

def getTypeAndLen(bb):
    """ bb should be 6 bytes at least
    Return (type, length, length_of_full_tag)
    """
    # Init
    value = ''
    
    # Get first 16 bits
    for i in range(2):
        b = bb[i:i+1]
        tmp = bin(ord(b))[2:]
        #value += tmp.rjust(8,'0')
        value = tmp.rjust(8,'0') + value
    
    # Get type and length
    type = int( value[:10], 2)
    L = int( value[10:], 2)
    L2 = L + 2
    
    # Long tag header?
    if L == 63: # '111111'
        value = ''
        for i in range(2,6):
            b = bb[i:i+1] # becomes a single-byte bytes() on both PY3 and PY2
            tmp = bin(ord(b))[2:]
            #value += tmp.rjust(8,'0')
            value = tmp.rjust(8,'0') + value
        L = int( value, 2)
        L2 = L + 6
    
    # Done    
    return type, L, L2


def signedIntToBits(i,n=None):
    """ convert signed int to a string of bits (0's and 1's in a string), 
    pad to n elements. Negative numbers are stored in 2's complement bit
    patterns, thus positive numbers always start with a 0.
    """
    
    # negative number?
    ii = i    
    if i<0:
        # A negative number, -n, is represented as the bitwise opposite of
        ii = abs(ii) -1  # the positive-zero number n-1.
    
    # make bits    
    bb = BitArray()
    while ii > 0:
        bb += str(ii % 2)
        ii = ii >> 1
    bb.Reverse()
    
    # justify
    bb = '0' + str(bb) # always need the sign bit in front
    if n is not None:
        if len(bb) > n:
            raise ValueError("signedIntToBits fail: len larger than padlength.")
        bb = bb.rjust(n,'0')
    
    # was it negative? (then opposite bits)
    if i<0:
        bb = bb.replace('0','x').replace('1','0').replace('x','1')
    
    # done
    return BitArray(bb)


def twitsToBits(arr):
    """ Given a few (signed) numbers, store them 
    as compactly as possible in the wat specifief by the swf format.
    The numbers are multiplied by 20, assuming they
    are twits.
    Can be used to make the RECT record.
    """
    
    # first determine length using non justified bit strings
    maxlen = 1
    for i in arr:
        tmp = len(signedIntToBits(i*20))
        if tmp > maxlen:
            maxlen = tmp
    
    # build array
    bits = intToBits(maxlen,5) 
    for i in arr:
        bits += signedIntToBits(i*20, maxlen)
    
    return bits


def floatsToBits(arr):
    """ Given a few (signed) numbers, convert them to bits, 
    stored as FB (float bit values). We always use 16.16. 
    Negative numbers are not (yet) possible, because I don't
    know how the're implemented (ambiguity).
    """
    bits = intToBits(31, 5) # 32 does not fit in 5 bits!
    for i in arr:
        if i<0:
            raise ValueError("Dit not implement negative floats!")
        i1 = int(i)
        i2 = i - i1
        bits += intToBits(i1, 15)
        bits += intToBits(i2*2**16, 16)
    return bits
    

def _readFrom(fp, n):
    bb = binary_type()
    try:
        while len(bb) < n:
            tmp = fp.read(n-len(bb))
            bb += tmp
            if not tmp:
                break
    except EOFError:
        pass
    return bb


## Base Tag

class Tag:
    
    def __init__(self):
        self.bytes = binary_type()    
        self.tagtype = -1
    
    def ProcessTag(self):
        """ Implement this to create the tag. """
        raise NotImplemented()
    
    def GetTag(self):
        """ Calls processTag and attaches the header. """
        self.ProcessTag()
        
        # tag to binary
        bits = intToBits(self.tagtype,10)
        
        # complete header uint16 thing
        bits += '1'*6 # = 63 = 0x3f
        # make uint16
        bb = intToUint16( int(str(bits),2) )
        
        # now add 32bit length descriptor
        bb += intToUint32(len(self.bytes))
        
        # done, attach and return
        bb += self.bytes
        return bb
    
    def MakeRectRecord(self, xmin, xmax, ymin, ymax):
        """ Simply uses makeCompactArray to produce
        a RECT Record. """
        return twitsToBits([xmin, xmax, ymin, ymax])

    def MakeMatrixRecord(self, scale_xy=None, rot_xy=None, trans_xy=None):
        
        # empty matrix?
        if scale_xy is None and rot_xy is None and trans_xy is None:
            return "0"*8
        
        # init
        bits = BitArray()
        
        # scale
        if scale_xy: 
            bits += '1'
            bits += floatsToBits([scale_xy[0], scale_xy[1]])
        else: 
            bits += '0'
        
        # rotation
        if rot_xy: 
            bits += '1'
            bits += floatsToBits([rot_xy[0], rot_xy[1]])
        else: 
            bits += '0'
        
        # translation (no flag here)
        if trans_xy: 
            bits += twitsToBits([trans_xy[0], trans_xy[1]])
        else: 
            bits += twitsToBits([0,0])
        
        # done
        return bits


## Control tags

class ControlTag(Tag):
    def __init__(self):
        Tag.__init__(self)


class FileAttributesTag(ControlTag):
    def __init__(self):
        ControlTag.__init__(self)
        self.tagtype = 69
    
    def ProcessTag(self):
        self.bytes = '\x00'.encode('ascii') * (1+3)


class ShowFrameTag(ControlTag):
    def __init__(self):
        ControlTag.__init__(self)
        self.tagtype = 1
    def ProcessTag(self):
        self.bytes = binary_type()

class SetBackgroundTag(ControlTag):
    """ Set the color in 0-255, or 0-1 (if floats given). """
    def __init__(self, *rgb):
        self.tagtype = 9
        if len(rgb)==1:
            rgb = rgb[0]
        self.rgb = rgb
    
    def ProcessTag(self):
        bb = binary_type()
        for i in range(3):            
            clr = self.rgb[i]
            if isinstance(clr, float):
                clr = clr * 255
            bb += intToUint8(clr)
        self.bytes = bb


class DoActionTag(Tag):
    def __init__(self, action='stop'):
        Tag.__init__(self)
        self.tagtype = 12
        self.actions = [action]
    
    def Append(self, action):
        self.actions.append( action )
    
    def ProcessTag(self):
        bb = binary_type()
        
        for action in self.actions:
            action = action.lower()            
            if action == 'stop':
                bb += '\x07'.encode('ascii')
            elif action == 'play':
                bb += '\x06'.encode('ascii')
            else:
                print("warning, unkown action: %s" % action)
        
        bb += intToUint8(0)
        self.bytes = bb
        


## Definition tags

class DefinitionTag(Tag):
    counter = 0 # to give automatically id's
    def __init__(self):
        Tag.__init__(self)
        DefinitionTag.counter += 1
        self.id = DefinitionTag.counter  # id in dictionary


class BitmapTag(DefinitionTag):
    
    def __init__(self, im):
        DefinitionTag.__init__(self)
        self.tagtype = 36 # DefineBitsLossless2
        
        # convert image (note that format is ARGB)
        # even a grayscale image is stored in ARGB, nevertheless,
        # the fabilous deflate compression will make it that not much
        # more data is required for storing (25% or so, and less than 10%
        # when storing RGB as ARGB).
        
        if len(im.shape)==3:
            if im.shape[2] in [3, 4]:
                tmp = np.ones((im.shape[0], im.shape[1], 4), dtype=np.uint8)*255
                for i in range(3):                    
                    tmp[:,:,i+1] = im[:,:,i]
                if im.shape[2]==4:
                    tmp[:,:,0] = im[:,:,3] # swap channel where alpha is in
            else:
                raise ValueError("Invalid shape to be an image.")
            
        elif len(im.shape)==2:
            tmp = np.ones((im.shape[0], im.shape[1], 4), dtype=np.uint8)*255
            for i in range(3):
                tmp[:,:,i+1] = im[:,:]
        else:
            raise ValueError("Invalid shape to be an image.")
        
        # we changed the image to uint8 4 channels.
        # now compress!
        self._data = zlib.compress(tmp.tostring(), zlib.DEFLATED)
        self.imshape = im.shape
    
    
    def ProcessTag(self):
        
        # build tag
        bb = binary_type()   
        bb += intToUint16(self.id)   # CharacterID    
        bb += intToUint8(5)     # BitmapFormat
        bb += intToUint16(self.imshape[1])   # BitmapWidth
        bb += intToUint16(self.imshape[0])   # BitmapHeight       
        bb += self._data            # ZlibBitmapData
        
        self.bytes = bb


class PlaceObjectTag(ControlTag):
    def __init__(self, depth, idToPlace=None, xy=(0,0), move=False):
        ControlTag.__init__(self)
        self.tagtype = 26
        self.depth = depth
        self.idToPlace = idToPlace
        self.xy = xy
        self.move = move
    
    def ProcessTag(self):
        # retrieve stuff
        depth = self.depth
        xy = self.xy
        id = self.idToPlace
        
        # build PlaceObject2
        bb = binary_type()
        if self.move:
            bb += '\x07'.encode('ascii')
        else:
            bb += '\x06'.encode('ascii')  # (8 bit flags): 4:matrix, 2:character, 1:move
        bb += intToUint16(depth) # Depth
        bb += intToUint16(id) # character id
        bb += self.MakeMatrixRecord(trans_xy=xy).ToBytes() # MATRIX record
        self.bytes = bb
    

class ShapeTag(DefinitionTag):
    def __init__(self, bitmapId, xy, wh):
        DefinitionTag.__init__(self)
        self.tagtype = 2
        self.bitmapId = bitmapId
        self.xy = xy
        self.wh = wh
    
    def ProcessTag(self):
        """ Returns a defineshape tag. with a bitmap fill """
        
        bb = binary_type()
        bb += intToUint16(self.id)
        xy, wh = self.xy, self.wh
        tmp = self.MakeRectRecord(xy[0],wh[0],xy[1],wh[1])  # ShapeBounds
        bb += tmp.ToBytes()
        
        # make SHAPEWITHSTYLE structure
        
        # first entry: FILLSTYLEARRAY with in it a single fill style
        bb += intToUint8(1)  # FillStyleCount
        bb += '\x41'.encode('ascii') # FillStyleType  (0x41 or 0x43, latter is non-smoothed)
        bb += intToUint16(self.bitmapId)  # BitmapId
        #bb += '\x00' # BitmapMatrix (empty matrix with leftover bits filled)
        bb += self.MakeMatrixRecord(scale_xy=(20,20)).ToBytes()
        
#         # first entry: FILLSTYLEARRAY with in it a single fill style
#         bb += intToUint8(1)  # FillStyleCount
#         bb += '\x00' # solid fill
#         bb += '\x00\x00\xff' # color
        
        
        # second entry: LINESTYLEARRAY with a single line style
        bb += intToUint8(0)  # LineStyleCount
        #bb += intToUint16(0*20) # Width
        #bb += '\x00\xff\x00'  # Color
        
        # third and fourth entry: NumFillBits and NumLineBits (4 bits each)
        bb += '\x44'.encode('ascii')  # I each give them four bits, so 16 styles possible.
        
        self.bytes = bb
        
        # last entries: SHAPERECORDs ... (individual shape records not aligned)
        # STYLECHANGERECORD
        bits = BitArray()
        bits += self.MakeStyleChangeRecord(0,1,moveTo=(self.wh[0],self.wh[1]))
        # STRAIGHTEDGERECORD 4x
        bits += self.MakeStraightEdgeRecord(-self.wh[0], 0)
        bits += self.MakeStraightEdgeRecord(0, -self.wh[1])
        bits += self.MakeStraightEdgeRecord(self.wh[0], 0)
        bits += self.MakeStraightEdgeRecord(0, self.wh[1])
        
        # ENDSHAPRECORD
        bits += self.MakeEndShapeRecord()
        
        self.bytes += bits.ToBytes()
        
        # done
        #self.bytes = bb

    def MakeStyleChangeRecord(self, lineStyle=None, fillStyle=None, moveTo=None):
        
        # first 6 flags
        # Note that we use FillStyle1. If we don't flash (at least 8) does not
        # recognize the frames properly when importing to library.
        
        bits = BitArray()
        bits += '0' # TypeFlag (not an edge record)
        bits += '0' # StateNewStyles (only for DefineShape2 and Defineshape3)
        if lineStyle:  bits += '1' # StateLineStyle
        else: bits += '0'
        if fillStyle: bits += '1' # StateFillStyle1
        else: bits += '0'
        bits += '0' # StateFillStyle0        
        if moveTo: bits += '1' # StateMoveTo
        else: bits += '0'
        
        # give information
        # todo: nbits for fillStyle and lineStyle is hard coded.
        
        if moveTo:
            bits += twitsToBits([moveTo[0], moveTo[1]])
        if fillStyle:
            bits += intToBits(fillStyle,4)
        if lineStyle:
            bits += intToBits(lineStyle,4)
        
        return bits
        #return bitsToBytes(bits)


    def MakeStraightEdgeRecord(self, *dxdy):
        if len(dxdy)==1:
            dxdy = dxdy[0]
        
        # determine required number of bits
        xbits, ybits = signedIntToBits(dxdy[0]*20), signedIntToBits(dxdy[1]*20)
        nbits = max([len(xbits),len(ybits)])
        
        bits = BitArray()
        bits += '11'  # TypeFlag and StraightFlag
        bits += intToBits(nbits-2,4)
        bits += '1' # GeneralLineFlag
        bits += signedIntToBits(dxdy[0]*20,nbits)
        bits += signedIntToBits(dxdy[1]*20,nbits)
        
        # note: I do not make use of vertical/horizontal only lines...
        
        return bits
        #return bitsToBytes(bits)
        

    def MakeEndShapeRecord(self):
        bits = BitArray()
        bits +=  "0"     # TypeFlag: no edge 
        bits += "0"*5   # EndOfShape
        return bits
        #return bitsToBytes(bits)


## Last few functions

    

def buildFile(fp, taglist, nframes=1, framesize=(500,500), fps=10, version=8):
    """ Give the given file (as bytes) a header. """
    
    # compose header
    bb = binary_type()
    bb += 'F'.encode('ascii')  # uncompressed 
    bb += 'WS'.encode('ascii')  # signature bytes
    bb += intToUint8(version) # version
    bb += '0000'.encode('ascii') # FileLength (leave open for now)
    bb += Tag().MakeRectRecord(0,framesize[0], 0, framesize[1]).ToBytes()
    bb += intToUint8(0) + intToUint8(fps) # FrameRate
    bb += intToUint16(nframes)    
    fp.write(bb)
    
    # produce all tags    
    for tag in taglist:
        fp.write( tag.GetTag() )
    
    # finish with end tag
    fp.write( '\x00\x00'.encode('ascii') )
    
    # set size
    sze = fp.tell()    
    fp.seek(4)
    fp.write( intToUint32(sze) )


def writeSwf(filename, images, duration=0.1, repeat=True):
    """ writeSwf(filename, images, duration=0.1, repeat=True)
    
    Write an swf-file from the specified images. If repeat is False, 
    the movie is finished with a stop action. Duration may also
    be a list with durations for each frame (note that the duration
    for each frame is always an integer amount of the minimum duration.)
    
    Images should be a list consisting of PIL images or numpy arrays. 
    The latter should be between 0 and 255 for integer types, and 
    between 0 and 1 for float types.
    
    """
    
    # Check Numpy
    if np is None:
        raise RuntimeError("Need Numpy to write an SWF file.")
    
    # Check images (make all Numpy)
    images2 = []
    images = checkImages(images)
    if not images:
        raise ValueError("Image list is empty!")
    for im in images:
        if PIL and isinstance(im, PIL.Image.Image):
            if im.mode == 'P':
                im = im.convert()
            im = np.asarray(im)
            if len(im.shape)==0:
                raise MemoryError("Too little memory to convert PIL image to array")
        images2.append(im)
    
    # Init 
    taglist = [ FileAttributesTag(), SetBackgroundTag(0,0,0) ]
    
    # Check duration
    if hasattr(duration, '__len__'):
        if len(duration) == len(images2):
            duration = [d for d in duration]
        else:
            raise ValueError("len(duration) doesn't match amount of images.")
    else:
        duration = [duration for im in images2]
    
    # Build delays list
    minDuration = float(min(duration))
    delays = [round(d/minDuration) for d in duration]
    delays = [max(1,int(d)) for d in delays]
    
    # Get FPS
    fps = 1.0/minDuration
    
    # Produce series of tags for each image
    t0 = time.time()
    nframes = 0
    for im in images2:
        bm = BitmapTag(im)
        wh = (im.shape[1], im.shape[0])
        sh = ShapeTag(bm.id, (0,0), wh)
        po = PlaceObjectTag(1,sh.id, move=nframes>0)
        taglist.extend( [bm, sh, po] )
        for i in range(delays[nframes]):
            taglist.append( ShowFrameTag() )
        nframes += 1
        
    if not repeat:
        taglist.append(DoActionTag('stop'))
    
    # Build file
    t1 = time.time()
    fp = open(filename,'wb')    
    try:
        buildFile(fp, taglist, nframes=nframes, framesize=wh, fps=fps)
    except Exception:
        raise
    finally:
        fp.close()
    t2 =  time.time()
    
    #print("Writing SWF took %1.2f and %1.2f seconds" % (t1-t0, t2-t1) )
    

def _readPixels(bb, i, tagType, L1):
    """ With pf's seed after the recordheader, reads the pixeldata.
    """
    
    # Check Numpy
    if np is None:
        raise RuntimeError("Need Numpy to read an SWF file.")
        
    # Get info
    charId = bb[i:i+2]; i+=2
    format = ord(bb[i:i+1]); i+=1
    width = bitsToInt( bb[i:i+2], 16 ); i+=2
    height = bitsToInt( bb[i:i+2], 16 ); i+=2
    
    # If we can, get pixeldata and make nunmpy array
    if format != 5:
        print("Can only read 24bit or 32bit RGB(A) lossless images.")
    else:
        # Read byte data
        offset = 2+1+2+2 # all the info bits
        bb2 = bb[i:i+(L1-offset)]
        
        # Decompress and make numpy array
        data = zlib.decompress(bb2)
        a = np.frombuffer(data, dtype=np.uint8)
        
        # Set shape
        if tagType == 20:
            # DefineBitsLossless - RGB data
            try:
                a.shape = height, width, 3
            except Exception:
                # Byte align stuff might cause troubles
                print("Cannot read image due to byte alignment")
        if tagType == 36:
            # DefineBitsLossless2 - ARGB data
            a.shape = height, width, 4
            # Swap alpha channel to make RGBA
            b = a
            a = np.zeros_like(a)
            a[:,:,0] = b[:,:,1]
            a[:,:,1] = b[:,:,2]
            a[:,:,2] = b[:,:,3]
            a[:,:,3] = b[:,:,0]
            
        return a


def readSwf(filename, asNumpy=True):
    """ readSwf(filename, asNumpy=True)
    
    Read all images from an SWF (shockwave flash) file. Returns a list 
    of numpy arrays, or, if asNumpy is false, a list if PIL images.
    
    Limitation: only read the PNG encoded images (not the JPG encoded ones).
    
    """
    
    # Check whether it exists
    if not os.path.isfile(filename):
        raise IOError('File not found: '+str(filename))
    
    # Check PIL
    if (not asNumpy) and (PIL is None):
        raise RuntimeError("Need PIL to return as PIL images.")
    
    # Check Numpy
    if np is None:
        raise RuntimeError("Need Numpy to read SWF files.")
    
    # Init images
    images = []
    
    # Open file and read all
    fp = open(filename, 'rb')
    bb = fp.read()
    
    try:
        # Check opening tag
        tmp = bb[0:3].decode('ascii', 'ignore')
        if tmp.upper() == 'FWS':
            pass # ok
        elif tmp.upper() == 'CWS':
            # Decompress movie
            bb = bb[:8] + zlib.decompress(bb[8:])
        else:
            raise IOError('Not a valid SWF file: ' + str(filename))
        
        # Set filepointer at first tag (skipping framesize RECT and two uin16's
        i = 8
        nbits = bitsToInt(bb[i:i+1], 5) # skip FrameSize    
        nbits = 5 + nbits * 4
        Lrect = nbits / 8.0
        if Lrect%1:
            Lrect += 1    
        Lrect = int(Lrect)
        i += Lrect+4
        
        # Iterate over the tags
        counter = 0
        while True:
            counter += 1
            
            # Get tag header
            head = bb[i:i+6]
            if not head:
                break # Done (we missed end tag)
            
            # Determine type and length
            T, L1, L2 = getTypeAndLen( head )
            if not L2:
                print('Invalid tag length, could not proceed')
                break
            #print(T, L2)
            
            # Read image if we can
            if T in [20, 36]:
                im = _readPixels(bb, i+6, T, L1)
                if im is not None:
                    images.append(im)
            elif T in [6, 21, 35, 90]:
                print('Ignoring JPEG image: cannot read JPEG.')
            else:
                pass # Not an image tag 
            
            # Detect end tag
            if T==0:
                break
            
            # Next tag!
            i += L2
    
    finally:
        fp.close()
    
    # Convert to normal PIL images if needed
    if not asNumpy:
        images2 = images
        images = []
        for im in images2:            
            images.append( PIL.Image.fromarray(im) )
    
    # Done
    return images
