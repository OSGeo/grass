from pathlib import Path


def on_config(config):
    scripts_dir = Path("scripts")
    scripts_tools = []

    # Automatically find all .md files in the scripts directory
    if scripts_dir.exists():
        for filename in scripts_dir.iterdir():
            if filename.endswith(".md"):
                tool_name = filename.replace(".md", "")
                scripts_tools.append(tool_name)

    # Store in MkDocs extra config
    config["extra"]["scripts_tools"] = scripts_tools

    return config
