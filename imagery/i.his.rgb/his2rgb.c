/******************************************************************************
 * FUNCTION:    his2rgb
 *
 * PURPOSE:     Converts a row of HIS (Hue, Intensity, Saturation) values
 *              into RGB (Red, Green, Blue) using normalized transformations.
 *
 * NOTES:       - Handles grayscale (S=0), black (I=0), and pure hues.
 *              - Assumes input images are already read into rowbuffer.
 *              - Overwrites the original rowbuffer with RGB results.
 *****************************************************************************/

#include <grass/gis.h>
#include "globals.h"
#include <math.h>

#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

// function to remove redundancy in the old code
static double hue2rgb(double m1, double m2, double h)
{
    if (h < 0.0)
        h += 360.0;
    if (h >= 360.0)
        h -= 360.0;

    if (h < 60.0)
        return m1 + (m2 - m1) * h / 60.0;
    else if (h < 180.0)
        return m2;
    else if (h < 240.0)
        return m1 + (m2 - m1) * (240.0 - h) / 60.0;
    else
        return m1;
}

void his2rgb(CELL *rowbuffer[3], int cols)
{
    // NULL values handling
    for (int i = 0; i < cols; i++) {
        if (Rast_is_c_null_value(&rowbuffer[0][i]) ||
            Rast_is_c_null_value(&rowbuffer[1][i]) ||
            Rast_is_c_null_value(&rowbuffer[2][i])) {
            for (int j = 0; j < 3; j++)
                Rast_set_c_null_value(&rowbuffer[j][i], 1);
            continue;
        }

        int h_raw = rowbuffer[0][i];
        int i_raw = rowbuffer[1][i];
        int s_raw = rowbuffer[2][i];

        // Range Check for the I and S (skip H range check to allow degrees)
        if (i_raw < 0 || i_raw > 255 || s_raw < 0 || s_raw > 255) {
            for (int j = 0; j < 3; j++)
                Rast_set_c_null_value(&rowbuffer[j][i], 1);
            continue;
        }

        double h = (double)h_raw;
        if (h <= 255.0)
            h = h * 360.0 / 255.0; // Scale 0–255 to 0-360 degrees

        double intensity = i_raw / 255.0;
        double saturation = s_raw / 255.0;

        double r, g, b;

        // Intensity 0 -> Black
        if (intensity < 1e-6) {
            r = g = b = 0.0;
        }
        // Saturation 0 -> Grayscale
        else if (saturation < 1e-6) {
            r = g = b = intensity;
        }
        // Pure Intensities and Saturation -> Pure Hues
        else if (intensity >= 1.0 - 1e-6 && saturation >= 1.0 - 1e-6) {
            if (h < 60.0 || h >= 360.0)
                r = 1.0, g = 0.0, b = 0.0;
            else if (h < 180.0)
                r = 0.0, g = 1.0, b = 0.0;
            else
                r = 0.0, g = 0.0, b = 1.0;
        }
        // General conversion
        else {
            double m2 = (intensity <= 0.5)
                            ? (intensity * (1.0 + saturation))
                            : (intensity + saturation - intensity * saturation);
            double m1 = 2.0 * intensity - m2;

            r = hue2rgb(m1, m2, h + 120.0);
            g = hue2rgb(m1, m2, h);
            b = hue2rgb(m1, m2, h - 120.0);
        }

        // Convert normalized RGB values [0.0, 1.0] to integer CELL values [0,
        // 255] Clamp to ensure values stay within valid range and add 0.5 for
        // rounding to nearest integer
        rowbuffer[0][i] = (CELL)(CLAMP(r, 0.0, 1.0) * 255.0 + 0.5);
        rowbuffer[1][i] = (CELL)(CLAMP(g, 0.0, 1.0) * 255.0 + 0.5);
        rowbuffer[2][i] = (CELL)(CLAMP(b, 0.0, 1.0) * 255.0 + 0.5);
    }
}
