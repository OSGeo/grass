#include "global.h"


int check_ready(void)
{
    int retval;


    flg.overwrlist = 0;

    retval = 0;

    if (check_required() || check_names() || check_io())
	retval = 1;

    if (!retval) {
	if (flg.overwrlist & FILL)
	    G_remove("cell", map.fill);

	if (flg.overwrlist & DIR)
	    G_remove("cell", map.dir);

	if (flg.overwrlist & BELEV)
	    G_remove("cell", map.belev);

	if (flg.overwrlist & TOPIDX)
	    G_remove("cell", map.topidx);

	if (flg.overwrlist & IDXSTATS)
	    unlink(file.idxstats);

	if (flg.overwrlist & OUTPUT)
	    unlink(file.output);
    }


    return retval;
}


int check_required(void)
{
    int retval;


    retval = 0;

    if (!flg.input) {
	if (!map.elev) {
	    G_warning("elevation required");
	    retval = 1;
	}

	if (!map.basin) {
	    G_warning("basin required");
	    retval = 1;
	}

	if (!map.belev) {
	    G_warning("belevation required");
	    retval = 1;
	}

	if (!map.topidx) {
	    G_warning("topidx required");
	    retval = 1;
	}

	if (map.fill && !map.dir) {
	    G_warning("direction required " "if depressionless is given");
	    retval = 1;
	}

	if (map.dir && !map.fill) {
	    G_warning("depressionless required " "if direction is given");
	    retval = 1;
	}
    }
    else {
	if (map.belev && !map.topidx) {
	    G_warning("topidx required " "if belevation is given");
	    retval = 1;
	}
    }


    return retval;
}


int check_names(void)
{
    int retval;


    retval = 0;

    if (!flg.input) {
	if (map.elev) {
	    if (map.basin && !strcmp(map.elev, map.basin)) {
		G_warning("elevation == basin");
		retval = 1;
	    }

	    if (map.fill && !strcmp(map.elev, map.fill)) {
		G_warning("elevation == " "depressionless");
		retval = 1;
	    }

	    if (map.dir && !strcmp(map.elev, map.dir)) {
		G_warning("elevation == direction");
		retval = 1;
	    }

	    if (map.belev && !strcmp(map.elev, map.belev)) {
		G_warning("elevation == belevation");
		retval = 1;
	    }

	    if (map.topidx && !strcmp(map.elev, map.topidx)) {
		G_warning("elevation == topidx");
		retval = 1;
	    }
	}

	if (map.basin) {
	    if (map.fill && !strcmp(map.basin, map.fill)) {
		G_warning("basin == depressionless");
		retval = 1;
	    }

	    if (map.dir && !strcmp(map.basin, map.dir)) {
		G_warning("basin == direction");
		retval = 1;
	    }

	    if (map.belev && !strcmp(map.basin, map.belev)) {
		G_warning("basin == belevation");
		retval = 1;
	    }

	    if (map.topidx && !strcmp(map.basin, map.topidx)) {
		G_warning("basin == topidx");
		retval = 1;
	    }
	}

	if (map.fill) {
	    if (map.dir && !strcmp(map.fill, map.dir)) {
		G_warning("depressionless == " "direction");
		retval = 1;
	    }

	    if (map.belev && !strcmp(map.fill, map.belev)) {
		G_warning("depressionless == " "belevation");
		retval = 1;
	    }

	    if (map.topidx && !strcmp(map.fill, map.topidx)) {
		G_warning("depressionless == topidx");
		retval = 1;
	    }
	}

	if (map.dir) {
	    if (map.belev && !strcmp(map.dir, map.belev)) {
		G_warning("direction == belevation");
		retval = 1;
	    }

	    if (map.topidx && !strcmp(map.dir, map.topidx)) {
		G_warning("direction == topidx");
		retval = 1;
	    }
	}
    }

    if (map.belev) {
	if (map.topidx && !strcmp(map.belev, map.topidx)) {
	    G_warning("belevation == topidx");
	    retval = 1;
	}
    }

    if (!strcmp(file.idxstats, file.params)) {
	G_warning("idxstats == parameters");
	retval = 1;
    }

    if (!strcmp(file.idxstats, file.input)) {
	G_warning("idxstats == input");
	retval = 1;
    }

    if (!strcmp(file.idxstats, file.output)) {
	G_warning("idxstats == output");
	retval = 1;
    }

    if (file.Qobs && !strcmp(file.idxstats, file.Qobs)) {
	G_warning("idxstats == Qobs");
	retval = 1;
    }

    if (!strcmp(file.params, file.input)) {
	G_warning("parameters == input");
	retval = 1;
    }

    if (!strcmp(file.params, file.output)) {
	G_warning("parameters == output");
	retval = 1;
    }

    if (file.Qobs && !strcmp(file.params, file.Qobs)) {
	G_warning("parameters == Qobs");
	retval = 1;
    }

    if (!strcmp(file.input, file.output)) {
	G_warning("input == output");
	retval = 1;
    }

    if (file.Qobs && !strcmp(file.input, file.Qobs)) {
	G_warning("input == Qobs");
	retval = 1;
    }

    if (file.Qobs && !strcmp(file.output, file.Qobs)) {
	G_warning("output == Qobs");
	retval = 1;
    }


    return retval;
}


