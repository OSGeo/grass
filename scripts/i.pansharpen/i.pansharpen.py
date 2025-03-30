#!/usr/bin/env python3

############################################################################
#
# MODULE:	    i.pansharpen
#
# AUTHOR(S):    Overall script by Michael Barton (ASU)
#               Brovey transformation in i.fusion.brovey by Markus Neteler <<neteler at osgeo org>>
#               i.fusion brovey converted to Python by Glynn Clements
#               IHS and PCA transformation added by Michael Barton (ASU)
#               histogram matching algorithm by Michael Barton and Luca Delucchi, Fondazione E. Mach (Italy)
#               Thanks to Markus Metz for help with PCA inversion
#               Thanks to Hamish Bowman for parallel processing algorithm
#
# PURPOSE:	Sharpening of 3 RGB channels using a high-resolution panchromatic channel
#
# COPYRIGHT:	(C) 2002-2019 by the GRASS Development Team
#
# 		This program is free software under the GNU General Public
# 		License (>=v2). Read the file COPYING that comes with GRASS
# 		for details.
#
# REFERENCES:
#   Roller, N.E.G. and Cox, S., 1980. Comparison of Landsat MSS and merged MSS/RBV
#   data for analysis of natural vegetation. Proc. of the 14th International
#   Symposium on Remote Sensing of Environment, San Jose, Costa Rica, 23-30 April, pp. 1001-1007.
#
#   Amarsaikhan, D., & Douglas, T. (2004). Data fusion and multisource image classification.
#   International Journal of Remote Sensing, 25(17), 3529-3539.
#
#   Behnia, P. (2005). Comparison between four methods for data fusion of ETM+
#   multispectral and pan images. Geo-spatial Information Science, 8(2), 98-103
#
#   for LANDSAT 5: see Pohl, C 1996 and others
#
#############################################################################

# %Module
# % description: Image fusion algorithms to sharpen multispectral with high-res panchromatic channels
# % keyword: imagery
# % keyword: fusion
# % keyword: sharpen
# % keyword: Brovey
# % keyword: IHS
# % keyword: HIS
# % keyword: PCA
# % overwrite: yes
# %End
# %option G_OPT_R_INPUT
# % key: red
# % description: Name of raster map to be used for <red>
# %end
# %option G_OPT_R_INPUT
# % key: green
# % description: Name of raster map to be used for <green>
# %end
# %option G_OPT_R_INPUT
# % key: blue
# % description: Name of raster map to be used for <blue>
# %end
# % option G_OPT_R_INPUT
# % key: pan
# % description: Name of raster map to be used for high resolution panchromatic channel
# %end
# %option G_OPT_R_BASENAME_OUTPUT
# %end
# %option
# % key: method
# % description: Method for pan sharpening
# % options: brovey,ihs,pca
# % answer: ihs
# % required: yes
# %end
# %option
# % key: bitdepth
# % type: integer
# % description: Bit depth of image (must be in range of 2-30)
# % options: 2-32
# % answer: 8
# % required: yes
# %end
# %flag
# % key: s
# % description: Serial processing rather than parallel processing
# %end
# %flag
# % key: l
# % description: Rebalance blue channel for LANDSAT
# %end
# %flag
# % key: r
# % description: Rescale (stretch) the range of pixel values in each channel to the entire 0-255 8-bit range for processing (see notes)
# %end

import os
from grass.exceptions import CalledModuleError

try:
    import numpy as np

    hasNumPy = True
except ImportError:
    hasNumPy = False

import grass.script as gs


