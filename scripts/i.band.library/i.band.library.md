## DESCRIPTION

*i.band.library* prints available band information of multispectral data
defined by GRASS GIS. The following multispectral sensors are supported
by default (other band reference registry files can be added, see
below):

Generic multispectral system:

- gen_r Visible red
- gen_g Visible green
- gen_b Visible blue
- gen_pan Visible panchromatic
- gen_nir Near infrared

Landsat-5:

- L5_1 Visible (Blue)
- L5_2 Visible (Green)
- L5_3 Visible (Red)
- L5_4 Near-Infrared
- L5_5 Short-wave infrared
- L5_6 Thermal
- L5_7 Short-wave infrared

Landsat-7:

- L7_1 Visible (Blue)
- L7_2 Visible (Green)
- L7_3 Visible (Red)
- L7_4 Near-Infrared
- L7_5 Near-Infrared
- L7_6 Thermal
- L7_7 Mid-Infrared
- L7_8 Panchromatic

Landsat-8:

- L8_1 Visible (Coastal/Aerosol)
- L8_2 Visible (Blue)
- L8_3 Visible (Green)
- L8_4 Visible (Red)
- L8_5 Near-Infrared
- L8_6 SWIR 1
- L8_7 SWIR 2
- L8_8 Panchromatic
- L8_9 Cirrus
- L8_10 TIRS 1
- L8_11 TIRS 1

Sentinel-2:

- S2_1 Visible (Coastal/Aerosol)
- S2_2 Visible (Blue)
- S2_3 Visible (Green)
- S2_4 Visible (Red)
- S2_5 Vegetation Red Edge 1
- S2_6 Vegetation Red Edge 2
- S2_7 Vegetation Red Edge 3
- S2_8 Near-Infrared
- S2_8A Narrow Near-Infrared
- S2_9 Water vapour
- S2_10 SWIR - Cirrus
- S2_11 SWIR 1
- S2_12 SWIR 2

Band references to be printed can be filtered by a search pattern (or
fully defined band reference identifier) which can be specified by
**pattern** option. For pattern syntax see [Python regular expression
operations](https://docs.python.org/3/library/re.html#regular-expression-syntax)
documentation. By default, *i.band.library* prints all available band
references.

Extended metadata (central wavelength, spatial resolution, etc.) is
printed only when the **-e** flag is given.

## Band reference and semantic label relation

Band references are a special case (a subset) of semantic labels. Any
string can be a semantic label but strings identifying specific remote
sensing platform bands (=band references) can have additional metadata
managed by *i.band.library*. Specific band reference can be assigned to
a raster map as a semantic label by
*[r.semantic.label](r.semantic.label.md)* or *[r.support](r.support.md)*
modules.

## NOTES

Semantic label concept is supported by temporal GRASS modules, see
*[t.register](t.register.md#support-for-semantic-labels)*,
*[t.rast.list](t.rast.list.md#filtering-the-result-by-semantic-label)*,
*[t.info](t.info.md#space-time-dataset-with-semantic-labels-assigned)*
and *[t.rast.mapcalc](t.rast.mapcalc.md#semantic-label-filtering)*
modules for examples.

### Image collections

Image collections are the common data type to reference time series of
multi band data. It is used in many frameworks (see [Google Earth Engine
API](https://developers.google.com/earth-engine/tutorial_api_04) for
example) to address multi spectral satellite images series. GRASS
supports a multi-band raster layer approach basically with the imagery
group concept (*[i.group](i.group.md)*). A new semantic label concept is
designed in order to support image collections in GRASS GIS.

### Band reference registry files

Band reference information is stored in JSON files with a pre-defined
internal data structure. A minimalistic example is shown below.

```json
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
```

Each series starts with an unique identifier ("Sentinel2" in example
above). Required attributes are only two: a `shortcut` and `bands`. Note
that a *shortcut* must be unique in all band reference registry files.
Number of other attributes is not defined or even limited (in example
above only `description` and `instruments` attributes are defined). List
of bands is defined by a `bands` attribute. Each band is defined by an
identifier ("1", "2" in example above). List of attributes describing
each band is not pre-defined or limited. In example above each band is
described by a `central wavelength (nm)`, `bandwidth (nm)`, and a `tag`.

Band reference identifier defined by **pattern** option is given by a
*shortcut* in order to print band reference information for whole series
or by specific *shortcut*\_*band* identifier.

System-defined registry files are located in GRASS GIS installation
directory (`$GISBASE/etc/i.band.library`). Note that currently
*i.band.library* allows managing only system-defined registry files.
Support for user-defined registry files is planned to be implemented,
see [KNOWN ISSUES](#known-issues) section for details.

## EXAMPLES

### Print all available band references

```sh
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
```

The module prints band reference and related tag if defined.

### Filter band references by a shortcut

Only band identifiers related to Sentinel-2 satellite will be printed.

```sh
i.band.library pattern=S2

S2_1 Visible (Coastal/Aerosol)
...
S2_12 SWIR 2
```

### Filter band references by a regular expression

Print all available 2nd bands:

```sh
i.band.library pattern=.*_2

S2_2 Visible (Blue)
L7_2 Visible (Green)
L8_2 Visible (Blue)
...
```

### Print extended metadata for specified band identifier

Extended metadata related to the first band of Sentinel-2 satellite will
be printed.

```sh
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
```

## KNOWN ISSUES

*i.band.library* has currently **very limited functionality**. Only
system-defined band references are supported. The final implementation
will support managing (add, modify, delete) user-defined band
references.

Only very limited number of band references is currently defined, namely
Sentinel-2, Landsat7, and Landsat8 satellites. This will be improved in
the near future.

## REFERENCES

- [Google Earth Engine
  API](https://developers.google.com/earth-engine/tutorial_api_04)

## SEE ALSO

*[r.semantic.label](r.semantic.label.md), [r.info](r.info.md)*

## AUTHORS

Martin Landa  
Development sponsored by [mundialis GmbH & Co.
KG](https://www.mundialis.de/en) (for the [openEO](https://openeo.org)
EU H2020 grant 776242)
