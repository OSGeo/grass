---
authors:
    - Linda Karlovska
    - GRASS Development Team
---

# Using Jupyter Notebooks from GUI

Starting with GRASS version 8.5, the GUI provides integrated support for launching
and managing Jupyter Notebooks directly from the interface.
This allows you to seamlessly combine interactive Python notebooks
with your GUI workflow.

## Getting Started

To launch Jupyter from GUI, go to **File → Jupyter Notebook** or
click the Jupyter button in the Tools toolbar at the top of the GRASS window.
A startup dialog will appear where you can configure your notebook environment:

![Jupyter Startup Dialog](jupyter_startup_dialog.png)

### Configuration Options

- **Where to Save Notebooks:** Select where your Jupyter notebooks will be stored.
  You can choose an existing directory, create a new one, or leave the field empty
  to use your current directory. Notebooks can be saved anywhere,
  including inside your current GRASS project.

- **Create Welcome Notebook:** Check this option to automatically create
  a `welcome.ipynb` template notebook with GRASS-specific examples and
  quick-start code. Recommended for new users.

### Display Modes

After configuring the storage, choose how to interact with Jupyter notebooks:

#### Browser Mode

![Browser Mode - Jupyter opened in your default web browser](jupyter_browser_mode.png)

In Browser Mode, Jupyter opens in your system's default web browser.

This mode provides:

- Full Jupyter Lab/Notebook interface with all features
- File browser and notebook management
- Terminal access and extensions support
- A control panel in GRASS GUI showing server URL, PID, and storage location
- Quick access to reopen the browser or stop the server

#### Integrated Mode

![Integrated Mode - Jupyter embedded directly in GRASS GUI](jupyter_integrated_mode.png)

In Integrated Mode, Jupyter notebooks are embedded directly in the GRASS GUI window
(if the wx.html2 library is available). This mode offers:

- Jupyter interface embedded as a native GRASS GUI tab
- Seamless integration with other GRASS tools and panels
- Import/export notebooks, and create new notebooks from the toolbar
- External links (documentation, tutorials) open in your system browser

### Toolbar Actions

The integrated mode toolbar provides quick access to common operations:

- **Create:** Create a new notebook with prepared GRASS module imports and session
initialization
- **Import:** Import existing .ipynb files into your notebook storage
- **Export:** Export the current notebook to a different location
- **Undock:** Open the notebook in a separate window (also available in
the browser mode)
- **Stop:** Stop the Jupyter server and close the notebook (also available in
the browser mode)

### Multiple Notebook Sessions

You can launch multiple Jupyter sessions with different storage locations.
Each session appears as a separate tab in the GRASS GUI, with the storage
location shown in the tab name. Hover over a tab to see the full storage path.

### Server Management

GRASS automatically manages Jupyter server instances:

- Each notebook storage location runs on a separate Jupyter server
- Servers are automatically stopped when GRASS exits
- Multiple notebooks from the same storage share one server instance
- Server information (URL, PID) is displayed in the interface

### Tips

- The `welcome.ipynb` template includes GRASS session initialization
  and practical examples of listing data, visualizing maps,
  and running analyses with the Tools API

- You can switch between browser and integrated modes by closing one and
  relaunching Jupyter with the same storage location

- Tab tooltips show the full storage path—useful when working with
  multiple storage locations