def main():
    if not hasNumPy:
        gs.fatal(_("Required dependency NumPy not found. Exiting."))

    sharpen = options["method"]  # sharpening algorithm
    ms1_orig = options["blue"]  # blue channel
    ms2_orig = options["green"]  # green channel
    ms3_orig = options["red"]  # red channel
    pan_orig = options["pan"]  # high res pan channel
    out = options["output"]  # prefix for output RGB maps
    bits = options["bitdepth"]  # bit depth of image channels
    bladjust = flags["l"]  # adjust blue channel
    sproc = flags["s"]  # serial processing
    rescale = flags["r"]  # rescale to spread pixel values to entire 0-255 range

    # Checking bit depth
    bits = float(bits)
    if bits < 2 or bits > 30:
        gs.warning(_("Bit depth is outside acceptable range"))
        return

    outb = gs.core.find_file("%s_blue" % out)
    outg = gs.core.find_file("%s_green" % out)
    outr = gs.core.find_file("%s_red" % out)

    if (
        outb["name"] != "" or outg["name"] != "" or outr["name"] != ""
    ) and not gs.overwrite():
        gs.warning(
            _(
                "Maps with selected output prefix names already exist."
                " Delete them or use overwrite flag"
            )
        )
        return

    pid = str(os.getpid())

    # convert input image channels to 8 bit for processing
    ms1 = "tmp%s_ms1" % pid
    ms2 = "tmp%s_ms2" % pid
    ms3 = "tmp%s_ms3" % pid
    pan = "tmp%s_pan" % pid

    if not rescale:
        if bits == 8:
            gs.message(_("Using 8bit image channels"))
            if sproc:
                # serial processing
                gs.run_command(
                    "g.copy",
                    raster="%s,%s" % (ms1_orig, ms1),
                    quiet=True,
                    overwrite=True,
                )
                gs.run_command(
                    "g.copy",
                    raster="%s,%s" % (ms2_orig, ms2),
                    quiet=True,
                    overwrite=True,
                )
                gs.run_command(
                    "g.copy",
                    raster="%s,%s" % (ms3_orig, ms3),
                    quiet=True,
                    overwrite=True,
                )
                gs.run_command(
                    "g.copy",
                    raster="%s,%s" % (pan_orig, pan),
                    quiet=True,
                    overwrite=True,
                )
            else:
                # parallel processing
                pb = gs.start_command(
                    "g.copy",
                    raster="%s,%s" % (ms1_orig, ms1),
                    quiet=True,
                    overwrite=True,
                )
                pg = gs.start_command(
                    "g.copy",
                    raster="%s,%s" % (ms2_orig, ms2),
                    quiet=True,
                    overwrite=True,
                )
                pr = gs.start_command(
                    "g.copy",
                    raster="%s,%s" % (ms3_orig, ms3),
                    quiet=True,
                    overwrite=True,
                )
                pp = gs.start_command(
                    "g.copy",
                    raster="%s,%s" % (pan_orig, pan),
                    quiet=True,
                    overwrite=True,
                )

                pb.wait()
                pg.wait()
                pr.wait()
                pp.wait()

        else:
            gs.message(_("Converting image channels to 8bit for processing"))
            maxval = pow(2, bits) - 1
            if sproc:
                # serial processing
                gs.run_command(
                    "r.rescale",
                    input=ms1_orig,
                    from_="0,%f" % maxval,
                    output=ms1,
                    to="0,255",
                    quiet=True,
                    overwrite=True,
                )
                gs.run_command(
                    "r.rescale",
                    input=ms2_orig,
                    from_="0,%f" % maxval,
                    output=ms2,
                    to="0,255",
                    quiet=True,
                    overwrite=True,
                )
                gs.run_command(
                    "r.rescale",
                    input=ms3_orig,
                    from_="0,%f" % maxval,
                    output=ms3,
                    to="0,255",
                    quiet=True,
                    overwrite=True,
                )
                gs.run_command(
                    "r.rescale",
                    input=pan_orig,
                    from_="0,%f" % maxval,
                    output=pan,
                    to="0,255",
                    quiet=True,
                    overwrite=True,
                )

            else:
                # parallel processing
                pb = gs.start_command(
                    "r.rescale",
                    input=ms1_orig,
                    from_="0,%f" % maxval,
                    output=ms1,
                    to="0,255",
                    quiet=True,
                    overwrite=True,
                )
                pg = gs.start_command(
                    "r.rescale",
                    input=ms2_orig,
                    from_="0,%f" % maxval,
                    output=ms2,
                    to="0,255",
                    quiet=True,
                    overwrite=True,
                )
                pr = gs.start_command(
                    "r.rescale",
                    input=ms3_orig,
                    from_="0,%f" % maxval,
                    output=ms3,
                    to="0,255",
                    quiet=True,
                    overwrite=True,
                )
                pp = gs.start_command(
                    "r.rescale",
                    input=pan_orig,
                    from_="0,%f" % maxval,
                    output=pan,
                    to="0,255",
                    quiet=True,
                    overwrite=True,
                )

                pb.wait()
                pg.wait()
                pr.wait()
                pp.wait()

    else:
        gs.message(_("Rescaling image channels to 8bit for processing"))

        min_ms1 = int(gs.raster_info(ms1_orig)["min"])
        max_ms1 = int(gs.raster_info(ms1_orig)["max"])
        min_ms2 = int(gs.raster_info(ms2_orig)["min"])
        max_ms2 = int(gs.raster_info(ms2_orig)["max"])
        min_ms3 = int(gs.raster_info(ms3_orig)["min"])
        max_ms3 = int(gs.raster_info(ms3_orig)["max"])
        min_pan = int(gs.raster_info(pan_orig)["min"])
        max_pan = int(gs.raster_info(pan_orig)["max"])

        maxval = pow(2, bits) - 1
        if sproc:
            # serial processing
            gs.run_command(
                "r.rescale",
                input=ms1_orig,
                from_="%f,%f" % (min_ms1, max_ms1),
                output=ms1,
                to="0,255",
                quiet=True,
                overwrite=True,
            )
            gs.run_command(
                "r.rescale",
                input=ms2_orig,
                from_="%f,%f" % (min_ms2, max_ms2),
                output=ms2,
                to="0,255",
                quiet=True,
                overwrite=True,
            )
            gs.run_command(
                "r.rescale",
                input=ms3_orig,
                from_="%f,%f" % (min_ms3, max_ms3),
                output=ms3,
                to="0,255",
                quiet=True,
                overwrite=True,
            )
            gs.run_command(
                "r.rescale",
                input=pan_orig,
                from_="%f,%f" % (min_pan, max_pan),
                output=pan,
                to="0,255",
                quiet=True,
                overwrite=True,
            )

        else:
            # parallel processing
            pb = gs.start_command(
                "r.rescale",
                input=ms1_orig,
                from_="%f,%f" % (min_ms1, max_ms1),
                output=ms1,
                to="0,255",
                quiet=True,
                overwrite=True,
            )
            pg = gs.start_command(
                "r.rescale",
                input=ms2_orig,
                from_="%f,%f" % (min_ms2, max_ms2),
                output=ms2,
                to="0,255",
                quiet=True,
                overwrite=True,
            )
            pr = gs.start_command(
                "r.rescale",
                input=ms3_orig,
                from_="%f,%f" % (min_ms3, max_ms3),
                output=ms3,
                to="0,255",
                quiet=True,
                overwrite=True,
            )
            pp = gs.start_command(
                "r.rescale",
                input=pan_orig,
                from_="%f,%f" % (min_pan, max_pan),
                output=pan,
                to="0,255",
                quiet=True,
                overwrite=True,
            )

            pb.wait()
            pg.wait()
            pr.wait()
            pp.wait()

    # get PAN resolution:
    kv = gs.raster_info(map=pan)
    nsres = kv["nsres"]
    ewres = kv["ewres"]
    panres = (nsres + ewres) / 2

    # clone current region
    gs.use_temp_region()
    gs.run_command("g.region", res=panres, align=pan)

    # Select sharpening method
    gs.message(_("Performing pan sharpening with hi res pan image: %f") % panres)
    if sharpen == "brovey":
        brovey(pan, ms1, ms2, ms3, out, pid, sproc)
    elif sharpen == "ihs":
        ihs(pan, ms1, ms2, ms3, out, pid, sproc)
    elif sharpen == "pca":
        pca(pan, ms1, ms2, ms3, out, pid, sproc)
    # Could add other sharpening algorithms here, e.g. wavelet transformation

    gs.message(_("Assigning grey equalized color tables to output images..."))

    # equalized grey scales give best contrast
    gs.message(_("setting pan-sharpened channels to equalized grey scale"))
    for ch in ["red", "green", "blue"]:
        gs.run_command(
            "r.colors", quiet=True, map="%s_%s" % (out, ch), flags="e", color="grey"
        )

    # Landsat too blue-ish because panchromatic band less sensitive to blue
    # light, so output blue channed can be modified
    if bladjust:
        gs.message(_("Adjusting blue channel color table..."))
        blue_colors = ["0 0 0 0\n5% 0 0 0\n67% 255 255 255\n100% 255 255 255"]
        # these previous colors are way too blue for landsat
        # blue_colors = ['0 0 0 0\n10% 0 0 0\n20% 200 200 200\n40% 230 230 230\n67%
        # 255 255 255\n100% 255 255 255']
        bc = gs.feed_command("r.colors", quiet=True, map="%s_blue" % out, rules="-")
        bc.stdin.write(gs.encode("\n".join(blue_colors)))
        bc.stdin.close()

    # output notice
    gs.verbose(_("The following pan-sharpened output maps have been generated:"))
    for ch in ["red", "green", "blue"]:
        gs.verbose(_("%s_%s") % (out, ch))

    gs.verbose(_("To visualize output, run: g.region -p raster=%s_red") % out)
    gs.verbose(_("d.rgb r=%s_red g=%s_green b=%s_blue") % (out, out, out))
    gs.verbose(
        _("If desired, combine channels into a single RGB map with 'r.composite'.")
    )
    gs.verbose(_("Channel colors can be rebalanced using i.colors.enhance."))

    # write cmd history:
    for ch in ["red", "green", "blue"]:
        gs.raster_history("%s_%s" % (out, ch))

    # create a group with the three outputs
    # gs.run_command('i.group', group=out,
    #                  input="{n}_red,{n}_blue,{n}_green".format(n=out))

    # Cleanup
    gs.message(_("cleaning up temp files"))
    try:
        gs.run_command(
            "g.remove", flags="f", type="raster", pattern="tmp%s*" % pid, quiet=True
        )
    except CalledModuleError:
        pass


