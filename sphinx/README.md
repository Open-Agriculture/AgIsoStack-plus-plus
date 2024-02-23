# Sphinx Documentation

This folder contains the files that auto-generate the tutorial website using sphinx.

You can build the tutorial yourself if you wish.

## Install Prerequisites

Generate the [doxygen docs](https://agisostack-plus-plus.readthedocs.io/en/latest/Developer%20Guide.html#doxygen):

```bash
doxygen doxyfile
```

Install required python modules:

```bash
pip install -r requirements.txt
```

## Build the Tutorial

Windows:

```bash
./make.bat html
```

Linux:

```bash
sphinx-build -M html source build
```

Then, you can view the documentation at `sphinx\build\html\index.html` in your web browser.
