/*
 * Copyright (C) 2013 Ryan Kavanagh <rak@debian.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>
#include <curses.h>
#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>

/*
 * __dead isn't defined everywhere; although it's typically installed
 * in sys/cdefs.h. The following is based off of OpenBSD's
 * sys/cdefs.h
 */
#if !defined(__dead) && defined(__dead2)
#define __dead	__dead2
#elif !defined(__dead) && defined(__GNUC__) && !defined(__STRICT_ANSI__)
#define __dead	__volatile
#elif !defined(__dead) && !defined(__STRICT_ANSI__)
#define __dead	__atribute__((__noreturn__))
#elif !defined(__dead)
#define __dead	/* NORETURN */
#endif

#define VERSION "1.0~beta1"

#define EMPTY 'E'
#define WHITEC 'W'
#define BLACKC 'B'
#define WHITE 0
#define BLACK 1
#define NORTH 0
#define SOUTH 1
#define WEST 2
#define EAST 3
#define NE 4
#define SW 5
#define NW 6
#define SE 7
#define MINDIR NORTH
#define MAXDIR SE
#define NORTHC "n"
#define EASTC "e"
#define SOUTHC "s"
#define WESTC "w"
#define NEC "ne"
#define NWC "nw"
#define SEC "se"
#define SWC "sw"

#define ROWS 3
#define COLS 9

#define NMM 4
#define TMM 3
#define TWMM 12

#define sbrow 3         /* score box */
#define sbcol 37
#define brdrow 2        /* board     */
#define brdcol 4
#define legendsep 3     /* distance board<->legend */
#define msgrow 20       /* message box */
#define msgcol 7
#define promptcol 16
#define promptrow 7
#define versrow 17      /* version col calculated based on helpcol */
#define helpcol 65
#define helprow 0

/* Board & game construction */
typedef struct point {
  char v; /* Value: what's currently here */
  struct point* n[8]; /* Our neighbours, indexed by NORTH, SOUTH, WEST, EAST,
			 NE, SW, NW, SE, see dirc below */
} point;

typedef point* nboard[ROWS][COLS];

typedef struct game {
  nboard board;
  int state;
  int pieces[2];
  int totalpieces;
  int phase;
  int type;
} game;

typedef struct scrgame {
  struct game *game;
  WINDOW *score_w;
  WINDOW *board_w;
  WINDOW *msg_w;
} scrgame;

/* Let's make looking up directions -> indices easier */
struct strkey { char *k; int v; };

static const struct strkey dirc[] = {
  { NORTHC , NORTH }, { SOUTHC, SOUTH }, { EASTC, EAST }, { WESTC, WEST },
  { NEC, NE }, { NWC, NW }, { SEC, SE }, { SWC, SW },
  { 0 } /* GCC will incorrectly complain about this, GCC BUG#53119 */
};

__BEGIN_DECLS
int	 joinpts(const int, point *, point *);
void	 joinns(point *, point *);
void	 joinwe(point *, point *);
void	 joinnesw(point *, point *);
void	 joinnwse(point *, point *);
void	 freepoint(point *);
int	 dirtoindex(const char *);
point	 makepoint(const char, point *, point *,
		   point *, point *);
void	 updatepoint(point *, const char, point *, point *,
		     point *, point *);
void	 init3board(nboard);
void	 init9board(nboard);
void	 init12board(nboard);
void	 initgame(game *, const int);
void	 initall(scrgame *, const int);
/*	 Rendering functions */
WINDOW	*create_scorebox(const game *);
void	 update_scorebox(WINDOW *, const game *);
WINDOW	*create_board(const game *);
WINDOW	*create_3board(const game *);
WINDOW	*create_9board(const game *);
WINDOW	*create_12board(const game *);
void	 update_board(WINDOW *, const game *);
void	 update_3board(WINDOW *, const game *);
void	 update_9board(WINDOW *, const game *);
WINDOW	*create_msgbox(void);
void	 update_msgbox(WINDOW *, const char *);
char	*getgname(const game *);
void	 full_redraw(scrgame *);
void	 printinstrs(scrgame *);
int	 gameend(const scrgame *);
__dead void	 quit(void);
/*	 During game */
int	 inmill(const point *);
point	*mill_handler(scrgame *, char *, int);
char	 statechar(const game *);
int	 validcoords(const game *, const char *);
int	 valid3coords(const char *);
int	 valid9coords(const char *);
int      checkdir(const char *);
char    *getinput(scrgame *, char *, const int);
char	*getmove(scrgame *, char *, int);
point	*getpoint(const game *, const char *);
point	*get3point(const game *, const char *);
point	*get9point(const game *, const char *);
point	*placepiece(const scrgame *, const char *);
point	*movepiece(point *, const char *);
point   *jumppiece(point *, game *, const char *);
point   *removepiece(game *, point *);
point	*phaseone(scrgame *);
point	*phasetwothree(scrgame *);
char	*lower(char *);
int	 main(int, char **);
__END_DECLS

