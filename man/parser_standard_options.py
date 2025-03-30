"""
Copyright 2015-2025 by Pietro Zambelli and the GRASS Development Team

@author: Pietro Zambelli
@author: Vaclav Petras (Markdown output)
"""

import argparse
import os
import re
import sys

from urllib.request import urlopen
from collections import defaultdict


def parse_options(lines, startswith="Opt"):
    def split_in_groups(lines):
        def count_par_diff(line):
            open_par = line.count("(")
            close_par = line.count(")")
            return open_par - close_par

        res = None
        diff = 0
        for line in lines:
            if line.startswith("case"):
                optname = line.split()[1][:-1]
                res = []
            elif line == "break;":
                diff = 0
                yield optname, res
            elif line.startswith("G_"):
                diff = count_par_diff(line)
            elif diff > 0:
                diff -= count_par_diff(line)
            else:
                res.append(line) if res is not None else None

    def split_opt_line(line):
        index = line.index("=")
        key = line[:index].strip()
        default = line[index + 1 :].strip()
        return key, default

    def parse_glines(glines):
        res = {}
        key = None
        dynamic_answer = False
        for line in glines:
            # Dynamic answer block is ignored, if non-dynamic default is set,
            # it will be used. (That's not ideal as that requires to have a hardcoded
            # value there, that value in in documentation and is ignored most of the
            # time.)
            if line.strip() == "/* start dynamic answer */":
                dynamic_answer = True
            if line.strip() == "/* end dynamic answer */":
                dynamic_answer = False
            if dynamic_answer or line.startswith("/*"):
                continue
            if line.startswith("/*"):
                continue
            if line.startswith(startswith) and line.endswith(";"):
                key, default = (w.strip() for w in split_opt_line(line[5:]))
                res[key] = default
            elif line.startswith(startswith):
                key, default = split_opt_line(line[5:])
                res[key] = [
                    default,
                ]
            elif key is not None:
                if key not in res:
                    res[key] = []
                res[key].append(line)
        return res

    def clean_value(val):
        if isinstance(val, list):
            val = "".join(val)
        val = val.strip()
        # semicolon at the end of the statement
        val = re.sub(r"^(.*)[\s]*;$", r"\1", val)
        # translation parenthesis
        val = re.sub(r"^_\((.*)\)$", r"\1", val)
        # string double quotes on the outside
        val = re.sub(r'^"(.*)"$', r"\1", val)
        # double quotes from a multiline string
        val = re.sub(r'"[\s]*"', "", val)
        # escape double quotes to show as quotes
        return val.replace(r"\"", '"')

    lines = [line.strip() for line in lines]
    result = []
    for optname, glines in split_in_groups(lines):
        if glines:
            res = parse_glines(glines)
            if res:
                for key, val in res.items():
                    res[key] = clean_value(val)
                result.append((optname, res))
    return result


def write_markdown_option(md: list, name: str, option: dict, keys: list) -> None:
    """Return single option in Markdown format"""
    md.extend(
        [
            "### {0}\n".format(name),
            f"The standard option {name} has the following default values:\n",
            "| item | value |",
            "| ---- | ----- |",
        ]
    )
    key_aka_name = None
    for item in keys:
        value = option.get(item, "")
        if re.match(r"G_[a-zA-Z0-9_]+\(.*\)", value):
            value = f"Computed dynamically by *{value}*"
        if value in ["YES", "NO"]:
            value = f"{value} (`{item}: {value.lower()}`)"
        result = re.match(r"TYPE_([A-Z0-9_]+)", value)
        if result:
            value = f"{value} (`{item}: {result.group(1).lower()}`)"
        if value:
            md.append(f"| {item} | {value} |")
            if item == "key":
                key_aka_name = value
    md.extend(
        [
            "\nPython usage:\n",
            "```python",
            f"# %option {name}",
            "# %end",
            "```",
        ]
    )
    if key_aka_name:
        # More developer doc if we have a key.
        md.extend(
            [
                "\nTo use the value in Python:\n",
                "```python",
                f'{key_aka_name} = options["{key_aka_name}"]',
                "```"
                # Example from user perspective.
                "\nWhat a tool user (caller) may write:\n",
                "```python",
            ]
        )
        answer = option.get("answer")
        key_desc = option.get("key_desc")
        if answer and not answer.startswith("DEFAULT_"):
            # The value of answer a great example except when it is a constant
            # such as DEFAULT_FG_COLOR.
            value = answer
        elif key_desc:
            value = key_desc
        else:
            value = "..."
        if option.get("type") and option.get("type") == "TYPE_STRING":
            md.append(f'{key_aka_name}="{value}"')
        else:
            md.append(f"{key_aka_name}={value}")
        md.append("```\n")


MARKDOWN_INTRO_TEXT = """\
Standard parser options are simplifying definitions of common
parameters when writing GRASS tools. Their identifiers are the same
across (programming) languages. In Python, place an identifier right
after `# %option` (on the same line separated by a space).
In C, use function `G_define_standard_option()`.
The individual items in the definition can be overridden if the tool
needs, for example, a different key (parameter name) or description.
The use of standard parser options is strongly recommended, with or without
overrides, for all common GRASS parameters of such as raster and vector inputs.
See *[g.parser](g.parser.md)* documentation for details on defining
tool parameters.
"""


