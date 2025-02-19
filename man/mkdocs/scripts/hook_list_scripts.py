from pathlib import Path
import re
from jinja2 import Environment


def github_path(toolname: str, source_docs: dict[str, str]):
    """Return the GitHub path for the given toolname"""
    try:
        return source_docs[toolname]
    except KeyError:
        return None


def on_env(env: Environment, config, files):
    """Enable loopcontrols extension in Jinja2"""
    env.add_extension("jinja2.ext.loopcontrols")
    env.globals["github_path"] = github_path
    return env


def on_config(config):
    """
    Read the list of tools from the source directory and
    store it in MkDocs extra config. These are used to generate
    the correct links to the documentation in GitHub.
    """
    source_dir = Path("source")

    # Dict to store the documentation path for each tool in GitHub
    # the key is the doc name and the value is the path in the GitHub repository
    source_docs = {}

    # Read the source files and extract the GitHub path from the Available at link at the bottom of the page
    pattern = re.compile(
        r"Available at:\s*\[(?P<text>.*?)\]\(https://github\.com/OSGeo/grass/tree/main/(?P<gh_path>.*?)\)"
    )

    for file in source_dir.glob("*.md"):
        with file.open() as f:
            for line in f:
                match = pattern.search(line)
                if match:
                    # Extract the entire link text and the GitHub path.
                    text = match.group("text")
                    gh_path = match.group("gh_path").strip()

                    # Remove the trailing "source code" from the text to get the tool name.
                    toolname = re.sub(
                        r"\s*source\s+code\s*$", "", text, flags=re.IGNORECASE
                    ).strip()
                    source_docs[toolname] = gh_path

    # Store in MkDocs extra config
    config["extra"]["source_docs"] = source_docs
    return config
