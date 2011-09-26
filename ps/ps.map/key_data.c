int key_data(char *buf, char **k, char **d)
{
    char *key, *data;

    for (key = buf; *key; key++)
	if (*key != ' ' && *key != '\t')
	    break;
    if (*key == 0)
	return 0;
    if (*key == '#')
	return 0;
    for (data = key + 1; *data; data++)
	if (*data == ' ' || *data == '\t')
	    break;
    if (*data)
	*data++ = 0;

    *k = key;
    *d = data;

    return 1;
}
