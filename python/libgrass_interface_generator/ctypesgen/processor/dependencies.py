"""
The dependencies module determines which descriptions depend on which other
descriptions.
"""

from ctypesgen.descriptions import MacroDescription, UndefDescription
from ctypesgen.ctypedescs import visit_type_and_collect_info


def find_dependencies(data, opts):
    """Visit each description in `data` and figure out which other descriptions
    it depends on, putting the results in desc.requirements. Also find errors in
    ctypedecls or expressions attached to the description and transfer them to the
    description."""

    struct_names = {}
    enum_names = {}
    typedef_names = {}
    ident_names = {}

    # Start the lookup tables with names from imported modules

    for name in opts.other_known_names:
        typedef_names[name] = None
        ident_names[name] = None
        if name.startswith("struct_") or name.startswith("enum_"):
            variety = name.split("_")[0]
            tag = "_".join(name.split("_")[1:])
            struct_names[(variety, tag)] = None
        if name.startswith("enum_"):
            enum_names[name] = None

    def depend(desc, nametable, name):
        """Try to add `name` as a requirement for `desc`, looking `name` up in
        `nametable`. Returns True if found."""

        if name in nametable:
            requirement = nametable[name]
            if requirement:
                desc.add_requirements([requirement])
            return True
        else:
            return False

    def co_depend(desc, nametable, name):
        """
        Try to add `name` as a requirement for `desc`, looking `name` up in
        `nametable`.  Also try to add desc as a requirement for `name`.

        Returns Description of `name` if found.
        """

        requirement = nametable.get(name, None)
        if requirement is None:
            return

        desc.add_requirements([requirement])
        requirement.add_requirements([desc])
        return requirement

    def find_dependencies_for(desc, kind):
        """Find all the descriptions that `desc` depends on and add them as
        dependencies for `desc`. Also collect error messages regarding `desc` and
        convert unlocateable descriptions into error messages."""

        if kind == "constant":
            roots = [desc.value]
        elif kind == "struct":
            roots = []
        elif kind == "struct-body":
            roots = [desc.ctype]
        elif kind == "enum":
            roots = []
        elif kind == "typedef":
            roots = [desc.ctype]
        elif kind == "function":
            roots = desc.argtypes + [desc.restype]
        elif kind == "variable":
            roots = [desc.ctype]
        elif kind == "macro":
            if desc.expr:
                roots = [desc.expr]
            else:
                roots = []
        elif kind == "undef":
            roots = [desc.macro]

        cstructs, cenums, ctypedefs, errors, identifiers = [], [], [], [], []

        for root in roots:
            s, e, t, errs, i = visit_type_and_collect_info(root)
            cstructs.extend(s)
            cenums.extend(e)
            ctypedefs.extend(t)
            errors.extend(errs)
            identifiers.extend(i)

        unresolvables = []

        for cstruct in cstructs:
            if kind == "struct" and desc.variety == cstruct.variety and desc.tag == cstruct.tag:
                continue
            if not depend(desc, struct_names, (cstruct.variety, cstruct.tag)):
                unresolvables.append('%s "%s"' % (cstruct.variety, cstruct.tag))

        for cenum in cenums:
            if kind == "enum" and desc.tag == cenum.tag:
                continue
            if not depend(desc, enum_names, cenum.tag):
                unresolvables.append('enum "%s"' % cenum.tag)

        for ctypedef in ctypedefs:
            if not depend(desc, typedef_names, ctypedef):
                unresolvables.append('typedef "%s"' % ctypedef)

        for ident in identifiers:
            if isinstance(desc, MacroDescription) and desc.params and ident in desc.params:
                continue

            elif opts.include_undefs and isinstance(desc, UndefDescription):
                macro_desc = None
                if ident == desc.macro.name:
                    macro_desc = co_depend(desc, ident_names, ident)
                if macro_desc is None or not isinstance(macro_desc, MacroDescription):
                    unresolvables.append('identifier "%s"' % ident)

            elif not depend(desc, ident_names, ident):
                unresolvables.append('identifier "%s"' % ident)

        for u in unresolvables:
            errors.append(("%s depends on an unknown %s." % (desc.casual_name(), u), None))

        for err, cls in errors:
            err += " %s will not be output" % desc.casual_name()
            desc.error(err, cls=cls)

    def add_to_lookup_table(desc, kind):
        """Add `desc` to the lookup table so that other descriptions that use
        it can find it."""
        if kind == "struct":
            if (desc.variety, desc.tag) not in struct_names:
                struct_names[(desc.variety, desc.tag)] = desc
        if kind == "enum":
            if desc.tag not in enum_names:
                enum_names[desc.tag] = desc
        if kind == "typedef":
            if desc.name not in typedef_names:
                typedef_names[desc.name] = desc
        if kind in ("function", "constant", "variable", "macro"):
            if desc.name not in ident_names:
                ident_names[desc.name] = desc

    # Macros are handled differently from everything else because macros can
    # call other macros that are referenced after them in the input file, but
    # no other type of description can look ahead like that.

    for kind, desc in data.output_order:
        add_to_lookup_table(desc, kind)
        if kind != "macro":
            find_dependencies_for(desc, kind)

    for kind, desc in data.output_order:
        if kind == "macro":
            find_dependencies_for(desc, kind)
