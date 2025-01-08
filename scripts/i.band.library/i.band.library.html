<h2>DESCRIPTION</h2>

<em>i.band.library</em> prints available band information of multispectral data
defined by GRASS GIS. The following multispectral sensors are supported
by default (other band reference registry files can be added, see below):

<p>
Generic multispectral system:
<ul>
<li>gen_r Visible red</li>
<li>gen_g Visible green</li>
<li>gen_b Visible blue</li>
<li>gen_pan Visible panchromatic</li>
<li>gen_nir Near infrared</li>
</ul>
<p>
Landsat-5:
<ul>
<li>L5_1 Visible (Blue)</li>
<li>L5_2 Visible (Green)</li>
<li>L5_3 Visible (Red)</li>
<li>L5_4 Near-Infrared</li>
<li>L5_5 Short-wave infrared</li>
<li>L5_6 Thermal</li>
<li>L5_7 Short-wave infrared</li>
</ul>
<p>
Landsat-7:
<ul>
<li>L7_1 Visible (Blue)</li>
<li>L7_2 Visible (Green)</li>
<li>L7_3 Visible (Red)</li>
<li>L7_4 Near-Infrared</li>
<li>L7_5 Near-Infrared</li>
<li>L7_6 Thermal</li>
<li>L7_7 Mid-Infrared</li>
<li>L7_8 Panchromatic</li>
</ul>
<p>
Landsat-8:
<ul>
<li>L8_1 Visible (Coastal/Aerosol)</li>
<li>L8_2 Visible (Blue)</li>
<li>L8_3 Visible (Green)</li>
<li>L8_4 Visible (Red)</li>
<li>L8_5 Near-Infrared</li>
<li>L8_6 SWIR 1</li>
<li>L8_7 SWIR 2</li>
<li>L8_8 Panchromatic</li>
<li>L8_9 Cirrus</li>
<li>L8_10 TIRS 1</li>
<li>L8_11 TIRS 1</li>
</ul>
<p>
Sentinel-2:
<ul>
<li>S2_1 Visible (Coastal/Aerosol)</li>
<li>S2_2 Visible (Blue)</li>
<li>S2_3 Visible (Green)</li>
<li>S2_4 Visible (Red)</li>
<li>S2_5 Vegetation Red Edge 1</li>
<li>S2_6 Vegetation Red Edge 2</li>
<li>S2_7 Vegetation Red Edge 3</li>
<li>S2_8 Near-Infrared</li>
<li>S2_8A Narrow Near-Infrared</li>
<li>S2_9 Water vapour</li>
<li>S2_10 SWIR - Cirrus</li>
<li>S2_11 SWIR 1</li>
<li>S2_12 SWIR 2</li>
</ul>

<p>
Band references to be printed can be filtered by a search pattern (or
fully defined band reference identifier) which can be specified
by <b>pattern</b> option. For pattern syntax
see <a href="https://docs.python.org/3/library/re.html#regular-expression-syntax">Python
regular expression operations</a> documentation. By
default, <em>i.band.library</em> prints all available band references.

<p>
Extended metadata (central wavelength, spatial resolution, etc.) is printed
only when the <b>-e</b> flag is given.

<h2>Band reference and semantic label relation</h2>

<p>
Band references are a special case (a subset) of semantic labels.
Any string can be a semantic label but strings identifying specific
remote sensing platform bands (=band references)
can have additional metadata managed by <em>i.band.library</em>.
Specific band reference can be assigned to a raster map as a semantic
label by <em><a href="r.semantic.label.html">r.semantic.label</a></em>
or <em><a href="r.support.html">r.support</a></em> modules.

<h2>NOTES</h2>

<p>
Semantic label concept is supported by temporal GRASS modules, see
<em><a href="t.register.html#support-for-semantic-labels">t.register</a></em>,
<em><a href="t.rast.list.html#filtering-the-result-by-semantic-label">t.rast.list</a></em>,
<em><a href="t.info.html#space-time-dataset-with-semantic-labels-assigned">t.info</a></em>
and <em><a href="t.rast.mapcalc.html#semantic-label-filtering">t.rast.mapcalc</a></em>
modules for examples.

<h3>Image collections</h3>

Image collections are the common data type to reference time series of
multi band data. It is used in many frameworks
(see <a href="https://developers.google.com/earth-engine/tutorial_api_04">Google
Earth Engine API</a> for example) to address multi spectral satellite
images series. GRASS supports a multi-band raster layer approach
basically with the imagery group concept
(<em><a href="i.group.html">i.group</a></em>). A new semantic label
concept is designed in order to support image collections in GRASS
GIS.

<h3>Band reference registry files</h3>

Band reference information is stored in JSON files with a pre-defined
internal data structure. A minimalistic example is shown below.

