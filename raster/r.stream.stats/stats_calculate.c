#include "local_proto.h"
double stats_linear_reg(int max_order, double *statistic)
{

    int i;
    double avg_x = 0, avg_y = 0;
    double sum_x_square = 0;
    double sum_x_y = 0;
    double sum_x = 0;
    double avg_x_y = 0;
    double avg_x_square;
    double result;

    for (i = 1; i <= max_order; ++i) {
	avg_y += statistic[i];
	sum_x += i;
	sum_x_square += i * i;
	sum_x_y += i * statistic[i];
    }
    avg_y /= (float)max_order;
    avg_x = sum_x / (float)max_order;
    avg_x_y = sum_x_y / (float)max_order;
    avg_x_square = sum_x_square / max_order;


    result = (avg_x_y - avg_x * avg_y) / (avg_x_square - avg_x * avg_x);
    return result;
}

int stats(int order_max)
{
    int i;
    int num, ord_num;
    float snum, ord_snum;
    float tmp_num;
    double *statistic;

    ord_stats = (STATS *) G_malloc((order_max + 1) * sizeof(STATS));

    stats_total.order = 0;
    stats_total.stream_num = 0;
    stats_total.sum_length = 0.;
    stats_total.avg_length = 0.;
    stats_total.std_length = 0.;
    stats_total.avg_slope = 0.;
    stats_total.std_slope = 0.;
    stats_total.avg_gradient = 0.;
    stats_total.std_gradient = 0.;
    stats_total.sum_area = 0.;
    stats_total.avg_area = 0.;
    stats_total.std_area = 0.;
    stats_total.avg_elev_diff = 0.;
    stats_total.std_elev_diff = 0.;
    stats_total.bifur_ratio = 0.;
    stats_total.std_bifur_ratio = 0.;
    stats_total.reg_bifur_ratio = 0.;
    stats_total.length_ratio = 0.;
    stats_total.std_length_ratio = 0.;
    stats_total.reg_length_ratio = 0.;
    stats_total.area_ratio = 0.;
    stats_total.std_area_ratio = 0.;
    stats_total.reg_area_ratio = 0.;
    stats_total.slope_ratio = 0.;
    stats_total.std_slope_ratio = 0.;
    stats_total.reg_slope_ratio = 0.;
    stats_total.gradient_ratio = 0.;
    stats_total.std_gradient_ratio = 0.;
    stats_total.reg_gradient_ratio = 0.;
    stats_total.stream_frequency = 0.;
    stats_total.drainage_density = 0.;

    for (i = 0; i <= order_max; ++i) {
	ord_stats[i].order = i;
	ord_stats[i].stream_num = 0;
	ord_stats[i].sum_length = 0.;
	ord_stats[i].avg_length = 0.;
	ord_stats[i].std_length = 0.;
	ord_stats[i].avg_slope = 0.;
	ord_stats[i].std_slope = 0.;
	ord_stats[i].avg_gradient = 0.;
	ord_stats[i].std_gradient = 0.;
	ord_stats[i].sum_area = 0.;
	ord_stats[i].avg_area = 0.;
	ord_stats[i].std_area = 0.;
	ord_stats[i].avg_elev_diff = 0.;
	ord_stats[i].std_elev_diff = 0.;
	ord_stats[i].bifur_ratio = 0.;
	ord_stats[i].std_bifur_ratio = 0.;
	ord_stats[i].length_ratio = 0.;
	ord_stats[i].std_length_ratio = 0.;
	ord_stats[i].area_ratio = 0.;
	ord_stats[i].std_area_ratio = 0.;
	ord_stats[i].slope_ratio = 0.;
	ord_stats[i].std_slope_ratio = 0.;
	ord_stats[i].gradient_ratio = 0.;
	ord_stats[i].std_gradient_ratio = 0.;
	ord_stats[i].stream_frequency = 0.;
	ord_stats[i].drainage_density = 0.;
    }
    for (i = 0; i < outlets_num; ++i) {	/* recalculate and unify */
	stat_streams[i].elev_diff =
	    stat_streams[i].elev_spring - stat_streams[i].elev_outlet;
	tmp_num =
	    ((stat_streams[i].cell_num - 1) <
	     1) ? 1 : stat_streams[i].cell_num - 1;
	stat_streams[i].slope /= tmp_num;
	stat_streams[i].gradient =
	    stat_streams[i].elev_diff / stat_streams[i].length;

	/* calculation */
	ord_stats[stat_streams[i].order].stream_num++;
	ord_stats[stat_streams[i].order].sum_length += stat_streams[i].length;
	ord_stats[stat_streams[i].order].std_length +=
	    (stat_streams[i].length * stat_streams[i].length);
	ord_stats[stat_streams[i].order].avg_slope += stat_streams[i].slope;
	ord_stats[stat_streams[i].order].std_slope +=
	    (stat_streams[i].slope * stat_streams[i].slope);
	ord_stats[stat_streams[i].order].avg_gradient +=
	    stat_streams[i].gradient;
	ord_stats[stat_streams[i].order].std_gradient +=
	    (stat_streams[i].gradient * stat_streams[i].gradient);
	ord_stats[stat_streams[i].order].sum_area +=
	    stat_streams[i].basin_area;
	ord_stats[stat_streams[i].order].std_area +=
	    (stat_streams[i].basin_area * stat_streams[i].basin_area);
	ord_stats[stat_streams[i].order].avg_elev_diff +=
	    stat_streams[i].elev_diff;
	ord_stats[stat_streams[i].order].std_elev_diff +=
	    (stat_streams[i].elev_diff * stat_streams[i].elev_diff);
    }

    for (i = 1; i <= order_max; ++i) {

	num = ord_stats[i].stream_num;
	snum = (ord_stats[i].stream_num > 1) ?
	    ((float)ord_stats[i].stream_num) / (ord_stats[i].stream_num -
						1) : 0.;

	ord_stats[i].avg_length = ord_stats[i].sum_length / num;
	ord_stats[i].avg_slope = ord_stats[i].avg_slope / num;
	ord_stats[i].avg_gradient = ord_stats[i].avg_gradient / num;
	ord_stats[i].avg_area = ord_stats[i].sum_area / num;
	ord_stats[i].avg_elev_diff = ord_stats[i].avg_elev_diff / num;

	ord_stats[i].std_length = sqrt((ord_stats[i].std_length / num -
					(ord_stats[i].avg_length *
					 ord_stats[i].avg_length)) * snum);

	ord_stats[i].std_slope = sqrt((ord_stats[i].std_slope / num -
				       (ord_stats[i].avg_slope *
					ord_stats[i].avg_slope)) * snum);

	ord_stats[i].std_gradient = sqrt((ord_stats[i].std_gradient / num -
					  (ord_stats[i].avg_gradient *
					   ord_stats[i].avg_gradient)) *
					 snum);

	ord_stats[i].std_area = sqrt((ord_stats[i].std_area / num -
				      (ord_stats[i].avg_area *
				       ord_stats[i].avg_area)) * snum);

	ord_stats[i].std_elev_diff = sqrt((ord_stats[i].std_elev_diff / num -
					   (ord_stats[i].avg_elev_diff *
					    ord_stats[i].avg_elev_diff)) *
					  snum);

	ord_stats[i - 1].bifur_ratio =
	    ord_stats[i - 1].stream_num / (float)ord_stats[i].stream_num;

	ord_stats[i - 1].length_ratio = (i == 1) ? 0 :
	    ord_stats[i].avg_length / ord_stats[i - 1].avg_length;

	ord_stats[i].area_ratio = (i == 1) ? 0 :
	    ord_stats[i].avg_area / ord_stats[i - 1].avg_area;

	ord_stats[i - 1].slope_ratio =
	    ord_stats[i - 1].avg_slope / ord_stats[i].avg_slope;

	ord_stats[i - 1].gradient_ratio =
	    ord_stats[i - 1].avg_gradient / ord_stats[i].avg_gradient;

	ord_stats[i].stream_frequency =
	    ord_stats[i].stream_num / ord_stats[i].sum_area;

	ord_stats[i].drainage_density =
	    ord_stats[i].sum_length / ord_stats[i].sum_area;

	/* total */
	stats_total.stream_num += ord_stats[i].stream_num;
	stats_total.sum_length += ord_stats[i].sum_length;

	stats_total.bifur_ratio += ord_stats[i - 1].bifur_ratio;
	stats_total.length_ratio += ord_stats[i - 1].length_ratio;
	stats_total.area_ratio += ord_stats[i - 1].area_ratio;
	stats_total.slope_ratio += ord_stats[i - 1].slope_ratio;
	stats_total.gradient_ratio += ord_stats[i - 1].gradient_ratio;

	stats_total.std_bifur_ratio +=
	    (ord_stats[i - 1].bifur_ratio * ord_stats[i - 1].bifur_ratio);
	stats_total.std_length_ratio +=
	    (ord_stats[i - 1].length_ratio * ord_stats[i - 1].length_ratio);
	stats_total.std_area_ratio +=
	    (ord_stats[i - 1].area_ratio * ord_stats[i - 1].area_ratio);
	stats_total.std_slope_ratio +=
	    (ord_stats[i - 1].slope_ratio * ord_stats[i - 1].slope_ratio);
	stats_total.std_gradient_ratio +=
	    (ord_stats[i - 1].gradient_ratio *
	     ord_stats[i - 1].gradient_ratio);

    }				/* end for ... orders */
    ord_num = order_max - 1;
    ord_snum = (ord_num == 1) ? 0 : (float)ord_num / (ord_num - 1);

    stats_total.order = order_max;
    stats_total.sum_area = total_basins;
    stats_total.sum_length = stats_total.sum_length;

    stats_total.bifur_ratio = stats_total.bifur_ratio / ord_num;
    stats_total.length_ratio = stats_total.length_ratio / ord_num;
    stats_total.area_ratio = stats_total.area_ratio / ord_num;
    stats_total.slope_ratio = stats_total.slope_ratio / ord_num;
    stats_total.gradient_ratio = stats_total.gradient_ratio / ord_num;


    stats_total.std_bifur_ratio =
	sqrt((stats_total.std_bifur_ratio / ord_num -
	      (stats_total.bifur_ratio * stats_total.bifur_ratio)) *
	     ord_snum);

    stats_total.std_length_ratio =
	sqrt((stats_total.std_length_ratio / ord_num -
	      (stats_total.length_ratio * stats_total.length_ratio)) *
	     ord_snum);

    stats_total.std_area_ratio = sqrt((stats_total.std_area_ratio / ord_num -
				       (stats_total.area_ratio *
					stats_total.area_ratio)) * ord_snum);

    stats_total.std_slope_ratio =
	sqrt((stats_total.std_slope_ratio / ord_num -
	      (stats_total.slope_ratio * stats_total.slope_ratio)) *
	     ord_snum);

    stats_total.std_gradient_ratio =
	sqrt((stats_total.std_gradient_ratio / ord_num -
	      (stats_total.gradient_ratio * stats_total.gradient_ratio)) *
	     ord_snum);

    stats_total.stream_frequency =
	stats_total.stream_num / stats_total.sum_area;
    stats_total.drainage_density =
	stats_total.sum_length / stats_total.sum_area;


    /* linerar regresion statistics */
    statistic = (double *)G_malloc((order_max + 1) * sizeof(double));

    for (i = 1; i <= order_max; ++i)
	statistic[i] = log10((double)ord_stats[i].stream_num);
    stats_total.reg_bifur_ratio =
	1 / pow(10, stats_linear_reg(order_max, statistic));

    for (i = 1; i <= order_max; ++i)
	statistic[i] = log10((double)ord_stats[i].avg_length);
    stats_total.reg_length_ratio =
	pow(10, stats_linear_reg(order_max, statistic));

    for (i = 1; i <= order_max; ++i)
	statistic[i] = log10((double)ord_stats[i].avg_area);
    stats_total.reg_area_ratio =
	pow(10, stats_linear_reg(order_max, statistic));

    for (i = 1; i <= order_max; ++i)
	statistic[i] = log10((double)ord_stats[i].avg_slope);
    stats_total.reg_slope_ratio =
	1 / pow(10, stats_linear_reg(order_max, statistic));

    for (i = 1; i <= order_max; ++i)
	statistic[i] = log10((double)ord_stats[i].avg_gradient);
    stats_total.reg_gradient_ratio =
	1 / pow(10, stats_linear_reg(order_max, statistic));

    G_free(statistic);

    return 0;
}
