int Cdhc_dcmp(const void *i, const void *j)
{
    double x = *(double *)i;
    double y = *(double *)j;

    if (x < y)
	return -1;

    if (x > y)
	return 1;

    return 0;
}
