#ifndef WXVDIGIT_DIGIT_H
#define WXVDIGIT_DIGIT_H

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
    int BreakLineAtIntersection(int, struct line_pnts*, int);
    
    int AddActionsBefore(void);
    void AddActionsAfter(int, int);

    /* settings */
    struct _settings {
	bool breakLines;
	bool addCentroid;
	bool catBoundary;
    } settings;

    /* undo/redo */
    enum action_type { ADD, DEL };
    struct action_meta {
	action_type type;
	int line;
	long offset;
    };

    std::map<int, std::vector<action_meta> > changesets;
    int changesetCurrent;  /* first changeset to apply */
    int changesetEnd;      /* last changeset to be applied */
    
    int AddActionToChangeset(int, action_type, int);
    int ApplyChangeset(int, bool);
    void FreeChangeset(int);
    int RemoveActionFromChangeset(int, action_type, int);

public:
    Digit(DisplayDriver *, wxWindow *);
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

    double GetLineLength(int);
    double GetAreaSize(int);
    double GetAreaPerimeter(int);

    int CopyCats(std::vector<int>, std::vector<int>, bool);
    int GetCategory(int);
    std::map<int, std::vector<int> > GetLineCats(int);
    int SetLineCats(int, int, std::vector<int>, bool);
    std::vector<int> GetLayers();

    int Undo(int);
    int GetUndoLevel();

    void UpdateSettings(bool, 
			bool, bool);
};

#endif /* WXVDIGIT_DIGIT_H */