int check_io(void)
{
    int retval;
    FILE *fp;


    retval = 0;

    if (!flg.input) {
	if (map.elev && !G_find_file("cell", map.elev, mapset)) {
	    G_warning("%s - not exists", map.elev);
	    retval = 1;
	}

	if (map.basin && !G_find_file("cell", map.basin, mapset)) {
	    G_warning("%s - not exists", map.basin);
	    retval = 1;
	}

	if (map.fill && G_find_file("cell", map.fill, mapset)) {
	    if (flg.overwr) {
		flg.overwrlist |= FILL;
	    }
	    else {
		G_warning("%s - already exists", map.fill);
		retval = 1;
	    }
	}

	if (map.dir && G_find_file("cell", map.dir, mapset)) {
	    if (flg.overwr) {
		flg.overwrlist |= DIR;
	    }
	    else {
		G_warning("%s - already exists", map.dir);
		retval = 1;
	    }
	}

	if (map.belev && G_find_file("cell", map.belev, mapset)) {
	    if (flg.overwr) {
		flg.overwrlist |= BELEV;
	    }
	    else {
		G_warning("%s - already exists", map.belev);
		retval = 1;
	    }
	}

	if (map.topidx && G_find_file("cell", map.topidx, mapset)) {
	    if (flg.overwr) {
		flg.overwrlist |= TOPIDX;
	    }
	    else {
		G_warning("%s - already exists", map.topidx);
		retval = 1;
	    }
	}

	if (file.idxstats && (fp = fopen(file.idxstats, "r"))) {
	    fclose(fp);
	    if (flg.overwr) {
		flg.overwrlist |= IDXSTATS;
	    }
	    else {
		G_warning("%s - file already exists", file.idxstats);
		retval = 1;
	    }
	}
    }
    else {
	if (map.belev) {
	    if (!G_find_file("cell", map.belev, mapset)) {
		G_warning("%s - not exists", map.belev);
		retval = 1;
	    }
	    else {
		if (map.topidx && G_find_file("cell", map.topidx, mapset)) {
		    if (flg.overwr) {
			flg.overwrlist |= TOPIDX;
		    }
		    else {
			G_warning("%s - " "already exists", map.topidx);
			retval = 1;
		    }
		}

		if (file.idxstats && (fp = fopen(file.idxstats, "r"))) {
		    fclose(fp);
		    if (flg.overwr) {
			flg.overwrlist |= IDXSTATS;
		    }
		    else {
			G_warning("%s - file "
				  "already exists", file.idxstats);
			retval = 1;
		    }
		}
	    }
	}
	else if (map.topidx) {
	    if (!G_find_file("cell", map.topidx, mapset)) {
		G_warning("%s - not exists", map.topidx);
		retval = 1;
	    }
	    else {
		if (file.idxstats && (fp = fopen(file.idxstats, "r"))) {
		    fclose(fp);
		    if (flg.overwr) {
			flg.overwrlist |= IDXSTATS;
		    }
		    else {
			G_warning("%s - file "
				  "already exists", file.idxstats);
			retval = 1;
		    }
		}
	    }
	}
	else if (file.idxstats) {
	    if (!(fp = fopen(file.idxstats, "r"))) {
		G_warning("%s - file not exists", file.idxstats);
		retval = 1;
	    }
	    else {
		fclose(fp);
	    }
	}
    }

    if (file.params) {
	if (!(fp = fopen(file.params, "r"))) {
	    G_warning("%s - file not exists", file.params);
	    retval = 1;
	}
	else {
	    fclose(fp);
	}
    }

    if (file.input) {
	if (!(fp = fopen(file.input, "r"))) {
	    G_warning("%s - file not exists", file.input);
	    retval = 1;
	}
	else {
	    fclose(fp);
	}
    }

    if (file.output && (fp = fopen(file.output, "r"))) {
	fclose(fp);
	if (flg.overwr) {
	    flg.overwrlist |= OUTPUT;
	}
	else {
	    G_warning("%s - file already exists", file.output);
	    retval = 1;
	}
    }

    if (file.Qobs) {
	if (!(fp = fopen(file.Qobs, "r"))) {
	    G_warning("%s - file not exists", file.Qobs);
	    retval = 1;
	}
	else {
	    fclose(fp);
	}
    }


    return retval;
}
