<h2>DESCRIPTION</h2>

<em>r.surf.gauss</em> produces a raster map of Gaussian deviates whose
mean and standard deviation can be expressed by the user. It uses a
Gaussian random number generator. It is essentially the same
as <em><a href="r.surf.random.html">r.surf.random</a></em>, but uses a
Gaussian random number generator instead.

<h2>EXAMPLE</h2>

<div class="code"><pre>
g.region -p n=228500 s=215000 w=630000 e=645000 res=10
r.surf.gauss out=gauss mean=0 sigma=10

# check result
r.univar gauss
</pre></div>

<!--
d.legend -d raster=gauss@user1 title=Gauss font=Vera
-->

<div align="center" style="margin: 10px">
<a href="r_surf_gauss.jpg">
<img src="r_surf_gauss.jpg" width="600" height="295" alt="r.surf.gauss example (mean: 0; sigma: 10)" border="0">
</a><br>
<i>Figure: Random Gaussian surface example (mean: 0; sigma: 10)</i>
</div>

<p>
With the histogram tool the cell values versus count can be shown.
<p>
<div align="center" style="margin: 10px">
<a href="r_surf_gauss_hist.png">
<img src="r_surf_gauss_hist.png" width="600" height="275" alt="r.surf.gauss example histogram (mean: 0; sigma: 10)" border="0">
</a><br>
<i>Figure: Histogram of random Gaussian surface example (mean: 0; sigma: 10)</i>
</div>

<h2>SEE ALSO</h2>

<em>
<a href="r.surf.contour.html">r.surf.contour</a>,
<a href="r.surf.fractal.html">r.surf.fractal</a>,
<a href="r.surf.idw.html">r.surf.idw</a>,
<a href="r.surf.random.html">r.surf.random</a>,
<a href="v.surf.rst.html">v.surf.rst</a>
</em>

<h2>AUTHOR</h2>

Jo Wood, <a href="https://web.archive.org/web/20070707015520/http://www.le.ac.uk/assist/index.html">ASSIST's home</a>