<div class="code"><pre>
{
    "Sentinel2": {
        "description": "The Sentinel-2 A/B bands",
        "shortcut": "S2",
        "instruments": "MultiSpectral Instrument (MSI) optical and infrared",
        "launched": "23 June 2015 (A); 07 March 2017 (B)",
        "source": "https://sentinel.esa.int/web/sentinel/missions/sentinel-2",
        "bands": {
            "1": {
                "Sentinel 2A" : {
                    "central wavelength (nm)": 443.9,
                    "bandwidth (nm)": 27
                },
                "Sentinel 2B" : {
                    "central wavelength (nm)": 442.3,
                    "bandwidth (nm)": 45
                },
                "spatial resolution (meters)": 60,
                "tag": "Visible (Coastal/Aerosol)"
            },
            "2": {
                "Sentinel 2A" : {
                    "central wavelength (nm)": 496.6,
                    "bandwidth (nm)": 98
                },
                "Sentinel 2B" : {
                    "central wavelength (nm)": 492.1,
                    "bandwidth (nm)": 98
                },
                "spatial resolution (meters)": 10,
                "tag": "Visible (Blue)"
            }
        }
    }
}
</pre></div>

Each series starts with an unique identifier (&quot;Sentinel2&quot;
in example above). Required attributes are only two:
a <code>shortcut</code> and <code>bands</code>. Note that a <i>shortcut</i>
must be unique in all band reference registry files. Number of other
attributes is not defined or even limited (in example above
only <code>description</code> and <code>instruments</code> attributes are
defined). List of bands is defined by a <code>bands</code> attribute. Each
band is defined by an identifier ("1", "2" in example above). List of
attributes describing each band is not pre-defined or limited. In
example above each band is described by a <code>central wavelength
(nm)</code>, <code>bandwidth (nm)</code>, and a <code>tag</code>.

<p>
Band reference identifier defined by <b>pattern</b> option is given by
a <i>shortcut</i> in order to print band reference information for
whole series or by specific <i>shortcut</i>_<i>band</i>
identifier.

<p>
System-defined registry files are located in GRASS GIS installation
directory (<code>$GISBASE/etc/i.band.library</code>). Note that
currently <i>i.band.library</i> allows managing only system-defined registry
files. Support for user-defined registry files is planned to be
implemented, see <a href="#known-issues">KNOWN ISSUES</a> section for
details.

<h2>EXAMPLES</h2>

<h3>Print all available band references</h3>

<div class="code"><pre>
i.band.library

S2_1 Visible (Coastal/Aerosol)
S2_2 Visible (Blue)
...
L7_1 Visible (Blue)
L7_2 Visible (Green)
...
L8_1 Visible (Coastal/Aerosol)
L8_2 Visible (Blue)
...
</pre></div>

The module prints band reference and related tag if defined.

<h3>Filter band references by a shortcut</h3>

Only band identifiers related to Sentinel-2 satellite will be
printed.

<div class="code"><pre>
i.band.library pattern=S2

S2_1 Visible (Coastal/Aerosol)
...
S2_12 SWIR 2
</pre></div>

<h3>Filter band references by a regular expression</h3>

Print all available 2nd bands:

<div class="code"><pre>
i.band.library pattern=.*_2

S2_2 Visible (Blue)
L7_2 Visible (Green)
L8_2 Visible (Blue)
...
</pre></div>

<h3>Print extended metadata for specified band identifier</h3>

Extended metadata related to the first band of Sentinel-2 satellite
will be printed.

<div class="code"><pre>
i.band.library -e pattern=S2_1

description: The Sentinel-2 A/B bands
shortcut: S2
instruments: MultiSpectral Instrument (MSI) optical and infrared
launched: 23 June 2015 (A); 07 March 2017 (B)
source: https://sentinel.esa.int/web/sentinel/missions/sentinel-2
    band: 1
        Sentinel 2A:
                central wavelength (nm): 443.9
                bandwidth (nm): 27
        Sentinel 2B:
                central wavelength (nm): 442.3
                bandwidth (nm): 45
        spatial resolution (meters): 60
        tag: Visible (Coastal/Aerosol)
</pre></div>

<h2>KNOWN ISSUES</h2>

<em>i.band.library</em> has currently <b>very limited functionality</b>. Only
system-defined band references are supported. The final implementation
will support managing (add, modify, delete) user-defined band
references.

<p>
Only very limited number of band references is currently
defined, namely Sentinel-2, Landsat7, and Landsat8 satellites. This
will be improved in the near future.

<h2>REFERENCES</h2>

<ul>
  <li><a href="https://developers.google.com/earth-engine/tutorial_api_04">Google Earth Engine API</a></li>
</ul>

<h2>SEE ALSO</h2>

<em>
  <a href="r.semantic.label.html">r.semantic.label</a>,
  <a href="r.info.html">r.info</a>
</em>

<h2>AUTHORS</h2>

Martin Landa<br>
Development sponsored by <a href="https://www.mundialis.de/en">mundialis
GmbH &amp; Co. KG</a> (for the <a href="https://openeo.org">openEO</a>
EU H2020 grant 776242)
