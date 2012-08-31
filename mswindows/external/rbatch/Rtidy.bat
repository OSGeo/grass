#Rscript %0.bat %*
# usage: Rtidy sourcefile.R > sourcefile.Rtidy.R
# based on 3.1 of R Extensions Manual
options(keep.source = FALSE)
source(commandArgs(TRUE))
dump(ls(all = TRUE), file = stdout())
q("no")

