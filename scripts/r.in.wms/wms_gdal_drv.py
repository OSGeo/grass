"""!
@brief GDAL WMS driver.

List of classes:
 - wms_drv::NullDevice
 - wms_drv::WMSGdalDrv

(C) 2012-2021 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (Mentor: Martin Landa)
"""

import grass.script as gs

try:
    from osgeo import gdal

    gdal.DontUseExceptions()
except ImportError:
    gs.fatal(
        _(
            "Unable to load GDAL Python bindings (requires package 'python-gdal' being "
            "installed)"
        )
    )

import xml.etree.ElementTree as ET

from wms_base import WMSBase, GetSRSParamVal


class NullDevice:
    def write(self, s):
        pass


class WMSGdalDrv(WMSBase):
    def __init__(self, createopt):
        super().__init__()
        self.proxy = None
        self.proxy_user_pw = None
        self.createopt = createopt

    def setProxy(self, proxy, proxy_user_pw=None):
        """Set the HTTP proxy and its user and password

        @input proxy HTTP proxy with [IP address]:[port]
        @input proxy_user_pw with [user name]:[password]
        """
        self.proxy = proxy
        self.proxy_user_pw = proxy_user_pw

    def _createXML(self):
        """!Create XML for GDAL WMS driver

        @return path to XML file
        """
        self._debug("_createXML", "started")

        gdal_wms = ET.Element("GDAL_WMS")
        service = ET.SubElement(gdal_wms, "Service")
        service.set("name", "WMS")

        version = ET.SubElement(service, "Version")
        version.text = self.params["wms_version"]

        server_url = ET.SubElement(service, "ServerUrl")
        server_url.text = self.params["url"]

        srs = ET.SubElement(service, self.params["proj_name"])
        srs.text = GetSRSParamVal(self.params["srs"])

        image_format = ET.SubElement(service, "ImageFormat")
        image_format.text = self.params["format"]

        image_format = ET.SubElement(service, "Transparent")
        image_format.text = self.params["transparent"]

        layers = ET.SubElement(service, "Layers")
        layers.text = self.params["layers"]

        styles = ET.SubElement(service, "Styles")
        styles.text = self.params["styles"]

        data_window = ET.SubElement(gdal_wms, "DataWindow")

        upper_left_x = ET.SubElement(data_window, "UpperLeftX")
        upper_left_x.text = str(self.bbox["minx"])

        upper_left_y = ET.SubElement(data_window, "UpperLeftY")
        upper_left_y.text = str(self.bbox["maxy"])

        lower_right_x = ET.SubElement(data_window, "LowerRightX")
        lower_right_x.text = str(self.bbox["maxx"])

        lower_right_y = ET.SubElement(data_window, "LowerRightY")
        lower_right_y.text = str(self.bbox["miny"])

        size_x = ET.SubElement(data_window, "SizeX")
        size_x.text = str(self.region["cols"])

        size_y = ET.SubElement(data_window, "SizeY")
        size_y.text = str(self.region["rows"])

        # RGB + alpha
        self.temp_map_bands_num = 4
        block_size_x = ET.SubElement(gdal_wms, "BandsCount")
        block_size_x.text = str(self.temp_map_bands_num)

        block_size_x = ET.SubElement(gdal_wms, "BlockSizeX")
        block_size_x.text = str(self.tile_size["cols"])

        block_size_y = ET.SubElement(gdal_wms, "BlockSizeY")
        block_size_y.text = str(self.tile_size["rows"])

        if self.params["username"] and self.params["password"]:
            user_password = ET.SubElement(gdal_wms, "UserPwd")
            user_password.text = "%s:%s" % (
                self.params["username"],
                self.params["password"],
            )

        xml_file = self._tempfile()

        ET.ElementTree(gdal_wms).write(xml_file)

        self._debug("_createXML", "finished -> %s" % xml_file)

        return xml_file

    def _download(self):
        """!Downloads data from WMS server using GDAL WMS driver

        @return temp_map with stored downloaded data
        """
        gs.message("Downloading data from WMS server...")

        # GDAL WMS driver does not flip geographic coordinates
        # according to WMS standard 1.3.0.
        if (
            "+proj=latlong" in self.proj_srs or "+proj=longlat" in self.proj_srs
        ) and self.params["wms_version"] == "1.3.0":
            gs.warning(
                _(
                    "If module will not be able to fetch the data in this "
                    "geographic projection, \n try 'WMS_GRASS' driver or use WMS "
                    "version 1.1.1."
                )
            )

        self._debug("_download", "started")
        temp_map = self._tempfile()

        xml_file = self._createXML()

        # print xml file content for debug level 1
        file = open(xml_file)
        gs.debug("WMS request XML:\n%s" % file.read(), 1)
        file.close()

        if self.proxy:
            gdal.SetConfigOption("GDAL_HTTP_PROXY", str(self.proxy))
        if self.proxy_user_pw:
            gdal.SetConfigOption("GDAL_HTTP_PROXYUSERPWD", str(self.proxy_user_pw))
        wms_dataset = gdal.Open(xml_file, gdal.GA_ReadOnly)
        gs.try_remove(xml_file)
        if wms_dataset is None:
            gs.fatal(_("Unable to open GDAL WMS driver"))

        self._debug("_download", "GDAL dataset created")

        driver = gdal.GetDriverByName(self.gdal_drv_format)
        if driver is None:
            gs.fatal(_("Unable to find %s driver") % self.gdal_drv_format)

        metadata = driver.GetMetadata()
        if (
            gdal.DCAP_CREATECOPY not in metadata
            or metadata[gdal.DCAP_CREATECOPY] == "NO"
        ):
            gs.fatal(_("Driver %s supports CreateCopy() method.") % self.gdal_drv_name)

        self._debug("_download", "calling GDAL CreateCopy...")

        if self.createopt is None:
            temp_map_dataset = driver.CreateCopy(temp_map, wms_dataset, 0)
        else:
            self._debug("_download", "Using GDAL createopt <%s>" % str(self.createopt))
            temp_map_dataset = driver.CreateCopy(
                temp_map, wms_dataset, 0, self.createopt
            )

        if temp_map_dataset is None:
            gs.fatal(_("Incorrect WMS query"))

        temp_map_dataset = None
        wms_dataset = None

        self._debug("_download", "finished")

        return temp_map