const char *const instructions[] = {
  "The board consists of positions connected with dashed lines, initially\n",
  "marked empty (E), but then filled with black (B) and white (W) pieces.\n",
  "Players place, move, and remove pieces, and the winner is the first to\n",
  "reduce the opponent to 2 pieces. Gameplay is split into three phases.\n",
  "\n",
  "Phase 1. Users alternatingly place a piece on the board, until each\n",
  "  player has placed 3/9/12 pieces. The location is selected by entering\n",
  "  its coordinates, e.g. `a1' or `g7'. A mill can be formed by aligning\n",
  "  three pieces along a dashed line, at which point an opponent's piece\n",
  "  is removed. This piece is selected by entering its coordinates.\n",
  "Phase 2. Users alternatingly slide one of their pieces along dashed line\n",
  "  to an adjacent empty position. The piece is selected by entering its\n",
  "  coordinates and its direction is chosen from n s e w ne nw se sw, for\n",
  "  North, South, East, West, etc. For example, `a1n' moves the piece at\n",
  "  a1 north, `g7sw' moves the piece at g7 south west. Mills are formed\n",
  "  and pieces removed as in phase 1. Phase 2 continues until a player\n",
  "  only has 3 pieces remaining.\n",
  "Phase 3. The player with three pieces may move a piece to any empty\n",
  "  location, e.g. `a1g7' moves the piece at a1 to location g7. The player\n",
  "  with more than three pieces moves as in phase 2. The first player to\n",
  "  reduce the opponent to two pieces wins.\n",
  "\n",
  "Press `?' to display these instructions, and `q' to quit.\n",
  "\n",
  "Press any key to continue...",
  NULL
};

/* **************************
 * Board and game creation
 * ************************** */

/*
 * Connect two points, with p1 in direction L1 of p2
 */
int
joinpts(const int L1, /*@null@*/ point *p1, /*@null@*/ point *p2)
{
  /* Check L1 % 2 == 0 to make sure L1 == NORTH,WEST,NE,NW */
  if (MINDIR <= L1 && L1 <= MAXDIR && L1 % 2 == 0) {
    if (p1) {
      p1->n[L1+1] = p2;
      freepoint(p1);
    }
    if (p2) {
      p2->n[L1] = p1;
      freepoint(p2);
    }
    return 1;
  }
  return 0;
}

/*
 * Join the north point n to the south point s
 */
void
joinns(/*@null@*/ point *n, /*@null@*/ point *s)
{
  joinpts(NORTH, n, s);
}

/*
 * Join the west point w to the east point e
 */
void
joinwe(/*@null@*/ point *w, /*@null@*/ point *e)
{
  joinpts(WEST, w, e);
}

/*
 * Join the north east point ne to the south west point sw
 */
void
joinnesw(/*@null@*/ point *ne, /*@null@*/ point *sw)
{
  joinpts(NE, ne, sw);
}

/*
 * Join the north west point nw to the south east point se
 */
void
joinnwse(/*@null@*/ point *nw, /*@null@*/ point *se)
{
  joinpts(NW, nw, se);
}

/*
 * Free the memory allocated to a point if it has no neighbours
 */
void
freepoint(point *p)
{
  int i, joined = 0;
  for (i = 0; i <= MAXDIR; i++) {
    if (p->n[i]) {
      joined = 1;
      break;
    }
  }
  if (!joined) {
    /* p has no neighbours */
    /* FIXME: Why does this crash with modified chunk-pointer? */
    /* free(p); */
  }
}

/*
 * Converts a direction to a neighbour index
 */
int
dirtoindex(const char *dir)
{
  const struct strkey *ptr;
  /* dirc is bound by a sentinal with null key */
  for (ptr = dirc; ptr->k; ptr++) {
    if (strncmp(ptr->k, dir, 2) == 0) {
      return ptr->v;
    }
  }
  return -1;
}

/*
 * Given a label and four main neighbours, create a point
 */
point
makepoint(const char v, /*@null@*/ point *n, /*@null@*/ point *s,
	  /*@null@*/ point *e, /*@null@*/ point *w)
{
  point temp;
  updatepoint(&temp, v, n, s, e, w);
  return temp;
}

/*
 * Update the value and four main neighbours of a point p
 */
void
updatepoint(point *p, const char v, point *n, point *s,
	    point *w, point *e)
{
  p->v = v;
  joinns(p, s);
  joinns(n, p);
  joinwe(w, p);
  joinwe(p, e);
}

/*
 * Create the board for Three Man Morris
 */
void
init3board(nboard b)
{
  int r = 0;
  /* Initialize everything to an empty grid */
  for (r = 0; r < 3; r++) {
    updatepoint(b[r][0], EMPTY, NULL, NULL, NULL, NULL);
    updatepoint(b[r][1], EMPTY, NULL, NULL, b[r][0], NULL);
    updatepoint(b[r][2], EMPTY, NULL, NULL, b[r][1], NULL);
    b[r][3] = NULL;
  }
  for (r = 0; r < 2; r++) {
    joinns(b[r][0], b[r+1][0]);
    joinns(b[r][1], b[r+1][1]);
    joinns(b[r][2], b[r+1][2]);
  }
}

/*
 * Create the board for Nine Man Morris
 */
