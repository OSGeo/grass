u_UPDIR=`pwd`
echo making diffs in directory
SRC_DIR=/sd1h/grass.data/fp_null/r.resample

for i in `ls *.c`
do
  echo $i
  diff -b $i $SRC_DIR/$i
done

for i in `ls *.h`
do
   echo $i
   diff -b $i $SRC_DIR/$i
done

exit 0
