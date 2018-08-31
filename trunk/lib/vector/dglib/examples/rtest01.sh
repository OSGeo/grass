#!/bin/sh

#
# This test captures correctness of flattening/unflattening operations
# asserting that the input graph to ./unflatten be identical to its
# output.
# There are a number of implicit tests here:
# - When a graph is unflattened all information kept in the buffers
#   are moved to an avl-tree using gnGrpAddLink(). Link insertions
#   identical to those made by a user program are performed.
#   Before returning gnGrpUnflatten() destroys the buffers.
# - When flattening buffers are rebuilt from the avl-tree.
#

rm -f A B A.txt B.txt

echo "create a version-1 digraph and save it to 'A'"
(./cr_large_graph -g A > /dev/null) || (echo "error"; return 1) || exit 1
echo "done"

echo "read 'A'; unflatten ; flatten back again and save it to 'B'"
(./unflatten -g A -o B > /dev/null) || (echo "error"; return 1) || exit 1
echo "done"

echo "convert 'A' to 'A.txt'"
(./view -g A > A.txt) || (echo "error"; return 1) || exit 1
echo "done"

echo "convert 'B' to 'B.txt'"
(./view -g B > B.txt) || (echo "error"; return 1) || exit 1
echo "done"

echo "compare 'A.txt' with 'B.txt'"
(diff -q A.txt B.txt && \
	 echo "'A.txt' and 'B.txt' are identical") ||
	(echo "'A.txt' and 'B.txt' differ"; exit 1) || exit 1

rm -f A B A.txt B.txt

echo "create a version-2 digraph and save it to 'A'"
(./cr_large_graph -g A -v 2 > /dev/null) || (echo "error"; return 1) || exit 1
echo "done"

echo "read 'A'; unflatten ; flatten back again and save it to 'B'"
(./unflatten -g A -o B > /dev/null) || (echo "error"; return 1) || exit 1
echo "done"

echo "convert 'A' to 'A.txt'"
(./view -g A > A.txt) || (echo "error"; return 1) || exit 1
echo "done"

echo "convert 'B' to 'B.txt'"
(./view -g B > B.txt) || (echo "error"; return 1) || exit 1
echo "done"

echo "compare 'A.txt' with 'B.txt'"
(diff -q A.txt B.txt && \
	 echo "'A.txt' and 'B.txt' are identical") ||
	(echo "'A.txt' and 'B.txt' differ"; exit 1) || exit 1

rm -f A B A.txt B.txt

echo "create a version-3 graph and save it to 'A'"
(./cr_large_graph -g A -v 2 > /dev/null) || (echo "error"; return 1) || exit 1
echo "done"

echo "read 'A'; unflatten ; flatten back again and save it to 'B'"
(./unflatten -g A -o B > /dev/null) || (echo "error"; return 1) || exit 1
echo "done"

echo "convert 'A' to 'A.txt'"
(./view -g A > A.txt) || (echo "error"; return 1) || exit 1
echo "done"

echo "convert 'B' to 'B.txt'"
(./view -g B > B.txt) || (echo "error"; return 1) || exit 1
echo "done"

echo "compare 'A.txt' with 'B.txt'"
(diff -q A.txt B.txt && \
	 echo "'A.txt' and 'B.txt' are identical") ||
	(echo "'A.txt' and 'B.txt' differ"; exit 1) || exit 1

rm -f A B A.txt B.txt

exit 0