void
init9board(nboard b)
{
  int r = 0;
  /* Initialize everything to an empty grid */
  for (r = 0; r < 3; r++) {
    updatepoint(b[r][0], EMPTY, NULL, NULL, b[r][7], NULL); /* Top mid   */
    updatepoint(b[r][1], EMPTY, NULL, NULL, b[r][0], NULL); /* Top right */
    updatepoint(b[r][2], EMPTY, b[r][1], NULL, NULL, NULL); /* Mid right */
    updatepoint(b[r][3], EMPTY, b[r][2], NULL, NULL, NULL); /* Bot right */
    updatepoint(b[r][4], EMPTY, NULL, NULL, NULL, b[r][3]); /* Bot mid   */
    updatepoint(b[r][5], EMPTY, NULL, NULL, NULL, b[r][4]); /* Bot left  */
    updatepoint(b[r][6], EMPTY, NULL, b[r][5], NULL, NULL); /* Mid left  */
    updatepoint(b[r][7], EMPTY, NULL, b[r][6], NULL, NULL); /* Top left  */
    b[r][8] = NULL;
  }
  for (r = 0; r < 2; r++) {
    printw("\n\n%d\n\n\n", r);
    joinns(b[r][0], b[r+1][0]); /* Top vertical line */
    joinns(b[r+1][4], b[r][4]); /* Bot vertical line */
    joinwe(b[r][6], b[r+1][6]); /* Left horiz line   */
    joinwe(b[r+1][2], b[r][2]); /* Right horiz line  */
  }
}

/*
 * Create the board for Twelve Man Morris
 */
void
init12board(nboard b)
{
  int r = 0;
  init9board(b);
  for (r = 0; r < 2; r++) {
    /* Top right */
    joinnesw(b[r][1], b[r+1][1]);
    /* Bottom right */
    joinnwse(b[r+1][3], b[r][3]);
    /* Bottom left */
    joinnesw(b[r+1][5], b[r][5]);
    /* Top left */
    joinnwse(b[r][7], b[r+1][7]);
  }
}

/*
 * Initialize a game with the appropriate boards, reset scores, etc.
 */
void
initgame(game *g, const int type)
{
  switch (type)
  {
  case TMM:
    init3board(g->board);
    g->type = TMM;
    break;

  case TWMM:
    init12board(g->board);
    g->type = TWMM;
    break;

  default:
    init9board(g->board);
    g->type = NMM;
  }
  /* We'll assume that, as in chess, black moves first */
  g->state = BLACK;
  g->phase = 1;
  g->totalpieces = 0;
  g->pieces[WHITE] = 0;
  g->pieces[BLACK] = 0;
}

/*
 * Initialize the board and all of the windors in a scrgame
 */
void
initall(scrgame *sg, const int type)
{
  initgame(sg->game, type);
  full_redraw(sg);
}

/* ********************
 * Drawing functions
 * ******************** */

/*
 * Create the scorebox / prompt window
 */
WINDOW *
create_scorebox(const game *g)
{
  WINDOW *local_win;

  /* WINDOW *newwin(int nlines, int ncols, int begin_y,
     int begin_x); */
  local_win = newwin(10, 27, sbrow, sbcol);
  wborder(local_win, '|', '|', '-', '-', '+', '+', '+', '+');
  /* Uncomment the following for the curses box, but I prefer the
     "rustic" boxes drawn with pipes, hyphens and plusses */
  /* box(local_win, 0, 0); */
  switch (g->type)
  {
  case TMM:
    mvwprintw(local_win, 1, 2, "Three Man Morris");
    break;
  case TWMM:
    mvwprintw(local_win, 1, 2, "Twelve Man Morris");
    break;
  default:
    mvwprintw(local_win, 1, 2, "Nine Man Morris");
    break;
  }
  /* update will call a refresh for us */
  update_scorebox(local_win, g);
  return local_win;
}

/*
 * Update the score, phase, and current player
 */
void
update_scorebox(WINDOW *w, const game *g)
{
  mvwprintw(w, 3, 2, "Phase %d", g->phase);
  mvwprintw(w, 4, 2, "White pieces: %d", g->pieces[WHITE]);
  mvwprintw(w, 5, 2, "Black pieces: %d", g->pieces[BLACK]);
  if (g->state == BLACK) {
    mvwprintw(w, 7, 2, "Black's move: ");
  } else {
    mvwprintw(w, 7, 2, "White's move: ");
  }
  wrefresh(w);
}

/*
 * Determine the type of board to draw and draw it
 */
WINDOW *
create_board(const game *g)
{
  switch (g->type)
  {
  case TMM:
    return create_3board(g);

  case TWMM:
    return create_12board(g);

  default:
    return create_9board(g);
  }
}

/*
 * Draw board for Three Man Morris
 */
WINDOW *
create_3board(const game *g)
{
  WINDOW *local_win;
  int r = 0;
  char c = 'a';

  local_win = newwin(13 + legendsep, 21 + legendsep, brdrow, brdcol);

  /* Draw the legend */
  for (r = 0; r < 3; r++) {
    mvwprintw(local_win, 6*r, 0, "%d", 3-r);
  }
  c = 'a';
  for (r = 0; r < 21; r += 9) {
    mvwaddch(local_win, 14, legendsep + r, c++);
  }
  /* Draw the board lines */
  for (r = 0; r < 3; r++) {
    mvwhline(local_win, 6*r, legendsep, '-', 18);
    mvwvline(local_win, 1, legendsep + 9*r, '|', 11);
  }

  update_3board(local_win, g);
  return local_win;
}

