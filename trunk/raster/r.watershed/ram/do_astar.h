#ifndef __DO_ASTAR_H__
#define __DO_ASTAR_H__

#define GET_PARENT(p, c) ((p) = (int) (((c) - 2) / 3 + 1))
#define GET_CHILD(c, p) ((c) = (int) (((p) * 3) - 1))

#endif /* __DO_ASTAR_H__ */
