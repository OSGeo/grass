"""
@package gmodeler.model_io

@brief wxGUI Graphical Modeler io/export/convert classes

Classes:
 - model_convert::BaseModelConverter
 - model_convert::ModelToActinia
 - model_convert::ModelToPyWPS
 - model_convert::ModelToPython

(C) 2010-2025 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Ondrej Pesek <pesej.ondrek gmail.com>
"""

import re
import time

import wx
from abc import ABC, abstractmethod

from core import utils
from gui_core.forms import GUI

from gmodeler.model_items import ModelAction, ModelComment, ModelCondition, ModelLoop


class BaseModelConverter(ABC):
    """Abstract class for scripts based on the model."""

    @abstractmethod
    def __init__(self, fd, model):
        """Constructor to be overridden."""
        self.fd = None
        self.model = None
        self.indent = None
        self.grassAPI = None

        # call method_write...()

    def _writeItem(self, item, ignoreBlock=True, variables={}):
        """Write model object to Python file"""
        if isinstance(item, ModelAction):
            if ignoreBlock and item.GetBlockId():
                # ignore items in loops of conditions
                return
            self._writePythonAction(
                item, variables, self.model.GetIntermediateData()[:3]
            )
        elif isinstance(item, (ModelLoop, ModelCondition)):
            # substitute condition
            cond = item.GetLabel()
            for variable in self.model.GetVariables():
                pattern = re.compile("%{" + variable + "}")
                if pattern.search(cond):
                    value = variables[variable].get("value", "")
                    if variables[variable].get("type", "string") == "string":
                        value = '"' + value + '"'
                    cond = pattern.sub(value, cond)
            if isinstance(item, ModelLoop):
                condVar, condText = (x.strip() for x in re.split(r"\s* in \s*", cond))
                cond = "%sfor %s in " % (" " * self.indent, condVar)
                if condText[0] == "`" and condText[-1] == "`":
                    task = GUI(show=None).ParseCommand(cmd=utils.split(condText[1:-1]))
                    cond += "grass.read_command("
                    cond += (
                        self._getPythonActionCmd(task, len(cond), variables=[condVar])
                        + ".splitlines()"
                    )
                else:
                    cond += condText
                self.fd.write("%s:\n" % cond)
                self.indent += 4
                variablesLoop = variables.copy()
                variablesLoop[condVar] = None
                for action in item.GetItems(self.model.GetItems(objType=ModelAction)):
                    self._writeItem(action, ignoreBlock=False, variables=variablesLoop)
                self.indent -= 4
            if isinstance(item, ModelCondition):
                self.fd.write("%sif %s:\n" % (" " * self.indent, cond))
                self.indent += 4
                condItems = item.GetItems()
                for action in condItems["if"]:
                    self._writeItem(action, ignoreBlock=False)
                if condItems["else"]:
                    self.indent -= 4
                    self.fd.write("%selse:\n" % (" " * self.indent))
                    self.indent += 4
                    for action in condItems["else"]:
                        self._writeItem(action, ignoreBlock=False)
                self.indent += 4
        self.fd.write("\n")
        if isinstance(item, ModelComment):
            self._writePythonComment(item)

    def _writePythonComment(self, item):
        """Write model comment to Python file"""
        for line in item.GetLabel().splitlines():
            self.fd.write("#" + line + "\n")

    def _getParamName(self, parameter_name, item):
        return "{module_nickname}_{param_name}".format(
            module_nickname=self._getModuleNickname(item),
            param_name=parameter_name,
        )

    @staticmethod
    def _getModuleNickname(item):
        return "{module_name}{module_id}".format(
            module_name=re.sub(r"[^a-zA-Z]+", "", item.GetLabel()),
            module_id=item.GetId(),
        )

    def _getItemFlags(self, item, opts, variables):
        """Get item flags that are needed to be parsed in the script.

        :param item: module
        :param opts: options of the task
        :param variables: variables of the item
        :return: string with flag names set to True, string with
            comma-separated flags that are parameterized, list of
            parameterized boolean parameters like verbose or overwrite (needed
            as they are also tagged as flags)
        """
        item_params = []
        item_true_flags = ""
        item_parameterized_flags = []

        parameterized_flags = [v["name"] for v in variables["flags"]]

        for f in opts["flags"]:
            if f.get("name") in parameterized_flags and len(f.get("name")) == 1:
                item_parameterized_flags.append(
                    '"{}"'.format(self._getParamName(f.get("name"), item))
                )
            if f.get("value", False):
                name = f.get("name", "")
                if len(name) > 1:
                    item_params.append("%s=True" % name)
                else:
                    item_true_flags += name

        item_parameterized_flags = ", ".join(item_parameterized_flags)

        return item_true_flags, item_parameterized_flags, item_params