/*
 * Draw board for Nine Man Morris
 */
WINDOW *
create_9board(const game *g)
{
  WINDOW *local_win;
  int r = 0;
  char c = 'a';

  local_win = newwin(13 + legendsep, 21 + legendsep, brdrow, brdcol);

  /* Draw the legend */
  for (r = 0; r < 7; r++) {
    mvwprintw(local_win, 2*r, 0, "%d", 7-r);
  }
  c = 'a';
  for (r = 0; r < 21; r += 3) {
    mvwaddch(local_win, 14, legendsep + r, c++);
  }

  /* Draw the board lines */
  for (r = 0; r < 3; r++) {
    /* Top line */
    mvwhline(local_win, 2*r, legendsep + 3*r, '-', 18 - 6*r);
    /* Left line */
    mvwvline(local_win, 2*r, legendsep + 3*r, '|', 13 - 4*r);
    /* Bottom line */
    mvwhline(local_win, 12 - 2*r, legendsep + 3*r, '-', 18 - 6*r);
    /* Right line */
    mvwvline(local_win, 2*r, 18 + legendsep - 3*r, '|', 13 - 4*r);
  }
  /* TV crossline */
  mvwvline(local_win, 1, legendsep + 9, '|', 3);
  /* LH crossline */
  mvwhline(local_win, 6, legendsep + 1, '-', 5);
  /* BV crossline */
  mvwvline(local_win, 9, legendsep + 9, '|', 3);
  /* RH crossline */
  mvwhline(local_win, 6, legendsep + 13, '-', 5);

  update_9board(local_win, g);
  return local_win;
}

/*
 * Draw board for Twelve Man Morris
 */
WINDOW *
create_12board(const game *g)
{
  WINDOW *local_win;
  int r = 0;
  local_win = create_9board(g);
  /* Draw the diagonal lines */
  for (r = 0; r < 2; r++) {
    /* Top left */
    mvwaddch(local_win, 2*r + 1, legendsep + 3*r + 2, '\\');
    /* Top right */
    mvwaddch(local_win, 2*r + 1, legendsep + 16 - 3*r, '/');
    /* Bottom left */
    mvwaddch(local_win, 11 - 2*r, legendsep + 3*r + 2, '/');
    /* Bottom right */
    mvwaddch(local_win, 11 - 2*r, legendsep + 16 - 3*r, '\\');
  }
  wrefresh(local_win);
  return local_win;
}

/*
 * Select the correct function to redraw points
 */
void
update_board(WINDOW *w, const game *g)
{
  switch (g->type)
  {
  case TMM:
    update_3board(w, g);
    break;

  case TWMM:
  case NMM:
  default:
    update_9board(w, g);
  }
}

/*
 * Redraw points for Three Man Morris
 */
void
update_3board(WINDOW *w, const game *g)
{
  int r = 0;
  for (r = 0; r < 3; r++) {
    mvwaddch(w, 6*r, legendsep, g->board[r][0]->v);
    mvwaddch(w, 6*r, legendsep + 9, g->board[r][1]->v);
    mvwaddch(w, 6*r, legendsep + 18, g->board[r][2]->v);
  }
  wrefresh(w);
}

/*
 * Redraw points for Nine and Twelve Man Morris
 */
void
update_9board(WINDOW *w, const game *g)
{
  int r = 0;
  for (r = 0; r < 3; r++) {
    mvwaddch(w, 2*r, legendsep + 9, g->board[r][0]->v);
    mvwaddch(w, 2*r, legendsep + 18 - 3*r, g->board[r][1]->v);
    mvwaddch(w, 6, legendsep + 18 - 3*r, g->board[r][2]->v);
    mvwaddch(w, 12-2*r, legendsep + 18 - 3*r, g->board[r][3]->v);
    mvwaddch(w, 12-2*r, legendsep + 9, g->board[r][4]->v);
    mvwaddch(w, 12-2*r, legendsep + 3*r, g->board[r][5]->v);
    mvwaddch(w, 6, legendsep + 3*r, g->board[r][6]->v);
    mvwaddch(w, 2*r, legendsep + 3*r, g->board[r][7]->v);
  }
  wrefresh(w);
}

/*
 * Create window for displaying messages
 */
WINDOW *
create_msgbox(void)
{
  return newwin(2, 80-msgcol, msgrow, msgcol);
}

/*
 * Change message displayed in window
 */
void
update_msgbox(WINDOW *w, const char *msg)
{
  werase(w);
  mvwaddstr(w, 0, 0, msg);
  wrefresh(w);
}

char *
getgname(const game *g)
{
  switch (g->type)
  {
  case TMM:
    return "tmm";
    
  case TWMM:
    return "twmm";
    
  case NMM:
  default:
    return "nmm";
  }
}

/*
 * Completely redraw *everything*
 */
void
full_redraw(scrgame *sg)
{
  static char vers[25];
  static size_t vers_len;
  static const char *helpstr = "? : help";
  static const size_t helplen = 8;
  snprintf(vers, 25, "%s version %s", getgname(sg->game), VERSION);
  vers_len = strnlen(vers, 25);
  delwin(sg->board_w);
  delwin(sg->score_w);
  delwin(sg->msg_w);
  clear();
  /* We want the ends of the version string and of
   * helpstr to line up. */
  mvprintw(versrow, helpcol + helplen - vers_len, vers);
  mvprintw(helprow, helpcol, helpstr);
  refresh();
  sg->board_w = create_board(sg->game);
  sg->score_w = create_scorebox(sg->game);
  sg->msg_w = create_msgbox();
}

/*
 * Print instructions to screen and prompt to continue
 */
void
printinstrs(scrgame *sg)
{
  /* TODO: Factor out the refresh-everything part */
  const char *const *instr;
  clear();
  for (instr = instructions; *instr; instr++) {
    printw("%s", *instr);
  }
  refresh();
  getch();
  full_redraw(sg);
}

/*
 * Announce winner, prompt user to play again, non-zero if user wants
 * to.
 */
int
gameend(const scrgame *sg)
{
  char c;
  if (sg->game->pieces[WHITE] < sg->game->pieces[BLACK]) {
    update_msgbox(sg->msg_w,
		  "Black wins! Play again?");
  } else {
    update_msgbox(sg->msg_w,
		  "White wins! Play again?");
  }
  mvwprintw(sg->score_w, promptrow, 2, "Play again?:     ");
  wrefresh(sg->score_w);
  c = mvwgetch(sg->score_w, promptrow, promptcol);
  c = tolower(c);
  return (c == 'y');
}

/*
 * Properly end curses and exit
 */
__dead void
quit(void)
{
  refresh();
  endwin();
  exit(0);
}

/* ********************************
 * Functions concerning game logic
 * ******************************** */

/*
 * Is the point p in a mill?
 */
int
inmill(const point *p)
{
  if (p->v == BLACKC || p->v == WHITEC) {
    point *neighbours[8];
    int counts[4] = {1, 1, 1, 1};
    int i, dir = MINDIR;
    for (dir = MINDIR; dir <= MAXDIR; dir++) {
      neighbours[dir] = p->n[dir];
    }

    /* We should walk at most 2 steps in any direction */
    for (i = 0; i < 2; i++) {
      for (dir = MINDIR; dir <= MAXDIR; dir++) {
	if (neighbours[dir] && neighbours[dir]->v == p->v) {
	  /* Makes use of the fact that directions along the same axis
	   * are 1 apart (e.g. SOUTH = NORTH + 1 = 1), and that
	   * division truncates toward zero */
	  counts[dir / 2]++;
	  neighbours[dir] = neighbours[dir]->n[dir];
	}
      }
    }
    /* If we have three in a row along any axis, we're in a mill */
    for (i = 0; i < 4; i++) {
      if (counts[i] == 3) {
	return 1;
      }
    }
  }
  /* If we're EMPTY, we're clearly not in a mill */
  return 0;
}

/*
 * Handle a mill by dealing with the removal of an opponent's piece.
 * Prompts the user for a piece to remove, checks its legality, and
 * removes it. Return the removed piece, recursing until a piece is
 * removed.
 */
point *
mill_handler(scrgame *sg, char *move, int count)
{
  point *p = NULL;

  if (count == 0) {
    update_msgbox(sg->msg_w,
		  "You've formed a mill, enter opponent piece to remove.");
  }
  getmove(sg, move, 3);
  if ((p = getpoint(sg->game, move))) {
    if (p->v != statechar(sg->game) && p->v != EMPTY) {
      if (inmill(p)) {
	/* We can only break an opponent's mill if there are no other
	   pieces to remove. We could keep a list of each player's
	   pieces, but this implies the headache of always updating
	   it. Alternatively, we can just iterate through at most 21
	   points, until we find an opponent piece not in a mill, or
	   nothing. */
	int r = 0;
	int c = 0;
	int opp = 0;
	if (sg->game->state == WHITE) {
	  opp = BLACKC;
	} else {
	  opp = WHITEC;
	}
	for (r = 0; r < 3; r++) {
	  for (c = 0; c < 7; c++) {
	    if (sg->game->board[r][c]->v == opp && !inmill(sg->game->board[r][c])) {
	      update_msgbox(sg->msg_w,
			    "It is possible to remove a piece not in a mill; do so.");
	      return mill_handler(sg, move, 1);
	    }
	  }
	}
	/* By this point, we've iterated through all of the opponent's
	   pieces without encountering one not in a mill. Allow the
	   desired piece to be removed. */
      }
      p->v = EMPTY;
      /* Decrement the opponent's pieces */
      sg->game->pieces[sg->game->state ^ BLACK]--;
      return p;
    } else if (p->v == EMPTY) {
      update_msgbox(sg->msg_w,
		    "You tried clearing an empty position. Please try again.");
      return mill_handler(sg, move, 1);
    } else {
      update_msgbox(sg->msg_w,
		    "You tried removing your own piece. Please try again.");
      return mill_handler(sg, move, 1);
    }
  } else {
    update_msgbox(sg->msg_w, "Invalid coordinates. Please try again.");
    return mill_handler(sg, move, 1);
  }
}

/*
 * Return the char corresponding to the current state
 */
char
statechar(const game *g)
{
  return g->state == WHITE ? WHITEC : BLACKC;
}

