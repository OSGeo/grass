############################################################################
#
# MODULE:       r.in.wms / wms_gdal
#
# AUTHOR(S):    Martin Landa <landa.martin gmail.com>
#
# PURPOSE:      To import data from web mapping servers
#
# COPYRIGHT:    (C) 2009 Martin Landa, and GRASS development team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

import os
import subprocess

def checkGdalWms():
    """Check if GDAL WMS driver is available"""
    ps = subprocess.Popen(['gdalinfo',
                            '--formats'], stdout = subprocess.PIPE)
    
    for line in ps.communicate()[0].splitlines():
        driver, desc = line.split(':')
        if 'WMS' in driver:
            return True
    
    return False

class GdalWms:
    def __init__(self, options, request):
        self.options = options

        # service description XML file path
        self.descFile = None
        self.__createDescFile(request)
        
    def __createDescFile(self, request):
        """Create local service description XML file"""
        # first create directory where to store XML file
        dir = os.path.join(self.options['folder'], self.options['output'])
        if not os.path.exists(self.options['folder']):
            os.mkdir(self.options['folder'])
        if not os.path.exists(dir):
            os.mkdir(dir)

        self.descFile = os.path.join(dir, self.options['output'] + '.xml')
        file = open(self.descFile, 'w')
        try:
            indent = 0
            file.write('<GDAL_WMS>\n')
            indent += 4
            file.write('%s<Service name="WMS">\n' % (' ' * indent))
            indent += 4
            if self.options['wmsquery'] and 'version=' in self.options['wmsquery']:
                for item in self.options['wmsquery'].split(';'):
                    key, value = item.split('=')
                    if key == 'version':
                        file.write('%s<Version>%s</Version>\n' % (' ' * indent, value))
                        break
            else:
                file.write('%s<Version>1.1.1</Version>\n' % (' ' * indent)) # -> default 1.1.1
            file.write('%s<ServerURL>%s?</ServerURL>\n' % (' ' * indent, self.options['mapserver']))
            file.write('%s<SRS>%s</SRS>\n' % (' ' * indent, self.options['srs']))
            file.write('%s<ImageFormat>image/%s</ImageFormat>\n' % (' ' * indent, self.options['format']))
            file.write('%s<Layers>%s</Layers>\n' % (' ' * indent, self.options['layers']))
            file.write('%s<Styles>%s</Styles>\n' % (' ' * indent, self.options['styles']))
            indent -= 4
            file.write('%s</Service>\n' % (' ' * indent))
            file.write('%s<DataWindow>\n' % (' ' * indent))
            indent += 4
            file.write('%s<UpperLeftX>%s</UpperLeftX>\n' % (' ' * indent, request.Get('w')))
            file.write('%s<UpperLeftY>%s</UpperLeftY>\n' % (' ' * indent, request.Get('n')))
            file.write('%s<LowerRightX>%s</LowerRightX>\n' % (' ' * indent, request.Get('e')))
            file.write('%s<LowerRightY>%s</LowerRightY>\n' % (' ' * indent, request.Get('s')))
            file.write('%s<SizeX>%s</SizeX>\n' % (' ' * indent, request.Get('width'))) #
            file.write('%s<SizeY>%s</SizeY>\n' % (' ' * indent, request.Get('height'))) #
            indent -= 4
            file.write('%s</DataWindow>\n' % (' ' * indent))
            file.write('%s<Projection>%s</Projection>\n' % (' ' * indent, self.options['srs']))
            file.write('%s<BandsCount>3</BandsCount>\n' % (' ' * indent))
            file.write('%s<BlockSizeX>%s</BlockSizeX>\n' % (' ' * indent, self.options['maxcols']))
            file.write('%s<BlockSizeY>%s</BlockSizeY>\n' % (' ' * indent, self.options['maxrows']))
            file.write('</GDAL_WMS>\n')
        finally:
            file.close()

    def GetFile(self):
        """Get path of service description XML file"""
        return self.descFile

