############################################################################
#
# MODULE:       r.in.wms / wms_parse
#
# AUTHOR(S):    Cedric Shock, 2006
#               Upgraded for GRASS 7 by Martin Landa <landa.martin gmail.com>, 2009
#
# PURPOSE:      To import data from web mapping servers
#               (based on Bash script by Cedric Shock)
#
# COPYRIGHT:    (C) 2009 Martin Landa, and GRASS development team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

import xml.sax
import xml.sax.handler
HandlerBase=xml.sax.handler.ContentHandler
from xml.sax import make_parser

class ProcessCapFile(HandlerBase):
    """
    A SAX handler for the capabilities file
    """
    def __init__(self):
        self.inTag = {}
        for tag in ('layer', 'name', 'style',
                    'title', 'srs'):
            self.inTag[tag] = False
        self.value = ''
        
        self.layers = []
        
    def startElement(self, name, attrs):
        if self.inTag.has_key(name.lower()):
            self.inTag[name.lower()] = True
        
        if name.lower() == 'layer':
            self.layers.append({})
        
    def endElement(self, name):
        if self.inTag.has_key(name.lower()):
            self.inTag[name.lower()] = False
        
        for tag in ('name', 'title', 'srs'):
            if name.lower() != tag:
                continue
            if self.inTag['style']:
                if not self.layers[-1].has_key('style'):
                    self.layers[-1]['style'] = {}
                if not self.layers[-1]['style'].has_key(tag):
                    self.layers[-1]['style'][tag] = []
                self.layers[-1]['style'][tag].append(self.value)
            elif self.inTag['layer']:
                self.layers[-1][tag] = self.value
            
        if name.lower() in ('name', 'title', 'srs'):
            self.value = ''
        
    def characters(self, ch):
        if self.inTag['name'] or \
                self.inTag['title'] or \
                self.inTag['srs']:
            self.value += ch
        
    def getLayers(self):
        """Print list of layers"""
        for ly in self.layers:
            print "\n\n-=-=-=-=-=-\n"
	    if ly.has_key('name'):
                print "LAYER: " + ly['name']
            else:
                print "LAYER: unknown"
            if ly.has_key('title'):
                print "  Title: " + ly['title']
            if ly.has_key('srs'):
                print "  SRS: " + ly['srs']
            if ly.has_key('style'):
                for idx in range(len(ly['style']['name'])):
                    print "  STYLE: " + ly['style']['name'][idx]
                    print "    Style title: " + ly['style']['title'][idx]
