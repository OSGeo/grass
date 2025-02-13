## DESCRIPTION

This numerical program calculates numerical implicit transient and
steady state solute transport in porous media in the saturated zone of
an aquifer. The computation is based on raster maps and the current
region settings. All initial- and boundary-conditions must be provided
as raster maps. The unit of the coordinate reference system must be
meters.  

This module is sensitive to mask settings. All cells which are outside
the mask are ignored and handled as no flow boundaries.  
This module calculates the concentration of the solution and optional
the velocity field, based on the hydraulic conductivity, the effective
porosity and the initial piecometric heads. The vector components can be
visualized with paraview if they are exported with r.out.vtk.  
  
Use [r.gwflow](r.gwflow.md) to compute the piezometric heights of the
aquifer. The piezometric heights and the hydraulic conductivity are used
to compute the flow direction and the mean velocity of the groundwater.
This is the base of the solute transport computation.  
  
The solute transport will always be calculated transient. For stady
state computation set the timestep to a large number (billions of
seconds).  
  
To reduce the numerical dispersion, which is a consequence of the
convection term and the finite volume discretization, you can use small
time steps and choose between full and exponential upwinding.

## NOTES

The solute transport calculation is based on a diffusion/convection
partial differential equation and a numerical implicit finite volume
discretization. Specific for this kind of differential equation is the
combination of a diffusion/dispersion term and a convection term. The
discretization results in an unsymmetric linear equation system in form
of *Ax = b*, which must be solved. The solute transport partial
differential equation is of the following form:

(dc/dt)\*R = div ( D grad c - uc) + cs -q/nf(c - c_in)

- c -- the concentration \[kg/m^3\]
- u -- vector of mean groundwater flow velocity
- dt -- the time step for transient calculation in seconds \[s\]
- R -- the linear retardation coefficient \[-\]
- D -- the diffusion and dispersion tensor \[m^2/s\]
- cs -- inner concentration sources/sinks \[kg/m^3\]
- c_in -- the solute concentration of influent water \[kg/m^3\]
- q -- inner well sources/sinks \[m^3/s\]
- nf -- the effective porosity \[-\]

Three different boundary conditions are implemented, the Dirichlet,
Transmission and Neumann conditions. The calculation and boundary status
of single cells can be set with the status map. The following states are
supported:

- 0 == inactive - the cell with status 0 will not be calculated, active
  cells will have a no flow boundary to an inactive cell
- 1 == active - this cell is used for sloute transport calculation,
  inner sources can be defined for those cells
- 2 == Dirichlet - cells of this type will have a fixed concentration
  value which do not change over time
- 3 == Transmission - cells of this type should be placed on out-flow
  boundaries to assure the flow of the solute stream out

Note that all required raster maps are read into main memory.
Additionally the linear equation system will be allocated, so the memory
consumption of this module rapidely grow with the size of the input
maps.  
  
The resulting linear equation system *Ax = b* can be solved with several
solvers. Several iterative solvers with unsymmetric sparse and quadratic
matrices support are implemented. The jacobi method, the Gauss-Seidel
method and the biconjugate gradients-stabilized (bicgstab) method.
Additionally a direct Gauss solver and LU solver are available. Those
direct solvers only work with quadratic matrices, so be careful using
them with large maps (maps of size 10.000 cells will need more than one
gigabyte of ram). Always prefer a sparse matrix solver.

## EXAMPLE

Use this small python script to create a working groundwater flow /
solute transport area and data. Make sure you are not in a lat/lon
projection.

```python
#!/usr/bin/env python3
# This is an example script how groundwater flow and solute transport are
# computed within GRASS GIS
import sys
import os
import grass.script as gs

# Overwrite existing maps
gs.run_command("g.gisenv", set="OVERWRITE=1")

gs.message(_("Set the region"))

# The area is 200m x 100m with a cell size of 1m x 1m
gs.run_command("g.region", res=1, res3=1, t=10, b=0, n=100, s=0, w=0, e=200)
gs.run_command("r.mapcalc", expression="phead = if(col() == 1 , 50, 40)")
gs.run_command("r.mapcalc", expression="phead = if(col() ==200  , 45 + row()/40, phead)")
gs.run_command("r.mapcalc", expression="status = if(col() == 1 || col() == 200 , 2, 1)")
gs.run_command("r.mapcalc", expression="well = if((row() == 50 && col() == 175) || (row() == 10 && col() == 135) , -0.001, 0)")
gs.run_command("r.mapcalc", expression="hydcond = 0.00005")
gs.run_command("r.mapcalc", expression="recharge = 0")
gs.run_command("r.mapcalc", expression="top_conf = 20")
gs.run_command("r.mapcalc", expression="bottom = 0")
gs.run_command("r.mapcalc", expression="poros = 0.17")
gs.run_command("r.mapcalc", expression="syield = 0.0001")
gs.run_command("r.mapcalc", expression="null = 0.0")

gs.message(_("Compute a steady state groundwater flow"))

gs.run_command("r.gwflow", solver="cg", top="top_conf", bottom="bottom", phead="phead",\
  status="status", hc_x="hydcond", hc_y="hydcond", q="well", s="syield",\
  recharge="recharge", output="gwresult_conf", dt=8640000000000, type="confined")

gs.message(_("generate the transport data"))
gs.run_command("r.mapcalc", expression="c = if(col() == 15 && row() == 75 , 500.0, 0.0)")
gs.run_command("r.mapcalc", expression="cs = if(col() == 15 && row() == 75 , 0.0, 0.0)")
gs.run_command("r.mapcalc", expression="tstatus = if(col() == 1 || col() == 200 , 3, 1)")
gs.run_command("r.mapcalc", expression="diff = 0.0000001")
gs.run_command("r.mapcalc", expression="R = 1.0")

# Compute the initial state
gs.run_command("r.solute.transport", solver="bicgstab", top="top_conf",\
  bottom="bottom", phead="gwresult_conf", status="tstatus", hc_x="hydcond", hc_y="hydcond",\
  rd="R", cs="cs", q="well", nf="poros", output="stresult_conf_0", dt=3600, diff_x="diff",\
  diff_y="diff", c="c", al=0.1, at=0.01)

# Compute the solute transport for 300 days in 10 day steps
for dt in range(30):
    gs.run_command("r.solute.transport", solver="bicgstab", top="top_conf",\
    bottom="bottom", phead="gwresult_conf", status="tstatus", hc_x="hydcond", hc_y="hydcond",\
    rd="R", cs="cs", q="well", nf="poros", output="stresult_conf_" + str(dt + 1), dt=864000, diff_x="diff",\
    diff_y="diff", c="stresult_conf_" + str(dt), al=0.1, at=0.01)
```

## SEE ALSO

*[r.gwflow](r.gwflow.md)*  
*[r3.gwflow](r3.gwflow.md)*  
*[r.out.vtk](r.out.vtk.md)*  

## AUTHOR

Sören Gebbert

This work is based on the Diploma Thesis of Sören Gebbert available
[here](https://grass.osgeo.org/gdp/hydrology/gebbert2007_diplom_stroemung_grass_gis.pdf)
at Technical University Berlin in Germany.
