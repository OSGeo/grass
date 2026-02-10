# Contributing to GRASS Translations

**Welcome!** Thank you for your interest in helping translate GRASS.
Your contributions make GRASS accessible to users around the world.

## For translators: How to contribute

**No programming or technical skills required!** All translation work is done through
a web-based interface at [Weblate](https://weblate.osgeo.org/projects/grass-gis/).

### Quick start

1. **Create an [OSGeo UserID](https://www.osgeo.org/community/getting-started-osgeo/osgeo_userid/)**
   (needed to sign in to Weblate)
2. **Sign in** to [OSGeo Weblate](https://weblate.osgeo.org/) with your OSGeo UserID
3. **Choose your language** at the [GRASS GIS project page](https://weblate.osgeo.org/projects/grass-gis/)
4. **Start translating** directly in your browser — no software installation needed!

If your language isn't listed, you can request it to be added through the
Weblate interface. Weblate administrators will set it up for you.

### Need help?

- Read the [Weblate user documentation](https://docs.weblate.org/en/latest/user/translating.html)
  for detailed guidance on using the translation interface
- Ask questions on the [GRASS development forum](https://discourse.osgeo.org/c/grass/developer/61)

Your time and effort are valuable - thank you for contributing!

## For developers: Technical notes

Translations from Weblate are automatically submitted to the GRASS source repository
as pull requests.

GRASS must be configured with `--with-nls` and recompiled to use translated messages.
Updating message catalogs requires a Unix-like system with `gettext` installed.

**Translation portal**: <https://weblate.osgeo.org/projects/grass-gis/>
