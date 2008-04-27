test_config = """
<qgisgrass name="Test config">
<modules>
  <section label="First Section">
    <grass name="g.gisenv"/>
    <grass name="d.vect"/>
  </section>
  <section label="Second Section">
    <grass name="r.to.vect.line"/>
    <grass name="r.to.vect.area"/>
  </section>
  <grass name="v.buffer"/>
</modules>
</qgisgrass>
"""
test_modules = {"g.gisenv":"""
<qgisgrassmodule label="GRASS enviroment variables" module="g.gisenv">
</qgisgrassmodule>
""","r.to.vect.area":"""
<qgisgrassmodule label="Convert a raster to vector areas" module="r.to.vect">
	<option key="input" />
	<option key="output" />
	<option key="feature" answer="area" hidden="yes" />
	<flag key="s" answer="on" hidden="yes" />
</qgisgrassmodule>
""","r.to.vect.line":"""
<qgisgrassmodule label="Convert a raster to vector areas" module="r.to.vect">
	<option key="input" />
	<option key="output" />
	<option key="feature" answer="line" hidden="yes" />
	<flag key="s" answer="on" hidden="yes" />
</qgisgrassmodule>
""","v.surf.idw":"""
<qgisgrassmodule label="Interpolate attribute values (IDW)" module="v.surf.idw">
        <option key="input" layeroption="layer" typemask="point,line" id="input" />
        <field key="column" layerid="input" type="integer,double" label="Attribute field (interpolated values)" />
        <option key="npoints" />
        <option key="output" />
</qgisgrassmodule>
"""
}
import xml.dom.minidom
import menuform

class handleQgisGrass:
    
    def __init__(self, qgisgrass):
        modules = qgisgrass.childNodes[0].GetElementsByTagName('modules')[0]
        for child in modules.childNodes:
            if child.localName == 'grass':
                self.handleGrass( child )
            elif child.localName == 'section':
                self.handleSection( child )

    def handleSection( self,section ):
        for child in section.GetElementsByTagName('grass'):
            self.handleGrass( child )

    def handleGrass( self, grass ):
        raise NotImplementedError

class printQgisGrass( handleQgisGrass ):
    def __init__(self, qgisgrass):
        print "in qgisgrass"
        handleQgisGrass.__init__( self, qgisgrass )

    def handleSection( self,section ):
        print "Section:",section.getAttribute('label')
        handleQgisGrass.handleSection( self, section )

    def handleGrass( self, grass ):
        print "Command:",grass.getAttribute('name')


class wxQgisGrass( handleQgisGrass ):
    pass


class handleQgisGrassModule:

    def __init__( self, qgisgrassmodule):
        qgisgrassm = qgisgrassmodule.GetElementsByTagName( "qgisgrassmodule" )[0]
        self.handleAttributes( qgisgrassm )
        for inner in qgisgrassm.childNodes:
            it = inner.localName
            if it == 'option':
                self.handleOption( inner )
            elif it == 'flag':
                self.handleFlag( inner )
            elif it is not None:
                self.handleOther( inner )

    def handleAttributes( self, node ):
        for (l,a) in node.attributes.items():
            self.handleAttribute( l, a, node )

    def handleAttribute( self, label, value, parent ):
        raise NotImplementedError
        
    def handleOption( self, option ):
        self.handleAttributes( option )

    def handleFlag( self, flag ):
        self.handleAttributes( flag )

    def handleOther( self, other ):
        self.handleAttributes( other )

class printQgisGrassModule( handleQgisGrassModule ):

    def __init__( self, qgisgrassmodule):
        self.handleOption = self.printHandler
        self.handleFlag = self.printHandler
        self.handleOther = self.printHandler
        print "in qgisgrassmodule"
        handleQgisGrassModule.__init__( self, qgisgrassmodule )

    def printHandler( self, opt ):
        print opt.localName
        handleQgisGrassModule.handleOther( self, opt )
        print

    def handleAttribute( self, label, value, option ):
        print "%s:%s" % (label, value)

class wxQgisGrassModule( handleQgisGrassModule ):
    def __init__(self, qgisgrassmodule, label='' ):
        """qgisGrassModule is a string containing the .qgm xml file"""
        self.task = None
        self.label = label
        self.description = ''
        handleQgisGrassModule.__init__( self, qgisgrassmodule )
        self.task.description = self.description
        menuform.GrassGUIApp( self.task ).MainLoop()

    def handleOption( self, opt, getit = menuform.grassTask.get_param, **other ):
        a = dict(opt.attributes.items())
        p = getit( self.task, a['key'] )
        # visibility:
        p['guisection'] = _( 'Main' ) # this should be the only tab present
        p['hidden'] = 'no' # unhide params ...
        if a.get('hidden','no') == 'yes': p['hidden'] = 'yes' # ...except when explicitly hidden
        # overrides:
        if a.has_key( 'answer' ): p['value'] = a['answer']
        if a.has_key( 'label' ): p['description'] = a['label']
        for (k,i) in other.items():
            p[k] = i
        # exclusions:
        if a.has_key('exclude'):
            vals = p['value'].split(',')
            exclusions = a['exclude'].split( ',' )
            for excluded in exclusions:
                if excluded in vals:
                    idx = vals.index(excluded)
                    vals[ idx:idx+1 ] = []
            p['value'] = ','.join( vals )
            if p.get('default','') in exclusions: p['default'] = ''

    def handleOther( self, opt ):
        tag = opt.localName
        att = dict( opt.attributes.items() )
        if tag == 'field':
            pass
        elif tag == 'file':
            filters = dict(opt.attributes.items()).get('filters','(*.*)')
            try:
                glob = re.match(r'\((.+)\)').groups(0)
            except KeyError:
                glob =  '*.*'
            self.handleOption( opt, gisprompt=True, element='file', filemask=glob  )
        elif tag == 'selection':
            pass
        

    def handleFlag( self, flag ):
        self.handleOption( flag, getit = menuform.grassTask.get_flag )

    def handleAttribute( self, label, value, option ):
        if option.localName == 'qgisgrassmodule':
            if label=='module':
                self.task = menuform.grassTask( grassModule = value )
                for pf in self.task.params + self.task.flags:
                    pf['hidden'] = 'yes' # hide eveything initially
            if label=='label':
                self.description = value

from sys import argv

if __name__ ==  '__main__':
    if len( argv ) != 2:
        print "Usage: %s <toolbox command>" % sys.argv[0]
    else:
        the_module =  argv[1]
        if the_module != 'test':
            qgm = open( the_module ).read()
            x = wxQgisGrassModule( xml.dom.minidom.parseString( qgm ), label = the_module )
        else:
            # self test
            config = xml.dom.minidom.parseString( test_config )
            printQgisGrass( config )
            print
            for m in test_modules.keys():
                print m
                module = xml.dom.minidom.parseString( test_modules[m] )
                printQgisGrassModule( module )
            print "----------------"
            m = "r.to.vect.area"
            x = wxQgisGrassModule( xml.dom.minidom.parseString( test_modules[ m ] ), label = m )
        