OPTIONS_TO_HEADINGS = {
    "DB": "Database",
    "I": "Imagery",
    "R": "Raster",
    "R3": "Raster 3D",
    "V": "Vector",
    "V3": "Vector",  # We have only one V3 which is G_OPT_V3_TYPE.
    "F": "File",
    "C": "Color",
    "CN": "Color",  # Legacy naming convention.
    "MAP": "Map (of Any Type)",
    "T": "Temporal",
    "STDS": "Temporal",
    "STRDS": "Temporal",
    "STR3DS": "Temporal",
    "STVDS": "Temporal",
    "M": "Miscellaneous",
}

OTHER_HEADING = "Other"


def get_heading(std_option_name):
    match = re.search(r"G_OPT_([^_]+).*", std_option_name)
    if match:
        try:
            return OPTIONS_TO_HEADINGS[match.group(1)]
        except KeyError:
            return OTHER_HEADING
    msg = f"Strangely formatted standard option identifier {std_option_name}"
    raise ValueError(msg)


def markdown(options, columns, endline="\n") -> str:
    """Return a Markdown table with the options"""
    md = []

    md.append(MARKDOWN_INTRO_TEXT)

    top_keys = [
        "key",
        "label",
        "description",
        "required",
        "multiple",
        "answer",
        "type",
        "gisprompt",
        "key_desc",
        "options",
    ]
    keys = list(top_keys)
    keys.extend(key for key in columns if key not in top_keys)

    options_by_heading = defaultdict(list)
    for name, option in options:
        heading = get_heading(name)
        options_by_heading[heading].append((name, option))

    headings = sorted(set(OPTIONS_TO_HEADINGS.values()))
    headings.append(OTHER_HEADING)
    for heading in headings:
        md.append("## {0}\n".format(heading))
        for name, option in options_by_heading[heading]:
            write_markdown_option(md, name, option, keys)
    return endline.join(md)


class OptTable:
    def __init__(self, list_of_dict):
        self.options = list_of_dict
        self.columns = sorted({key for _, d in self.options for key in d.keys()})

    def csv(self, delimiter=";", endline="\n"):
        """Return a CSV string with the options"""
        csv = []
        csv.append(delimiter.join(self.columns))
        for optname, options in self.options:
            opts = [options.get(col, "") for col in self.columns]
            csv.append(
                delimiter.join(
                    [
                        optname,
                    ]
                    + opts
                )
            )
        return endline.join(csv)

    def markdown(self, endline="\n"):
        return markdown(options=self.options, columns=self.columns, endline=endline)

    def html(self, endline="\n", indent="  ", toptions="border=1"):
        """Return a HTML table with the options"""
        html = ["<table{0}>".format(" " + toptions if toptions else "")]
        # write headers
        html.extend(
            (
                indent + "<thead>",
                indent + "<tr>",
                indent * 2 + "<th>{0}</th>".format("option"),
            )
        )
        for col in self.columns:
            html.append(indent * 2 + "<th>{0}</th>".format(col))
        html.extend((indent + "</tr>", indent + "</thead>", indent + "<tbody>"))
        for optname, options in self.options:
            html.extend((indent + "<tr>", indent * 2 + "<td>{0}</td>".format(optname)))
            for col in self.columns:
                html.append(indent * 2 + "<td>{0}</td>".format(options.get(col, "")))
            html.append(indent + "</tr>")
        html.extend((indent + "</tbody>", "</table>"))
        return endline.join(html)

    def _repr_html_(self):
        """Method used by IPython notebook"""
        return self.html()


if __name__ == "__main__":
    URL = (
        "https://raw.githubusercontent.com/OSGeo/grass/main/"
        "lib/gis/parser_standard_options.c"
    )

    parser = argparse.ArgumentParser(
        description="Extract GRASS default options from link."
    )
    parser.add_argument(
        "-f",
        "--format",
        default="html",
        dest="format",
        choices=["html", "csv", "grass", "markdown"],
        help="Define the output format",
    )
    parser.add_argument(
        "-l",
        "--link",
        default=URL,
        dest="url",
        type=str,
        help="Provide the url with the file to parse",
    )
    parser.add_argument(
        "-t",
        "--text",
        dest="text",
        type=argparse.FileType("r"),
        help="Provide the file to parse",
    )
    parser.add_argument(
        "-o",
        "--output",
        default=sys.stdout,
        dest="output",
        type=argparse.FileType("w"),
        help="Provide the url with the file to parse",
    )
    parser.add_argument(
        "-s",
        "--starts-with",
        default="Opt",
        dest="startswith",
        type=str,
        help="Extract only the options that starts with this",
    )
    parser.add_argument(
        "-p",
        "--html_params",
        default="border=1",
        type=str,
        dest="htmlparmas",
        help="Options for the HTML table",
    )
    args = parser.parse_args()

    cfile = args.text or urlopen(args.url, proxies=None)

    options = OptTable(parse_options(cfile.readlines(), startswith=args.startswith))
    outform = args.format
    if outform in ("csv", "html", "markdown"):
        print(getattr(options, outform)(), file=args.output)
        args.output.close()
    else:
        year = os.getenv("VERSION_DATE")
        name = args.output.name
        args.output.close()

        def write_output(ext):
            with open(name, "w") as outfile:
                outfile.write(header1_tmpl.substitute(title="Standard Parser Options"))
                outfile.write(headerpso_tmpl)
                if ext == "html":
                    outfile.write(options.html(toptions=args.htmlparmas))
                else:
                    outfile.write(options.markdown())
                write_footer(outfile, f"index.{ext}", year, template=ext)

        from build import (
            write_footer,
        )

        ext = os.path.splitext(name)[1][1:]

        if ext == "html":
            from build_html import (
                header1_tmpl,
                headerpso_tmpl,
            )
        else:
            from build_md import (
                header1_tmpl,
                headerpso_tmpl,
            )

        write_output(ext)  # html or md
