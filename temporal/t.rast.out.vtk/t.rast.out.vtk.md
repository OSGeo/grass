## DESCRIPTION

*t.rast.out.vtk* exports all maps registered in a space time raster
datasets as VTK legacy files using a numerical numbering scheme. The VTK
files can be visualized with any VTK based visualize. Our preferred tool
is ParaView. The VTK legacy files are created using **r.out.vtk**.

## EXAMPLE

In this simple example we create several raster maps with random values
and one elevation map. The random value raster maps are registered in a
space time raster dataset. All maps of this space time raster dataset
are exported using **t.rast.out.vtk** into the directory
**/tmp/export**.

```sh
mkdir /tmp/export
t.rast.out.vtk input=precip_abs directory=/tmp/export/ elevation=elevation

ls -1 /tmp/export
000000_tempmean_monthly.vtk
000001_tempmean_monthly.vtk
000002_tempmean_monthly.vtk
000003_tempmean_monthly.vtk
000004_tempmean_monthly.vtk
000005_tempmean_monthly.vtk
000006_tempmean_monthly.vtk
000007_tempmean_monthly.vtk
000008_tempmean_monthly.vtk
000009_tempmean_monthly.vtk
000010_tempmean_monthly.vtk
000011_tempmean_monthly.vtk
```

## SEE ALSO

*[r.out.vtk](r.out.vtk.md), [ParaView](https://www.paraview.org)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture
