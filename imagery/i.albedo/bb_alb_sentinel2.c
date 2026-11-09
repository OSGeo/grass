/* Broadband albedo Sentinel-2 MSI
 *
 * Narrow-to-broadband conversion coefficients from:
 * Bonafoni, S. and Sekertekin, A. (2020). Albedo Retrieval From Sentinel-2
 * by New Narrow-to-Broadband Conversion Coefficients. IEEE Geoscience and
 * Remote Sensing Letters, 17(9), 1618-1622.
 *
 * Input bands are surface reflectance in [0, 1], in Sentinel-2 band order
 * B2 (blue), B3 (green), B4 (red), B8 (NIR), B11 (SWIR1), B12 (SWIR2).
 */
double bb_alb_sentinel2(double b2chan, double b3chan, double b4chan,
                        double b8chan, double b11chan, double b12chan)
{
    double result;

    result = (0.2266 * b2chan + 0.1236 * b3chan + 0.1573 * b4chan +
              0.3417 * b8chan + 0.1170 * b11chan + 0.0338 * b12chan);
    return result;
}