/*
 * Checks the validity of coordinates for a game
 * Assumes coords are lower case and of length 2
 */
int
validcoords(const game *g, const char *coords)
{
  switch (g->type)
  {
  case TMM:
    return valid3coords(coords);

  case NMM:
  case TWMM:
  default:
    return valid9coords(coords);
  }
}

/*
 * Checks the validity of coordinates for Three Man Morris
 * Assumes coords are lower case and of length 2
 */
int
valid3coords(const char *coords)
{
  return ('a' <= coords[0] && coords[0] <= 'c' &&
	  '1' <= coords[1] && coords[1] <= '3');
}

/*
 * Checks the validity of coordinates for Nine, Twelve Man Morris
 * Assumes coords are lower case and of length 2
 */
int
valid9coords(const char *coords)
{
  if (coords[0] < 'a' || 'g' < coords[0] || coords[1] < '0' || '7' < coords[1]) {
    return 0;
  }
  switch (coords[0])
  {
  case 'a': case 'g':
    return (coords[1] == '1' || coords[1] == '4' || coords[1] == '7');

  case 'b': case 'f':
    return (coords[1] == '2' || coords[1] == '4' || coords[1] == '6');

  case 'c': case 'e':
    return ('3' <= coords[1] && coords[1] <= '5');

  case 'd':
    return (coords[1] != '4');

  default:
    /* UNREACHABLE */
    return 0;
  }
}

/*
 * Checks the validity of a direction
 */
int
checkdir(const char *dir)
{
  if (isalpha(dir[0])) {
    char d[3];
    int i;
    for (i = 0; i < 2; i++) {
      d[i] = tolower(dir[0]);
    }
    d[2] = '\0';
    i = dirtoindex(d);
    return (MINDIR <= i && i <= MAXDIR);
  }
  return 0;
}

/*
 * Prompt user for input, handling backspace, return.
 * q twice at start of line: quit()
 * ? at start of line: printinstrs()
 * Otherwise, read up to length characters from sg->scorer_w into inp
 * Assume than inp always has room for at least '\0'
 * Recurse until no errors, return pointer to inp.
 */
char *
getinput(scrgame *sg, char *inp, const int length)
{
  int l, ch, i, quitc;
  quitc = 0;
  /* Clear the prompt area */
  mvwaddstr(sg->score_w, promptrow, promptcol, "          |");
  wrefresh(sg->score_w);
  for (l = 0; l < length - 1; l++) {
    ch = mvwgetch(sg->score_w, promptrow, promptcol + l);
    if (ch == 12) {
      /* We were given a ^L */
      full_redraw(sg);
      update_msgbox(sg->msg_w, "");
      for (i = 0; i < l; i++) {
	mvwaddch(sg->score_w, promptrow, promptcol + i, inp[i]);
      }
      wrefresh(sg->score_w);
      l--;
      continue;
    }
    if (ch == '\b' || ch == KEY_BACKSPACE ||
	ch == KEY_DC || ch == 127) {
      /* We need to retake the current character; l++ will increase
       * it, so counteract that with an l-- to remove the BACKSPACE. */
      l--;
      if (l >= 0) {
	/* Erase the input char from the screen */
	mvwaddch(sg->score_w, promptrow, promptcol + l, ' ');
	/* And another l-- so that we overwrite the input char from
	 * inp on the next iteration of this loop. */
	l--;
      } else {
	/* We're at the start of the line, so there was no input char
	 * to erase */
      }
      wrefresh(sg->score_w);
      update_msgbox(sg->msg_w, "");
      continue;
    } else if (isalnum(ch)) {
      if (ch == 'q' && l == 0) {
	if (quitc) {
	  quit();
	} else {
	  quitc = 1;
	  update_msgbox(sg->msg_w, "Enter 'q' again to quit");
	  mvwaddch(sg->score_w, promptrow, promptcol, ' ');
	  l--;
	}
      } else if (ch == '?' && l == 0) {
	printinstrs(sg);
	mvwaddch(sg->score_w, promptrow, promptcol, ' ');
	l--;
      } else {
	inp[l] = (char) ch;
	mvwaddch(sg->score_w, promptrow, promptcol + l, ch);
      }
      wrefresh(sg->score_w);
    } else if (ch == '\n') {
      inp[l] = '\0';
      break;
    } else {
      update_msgbox(sg->msg_w, "Unexpected non-ASCII input");
      return getinput(sg, inp, length);
    }
  }
  /* l <= length-1 */
  inp[l] = '\0';
  return inp;
}

/*
 * Prompts the user for a move with getinput, sanitizes it, and makes
 * the move. It stores the move in *move, you can specify the maximum
 * length l of the move to take, this should be at most the length of
 * move. If mill is less than three or more than five, move should
 * hold at least 5 characters. Returns pointer to the move.
 */
char *
getmove(scrgame *sg, char *move, int l)
{
  int length, pieces, index, twmm = 0;
  /* Do we only have 3 pieces, i.e. are we in skipping mode? */
  pieces = sg->game->pieces[(int) sg->game->state];
  if (l < 3 || 5 < l) {
    if (sg->game->type == TWMM) {
      /* We need to add one to our length to make up for the
       * possibility of ne/nw/se/sw over n/e/s/w */
      twmm = 1;
    }
    switch (sg->game->phase)
    {
    case 1:
      /* In piece placing mode */
      length = 3;
      break;

    case 2:
      /* Of form d3s or d3sw */
      length = 4 + twmm;
      break;

    case 3:
      if (pieces == 3) {
	/* Of form d3a1 */
	length = 5;
      } else {
	/* Of form d3s or d3sw */
	length = 4 + twmm;
      }
      break;

    default:
      length = 0;
      /* NOTREACHED */
    }
  } else {
    length = l;
  }
  move = getinput(sg, move, length);
  move = lower(move);
  if (!validcoords(sg->game, move) ||
      /* Check the length to make sure we're not in mill mode */
      (pieces == 3 && length != 3 && !validcoords(sg->game, &move[2]))) {
    update_msgbox(sg->msg_w, "Invalid coordinates");
    return getmove(sg, move, length);
  }
  /* If we'e in piece sliding stage. The length check makes sure we're
     not in mill mode. */
  if (sg->game->phase != 1 && pieces != 3 && length != 3) {
    index = dirtoindex(&move[2]);
    if (index == -1) {
      update_msgbox(sg->msg_w, "Invalid direction");
      return getmove(sg, move, length);
    } else if (!(getpoint(sg->game, /* We already checked that move[2]
				       is safe */
			  move)->n[index])) {
      update_msgbox(sg->msg_w, "Impossible to move in that direction");
      return getmove(sg, move, length);
    }
  }
  move[length - 1] = '\0';
  return move;
}

/*
 * Given a coordinate array, retrieve the corresponding point by
 * calling the appropriate function.
 */
point *
getpoint(const game *g, const char *coords)
{
  switch (g->type)
  {
  case TMM:
    return get3point(g, coords);

  case NMM:
  case TWMM:
  default:
    return get9point(g, coords);
  }
}

/*
 * Given a coordinate array, retrieve the corresponding point
 */
point *
get3point(const game *g, const char *coords)
{
  int r;
  if (validcoords(g, coords)) {
    r = 2 + '1' - coords[1];
    /* = 2 - (coords[0] - '1'); */
    return g->board[r][coords[0] - 'a'];
  }
  return NULL;
}


/*
 * Given a coordinate array, retrieve the corresponding point
 */
point *
get9point(const game *g, const char *coords)
{
  int r, c = 0;
  if (validcoords(g, coords)) {
    switch (coords[1])
    {
    case '4':
      if (coords[0] < 'd') {
	r = coords[0] - 'a';
	c = 6;
      } else {
	r = 'g' - coords[0];
	c = 2;
      }
      break;

    case '1': case '7':
      r = 0;
      break;

    case '2': case '6':
      r = 1;
      break;

    case '3': case '5':
      r = 2;
      break;

    default:
      /* NOTREACHED */
      endwin();
      errx(1, "Reached an unreachable branch in getpoint");
      r = 0; /* Shut up gcc complaining that r may be
		used unitialized */
    }
    if (coords[1] != '4') {
      if (coords[0] < 'd') {
	c = coords[1] < '4' ? 5 : 7;
      } else if (coords[0] == 'd') {
	c = coords[1] < '4' ? 4 : 0;
      } else {
	c = coords[1] < '4' ? 3 : 1;
      }
    }
    return g->board[r][c];
  }
  return NULL;
}

/*
 * Place a piece corresponding to the current player at the coordinates coords.
 */
point *
placepiece(const scrgame *sg, const char *coords)
{
  point *p = NULL;
  if ((p = getpoint(sg->game, coords))) {
    if (p->v == EMPTY) {
      p->v = statechar(sg->game);
      return p;
    } else {
      update_msgbox(sg->msg_w, "That location is already occupied, please try again.");
      return NULL;
    }
  } else {
    update_msgbox(sg->msg_w, "Invalid coordinates. Please try again.");
    return NULL;
  }
}


/*
 * Move a piece from p in direction dir
 */
point *
movepiece(point *p, const char *dir)
{
  int index;
  index = dirtoindex(dir);
  if (p->n[index] && p->n[index]->v == EMPTY) {
    p->n[index]->v = p->v;
    p->v = EMPTY;
    return p->n[index];
  }
  return NULL;
}

/*
 * Move point p to the given position
 */
point *
jumppiece(point *p, game *g, const char *position)
{
  point *ptr = getpoint(g, position);
  if (ptr && ptr->v == EMPTY) {
    ptr->v = p->v;
    p->v = EMPTY;
    return ptr;
  }
  return NULL;
}

/*
 * Remove a point by setting it to empty and decrementing the piece
 * counts
 */
point *
removepiece(game *g, point *p)
{
  if (p && g) {
    /* We let stupid people remove their own pieces */
    if (p->v == BLACKC) {
      g->pieces[BLACK]--;
    } else if (p->v == WHITE) {
      g->pieces[WHITE]--;
    } else {
      errno = ENOTSUP;
      endwin();
      errx(ENOTSUP, "Tried removing EMPTY. This should NEVER happen.");
    }
    p->v = EMPTY;
    return p;
  }
  return NULL;
}

/*
 * Phase one of the game
 * Returns NULL if one player is guaranteed to have less than 3 pieces
 * at the end of the phase, thus loosing.
 */
point *
phaseone(scrgame *sg)
{
  point *p = NULL;
  char coords[4];
  int maxrem = 0;
  for (; sg->game->totalpieces < 2 * sg->game->type; sg->game->totalpieces++) {
    getmove(sg, coords, 0);
    if (!(p = placepiece(sg, coords))) {
      sg->game->totalpieces--;
      continue;
    }
    sg->game->pieces[sg->game->state]++;
    if (inmill(p)) {
      /* We should redraw the board now so that the player can see the
	 piece he just played */
      update_scorebox(sg->score_w, sg->game);
      update_board(sg->board_w, sg->game);
      update_msgbox(sg->msg_w, "");
      mill_handler(sg, coords, 0);
    }
    sg->game->state ^= BLACK;
    update_scorebox(sg->score_w, sg->game);
    update_board(sg->board_w, sg->game);
    update_msgbox(sg->msg_w, "");
    maxrem = (2 * sg->game->type - sg->game->totalpieces) / 2;
    if (sg->game->pieces[0] + maxrem < 3 ||
	sg->game->pieces[1] + maxrem < 3) {
      /* It's impossible for a player to place more than 3 pieces, abort */
      return NULL;
    }
  }
  if (sg->game->type == TMM) {
    /* Three Man Morris doesn't have a phase 2 */
    sg->game->phase = 3;
  } else {
    sg->game->phase = 2;
  }
  update_scorebox(sg->score_w, sg->game);
  return p;
}

/*
 * Phases two and three of the game
 */
point *
phasetwothree(scrgame *sg)
{
  point *p = NULL;
  char coords[6];
  while (sg->game->pieces[WHITE] >= 3 && sg->game->pieces[BLACK] >= 3) {
    getmove(sg, coords, 0);
    if (!(p = getpoint(sg->game, coords))) {
      update_msgbox(sg->msg_w, "Something went wrong...");
      continue;
    }
    if (p->v != statechar(sg->game)) {
      update_msgbox(sg->msg_w, "Please move your own piece.");
      continue;
    }
    if (sg->game->pieces[sg->game->state] == 3) {
      if (!(p = jumppiece(p, sg->game, &coords[2]))) {
	update_msgbox(sg->msg_w,
		      "That location is already occupied. Please try again");
	continue;
      }
    } else if (!(p = movepiece(p, &coords[2]))) {
      update_msgbox(sg->msg_w,
		    "That location is already occupied. Please try again");
      continue;
    }
    if (inmill(p)) {
      /* We should redraw the board now so that the player can see the
	 piece he just played */
      update_scorebox(sg->score_w, sg->game);
      update_board(sg->board_w, sg->game);
      update_msgbox(sg->msg_w, "");
      mill_handler(sg, coords, 0);
    }
    sg->game->state ^= BLACK;
    if (sg->game->pieces[BLACK] == 3 || sg->game->pieces[WHITE] == 3) {
      sg->game->phase = 3;
    }
    update_scorebox(sg->score_w, sg->game);
    update_board(sg->board_w, sg->game);
    update_msgbox(sg->msg_w, "");
  }
  return p;
}

/*
 * Applies tolower() to every element of a null-terminated string
 * Will do bad things(TM) if the string is not null-terminated
 */
char*
lower(char *s)
{
  char *c = s;
  for (c = s; *c; c++) {
    *c = tolower(*c);
  }
  return s;
}

/*
 * The Big Cheese
 */
int
main(int argc, char *argv[])
{
  scrgame *sg;
  int c;
  char *bn = basename(argv[0]);
  if (!bn || errno) {
    /* basename can return a NULL pointer, causing a segfault on
       strncmp below */
    errx(errno, "Something went wrong in determining the %s",
	 "filename by which nmm was called.");
  }
  if (argc != 1) {
    errx(EINVAL, "%s doesn't take any arguments.", bn);
  }
  if ((sg = malloc(sizeof(*sg)))) {
    if ((sg->game = malloc(sizeof(*sg->game)))) {
      point *board;
      if ((board = calloc(ROWS * COLS, sizeof(*board)))) {
	int r;
	for (r = 0; r < ROWS; r++) {
	  for (c = 0; c < COLS; c++) {
	    sg->game->board[r][c] = board++;
	  }
	}
      } else {
	errx(errno, "Unable to allocate memory for the board");
      }
    } else {
      errx(errno, "Unable to allocate memory for the game");
    }
  } else {
    errx(errno, "Unable to allocate memory");
  }
  initscr();
  cbreak();
  keypad(stdscr, TRUE);
  clear();
  printw("Display instructions? (y/n) ");
  refresh();
  c = getch();
  c = tolower(c);
  if (c == 'y') {
    printinstrs(sg);
  }
  clear();
  noecho();
  refresh();
  if (strncmp("tmm", bn, 3) == 0) {
    c = TMM;
  } else if (strncmp("twmm", bn, 4) == 0) {
    c = TWMM;
  } else {
    c = NMM;
  }
  for (;;) {
    initall(sg, c);
    refresh();
    phaseone(sg);
    phasetwothree(sg);
    if (!gameend(sg)) {
      break;
    }
  }
  refresh();
  endwin();
  return 0;
}