def brovey(pan, ms1, ms2, ms3, out, pid, sproc):
    gs.verbose(_("Using Brovey algorithm"))

    # pan/intensity histogram matching using linear regression
    gs.message(_("Pan channel/intensity histogram matching using linear regression"))
    outname = "tmp%s_pan1" % pid
    panmatch1 = matchhist(pan, ms1, outname)

    outname = "tmp%s_pan2" % pid
    panmatch2 = matchhist(pan, ms2, outname)

    outname = "tmp%s_pan3" % pid
    panmatch3 = matchhist(pan, ms3, outname)

    outr = "%s_red" % out
    outg = "%s_green" % out
    outb = "%s_blue" % out

    # calculate brovey transformation
    gs.message(_("Calculating Brovey transformation..."))

    if sproc:
        # serial processing
        e = """eval(k = "$ms1" + "$ms2" + "$ms3")
            "$outr" = 1 * round("$ms3" * "$panmatch3" / k)
            "$outg" = 1 * round("$ms2" * "$panmatch2" / k)
            "$outb" = 1 * round("$ms1" * "$panmatch1" / k)"""
        gs.mapcalc(
            e,
            outr=outr,
            outg=outg,
            outb=outb,
            panmatch1=panmatch1,
            panmatch2=panmatch2,
            panmatch3=panmatch3,
            ms1=ms1,
            ms2=ms2,
            ms3=ms3,
            overwrite=True,
        )
    else:
        # parallel processing
        pb = gs.mapcalc_start(
            "%s_blue = 1 * round((%s * %s) / (%s + %s + %s))"
            % (out, ms1, panmatch1, ms1, ms2, ms3),
            overwrite=True,
        )
        pg = gs.mapcalc_start(
            "%s_green = 1 * round((%s * %s) / (%s + %s + %s))"
            % (out, ms2, panmatch2, ms1, ms2, ms3),
            overwrite=True,
        )
        pr = gs.mapcalc_start(
            "%s_red = 1 * round((%s * %s) / (%s + %s + %s))"
            % (out, ms3, panmatch3, ms1, ms2, ms3),
            overwrite=True,
        )

        pb.wait(), pg.wait(), pr.wait()
        try:
            pb.terminate(), pg.terminate(), pr.terminate()
        except OSError:
            pass

    # Cleanup
    try:
        gs.run_command(
            "g.remove",
            flags="f",
            quiet=True,
            type="raster",
            name="%s,%s,%s" % (panmatch1, panmatch2, panmatch3),
        )
    except CalledModuleError:
        pass


