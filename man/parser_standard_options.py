"""
Created on Fri Jun 26 19:10:58 2015

@author: pietro
"""

import argparse
import os
import sys

from urllib.request import urlopen


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
            #                if optname == 'G_OPT_R_BASENAME_INPUT':
            #                    import ipdb; ipdb.set_trace()
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
        default = default.removeprefix("_(")
        return key, default

    def parse_glines(glines):
        res = {}
        key = None
        dynamic_answer = False
        for line in glines:
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
                start, end = 0, -1
                if line.startswith("_("):
                    start = 2
                if line.endswith(");"):
                    end = -3
                elif line.endswith(";"):
                    end = -2
                res[key].append(line[start:end])
        # pprint(glines)
        # pprint(res)
        return res

    def clean_value(val):
        if isinstance(val, list):
            val = " ".join(val)
        return (
            (val.replace('"', "").replace("'", "'").strip().strip(";"))
            .strip()
            .strip("_(")
            .strip()
            .strip(")")
            .strip()
        )

    # with open(optionfile, mode='r') as optfile:
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
        """Return a Markdown table with the options"""
        # write header
        md = ["| " + " | ".join(self.columns) + " |"]
        md.append("| " + " | ".join(len(x) * "-" for x in self.columns) + " |")

        # write body
        for optname, options in self.options:
            row = "| {0} ".format(optname)
            for col in self.columns:
                row += "| {0} ".format(options.get(col, ""))
            md.append(row + "|")

        return endline.join(md)

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
                outfile.write(
                    header1_tmpl.substitute(
                        title=f"GRASS GIS {grass_version} Reference Manual: "
                        "Parser standard options index"
                    )
                )
                outfile.write(headerpso_tmpl)
                if ext == "html":
                    outfile.write(options.html(toptions=args.htmlparmas))
                else:
                    outfile.write(options.markdown())
                write_footer(outfile, f"index.{ext}", year, template=ext)

        from build import (
            grass_version,
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
