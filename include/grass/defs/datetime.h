#ifndef GRASS_DATETIMEDEFS_H
#define GRASS_DATETIMEDEFS_H

/* between.c */
int datetime_is_between(int x, int a, int b);

/* change.c */
int datetime_change_from_to(DateTime * dt, int from, int to, int round);

/* copy.c */
void datetime_copy(DateTime * src, const DateTime * dst);

/* diff.c */
int datetime_difference(const DateTime * a, const DateTime * b,
			DateTime * result);

/* error.c */
int datetime_error(int code, char *msg);
int datetime_error_code(void);
char *datetime_error_msg(void);
void datetime_clear_error(void);

/* format.c */
int datetime_format(const DateTime * dt, char *buf);

/* incr1.c */
int datetime_increment(DateTime * src, DateTime * incr);

/* incr2.c */
int datetime_is_valid_increment(const DateTime * src, const DateTime * incr);
int datetime_check_increment(const DateTime * src, const DateTime * incr);

/* incr3.c */
int datetime_get_increment_type(const DateTime * dt, int *mode, int *from,
				int *to, int *fracsec);
int datetime_set_increment_type(const DateTime * src, DateTime * incr);

/* local.c */
int datetime_get_local_timezone(int *minutes);
void datetime_get_local_time(DateTime * dt);

/* misc.c */
int datetime_days_in_month(int year, int month, int ad);
int datetime_is_leap_year(int year, int ad);
int datetime_days_in_year(int year, int ad);

/* same.c */
int datetime_is_same(const DateTime * src, const DateTime * dst);

/* scan.c */
int datetime_scan(DateTime * dt, const char *buf);

/* sign.c */
int datetime_is_positive(const DateTime * dt);
int datetime_is_negative(const DateTime * dt);
void datetime_set_positive(DateTime * dt);
void datetime_set_negative(DateTime * dt);
void datetime_invert_sign(DateTime * dt);

/* type.c */
int datetime_set_type(DateTime * dt, int mode, int from, int to, int fracsec);
int datetime_get_type(const DateTime * dt, int *mode, int *from, int *to,
		      int *fracsec);
int datetime_is_valid_type(const DateTime * dt);
int datetime_check_type(const DateTime * dt);
int datetime_in_interval_year_month(int x);
int datetime_in_interval_day_second(int x);
int datetime_is_absolute(const DateTime * dt);
int datetime_is_relative(const DateTime * dt);

/* tz1.c */
int datetime_check_timezone(const DateTime * dt, int minutes);
int datetime_get_timezone(const DateTime * dt, int *minutes);
int datetime_set_timezone(DateTime * dt, int minutes);
int datetime_unset_timezone(DateTime * dt);
int datetime_is_valid_timezone(int minutes);

/* tz2.c */
int datetime_change_timezone(DateTime * dt, int minutes);
int datetime_change_to_utc(DateTime * dt);
void datetime_decompose_timezone(int tz, int *hours, int *minutes);

/* values.c */
int datetime_check_year(const DateTime * dt, int year);
int datetime_check_month(const DateTime * dt, int month);
int datetime_check_day(const DateTime * dt, int day);
int datetime_check_hour(const DateTime * dt, int hour);
int datetime_check_minute(const DateTime * dt, int minute);
int datetime_check_second(const DateTime * dt, double second);
int datetime_check_fracsec(const DateTime * dt, int fracsec);
int datetime_get_year(const DateTime * dt, int *year);
int datetime_set_year(DateTime * dt, int year);
int datetime_get_month(const DateTime * dt, int *month);
int datetime_set_month(DateTime * dt, int month);
int datetime_get_day(const DateTime * dt, int *day);
int datetime_set_day(DateTime * dt, int day);
int datetime_get_hour(const DateTime * dt, int *hour);
int datetime_set_hour(DateTime * dt, int hour);
int datetime_get_minute(const DateTime * dt, int *minute);
int datetime_set_minute(DateTime * dt, int minute);
int datetime_get_second(const DateTime * dt, double *second);
int datetime_set_second(DateTime * dt, double second);
int datetime_get_fracsec(const DateTime * dt, int *fracsec);
int datetime_set_fracsec(DateTime * dt, int fracsec);

#endif /* GRASS_DATETIMEDEFS_H */