def ihs(pan, ms1, ms2, ms3, out, pid, sproc):
    gs.verbose(_("Using IHS<->RGB algorithm"))
    # transform RGB channels into IHS color space
    gs.message(_("Transforming to IHS color space..."))
    gs.run_command(
        "i.rgb.his",
        overwrite=True,
        red=ms3,
        green=ms2,
        blue=ms1,
        hue="tmp%s_hue" % pid,
        intensity="tmp%s_int" % pid,
        saturation="tmp%s_sat" % pid,
    )

    # pan/intensity histogram matching using linear regression
    target = "tmp%s_int" % pid
    outname = "tmp%s_pan_int" % pid
    panmatch = matchhist(pan, target, outname)

    # substitute pan for intensity channel and transform back to RGB color space
    gs.message(_("Transforming back to RGB color space and sharpening..."))
    gs.run_command(
        "i.his.rgb",
        overwrite=True,
        hue="tmp%s_hue" % pid,
        intensity="%s" % panmatch,
        saturation="tmp%s_sat" % pid,
        red="%s_red" % out,
        green="%s_green" % out,
        blue="%s_blue" % out,
    )

    # Cleanup
    try:
        gs.run_command("g.remove", flags="f", quiet=True, type="raster", name=panmatch)
    except CalledModuleError:
        pass


def pca(pan, ms1, ms2, ms3, out, pid, sproc):
    gs.verbose(_("Using PCA/inverse PCA algorithm"))
    gs.message(_("Creating PCA images and calculating eigenvectors..."))

    # initial PCA with RGB channels
    pca_out = gs.read_command(
        "i.pca",
        quiet=True,
        rescale="0,0",
        input="%s,%s,%s" % (ms1, ms2, ms3),
        output="tmp%s.pca" % pid,
    )
    if len(pca_out) < 1:
        gs.fatal(_("Input has no data. Check region settings."))

    b1evect = []
    b2evect = []
    b3evect = []
    for line in pca_out.replace("(", ",").replace(")", ",").splitlines():
        b1evect.append(float(line.split(",")[1]))
        b2evect.append(float(line.split(",")[2]))
        b3evect.append(float(line.split(",")[3]))

    # inverse PCA with hi res pan channel substituted for principal component 1
    pca2 = "tmp%s.pca.2" % pid
    pca3 = "tmp%s.pca.3" % pid
    b1evect1 = b1evect[0]
    b1evect2 = b1evect[1]
    b1evect3 = b1evect[2]
    b2evect1 = b2evect[0]
    b2evect2 = b2evect[1]
    b2evect3 = b2evect[2]
    b3evect1 = b3evect[0]
    b3evect2 = b3evect[1]
    b3evect3 = b3evect[2]

    # Histogram matching
    outname = "tmp%s_pan1" % pid
    panmatch1 = matchhist(pan, ms1, outname)

    outname = "tmp%s_pan2" % pid
    panmatch2 = matchhist(pan, ms2, outname)

    outname = "tmp%s_pan3" % pid
    panmatch3 = matchhist(pan, ms3, outname)

    gs.message(_("Performing inverse PCA ..."))

    # Get mean value of each channel
    stats1 = gs.parse_command(
        "r.univar", map=ms1, flags="g", parse=(gs.parse_key_val, {"sep": "="})
    )
    stats2 = gs.parse_command(
        "r.univar", map=ms2, flags="g", parse=(gs.parse_key_val, {"sep": "="})
    )
    stats3 = gs.parse_command(
        "r.univar", map=ms3, flags="g", parse=(gs.parse_key_val, {"sep": "="})
    )

    b1mean = float(stats1["mean"])
    b2mean = float(stats2["mean"])
    b3mean = float(stats3["mean"])

    if sproc:
        # serial processing
        outr = "%s_red" % out
        outg = "%s_green" % out
        outb = "%s_blue" % out

        cmd1 = "$outb = 1 * round(($panmatch1 * $b1evect1) + ($pca2 * $b1evect2) + ($pca3 * $b1evect3) + $b1mean)"  # noqa: E501
        cmd2 = "$outg = 1 * round(($panmatch2 * $b2evect1) + ($pca2 * $b2evect2) + ($pca3 * $b2evect3) + $b2mean)"  # noqa: E501
        cmd3 = "$outr = 1 * round(($panmatch3 * $b3evect1) + ($pca2 * $b3evect2) + ($pca3 * $b3evect3) + $b3mean)"  # noqa: E501

        cmd = f"{cmd1}\n{cmd2}\n{cmd3}"

        gs.mapcalc(
            cmd,
            outb=outb,
            outg=outg,
            outr=outr,
            panmatch1=panmatch1,
            panmatch2=panmatch2,
            panmatch3=panmatch3,
            pca2=pca2,
            pca3=pca3,
            b1evect1=b1evect1,
            b2evect1=b2evect1,
            b3evect1=b3evect1,
            b1evect2=b1evect2,
            b2evect2=b2evect2,
            b3evect2=b3evect2,
            b1evect3=b1evect3,
            b2evect3=b2evect3,
            b3evect3=b3evect3,
            b1mean=b1mean,
            b2mean=b2mean,
            b3mean=b3mean,
            overwrite=True,
        )
    else:
        # parallel processing
        pb = gs.mapcalc_start(
            "%s_blue = 1 * round((%s * %f) + (%s * %f) + (%s * %f) + %f)"
            % (out, panmatch1, b1evect1, pca2, b1evect2, pca3, b1evect3, b1mean),
            overwrite=True,
        )

        pg = gs.mapcalc_start(
            "%s_green = 1 * round((%s * %f) + (%s * %f) + (%s * %f) + %f)"
            % (out, panmatch2, b2evect1, pca2, b2evect2, pca3, b2evect3, b2mean),
            overwrite=True,
        )

        pr = gs.mapcalc_start(
            "%s_red = 1 * round((%s * %f) + (%s * %f) + (%s * %f) + %f)"
            % (out, panmatch3, b3evect1, pca2, b3evect2, pca3, b3evect3, b3mean),
            overwrite=True,
        )

        pb.wait(), pg.wait(), pr.wait()
        try:
            pb.terminate(), pg.terminate(), pr.terminate()
        except OSError:
            pass

    # Cleanup
    gs.run_command(
        "g.remove",
        flags="f",
        quiet=True,
        type="raster",
        name="%s,%s,%s" % (panmatch1, panmatch2, panmatch3),
    )


