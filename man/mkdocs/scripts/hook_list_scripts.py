from pathlib import Path
import re
from jinja2 import Environment


def github_path(toolname: str, source_docs: dict[str, str]):
    """Return the GitHub path for the given toolname"""
    try:
        return source_docs[toolname]
    except KeyError:
        return None


def github_edit_path(ghpath: str, base_repo_url: str):
    """Return the GitHub edit path for the given path"""
    path_parts = ghpath.split("/")

    # Gracefully handle invalid paths
    if len(path_parts) < 3:
        return None
    repo = path_parts[0].strip()
    doc_path = "/".join(path_parts[2:]).strip().rstrip("/")  # Drop repo_name/tree
    return f"{base_repo_url}/{repo}/edit/{doc_path}"


def on_env(env: Environment, config, files):
    """Enable loopcontrols extension in Jinja2"""
    env.add_extension("jinja2.ext.loopcontrols")
    env.globals["github_path"] = github_path
    return env


def set_extra_source_docs(source_dir: str, config: dict):
    """
    Read the list of tools from the source directory and
    store it in MkDocs extra config. These are used to generate
    the correct links to the documentation in GitHub.
    """

    base_repo_url = config["repo_url"]  # Set in mkdocs.yml as repo_url
    source_dir = Path(source_dir)

    # Dict to store the documentation path for each tool in GitHub
    # the key is the doc name and the value is the path in the GitHub repository
    source_docs = {}

    # Read the source files and extract the GitHub path from the Available at link at the bottom of the page
    pattern = re.compile(
        r"Available at:\s*\[(?P<text>.*?)\]\(https://github\.com/OSGeo/(?P<gh_path>.*?)\)"
    )

    for file in source_dir.rglob("*.md"):
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
                    print(f"Found {toolname} at {gh_path}")
                    source_docs[toolname] = github_edit_path(gh_path, base_repo_url)

    # Store in MkDocs extra config
    return source_docs


def on_config(config):
    """
    Runs on MkDocs config to set the extra config with the source_docs
    """

    # Store in MkDocs extra config
    config["extra"]["source_docs"] = set_extra_source_docs("source", config)

    return config
