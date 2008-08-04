/*
 * command esc character
 * set to have most bits (but not all) set to 1
 */

#define COMMAND_ESC 127

/*
 * BEGIN sync count
 * This is number of zero bytes to be returned by
 * driver in response to a BEGIN request
 */

#define BEGIN_SYNC_COUNT 32

/*
 * command tokens
 * note: none should be zero, or equal to COMMAND_ESC
 */

#define GET_NUM_COLORS              1
#define CONT_ABS                    4
#define CONT_REL                    5
#define RGB_COLOR                   6
#define ERASE                       7
#define GET_LOCATION_WITH_BOX       8
#define GET_LOCATION_WITH_LINE      9
#define GET_LOCATION_WITH_POINTER   10
#define GRAPH_CLOSE                 11
#define MOVE_ABS                    13
#define MOVE_REL                    14
#define POLYGON_ABS                 15
#define POLYGON_REL                 16
#define POLYLINE_ABS                17
#define POLYLINE_REL                18
#define POLYDOTS_ABS                19
#define POLYDOTS_REL                20
#define SCREEN_LEFT                 25
#define SCREEN_RITE                 26
#define SCREEN_BOT                  27
#define SCREEN_TOP                  28
#define TEXT                        29
#define TEXT_SIZE                   30
#define TEXT_ROTATION               39
#define SET_WINDOW                  40
#define GET_TEXT_BOX                42
#define FONT                        44
#define RESPOND                     45
#define BEGIN                       46
#define STANDARD_COLOR              47
#define BOX_ABS                     48
#define BOX_REL                     49
#define PANEL_SAVE                  53
#define PANEL_RESTORE               54
#define PANEL_DELETE                55

#define PAD_CREATE                  61
#define PAD_CURRENT                 62
#define PAD_DELETE                  63
#define PAD_INVENT                  64
#define PAD_LIST                    65
#define PAD_SELECT                  66

#define PAD_APPEND_ITEM             71
#define PAD_DELETE_ITEM             72
#define PAD_GET_ITEM                73
#define PAD_LIST_ITEMS              74
#define PAD_SET_ITEM                75

/* fonts */
#define FONT_INFO                   76
#define FONT_LIST                   77
#define CHARSET                     78

/* line attributes */
#define LINE_WIDTH                  79

/* bitmap */
#define BITMAP                      80

/* scaled raster */
#define BEGIN_SCALED_RASTER         84
#define SCALED_RASTER               85
#define END_SCALED_RASTER           86

/* return codes for pad routines */

#define OK 0
#define NO_MEMORY 1
#define NO_PAD 2
#define NO_CUR_PAD 3
#define NO_ITEM 4
#define DUPLICATE 5
#define ILLEGAL 6
