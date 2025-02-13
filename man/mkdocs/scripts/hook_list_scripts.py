from pathlib import Path


def on_config(config):
    scripts_dir = Path("gh-paths.yml")
    scripts_tools = []
    # Read the mkdocs.yml file
    with open(scripts_dir) as file:
        for line in file:
            if line.startswith("scripts_tools:"):
                pass
            else:
                toolname = line.split("/")[-1].replace(".md", "").strip()
                scripts_tools.append(toolname)

    # Store in MkDocs extra config
    config["extra"]["scripts_tools"] = scripts_tools
    return config


# if __name__ == "__main__":
#     mock_config = {"extra": {"scripts_tools": []}}
#     script_list = on_config(mock_config)
#     assert len(script_list["extra"]["scripts_tools"]) > 0
