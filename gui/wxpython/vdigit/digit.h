#ifndef __DIGIT_H__
#define __DIGIT_H__

#define GSQL_MAX 4000

#include <vector>
#include <map>

class Digit
{
private:
    /* layer / max category */
    std::map<int, int> cats;

    DisplayDriver *display;

    int SetCategory(int, int);
    struct Map_info** OpenBackgroundVectorMap(const char *);

    /* undo/redo */
    enum action_type { ADD, DELETE, REWRITE };
    struct action_meta {
	action_type type;
	int line;
	/* TODO: replace by new Vect_restore_line() */
	int ltype; // line type
	struct line_pnts *Points;
	struct line_cats *Cats;
    };
    std::map<int, std::vector<action_meta> > changesets;
    int changesetCurrent;  /* first changeset to apply */
    int changesetDead;     /* first dead changeset */

    int AddActionToChangeset(int, action_type, int);
    int ApplyChangeset(int, bool);
    void FreeChangeset(int);
    int RemoveActionFromChangeset(int, action_type, int);

public:
    Digit(DisplayDriver *);
    ~Digit();

    int InitCats();

    int AddLine(int, std::vector<double>, int, int,
		const char*, int, double);
    int RewriteLine(int, std::vector<double>,
		    const char*, int, double);
    int SplitLine(double, double, double,
		  double);

    int DeleteLines(bool);
    int MoveLines(double, double, double,
		  const char*, int, double);
    int FlipLines();
    int MergeLines();
    int BreakLines();
    int SnapLines(double);
    int ConnectLines(double);
    int TypeConvLines();
    int ZBulkLabeling(double, double, double, double,
		      double, double);
    int CopyLines(std::vector<int>, const char*);

    int MoveVertex(double, double, double,
		   double, double, double,
		   const char*, int,
		   double, double);
    int ModifyLineVertex(int, double, double, double,
			 double);

    std::vector<int> SelectLinesByQuery(double, double, double,
					double, double, double, bool,
					int, int, double);

    int CopyCats(std::vector<std::vector<int> >, std::vector<int>);
    int GetCategory(int);
    std::map<int, std::vector<int> > GetLineCats(int);
    int SetLineCats(int, int, std::vector<int>, bool);
    std::vector<int> GetLayers();

    int Undo(int);
};

#endif /* __DIGIT_H__ */

