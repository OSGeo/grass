# Load all of the layers, etc only as they are used.
package ifneeded GisM 1.0 [list tclPkgSetup $dir GisM 1.0 {
	{gmtool1.tcl source {GmToolBar1::create}}
	{gmtool2.tcl source {GmToolBar2::create}}
	{maptool.tcl source {MapToolBar::create}}
	{mapcanvas.tcl source {MapCanvas::create}}
	{commonlayer.tcl source {GmCommonLayer::display_command GmCommonLayer::display_commands}}
	{gmtree.tcl source {GmTree::create GmTree::load}}
	{group.tcl source {GmGroup::create GmGroup::display}}
	{cmd.tcl source {GmCmd::create}}
	{vector.tcl source {GmVector::create}}
	{raster.tcl source {GmRaster::create}}
	{labels.tcl source {GmLabels::create}}
	{gridline.tcl source {GmGridline::create}}
	{rgbhis.tcl source {GmRgbhis::create}}
	{histogram.tcl source {GmHist::create}}
	{rastnums.tcl source {GmRnums::create}}
	{rastarrows.tcl source {GmArrows::create}}
	{legend.tcl source {GmLegend::create}}
	{frames.tcl source {GmDframe::create}}
	{barscale.tcl source {GmBarscale::create}}
	{chart.tcl source {GmChart::create}}
	{thematic.tcl source {GmThematic::create}}
	{dtext.tcl source {GmDtext::create}}
	{maptext.tcl source {GmCtext::create}}
	{maplabels.tcl source {GmCLabels::create}}
	{mapprint.tcl source {psprint::init}}
	{profile.tcl source {GmProfile::create}}
	{georect.tcl source {GRMap::startup}}
	{georecttool.tcl source {GRToolBar::create}}
	{rules.tcl source {GmRules::main}}
	{dnviz.tcl source {GmDnviz::main}}
	{animate.tcl source {GmAnim::main}}
}]