def matchhist(original, target, matched):
    # pan/intensity histogram matching using numpy arrays
    gs.message(_("Histogram matching..."))

    # input images
    original = original.split("@")[0]
    target = target.split("@")[0]
    images = [original, target]

    # create a dictionary to hold arrays for each image
    arrays = {}

    for img in images:
        # calculate number of cells for each grey value for for each image
        stats_out = gs.pipe_command("r.stats", flags="cin", input=img, sep=":")
        stats = gs.decode(stats_out.communicate()[0]).split("\n")[:-1]
        stats_dict = dict(s.split(":", 1) for s in stats)
        total_cells = 0  # total non-null cells
        for j in stats_dict.keys():  # noqa: PLC0206
            stats_dict[j] = int(stats_dict[j])
            if j != "*":
                total_cells += stats_dict[j]

        if total_cells < 1:
            gs.fatal(_("Input has no data. Check region settings."))

        # Make a 2x256 structured array for each image with a
        #   cumulative distribution function (CDF) for each grey value.
        #   Grey value is the integer (i4) and cdf is float (f4).

        arrays[img] = np.zeros((256,), dtype=("i4,f4"))
        cum_cells = (
            0  # cumulative total of cells for sum of current and all lower grey values
        )

        for n in range(256):
            num_cells = stats_dict.get(str(n), 0)

            cum_cells += num_cells

            # cdf is the the number of cells at or below a given grey value
            #   divided by the total number of cells
            cdf = float(cum_cells) / float(total_cells)

            # insert values into array
            arrays[img][n] = (n, cdf)

    # open file for reclass rules
    with open(gs.tempfile(), "w") as outfile:
        for i in arrays[original]:
            # for each grey value and corresponding cdf value in original, find the
            #   cdf value in target that is closest to the target cdf value
            difference_list = []
            for j in arrays[target]:
                # make a list of the difference between each original cdf value and
                #   the target cdf value
                difference_list.append(abs(i[1] - j[1]))

            # get the smallest difference in the list
            min_difference = min(difference_list)

            for j in arrays[target]:
                # find the grey value in target that corresponds to the cdf
                #   closest to the original cdf
                if j[1] <= i[1] + min_difference and j[1] >= i[1] - min_difference:
                    # build a reclass rules file from the original grey value and
                    #   corresponding grey value from target
                    out_line = "%d = %d\n" % (i[0], j[0])
                    outfile.write(out_line)
                    break

    # create reclass of target from reclass rules file
    result = gs.core.find_file(matched, element="cell")
    if result["fullname"]:
        gs.run_command("g.remove", flags="f", quiet=True, type="raster", name=matched)
        gs.run_command("r.reclass", input=original, out=matched, rules=outfile.name)
    else:
        gs.run_command("r.reclass", input=original, out=matched, rules=outfile.name)

    # Cleanup
    # remove the rules file
    gs.try_remove(outfile.name)

    # return reclass of target with histogram that matches original
    return matched


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
