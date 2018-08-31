int uppercase(char *s)
{
    for (; *s; s++)
	if (*s >= 'a' && *s <= 'z')
	    *s += 'A' - 'a';

    return 0;
}
