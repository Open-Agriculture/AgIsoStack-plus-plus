import sys
import subprocess, os

# -- Project information -----------------------------------------------------

project = "AgIsoStack++"
copyright = "2022-2023, The Open-Agriculture Developers"
author = "Adrian Del Grosso, Daan Steenbergen"
release = "1.0.0"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "breathe",
    "sphinx.ext.imgmath",
    "sphinx.ext.todo",
    "sphinx.ext.graphviz",
    "sphinxext.opengraph",
    "sphinx_copybutton",
]

templates_path = ["_templates"]
language = "en"
exclude_patterns = []

# -- Options for HTML output -------------------------------------------------

html_theme = "sphinx_rtd_theme"
html_theme_options = {
    "style_external_links": True,
}
html_static_path = ["_static"]
html_css_files = ["custom.css"]

primary_domain = "cpp"
highlight_language = "c++"

# -- Breathe configuration ---------------------------------------------------
sys.path.append("../ext/breathe/")
breathe_projects = {"AgIsoStack": "../doxyxml/"}
breathe_default_project = "AgIsoStack"

# Generate doxygen xml files if on readthedocs
if os.environ.get("READTHEDOCS", None) == "True":
    subprocess.call("cd ../../; doxygen", shell=True)
