# WideField Assembly

**WideField Assembly** is an interactive Unreal Engine project designed to present and teach the structure of a wide-field microscope system. The project combines a 3D model showcase, an assembly animation, and a hands-on free assembly experience.

!\[WideField Assembly Main Menu](docs/images/main-menu.png)

## Project Overview

The project has three main parts:

### 1\. Model Showcase

The **Model** section allows users to explore the complete wide-field microscope system as a 3D model.

Users can:

* View the fully assembled microscope system.
* Inspect the system from different angles.
* Learn about individual optical and mechanical components.
* Highlight or select components to view their names and descriptions.

This mode is intended to help users understand the overall structure of the microscope before learning how it is assembled.

### 2\. Assembly Animation

The **Animation** section demonstrates how the wide-field microscope system is assembled step by step.

Users can:

* Watch the assembly process in the correct order.
* Observe how each component is positioned and connected.
* Use the animation timeline to review different stages of the assembly.
* Read descriptions explaining the purpose of each component or assembly step.

This mode provides a guided visual explanation of the complete assembly process.

### 3\. Free Assembly System

The **Assemble** section is an interactive mode in which users can build the microscope system themselves.

Users can:

* Select available microscope components.
* Move and place components into their correct positions.
* Follow on-screen instructions during the assembly process.
* Learn the correct order and spatial relationship of the components.
* Practice assembling the system without relying only on the animation.

This mode is intended for hands-on learning and self-guided practice.

## Main Menu

The main menu provides access to the following sections:

* **Model** — Opens the 3D model showcase.
* **Animation** — Opens the guided assembly animation.
* **Description** — Displays information about the project and microscope system.
* **Assemble** — Opens the free assembly experience.
* **Instruction** — Displays instructions for using the application.
* **Exit** — Closes the application.

## How to Run the Project

## From Unreal Engine

This project uses **Git Large File Storage (Git LFS)** for Unreal Engine assets such as `.umap` and `.uasset` files. Do not use GitHub's **Download ZIP** option unless the repository archive is configured to include Git LFS objects. A normal ZIP download may contain only LFS pointer files, which can cause Unreal Engine to display errors such as `Failed to load map` or `appears to be an asset file`.

### 1. Install Git and Git LFS

Install Git and Git LFS on your computer. Then open Command Prompt or PowerShell and run:

```bat
git lfs install
```

### 2. Clone the Repository

Choose a folder where you want to store the project, and run:

```bat
cd "your file location"
git clone https://github.com/uw-loci/WideFieldAssembly.git
cd WideFieldAssembly
```

### 3. Download the Git LFS Files

Run:

```bat
git lfs pull
```

To verify that the large Unreal assets were downloaded correctly, run:

```bat
git lfs fsck
```

If no errors are reported, the project assets are ready.

### 4. Open the Unreal Project

1. Open the cloned `WideFieldAssembly` folder.
2. Double-click the `.uproject` file.
3. Select the required Unreal Engine version if prompted.
4. Allow Unreal Engine to compile shaders, modules, or project files if needed.
5. Open the main menu level.
6. Click **Play** to run the project in the Unreal Editor.

### Troubleshooting: Map or Asset Files Will Not Open

If Unreal Engine reports that a `.umap` file “appears to be an asset file,” the repository was probably downloaded as a ZIP without the real Git LFS objects. Delete that extracted folder and clone the repository again using the commands above.

You can also try the following commands inside the cloned project folder:

```bat
git lfs fetch --all
git lfs checkout
git lfs pull
```
### From a Packaged Build

1. Download or extract the packaged project folder.
2. Open the folder containing the executable file.
3. Run the `.exe` file.
4. Use the main menu to select a project mode.

## Basic Interaction

The project uses mouse and keyboard interaction. Detailed controls are available through the **Instruction** section in the main menu.

Typical interactions may include:

* Moving the mouse to point at or highlight a component.
* Clicking a component to select it or display its description.
* Dragging components during the free assembly mode.
* Using the animation controls to play, pause, or review the assembly process.
* Pressing the designated menu or escape key to return to the previous screen.

> Update this section with the exact keyboard and mouse controls used in the final build.

## Project Goals

The goals of this project are to:

* Provide a clear visual introduction to a wide-field microscope system.
* Explain the relationship between optical and mechanical components.
* Demonstrate the correct assembly order.
* Give users an interactive way to practice assembling the system.
* Support education, training, and technical demonstrations.

## Built With

* Unreal Engine
* Unreal Engine Blueprints and/or C++
* 3D assets prepared for real-time visualization
* Interactive UI, animation, and assembly systems

> Add the exact Unreal Engine version and any required plugins before release.

## Project Status

This project is under active development. Features, controls, models, and instructions may change as the project is refined.

## Authors and Acknowledgments

Developed as an interactive visualization and training project for a wide-field microscope assembly system.

Add contributor names, organization information, asset credits, and acknowledgments here.

## License

Add the appropriate license for this project. If the project is private or intended only for internal use, state the usage restrictions here.


## Additional Documentation

 - Repo for Unreal Docs for Engineers [Link](https://github.com/uw-loci/unreal-pipeline-docs)