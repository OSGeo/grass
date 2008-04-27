%typemap(perl5,in) char ** {
	AV *tempav;
	I32 len;
	int i;
	SV **tv;
	if (!SvROK($input))
	croak("$input is not a reference.");
	if (SvTYPE(SvRV($input)) != SVt_PVAV)
	croak("$input is not an array.");
	tempav = (AV*)SvRV($input);
	len = av_len(tempav);
	$1 = (char **) malloc((len+2)*sizeof(char *));
	for (i = 0; i <= len; i++) {
		tv = av_fetch(tempav, i, 0);
		$1[i] = (char *) SvPV_nolen(*tv);
	}
	$1[i] = 0;	
}

%typemap(perl5,freearg) char ** {
	free($input);
}
