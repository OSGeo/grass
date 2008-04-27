#!/bin/sh

#
# 1. create graph A
# 2. convert A to A.txt
# 3. import A.txt to B
# 4. convert B to B.txt
# 5. compare A.txt and B.txt
#

rm -f A A.txt B B.txt

echo "create a large graph and save it to 'A'"
(./cr_large_graph -g A > /dev/null) || (echo "error"; return 1) || exit 1
echo "done"

echo "convert 'A' to 'A.txt'"
(./view -g A > A.txt) || (echo "error"; return 1) || exit 1
echo "done"

echo "import 'A.txt' to 'B'"
(./parse -i A.txt -o B) || (echo "error"; return 1) || exit 1
echo "done"

echo "convert 'B' to 'B.txt'"
(./view -g B > B.txt) || (echo "error"; return 1) || exit 1
echo "done"

echo "compare 'A.txt' with 'B.txt'"
(diff -q A.txt B.txt && \
	 echo "'A.txt' and 'B.txt' are identical") ||
	(echo "'A.txt' and 'B.txt' differ"; exit 1) || exit 1
exit 0