class ModelToActinia(BaseModelConverter):
    """Class for exporting model to an actinia script."""

    def __init__(self, fd, model, grassAPI=None):
        """Class for exporting model to actinia script."""
        self.fd = fd
        self.model = model
        self.indent = 2

        self._writeActinia()

    def _writeActinia(self):
        """Write actinia model to file."""
        properties = self.model.GetProperties()

        description = properties["description"]

        self.fd.write(
            f"""{{
{" " * self.indent * 1}"id": "model",
{" " * self.indent * 1}"description": "{'""'.join(description.splitlines())}",
{" " * self.indent * 1}"version": "1",
"""
        )

        parameterized = False
        module_list_str = ""
        for item in self.model.GetItems(ModelAction):
            parameterizedParams = item.GetParameterizedParams()
            if len(parameterizedParams["params"]) > 0:
                parameterized = True

            module_list_str += self._getPythonAction(item, parameterizedParams)
            module_list_str += f"{' ' * self.indent * 3}}},\n"

        if parameterized is True:
            self.fd.write(f'{" " * self.indent * 1}"template": {{\n')
            self.fd.write(
                f"""{" " * self.indent * 2}"list": [
    """
            )
        else:
            self.fd.write(
                f"""{" " * self.indent}"list": [
    """
            )

        # module_list_str[:-2] to get rid of the trailing comma and newline
        self.fd.write(module_list_str[:-2] + "\n")

        if parameterized is True:
            self.fd.write(f"{' ' * self.indent * 2}]\n{' ' * self.indent * 1}}}\n}}")
        else:
            self.fd.write(f"{' ' * self.indent * 1}]\n}}")

    def _getPythonAction(self, item, variables={}, intermediates=None):
        """Write model action to Python file"""
        task = GUI(show=None).ParseCommand(cmd=item.GetLog(string=False))
        strcmd = f"{' ' * self.indent * 3}{{\n"

        return (
            strcmd + self._getPythonActionCmd(item, task, len(strcmd), variables) + "\n"
        )

    def _getPythonActionCmd(self, item, task, cmdIndent, variables={}):
        opts = task.get_options()

        ret = ""
        parameterizedParams = [v["name"] for v in variables["params"]]

        flags, itemParameterizedFlags, params = self._getItemFlags(
            item, opts, variables
        )
        inputs = []
        outputs = []

        if len(itemParameterizedFlags) > 0:
            dlg = wx.MessageDialog(
                self.model.canvas,
                message=_(
                    "Module {task_name} in your model contains "
                    "parameterized flags. Actinia does not support "
                    "parameterized flags. The following flags are therefore "
                    "not being written in the generated JSON: {flags}"
                ).format(task_name=task.get_name(), flags=itemParameterizedFlags),
                caption=_("Warning"),
                style=wx.OK_DEFAULT | wx.ICON_WARNING,
            )
            dlg.ShowModal()
            dlg.Destroy()

        for p in opts["params"]:
            name = p.get("name", None)
            value = p.get("value", None)

            if (not name or not value) and name not in parameterizedParams:
                continue

            if name in parameterizedParams:
                parameterizedParam = self._getParamName(name, item)
                default_val = p.get("value", "")

                if len(default_val) > 0:
                    parameterizedParam += f"|default({default_val})"
                value = f"{{{{ {parameterizedParam} }}}}"

            param_string = f'{{"param": "{name}", "value": "{value}"}}'
            age = p.get("age", "old")
            if age == "new":
                outputs.append(param_string)
            else:
                inputs.append(param_string)

        ret += f'{" " * self.indent * 4}"module": "{task.get_name()}",\n'
        ret += f'{" " * self.indent * 4}"id": "{self._getModuleNickname(item)}",\n'

        # write flags
        if flags:
            ret += f'{" " * self.indent * 4}"flags": "{flags}",\n'

        # write inputs and outputs
        if len(inputs) > 0:
            ret += self.write_params("inputs", inputs)
        else:
            ret += "}"

        if len(outputs) > 0:
            ret += self.write_params("outputs", outputs)

        # ret[:-2] to get rid of the trailing comma and newline
        # (to make the json valid)
        return ret[:-2]

    def write_params(self, param_type, params):
        """Write the full list of parameters of one type.

        :param param_type: type of parameters (inputs or outputs)
        :params: list of the parameters
        """
        ret = f'{" " * self.indent * 4}"{param_type}": [\n'
        for opt in params[:-1]:
            ret += f"{' ' * self.indent * 5}{opt},\n"
        ret += f"{' ' * self.indent * 5}{params[-1]}\n"
        ret += f"{' ' * self.indent * 4}],\n"

        return ret


