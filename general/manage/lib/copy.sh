:
# copies files or directories
# new file or directory should not exist, but this
# is not enforced - copy a a should be avoided
# files beginning with . are not copied

case $# in
    2) a=$1 ; b=$2 ;;
    *) echo "usage: $0 a b"; exit 1 ;;
esac

if [ -d $a ]
then
    if [ -f $b ]
    then
	rm -f $b
    fi
    if [ ! -d $b ]
    then
	mkdir $b
    fi
    for f in . `cd $a; ls`
    do
	if [ "$f" != "." ]
	then
	    $0 $a/$f $b/$f
	fi
    done
elif [ -f $a ]
then
    cp $a $b
else
    echo $0: $a not found
    exit 2
fi
exit 0
