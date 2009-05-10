Test suite i.atcorr

ETM4_atmospheric_input.txt = File with the atmospheric and sensor conditions
ETM4.res                   = Output file showing the atmospheric and sensor conditions
ETM4_400x400.raw           = Image input file
ETM4_400x400_atms_corr.raw = Image output file (expected output)


# try in Spearfish location or other:

# import raw Landsat channel 4:
r.in.gdal ETM4_400x400.raw out=ETM4_400x400.raw -o
# set region to this map
g.region rast=ETM4_400x400.raw

# run atmospheric correction on imported channel 4:
i.atcorr -r --o iimg=ETM4_400x400.raw \
         icnd=./ETM4_atmospheric_input_GRASS.txt \
         ialt=dem \
         oimg=ETM4_400x400.corrected

# import already corrected test data set for comparison:
r.in.gdal ETM4_400x400_atms_corr.raw out=ETM4_400x400_atms_corr -o

# compare:
r.mapcalc "diff = ETM4_400x400_atms_corr - ETM4_400x400.corrected"
r.colors diff color=differences

r.univar diff