class ModelToPyWPS(BaseModelConverter):
    """Class for exporting model to PyWPS script."""

    def __init__(self, fd, model, grassAPI="script"):
        """Class for exporting model to PyWPS script."""
        self.fd = fd
        self.model = model
        self.indent = 8
        self.grassAPI = grassAPI

        self._writePyWPS()

    def _writePyWPS(self):
        """Write PyWPS model to file"""
        properties = self.model.GetProperties()

        self.fd.write(
            r"""#!/usr/bin/env python3

import sys
import os
import atexit
import tempfile
"""
        )
        if self.grassAPI == "script":
            self.fd.write("from grass.script import run_command\n")
        else:
            self.fd.write("from grass.pygrass.modules import Module\n")

        self.fd.write(
            r"""from pywps import Process, LiteralInput, ComplexInput, ComplexOutput, Format


class Model(Process):

    def __init__(self):
        inputs = list()
        outputs = list()

"""  # noqa: E501
        )

        for item in self.model.GetItems(ModelAction):
            self._write_input_outputs(item, self.model.GetIntermediateData()[:3])

        self.fd.write(
            r"""        super(Model, self).__init__(
            self._handler,
            identifier="{identifier}",
            title="{title}",
            inputs=inputs,
            outputs=outputs,
            # here you could also specify the GRASS location, for example:
            # grass_location="EPSG:5514",
            abstract="{abstract}",
            version="1.0",
            store_supported=True,
            status_supported=True)
""".format(
                identifier=properties["name"],
                title=properties["name"],
                abstract='""'.join(properties["description"].splitlines()),
            )
        )

        self.fd.write(
            """
    @staticmethod
    def _handler(request, response):"""
        )

        self._writeHandler()

        for item in self.model.GetItems(ModelAction):
            if item.GetParameterizedParams()["flags"]:
                self.fd.write(
                    r"""

def getParameterizedFlags(paramFlags, itemFlags):
    fl = ""
    for i in [key for key, value in paramFlags.items() if value[0].data == "True"]:
        if i in itemFlags:
            fl += i[-1]

    return fl
"""
                )
                break

        self.fd.write(
            """

if __name__ == "__main__":
    from pywps.app.Service import Service

    processes = [Model()]
    application = Service(processes)
"""
        )

    def _write_input_outputs(self, item, intermediates):
        parameterized_params = item.GetParameterizedParams()

        for flag in parameterized_params["flags"]:
            desc = flag["label"] or flag["description"]

            if flag["value"]:
                value = '\n{}default="{}"'.format(
                    " " * (self.indent + 4), flag["value"]
                )
            else:
                value = '\n{}default="False"'.format(" " * (self.indent + 4))

            io_data = "inputs"
            object_type = "LiteralInput"
            format_spec = 'data_type="string",'

            self._write_input_output_object(
                io_data,
                object_type,
                flag["name"],
                item,
                desc,
                format_spec,
                value,
            )

            self.fd.write("\n")

        for param in parameterized_params["params"]:
            desc = self._getParamDesc(param)
            value = self._getParamValue(param)

            if "input" in param["name"]:
                io_data = "inputs"
                object_type = "ComplexInput"
                format_spec = self._getSupportedFormats(param["prompt"])
            else:
                io_data = "inputs"
                object_type = "LiteralInput"
                format_spec = 'data_type="{}"'.format(param["type"])

            self._write_input_output_object(
                io_data,
                object_type,
                param["name"],
                item,
                desc,
                format_spec,
                value,
            )

            self.fd.write("\n")

        # write ComplexOutputs
        for param in item.GetParams()["params"]:
            desc = self._getParamDesc(param)
            value = param["value"]
            age = param["age"]

            # ComplexOutput if: outputting a new non-intermediate layer and
            # either not empty or parameterized
            if (
                age != "new"
                or any(value in i for i in intermediates)
                or (
                    value == "" and param not in item.GetParameterizedParams()["params"]
                )
            ):
                continue
            io_data = "outputs"
            object_type = "ComplexOutput"
            format_spec = self._getSupportedFormats(param["prompt"])
            self._write_input_output_object(
                io_data, object_type, param["name"], item, desc, format_spec, ""
            )
            self.fd.write("\n")

    def _write_input_output_object(
        self,
        io_data,
        object_type,
        name,
        item,
        desc,
        format_spec,
        value,
    ):
        self.fd.write(
            """        {ins_or_outs}.append({lit_or_complex}(
            identifier="{param_name}",
            title="{description}",
            {special_params}{value}))
""".format(
                ins_or_outs=io_data,
                lit_or_complex=object_type,
                param_name=self._getParamName(name, item),
                description=desc,
                special_params=format_spec,
                value=value,
            )
        )

    def _writeHandler(self):
        for item in self.model.GetItems(ModelAction):
            self._writeItem(item, variables=item.GetParameterizedParams())

        self.fd.write("\n{}return response\n".format(" " * self.indent))

    def _writePythonAction(self, item, variables={}, intermediates=None):
        """Write model action to Python file"""
        task = GUI(show=None).ParseCommand(cmd=item.GetLog(string=False))
        strcmd = "\n%s%s(" % (
            " " * self.indent,
            "run_command" if self.grassAPI == "script" else "Module",
        )
        self.fd.write(
            strcmd + self._getPythonActionCmd(item, task, len(strcmd) - 1, variables)
        )

        # write v.out.ogr and r.out.gdal exports for all outputs
        for param in item.GetParams()["params"]:
            value = param["value"]
            age = param["age"]

            # output if: outputting a new non-intermediate layer and
            # either not empty or parameterized
            if (
                age != "new"
                or any(value in i for i in intermediates)
                or (
                    value == "" and param not in item.GetParameterizedParams()["params"]
                )
            ):
                continue

            if param["prompt"] == "vector":
                command = "v.out.ogr"
                format = '"GML"'
                extension = ".gml"
            elif param["prompt"] == "raster":
                command = "r.out.gdal"
                format = '"GTiff"'
                extension = ".tif"
            else:
                # TODO: Support 3D
                command = "WRITE YOUR EXPORT COMMAND"
                format = '"WRITE YOUR EXPORT FORMAT"'
                extension = "WRITE YOUR EXPORT EXTENSION"

            n = param.get("name", None)
            param_name = self._getParamName(n, item)

            keys = param.keys()
            if "parameterized" in keys and param["parameterized"] is True:
                param_request = 'request.inputs["{}"][0].data'.format(param_name)
            else:
                param_request = '"{}"'.format(param["value"])

            # if True, write the overwrite parameter to the model command
            overwrite = self.model.GetProperties().get("overwrite", False)
            if overwrite is True:
                overwrite_string = ",\n{}overwrite=True".format(
                    " " * (self.indent + 12)
                )
            else:
                overwrite_string = ""

            strcmd_len = len(strcmd.strip())
            self.fd.write(
                """
{run_command}"{cmd}",
{indent1}input={input},
{indent2}output=os.path.join(
{indent3}tempfile.gettempdir(),
{indent4}{out} + "{format_ext}"),
{indent5}format={format}{overwrite_string})
""".format(
                    run_command=strcmd,
                    cmd=command,
                    indent1=" " * (self.indent + strcmd_len),
                    input=param_request,
                    indent2=" " * (self.indent + strcmd_len),
                    indent3=" " * (self.indent * 2 + strcmd_len),
                    indent4=" " * (self.indent * 2 + strcmd_len),
                    out=param_request,
                    format_ext=extension,
                    indent5=" " * (self.indent + strcmd_len),
                    format=format,
                    overwrite_string=overwrite_string,
                )
            )

            left_side_out = 'response.outputs["{}"].file'.format(param_name)
            right_side_out = (
                "os.path.join(\n{indent1}"
                'tempfile.gettempdir(),\n{indent2}{out} + "'
                '{format_ext}")'.format(
                    indent1=" " * (self.indent + 4),
                    indent2=" " * (self.indent + 4),
                    out=param_request,
                    format_ext=extension,
                )
            )
            self.fd.write(
                "\n{}{} = {}".format(" " * self.indent, left_side_out, right_side_out)
            )

    def _getPythonActionCmd(self, item, task, cmdIndent, variables={}):
        opts = task.get_options()

        ret = ""
        parameterizedParams = [v["name"] for v in variables["params"]]

        flags, itemParameterizedFlags, params = self._getItemFlags(
            item, opts, variables
        )

        out = None

        for p in opts["params"]:
            name = p.get("name", None)
            value = p.get("value", None)

            if (not name or not value) and name not in parameterizedParams:
                continue

            ptype = p.get("type", "string")
            foundVar = False

            if name in parameterizedParams:
                foundVar = True
                if "input" in name:
                    value = 'request.inputs["{}"][0].file'.format(
                        self._getParamName(name, item)
                    )
                else:
                    value = 'request.inputs["{}"][0].data'.format(
                        self._getParamName(name, item)
                    )
            if foundVar or ptype != "string":
                params.append("{}={}".format(name, value))
            else:
                params.append('{}="{}"'.format(name, value))

        ret += '"%s"' % task.get_name()
        if flags:
            ret += ',\n{indent}flags="{fl}"'.format(indent=" " * cmdIndent, fl=flags)
            if itemParameterizedFlags:
                ret += " + getParameterizedFlags(\n{}request.inputs, [{}])".format(
                    " " * (cmdIndent + 4), itemParameterizedFlags
                )
        elif itemParameterizedFlags:
            ret += ",\n{}flags=getParameterizedFlags(request.inputs, [{}])".format(
                " " * cmdIndent, itemParameterizedFlags
            )

        if len(params) > 0:
            ret += ",\n"
            for opt in params[:-1]:
                ret += "%s%s,\n" % (" " * cmdIndent, opt)
            ret += "%s%s)" % (" " * cmdIndent, params[-1])
        else:
            ret += ")\n"

        # TODO: Write the next line only for those not-tagged as intermediate
        if out:
            ret += '\n\n{}response.outputs["{}"].file = "{}"'.format(
                " " * self.indent, out, out
            )

        return ret

    def _getParamDesc(self, param):
        return param["label"] or param["description"]

    def _getParamValue(self, param):
        if param["value"] and "output" not in param["name"]:
            if param["type"] in {"float", "integer"}:
                value = param["value"]
            else:
                value = '"{}"'.format(param["value"])

            value = ",\n{}default={}".format(" " * (self.indent + 4), value)
        else:
            value = ""

        return value

    @staticmethod
    def _getSupportedFormats(prompt):
        """Get supported formats of an item.

        :param prompt: param['prompt'] of an item
        :return:
        """
        if prompt == "vector":
            sup_formats = 'Format("application/gml+xml")'
        elif prompt == "raster":
            sup_formats = 'Format("image/tif")'
        else:
            sup_formats = "FORMAT UNKNOWN - WRITE YOUR OWN"

        return "supported_formats=[{}]".format(sup_formats)


