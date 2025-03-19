<h2>DESCRIPTION</h2>

<p><em>i.svm.predict</em> predicts values with a Support Vector Machine (SVM)
and stores them in a raster file. Predictions are based on a signature file
generated with <a href="i.svm.train.html">i.svm.train</a>.
</p>

<p>Internally the module performs input value rescaling of each of imagery
group rasters by minimum and maximum range determined during training.</p>

<h2>NOTES</h2>

<p><em>i.svm.train</em> internally is using the LIBSVM. For introduction
into value prediction or estimation with LIBSVM, see
<a href="https://www.csie.ntu.edu.tw/~cjlin/papers/guide/guide.pdf">a
Practical Guide to Support Vector Classification</a> by
Chih-Wei Hsu, Chih-Chung Chang, and Chih-Jen Lin.</p>

<p>It is strongly suggested to have semantic labels set for each raster
map in the training data (feature value) and in value prediction imagery groups.
Use <a href="r.support.html">r.support</a> to set semantic labels.</p>

<h2>PERFORMANCE</h2>

<p>Value prediction is done cell by cell and thus memory consumption
should be constant.</p>

<p>The <em>cache</em> parameter determines the maximum memory allocated
for kernel caching to enhance computational speed. It's important to
note that the actual module's memory consumption may vary from this
setting, as it solely impacts LIBSVM's internal caching. The cache is
utilized on an as-needed basis, so it's unlikely to reach the specified value.</p>

<h2>EXAMPLE</h2>

<p>This is the second part of classification process. See
<a href="i.svm.train.html">i.svm.train</a> for the first part.</p>

<p>Predict land use classes form a LANDSAT scene from
October of 2002 with a SVM trained on a 1996 land
use map <em>landuse96_28m</em>.</p>
<div class="code"><pre>
i.svm.predict group=lsat7_2002 subgroup=res_30m \
    signaturefile=landuse96_rnd_points output=pred_landuse_2002
</pre></div>

<h2>SEE ALSO</h2>

<em>
Train SVM: <a href="i.svm.train.html">i.svm.train</a><br>
Set semantic labels: <a href="r.support.html">r.support</a><br>
Other classification modules: <a href="i.maxlik.html">i.maxlik</a>,
<a href="i.smap.html">i.smap</a>
</em><br>
LIBSVM home page: <a href="https://www.csie.ntu.edu.tw/~cjlin/libsvm/">LIBSVM - A
Library for Support Vector Machines</a>

<h2>REFERENCES</h2>

<p>Please cite both - LIBSVM and i.svm.</p>
<ul>
    <li>
        For i.svm.* modules:<br>
        Nartiss, M., &amp; Melniks, R. (2023). Improving pixel-­based classification of GRASS
        GIS with support vector machine. Transactions in GIS, 00, 1–16.
        https://doi.org/10.1111/tgis.13102
    </li>
    <li>
        For LIBSVM:<br>
        Chang, C.-C., &amp; Lin, C.-J. (2011). LIBSVM : a library for support vector machines.
        ACM Transactions on Intelligent Systems and Technology, 2:27:1--27:27.
    </li>
</ul>

<h2>AUTHOR</h2>

Maris Nartiss, University of Latvia.
