#include "local_proto.h"

int print_stats(int order_max)
{

    int i;

    fflush(stdout);
    /* summary statistics */
    fprintf(stdout, "\n");
    fprintf(stdout, "Summary:\n");
    fprintf(stdout,
	    "Max order | Tot.N.str. | Tot.str.len. | Tot.area. | Dr.dens. | Str.freq. \n");
    fprintf(stdout,
	    "  (num)   |    (num)   |     (km)     |   (km2)   | (km/km2) | (num/km2) \n");
    fprintf(stdout, " %8d | %10d | %12.4f | %9.4f | %8.4f | %7.4f \n",
	    stats_total.order, stats_total.stream_num,
	    stats_total.sum_length / 1000, stats_total.sum_area / 1000000,
	    stats_total.drainage_density * 1000,
	    stats_total.stream_frequency * 1000000);

    fprintf(stdout, "\n");
    fprintf(stdout, "Stream ratios based on regresion coefficient:\n");
    fprintf(stdout, " Bif.rt. | Len.rt. | Area.rt. | Slo.rt. | Grd.rt. \n");
    fprintf(stdout, " %7.4f | %7.4f | %8.4f | %7.4f | %7.4f\n",
	    stats_total.reg_bifur_ratio,
	    stats_total.reg_length_ratio,
	    stats_total.reg_area_ratio,
	    stats_total.reg_slope_ratio, stats_total.reg_gradient_ratio);
    fprintf(stdout, "\n");
    fprintf(stdout, "Avaraged stream ratios with standard deviations:\n");
    fprintf(stdout, " Bif.rt. | Len.rt. | Area.rt. | Slo.rt. | Grd.rt. \n");
    fprintf(stdout, " %7.4f | %7.4f | %8.4f | %7.4f | %7.4f\n",
	    stats_total.bifur_ratio,
	    stats_total.length_ratio,
	    stats_total.area_ratio,
	    stats_total.slope_ratio, stats_total.gradient_ratio);

    fprintf(stdout, " %7.4f | %7.4f | %8.4f | %7.4f | %7.4f\n",
	    stats_total.std_bifur_ratio,
	    stats_total.std_length_ratio,
	    stats_total.std_area_ratio,
	    stats_total.std_slope_ratio, stats_total.std_gradient_ratio);
    fprintf(stdout, "\n");

    /* base parameters */
    fprintf(stdout,
	    "Order | Avg.len |  Avg.ar  |  Avg.sl |  Avg.grad. | Avg.el.dif\n");
    fprintf(stdout,
	    " num  |   (km)  |  (km2)   |  (m/m)  |    (m/m)   |     (m)   \n");
    for (i = 1; i <= order_max; ++i) {
	fprintf(stdout, "%5d | %7.4f | %8.4f | %7.4f | %10.4f | %7.4f\n",
		ord_stats[i].order,
		ord_stats[i].avg_length / 1000,
		ord_stats[i].avg_area / 1000000,
		ord_stats[i].avg_slope,
		ord_stats[i].avg_gradient, ord_stats[i].avg_elev_diff);
    }
    fprintf(stdout, "\n");
    /* std dev of base parameters */
    fprintf(stdout,
	    "Order | Std.len |  Std.ar  |  Std.sl |  Std.grad. | Std.el.dif\n");
    fprintf(stdout,
	    " num  |   (km)  |  (km2)   |  (m/m)  |    (m/m)   |     (m)   \n");
    for (i = 1; i <= order_max; ++i) {
	fprintf(stdout, "%5d | %7.4f | %8.4f | %7.4f | %10.4f | %7.4f\n",
		ord_stats[i].order,
		ord_stats[i].std_length / 1000,
		ord_stats[i].std_area / 1000000,
		ord_stats[i].std_slope,
		ord_stats[i].std_gradient, ord_stats[i].std_elev_diff);
    }
    /* sum statistics of orders */
    fprintf(stdout, "\n");
    fprintf(stdout, "Order | N.streams | Tot.len (km) | Tot.area (km2)\n");
    for (i = 1; i <= order_max; ++i) {
	fprintf(stdout, "%5d | %9d | %12.4f | %7.4f\n",
		ord_stats[i].order,
		ord_stats[i].stream_num,
		ord_stats[i].sum_length / 1000,
		ord_stats[i].sum_area / 1000000);
    }
    /* order ratios */
    fprintf(stdout, "\n");
    fprintf(stdout,
	    "Order | Bif.rt. | Len.rt. | Area.rt. | Slo.rt. | Grd.rt. | d.dens. | str.freq.\n");
    for (i = 1; i <= order_max; ++i) {
	fprintf(stdout,
		"%5d | %7.4f | %7.4f | %8.4f | %7.4f | %7.4f | %7.4f | %7.4f\n",
		ord_stats[i].order, ord_stats[i].bifur_ratio,
		ord_stats[i].length_ratio, ord_stats[i].area_ratio,
		ord_stats[i].slope_ratio, ord_stats[i].gradient_ratio,
		ord_stats[i].drainage_density * 1000,
		ord_stats[i].stream_frequency * 1000000);
    }
    fflush(stdout);
    return 0;
}

int print_stats_total(void)
{
    fflush(stdout);
    fprintf(stdout, "Catchment's characteristics (based on regresion):  \n");
    fprintf(stdout, "Max order: %d \n", stats_total.order);
    fprintf(stdout, "Total number of streams: %d \n", stats_total.stream_num);
    fprintf(stdout, "Total stream length (km): %2.4f \n",
	    stats_total.sum_length / 1000);
    fprintf(stdout, "Total cachment area (km2): %2.4f \n",
	    stats_total.sum_area / 1000000);
    fprintf(stdout, "Drainage density: %2.4f\n",
	    stats_total.drainage_density * 1000);
    fprintf(stdout, "Stream frequency: %2.4f \n",
	    stats_total.stream_frequency * 1000000);
    fprintf(stdout, "Bifurcation ratio: %2.4f \n",
	    stats_total.reg_bifur_ratio);
    fprintf(stdout, "Length ratio: %2.4f \n", stats_total.reg_length_ratio);
    fprintf(stdout, "Area ratio: %2.4f \n", stats_total.reg_area_ratio);
    fprintf(stdout, "Slope ratio: %2.4f \n", stats_total.reg_slope_ratio);
    fprintf(stdout, "Gradient ratio: %2.4f \n",
	    stats_total.reg_gradient_ratio);
    fflush(stdout);
    return 0;
}

int print_stats_orders(int order_max)
{

    int i;

    fflush(stdout);
    fprintf(stdout, "Order's summary: \n");
    fprintf(stdout,
	    "order,num_of_streams,avg_length,avg_area,avg_slope,avg_grad,avg_elev.diff,sum_length,sum_area\n");

    for (i = 1; i <= order_max; ++i) {
	fprintf(stdout, "%d,%d,%f,%f,%f,%f,%f,%f,%f\n",
		ord_stats[i].order,
		ord_stats[i].stream_num,
		ord_stats[i].avg_length / 1000,
		ord_stats[i].avg_area / 1000000,
		ord_stats[i].avg_slope,
		ord_stats[i].avg_gradient,
		ord_stats[i].avg_elev_diff,
		ord_stats[i].sum_length / 1000,
		ord_stats[i].sum_area / 1000000);
    }
    fflush(stdout);
    return 0;
}
