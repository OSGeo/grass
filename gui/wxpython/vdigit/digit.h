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
    wxWindow *parentWin;
    
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

    /* settings */
    struct _settings {
	bool breakLines;
    } settings;

    std::map<int, std::vector<action_meta> > changesets;
    int changesetCurrent;  /* first changeset to apply */
    int changesetDead;     /* first dead changeset */

    int AddActionToChangeset(int, action_type, int);
    int ApplyChangeset(int, bool);
    void FreeChangeset(int);
    int RemoveActionFromChangeset(int, action_type, int);

    /* message dialogs */
    wxString msgCaption;
    void DisplayMsg(void);
    void Only2DMsg(void);
    void ReadLineMsg(int);
    void DeadLineMsg(int);
    void WriteLineMsg(void);
    void BackgroundMapMsg(const char *);
    void DblinkMsg(int);
    void DbDriverMsg(const char *);
    void DbDatabaseMsg(const char *, const char *);
    void DbExecuteMsg(const char *);
    void GetLineCatsMsg(int);

public:
    Digit(DisplayDriver *, void *);
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

    int CopyCats(std::vector<int>, std::vector<int>);
    int GetCategory(int);
    std::map<int, std::vector<int> > GetLineCats(int);
    int SetLineCats(int, int, std::vector<int>, bool);
    std::vector<int> GetLayers();

    int Undo(int);
    int GetUndoLevel();

    void UpdateSettings(bool);
};

#endif /* WXVDIGIT_DIGIT_H */
