## DESCRIPTION

*v.colors.out* allows the user to export the color table for a vector
map to a file which is suitable as input to *[v.colors](v.colors.md)*.

Alternatively, the color rules can be exported as a JSON format
with **format=json**, to use them in other software.
The color format in JSON can be modified using the **color_format** parameter,
which includes the following options:

- hex: #00BFBF
- rgb: rgb(0, 191, 191)
- hsv: hsv(180, 100, 74)
- triplet: 0:191:191

## EXAMPLES

== "Command line"

    ```sh
    v.colors map=bridges color=plasma column="YEAR_BUILT"
    v.colors.out map=bridges
    ```

    Shortened output:

    ```text
    6388 13:8:135
    662 79:2:162
    9097 94:1:166
    20 98:0:167
    903 98:0:167
    905 98:0:167
    2161 98:0:167
    3439 98:0:167
    5192 105:0:168
    7157 105:0:168
    ...
    ```

 === "Python (grass.script)"

    ```python
    import grass.script as gs

    gs.run_command("v.colors", map="bridges", color="plasma", column="YEAR_BUILT")
    colors = gs.parse_command("v.colors.out", map="bridges", format="json")
    ```

    The shortened JSON output looks like:

    ```json
    [
        {
            "value": 6388,
            "color": "#0D0887"
        },
        {
            "value": 662,
            "color": "#4F02A2"
        },
        {
            "value": 9097,
            "color": "#5E01A6"
        },
        {
            "value": 20,
            "color": "#6200A7"
        },
        ...
    ]
    ```

## SEE ALSO

*[v.colors](v.colors.md), [r.colors](r.colors.md),
[r3.colors](r3.colors.md), [r.colors.out](r.colors.out.md),
[r3.colors.out](r3.colors.out.md)*

## AUTHOR

Martin Landa, Czech Technical University in Prague, Czech Republic