class ModelToPython(BaseModelConverter):
    def __init__(self, fd, model, grassAPI="script"):
        """Class for exporting model to Python script

        :param fd: file descriptor
        :param model: model to translate
        :param grassAPI: script or pygrass
        """
        self.fd = fd
        self.model = model
        self.indent = 4
        self.grassAPI = grassAPI

        self._writePython()

    def _getStandardizedOption(self, string):
        """Return GRASS standardized option based on specified string.

        :param string: input string to be converted

        :return: GRASS standardized option as a string or None if not converted
        """
        if string == "raster":
            return "G_OPT_R_MAP"
        if string == "vector":
            return "G_OPT_V_MAP"
        if string == "mapset":
            return "G_OPT_M_MAPSET"
        if string == "file":
            return "G_OPT_F_INPUT"
        if string == "dir":
            return "G_OPT_M_DIR"
        if string == "region":
            return "G_OPT_M_REGION"

        return None

    def _writePython(self):
        """Write model to file"""
        properties = self.model.GetProperties()

        # header
        self.fd.write(
            r"""#!/usr/bin/env python3
#
#{header_begin}
#
# MODULE:       {module_name}
#
# AUTHOR(S):    {author}
#
# PURPOSE:      {purpose}
#
# DATE:         {date}
#
#{header_end}
""".format(
                header_begin="#" * 77,
                module_name=properties["name"],
                author=properties["author"],
                purpose="\n# ".join(properties["description"].splitlines()),
                date=time.asctime(),
                header_end="#" * 77,
            )
        )

        # UI
        self.fd.write(
            r"""
# %module
# % description: {description}
# %end
""".format(description=" ".join(properties["description"].splitlines()))
        )

        modelItems = self.model.GetItems(ModelAction)
        for item in modelItems:
            parametrizedParams = item.GetParameterizedParams()
            for flag in parametrizedParams["flags"]:
                desc = flag["label"] or flag["description"]
                self.fd.write(
                    r"""# %option
# % key: {flag_name}
# % description: {description}
# % required: yes
# % type: string
# % options: True, False
# % guisection: Flags
""".format(
                        flag_name=self._getParamName(flag["name"], item),
                        description=desc,
                    )
                )
                if flag["value"]:
                    self.fd.write("# % answer: {}\n".format(flag["value"]))
                else:
                    self.fd.write("# % answer: False\n")
                self.fd.write("# %end\n")

            for param in parametrizedParams["params"]:
                desc = param["label"] or param["description"]
                self.fd.write(
                    r"""# %option
# % key: {param_name}
# % description: {description}
# % required: yes
""".format(
                        param_name=self._getParamName(param["name"], item),
                        description=desc,
                    )
                )
                if param["type"] != "float":
                    self.fd.write("# % type: {}\n".format(param["type"]))
                else:
                    self.fd.write("# % type: double\n")
                if param["key_desc"]:
                    self.fd.write("# % key_desc: ")
                    self.fd.write(", ".join(param["key_desc"]))
                    self.fd.write("\n")
                if param["value"]:
                    self.fd.write("# % answer: {}\n".format(param["value"]))
                self.fd.write("# %end\n")

        # variables
        for vname, vdesc in self.model.GetVariables().items():
            self.fd.write("# %option")
            optionType = self._getStandardizedOption(vdesc["type"])
            if optionType:
                self.fd.write(" {}".format(optionType))
            self.fd.write("\n")
            self.fd.write(
                r"""# % key: {param_name}
# % description: {description}
# % required: yes
""".format(
                    param_name=vname,
                    description=vdesc.get("description", ""),
                )
            )
            if optionType is None and vdesc["type"]:
                self.fd.write("# % type: {}\n".format(vdesc["type"]))

            if vdesc["value"]:
                self.fd.write("# % answer: {}\n".format(vdesc["value"]))
            self.fd.write("# %end\n")

        # import modules
        self.fd.write(
            r"""
import sys
import os
import atexit

from grass.script import parser
"""
        )
        if self.grassAPI == "script":
            self.fd.write("from grass.script import run_command\n")
        else:
            self.fd.write("from grass.pygrass.modules import Module\n")

        # cleanup()
        rast, vect, rast3d, msg = self.model.GetIntermediateData()
        self.fd.write(
            r"""
def cleanup():
"""
        )
        run_command = "run_command" if self.grassAPI == "script" else "Module"
        if rast:
            self.fd.write(
                r"""    %s("g.remove", flags="f", type="raster",
                name=%s)
"""
                % (run_command, ",".join(f'"{x}"' for x in rast3d))
            )
        if vect:
            self.fd.write(
                r"""    %s("g.remove", flags="f", type="vector",
                name=%s)
"""
                % (run_command, ",".join(f'"{x}"' for x in vect))
            )
        if rast3d:
            self.fd.write(
                r"""    %s("g.remove", flags="f", type="raster_3d",
                name=%s)
"""
                % (run_command, ",".join(f'"{x}"' for x in rast3d))
            )
        if not rast and not vect and not rast3d:
            self.fd.write("    pass\n")

        self.fd.write("\ndef main(options, flags):\n")
        modelVars = self.model.GetVariables()
        for item in self.model.GetItems(ModelAction):
            modelParams = item.GetParameterizedParams()
            modelParams["vars"] = modelVars
            self._writeItem(item, variables=modelParams)

        self.fd.write("    return 0\n")

        for item in modelItems:
            if item.GetParameterizedParams()["flags"]:
                self.fd.write(
                    r"""
def getParameterizedFlags(paramFlags, itemFlags):
    fl = ""
    for i in [key for key, value in paramFlags.items() if value == "True"]:
        if i in itemFlags:
            fl += i[-1]

    return fl
"""
                )
                break

        self.fd.write(
            r"""
if __name__ == "__main__":
    options, flags = parser()
    atexit.register(cleanup)
"""
        )

        if properties.get("overwrite"):
            self.fd.write('    os.environ["GRASS_OVERWRITE"] = "1"\n')

        self.fd.write("    sys.exit(main(options, flags))\n")

    def _writePythonAction(self, item, variables={}, intermediates=None):
        """Write model action to Python file"""
        task = GUI(show=None).ParseCommand(cmd=item.GetLog(string=False))
        strcmd = "%s%s(" % (
            " " * self.indent,
            "run_command" if self.grassAPI == "script" else "Module",
        )
        self.fd.write(
            strcmd + self._getPythonActionCmd(item, task, len(strcmd), variables) + "\n"
        )

    def _substitutePythonParamValue(
        self, value, name, parameterizedParams, variables, item
    ):
        """Substitute parameterized options or variables.

        :param value: parameter value to be substituted
        :param name: parameter name
        :param parameterizedParams: list of parameterized options
        :param variables: list of user-defined variables
        :param item: item object

        :return: substituted value
        """
        foundVar = False
        parameterizedValue = value

        if name in parameterizedParams:
            foundVar = True
            parameterizedValue = 'options["{}"]'.format(self._getParamName(name, item))
        else:
            # check for variables
            formattedVar = False
            for var in variables["vars"]:
                pattern = re.compile("%{" + var + "}")
                found = pattern.search(value)
                if found:
                    foundVar = True
                    if found.end() != len(value):
                        formattedVar = True
                        parameterizedValue = pattern.sub(
                            "{options['" + var + "']}", value
                        )
                    else:
                        parameterizedValue = f'options["{var}"]'
            if formattedVar:
                parameterizedValue = 'f"' + parameterizedValue + '"'

        return foundVar, parameterizedValue

    def _getPythonActionCmd(self, item, task, cmdIndent, variables={}):
        opts = task.get_options()

        ret = ""
        parameterizedParams = [v["name"] for v in variables["params"]]

        flags, itemParameterizedFlags, params = self._getItemFlags(
            item, opts, variables
        )

        for p in opts["params"]:
            name = p.get("name", None)
            value = p.get("value", None)
            ptype = p.get("type", "string")

            if (
                self.grassAPI == "pygrass"
                and (p.get("multiple", False) is True or len(p.get("key_desc", [])) > 1)
                and "," in value
            ):
                value = value.split(",")
                if ptype == "integer":
                    value = list(map(int, value))
                elif ptype == "float":
                    value = list(map(float, value))

            if (not name or not value) and name not in parameterizedParams:
                continue

            if isinstance(value, list):
                foundVar = False
                for idx in range(len(value)):
                    foundVar_, value[idx] = self._substitutePythonParamValue(
                        value[idx], name, parameterizedParams, variables, item
                    )
                    if foundVar_ is True:
                        foundVar = True
            else:
                foundVar, value = self._substitutePythonParamValue(
                    value, name, parameterizedParams, variables, item
                )
            if (
                foundVar
                or isinstance(value, list)
                or (ptype != "string" and len(p.get("key_desc", [])) < 2)
            ):
                params.append("{}={}".format(name, value))
            else:
                params.append('{}="{}"'.format(name, value))

        ret += '"%s"' % task.get_name()
        if flags:
            ret += ',\n{indent}flags="{fl}"'.format(indent=" " * cmdIndent, fl=flags)
            if itemParameterizedFlags:
                ret += " + getParameterizedFlags(options, [{}])".format(
                    itemParameterizedFlags
                )
        elif itemParameterizedFlags:
            ret += ",\n{}flags=getParameterizedFlags(options, [{}])".format(
                " " * cmdIndent, itemParameterizedFlags
            )

        if len(params) > 0:
            ret += ",\n"
            for opt in params[:-1]:
                ret += "%s%s,\n" % (" " * cmdIndent, opt)
            ret += "%s%s)" % (" " * cmdIndent, params[-1])
        else:
            ret += ")"

        return ret
