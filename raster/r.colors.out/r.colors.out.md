## DESCRIPTION

*r.colors.out* allows the user to export the color table for a raster
map to a file which is suitable as input to *[r.colors](r.colors.md)*.

Alternatively, the color rules can be exported as a JSON format
with **format=json**, to use them in other software.
The color format in JSON can be modified using the **color_format** parameter,
which includes the following options:

- hex: #00BFBF
- rgb: rgb(0, 191, 191)
- hsv: hsv(180, 100, 74)
- triplet: 0:191:191

## EXAMPLES

=== "Command line"

    ```sh
    r.colors.out map=elevation
    ```

    Output:

    ```text
    55.5788 0:191:191
    75.729 0:255:0
    95.8792 255:255:0
    116.029 255:127:0
    136.18 191:127:63
    156.33 20:20:20
    nv 255:255:255
    default 255:255:255
    ```

 === "Python (grass.script)"

    ```python
    import grass.script as gs

    colors = gs.parse_command("r.colors.out", map="elevation", format="json")
    ```

    The JSON output looks like:

    ```json
    [
        {
            "value": 55.578792572021484,
            "color": "#00BFBF"
        },
        {
            "value": 75.729006957999999,
            "color": "#00FF00"
        },
        {
            "value": 95.879221344000001,
            "color": "#FFFF00"
        },
        {
            "value": 116.02943573,
            "color": "#FF7F00"
        },
        {
            "value": 136.179650116,
            "color": "#BF7F3F"
        },
        {
            "value": 156.32986450195312,
            "color": "#141414"
        },
        {
            "value": "nv",
            "color": "#FFFFFF"
        },
        {
            "value": "default",
            "color": "#FFFFFF"
        }
    ]
    ```

## SEE ALSO

*[r.colors](r.colors.md)*

## AUTHOR

Glynn Clements
