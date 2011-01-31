#!/bin/bash


rm -f X1 X2 X3 X1.txt X2.txt X3.txt

cat > X1.txt << EOF
1 12
A    1       2      1000   1
A    1       3      20     2
A    1       4      21     8
A    1       5      8      9
A    3       4      1      10
A    3       5      100    11
A    3       11     100    12
A    2       80     8      13
A    2       81     22     14
A    2       5      7      15
A    3       1      22     16
A    3       80     10     17
A    4       5      10     18

A	10		20		10		1020
A	10		30		10		1030
A	10		40		10		1040
A	20		30		10		2030
A	30		40		10		3040
A	40		50		10		4050
A	40		60		10		4060
A	60		50		10		6050
EOF

cat > X2.txt << EOF
2 12
A    1       2      1000   1
A    1       3      20     2
A    1       4      21     8
A    1       5      8      9
A    3       4      1      10
A    3       5      100    11
A    3       11     100    12
A    2       80     8      13
A    2       81     22     14
A    2       5      7      15
A    3       1      22     16
A    3       80     10     17
A    4       5      10     18

A	10		20		10		1020
A	10		30		10		1030
A	10		40		10		1040
A	20		30		10		2030
A	30		40		10		3040
A	40		50		10		4050
A	40		60		10		4060
A	60		50		10		6050
EOF

cat > X3.txt << EOF
3 12
A    1       2      1000   1
A    1       3      20     2
A    1       4      21     8
A    1       5      8      9
A    3       4      1      10
A    3       5      100    11
A    3       11     100    12
A    2       80     8      13
A    2       81     22     14
A    2       5      7      15
A    3       1      22     16
A    3       80     10     17
A    4       5      10     18

A	10		20		10		1020
A	10		30		10		1030
A	10		40		10		1040
A	20		30		10		2030
A	30		40		10		3040
A	40		50		10		4050
A	40		60		10		4060
A	60		50		10		6050
EOF

function check_path () {
	fromNode=$1
	toNode=$2
	totDistance=$3
	nLinks=$4
	nodeList=($5)
	file=$6

	echo "check_path $fromNode -> $toNode - tot. distance $totDistance - n. links $nLinks"

	vlist=(`./shortest_path -g $file -f $fromNode -t $toNode`) || { echo "compute shortest path: command execution failed."; exit 1; }

	test "${vlist[10]}" = "unreachable" && {
		echo "node $toNode is unreachable - test failed"; exit 1
		}
	test "${vlist[12]}" = "$nLinks" || {
		echo "link count is ${vlist[12]} instead of $nLinks - test failed"; exit 1
		}
	test "${vlist[16]}" = "$totDistance" || {
		echo "total distance is ${vlist[16]} instead of $totDistance - test failed"; exit 1
		}

	i=19
	iNodeList=0

	for (( iLink=0 ; iLink < $nLinks ; iLink++ ))
	do
		test ! "${vlist[$i]}" == "${nodeList[$iNodeList]}" && {
			echo "wrong link $iLink head (${vlist[$i]}, ${nodeList[$iNodeList]}) - test failed"; exit 1
			}
		let i=i+2
		let iNodeList=iNodeList+1
		test ! "${vlist[$i]}" == "${nodeList[$iNodeList]}" && {
			echo "wrong link $iLink tail (${vlist[$i]}, ${nodeList[$iNodeList]}) - test failed"; exit 1
			}
		let i=i+17
		let iNodeList=iNodeList+1
	done

	echo "done"
}


#
#
#

echo "script rtest03.sh: test shortest path computations..."

./cr_from_a -f X1.txt -g X1 || { echo "create test graph: command execution failed."; exit 1; }

check_path 1 80 30 2 "1 3 3 80" X1 || exit 1
check_path 3 1 22 1 "3 1" X1 || exit 1

./cr_from_a -f X2.txt -g X2 || { echo "create test graph: command execution failed."; exit 1; }

check_path 1 80 30 2 "1 3 3 80" X2 || exit 1
check_path 3 1 22 1 "3 1" X2 || exit 1

echo "script done"

rm -f X1 X2 X3 X1.txt X2.txt X3.txt
exit 0
