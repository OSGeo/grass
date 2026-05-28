/* eslint-env jquery */
/* global $ */
$(document).ready(function() {
    $('#opts_table')
        .fixedHeaderTable(
            {footer : false, cloneHeadToFoot : true, fixedColumn : true});
});
