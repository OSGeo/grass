
int struct_copy(char *To, char *From, int size)
{
    for (; size; size--)
	*To++ = *From++;

    return 0;
}
