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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.1"

#define EMPTY 'E'
#define WHITEC 'W'
#define BLACKC 'B'
#define WHITE 0
#define BLACK 1
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
#define NORTHC 'n'
#define EASTC 'e'
#define SOUTHC 's'
#define WESTC 'w'
#define PIECECOUNT 8

#define sbrow 3         /* score box */
#define sbcol 37
#define brdrow 2        /* board     */
#define brdcol 4
#define legendsep 3     /* distance board<->legend */
#define msgrow 20       /* message box */
#define msgcol 7
#define promptcol 16
#define promptrow 7

/* Board & game construction */
struct point {
  char v; /* Value: what's currently here */
  struct point* n[4]; /* Our neighbours, indexed by NORTH, SOUTH, EAST, WEST */
};

typedef struct point nboard[3][8];

struct game {
  nboard board;
  int state;
  int pieces[2];
  int totalpieces;
  int phase;
};

struct scrgame {
  struct game *game;
  WINDOW *score_w;
  WINDOW *board_w;
  WINDOW *msg_w;
};

void joinns(struct point *, struct point *);
void joinwe(struct point *, struct point *);
void freepoint(struct point *);
struct point makepoint(const char, struct point *, struct point *,
		       struct point *, struct point *);
void updatepoint(struct point *, const char, struct point *, struct point *,
		 struct point *, struct point *);
void initboard(nboard);
void initgame(struct game *);
int gameend(struct scrgame *);
/* Rendering functions */
WINDOW *create_scorebox(const struct game *);
void update_scorebox(WINDOW *, const struct game *);
WINDOW *create_board(const struct game *);
void update_board(WINDOW *, const struct game *);
WINDOW *create_msgbox(void);
void update_msgbox(WINDOW *, const char *);
void initall(struct scrgame *);
/* During game */
void printgame(const struct game *);
int inmill(const struct point *);
struct point *mill_handler(struct scrgame *, char *, int);
int validcoords(const char *);
int dirtoindex(const char);
char statechar(const struct game *);
char *getmove(struct scrgame *, char *, int);
struct point *getpoint(struct game *, const char *);
struct point *placepiece(struct scrgame *, const char *);
int checkadjacent(const struct point *, const struct point *);
struct point *removepiece(struct game *, struct point *);
struct point *jumppiece(struct point *, struct game *, const char *);
struct point *movepiece(struct point *, char);
struct point *phaseone(struct scrgame *);
struct point *phasetwo(struct scrgame *);
int checkdir(const char *);
char* prompt(const char *, char *, const int);
char* promptcheck(const char *, char *, const int, const int,
		  const char *, int (*function)(const char *));
int main(void);

const char *const instructions[] = {
  "The board consists of positions connected with dashed lines, initially\n",
  "marked empty (E), but then filled with black (B) and white (W) pieces.\n",
  "Players place, move, and remove pieces, and the winner is the first to\n",
  "reduce the opponent to 2 pieces. Gameplay is split into three phases.\n",
  "\n",
  "Phase 1. Users alternatingly place a piece on the board, until each\n",
  "  player has placed 7 pieces. The location is selected by entering its\n",
  "  coordinates, e.g. `a1' or `g7'. A mill can be formed by aligning\n",
  "  three pieces along a dashed line, at which point an opponent's piece\n",
  "  is removed. This piece is selected by entering its coordinates.\n",
  "Phase 2. Users alternatingly slide one of their pieces along dashed line\n",
  "  to an adjacent empty position. The piece is selected by entering its\n",
  "  coordinates and its direction is chosen from n s e w, for North, South,\n",
  "  East, West. For example, `a1n' moves the piece at a1 north, `g7s' moves\n",
  "  the piece at g7 south. Mills are formed and pieces removed as in\n",
  "  phase 1. Phase 2 continues until a player only has 3 pieces remaining.\n",
  "Phase 3. The player with three pieces may move a piece to any empty\n",
  "  location, e.g. `a1g7' moves the piece at a1 to location g7. The player\n",
  "  with more than three pieces moves as in phase 2. The first player to\n",
  "  reduce the opponent to two pieces wins.\n",
  "\n",
  "Press any key to continue...",
  NULL
};

/*
 * Board and game creation
 */

void
joinns(/*@null@*/ struct point *n, /*@null@*/ struct point *s)
{
  if (n) {
    n->n[SOUTH] = s;
    freepoint(n);
  }
  if (s) {
    s->n[NORTH] = n;
    freepoint(s);
  }
}

void
joinwe(/*@null@*/ struct point *w, /*@null@*/ struct point *e)
{
  if (e) {
    e->n[WEST] = w;
    freepoint(e);
  }
  if (w) {
    w->n[EAST] = e;
    freepoint(w);
  }
}

void freepoint(struct point *p)
{
  if (!p->n[NORTH] && !p->n[SOUTH] && !p->n[EAST] && !p->n[WEST]) {
    /* free(p); */
  }
}

struct point
makepoint(const char v, /*@null@*/ struct point *n, /*@null@*/ struct point *s,
	  /*@null@*/ struct point *e, /*@null@*/ struct point *w)
{
  struct point temp;
  updatepoint(&temp, v, n, s, e, w);
  return temp;
}

void
updatepoint(struct point *p, const char v, struct point *n, struct point *s,
	    struct point *e, struct point *w)
{
  p->v = v;
  joinns(p, s);
  joinns(n, p);
  joinwe(w, p);
  joinwe(p, e);
}

void
initboard(nboard b)
{
  int r = 0;
  /* Initialize everything to an empty grid */
  for (r = 0; r < 3; r++) {
    updatepoint(&b[r][0], EMPTY, NULL, NULL, NULL, NULL);        /* Top mid   */
    updatepoint(&b[r][1], EMPTY, NULL, NULL, &b[r][0], NULL);    /* Top right */
    updatepoint(&b[r][2], EMPTY, &b[r][1], NULL, NULL, NULL);    /* Mid right */
    updatepoint(&b[r][3], EMPTY, &b[r][2], NULL, NULL, NULL);    /* Bot right */
    updatepoint(&b[r][4], EMPTY, NULL, NULL, &b[r][3], NULL);    /* Bot mid   */
    updatepoint(&b[r][5], EMPTY, NULL, NULL, &b[r][4], NULL);    /* Bot left  */
    updatepoint(&b[r][6], EMPTY, NULL, &b[r][5], NULL, NULL);    /* Mid left  */
    updatepoint(&b[r][7], EMPTY, NULL, &b[r][6], &b[r][0], NULL); /* Top left */
  }
  for (r = 0; r < 2; r++) {
    joinns(&b[r][0], &b[r+1][0]); /* Top vertical line */
    joinns(&b[r+1][4], &b[r][4]); /* Bot vertical line */
    joinwe(&b[r][6], &b[r+1][6]); /* Left horiz line   */
    joinwe(&b[r+1][2], &b[r][2]); /* Right horiz line  */
  }
}

void
initgame(struct game *g)
{
  initboard(g->board);
  /* We'll assume that, as in chess, black moves first */
  g->state = BLACK;
  g->phase = 1;
  g->totalpieces = 0;
  g->pieces[WHITE] = 0;
  g->pieces[BLACK] = 0;
}

void
initall(struct scrgame *sg)
{
  initgame(sg->game);
  sg->score_w = create_scorebox(sg->game);
  sg->board_w = create_board(sg->game);
  sg->msg_w = create_msgbox();
}

/*
 * Drawing functions
 */

WINDOW
*create_scorebox(const struct game *g)
{
  WINDOW *local_win;

  /* WINDOW *newwin(int nlines, int ncols, int begin_y,
     int begin_x); */
  local_win = newwin(10, 27, sbrow, sbcol);
  wborder(local_win, '|', '|', '-', '-', '+', '+', '+', '+');
  /* Uncomment the following for the curses box, but I prefer the
     "rustic" boxes drawn with pipes, hyphens and plusses */
  /* box(local_win, 0, 0); */
  mvwprintw(local_win, 1, 2, "Nine Man Morris");
  /* update will call a refresh for us */
  update_scorebox(local_win, g);
  return local_win;
}

void
update_scorebox(WINDOW *w, const struct game *g)
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

WINDOW
*create_board(const struct game *g)
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
    mvwaddch(local_win, 14, legendsep + r, c);
    c++;
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

  update_board(local_win, g);
  return local_win;
}

void
update_board(WINDOW *w, const struct game *g)
{
  int r = 0;
  for (r = 0; r < 3; r++) {
    mvwaddch(w, 2*r, legendsep + 9, g->board[r][0].v);
    mvwaddch(w, 2*r, legendsep + 18 - 3*r, g->board[r][1].v);
    mvwaddch(w, 6, legendsep + 18 - 3*r, g->board[r][2].v);
    mvwaddch(w, 12-2*r, legendsep + 18 - 3*r, g->board[r][3].v);
    mvwaddch(w, 12-2*r, legendsep + 9, g->board[r][4].v);
    mvwaddch(w, 12-2*r, legendsep + 3*r, g->board[r][5].v);
    mvwaddch(w, 6, legendsep + 3*r, g->board[r][6].v);
    mvwaddch(w, 2*r, legendsep + 3*r, g->board[r][7].v);
  }
  wrefresh(w);
}

WINDOW
*create_msgbox(void)
{
  return newwin(2, 80-msgcol, msgrow, msgcol);
}

void
update_msgbox(WINDOW *w, const char *msg)
{
  werase(w);
  mvwaddstr(w, 0, 0, msg);
  wrefresh(w);
}

int
gameend(struct scrgame *sg)
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
 * Functions concerning game logic
 */

int
inmill(const struct point *p)
{
  if (p->v == BLACKC || p->v == WHITEC) {
    int steps = 0;
    int hc = 1;
    int vc = 1; /* horizontal count, vertical count */
    struct point *np = p->n[NORTH];
    struct point *sp = p->n[SOUTH];
    struct point *ep = p->n[EAST];
    struct point *wp = p->n[WEST];

    /* We make up to three steps in each direction to account for when
       we're on the edge */
    for (steps = 0; steps < 3; steps++) {
      if (np && np->v == p->v) {
	vc++;
	np = np->n[NORTH];
      }
      if (sp && sp->v == p->v) {
	vc++;
	sp = sp->n[SOUTH];
      }
      if (ep && ep->v == p->v) {
	hc++;
	ep = ep->n[EAST];
      }
      if (wp && wp->v == p->v) {
	hc++;
	wp = wp->n[WEST];
      }
    }
    return (hc == 3 || vc == 3);
  }
  /* If we're EMPTY, we're clearly not in a mill */
  return 0;
}

int
adjacent(const struct point *p1, const struct point *p2)
{
  return (p2 == p1->n[EAST] || p2 == p1->n[WEST] ||
	  p2 == p1->n[NORTH] || p2 == p1->n[SOUTH]);
}

char
statechar(const struct game *g)
{
  return g->state == WHITE ? WHITEC : BLACKC;
}

struct point*
mill_handler(struct scrgame *sg, char *move, int count)
{
  struct point *p = NULL;

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
	    if (sg->game->board[r][c].v == opp && !inmill(&sg->game->board[r][c])) {
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

struct point*
placepiece(struct scrgame *sg, const char *coords)
{
  struct point *p = NULL;
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

int
checkdir(const char *dir)
{
  if (isalpha(dir[0])) {
    char d;
    d = tolower(dir[0]);
    return (d == NORTHC || d == SOUTHC || d == EASTC || d == WESTC);
  }
  return 0;
}

int
validcoords(const char *coords)
{
  /* TODO: Why doesn't this work? */
  if ((sizeof(coords) / sizeof(*coords)) < 2) {
    return 0;
  }
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

char
*getmove(struct scrgame *sg, char *move, int l)
{
  int length = 0;
  int ch = 0;
  int c = 0;
  if (l < 3 || 5 < l) {
    if (sg->game->phase == 1) {
      /* Of form d3 */
      length = 3;
    } else if (sg->game->phase == 2) {
      /* Of form d3s */
      length = 4;
    } else if (sg->game->phase == 3) {
      if (sg->game->pieces[(int) sg->game->state] == 3) {
	/* Of form d3a1 */
	length = 5;
      } else {
	/* Of form d3s */
	length = 4;
      }
    }
  } else {
    length = l;
  }
  /* Clear the prompt area */
  mvwaddstr(sg->score_w, promptrow, promptcol, "          |");
  for (c = 0; c < length - 1; c++) {
    /* Store into int to make sure we don't miss out on special
       ncurses keys, such as KEY_BACKSPACE */
    ch = mvwgetch(sg->score_w, promptrow, promptcol + c);
    if (ch == '\b' || ch == KEY_BACKSPACE) {
      /* We don't want users erasing the whole screen */
      if (c > 0) {
	mvwaddch(sg->score_w, promptrow, promptcol + c, ' ');
	c--;
      } else {
	mvwaddch(sg->score_w, promptrow, promptcol, ' ');
      }
      continue;
    } else if (isascii(ch)) {
      if (ch == '\n') {
	move[c] = ch;
	c++;
	break;
      } else {
	move[c] = ch;
	mvwaddch(sg->score_w, promptrow, promptcol + c, ch);
      }
    } else {
      update_msgbox(sg->msg_w, "Unexpected non-ASCII input");
      return getmove(sg, move, length);
    }
  }
  move[0] = tolower(move[0]);
  move[2] = tolower(move[2]);
  if (!validcoords(move) ||
      (length == 5 && !validcoords(&move[2]))) {
    update_msgbox(sg->msg_w, "Invalid coordinates");
    return getmove(sg, move, length);
  }
  if (length == 4) {
    if (!(move[2] == NORTHC || move[2] == SOUTHC ||
	  move[2] == EASTC || move[2] == WESTC)) {
      update_msgbox(sg->msg_w, "Invalid direction");
      return getmove(sg, move, length);
    } else if (!(getpoint(sg->game, /* We already checked that move[2]
				       is safe */
			  move)->n[dirtoindex(move[2])])) {
      update_msgbox(sg->msg_w, "Impossible to move in that direction");
      return getmove(sg, move, length);
    }
  }
  move[length - 1] = '\0';
  return move;
}

struct point
*getpoint(struct game *g, const char *coords)
{
  int r, c = 0;
  if (validcoords(coords)) {
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
	errx(1, "Reached an unreachable branch in getpoint");
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
    return &(g->board[r][c]);
  }
  return NULL;
}


struct point*
phaseone(struct scrgame *sg)
{
  struct point *p = NULL;
  char coords[4];
  for (; sg->game->totalpieces < PIECECOUNT; sg->game->totalpieces++) {
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
  }
  sg->game->phase = 2;
  update_scorebox(sg->score_w, sg->game);
  return p;
}

struct point*
phasetwo(struct scrgame *sg)
{
  struct point *p = NULL;
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
    } else if (!(p = movepiece(p, coords[2]))) {
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

int
dirtoindex(const char dir)
{
  switch (dir)
    {
    case NORTHC:
      return NORTH;

    case SOUTHC:
      return SOUTH;

    case EASTC:
      return EAST;

    case WESTC:
      return WEST;

    default:
      return -1;
    }
}

struct point*
movepiece(struct point *p, const char dir)
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

struct point*
jumppiece(struct point *p, struct game *g, const char *position)
{
  struct point *ptr = getpoint(g, position);
  if (ptr && ptr->v == EMPTY) {
    ptr->v = p->v;
    p->v = EMPTY;
    return ptr;
  }
  return NULL;
}

struct point*
removepiece(struct game *g, struct point *p)
{
  if (p && g) {
    /* We let stupid people remove their own pieces */
    if (p->v == BLACKC) {
      g->pieces[BLACK]--;
    } else if (p->v == WHITE) {
      g->pieces[WHITE]--;
    } else {
      errno = ENOTSUP;
      errx(ENOTSUP, "Tried removing EMPTY. This should NEVER happen.");
    }
    p->v = EMPTY;
    return p;
  }
  return NULL;
}

int
main(void)
{
  struct scrgame *sg;
  const char *const *instr;
  int c;
  initscr();
  keypad(stdscr, TRUE);
  clear();
  refresh();
  printw("Display instructions? (y/n) ");
  c = getch();
  c = tolower(c);
  if (c == 'y') {
    clear(); 
    for (instr = instructions; *instr; instr++) {
      printw("%s", *instr);
    }
    refresh();
    getch();
  }
  clear();
  refresh();
  noecho();
  sg = malloc(sizeof(struct scrgame));
  if (sg) {
    sg->game = malloc(sizeof(struct game));
    if (sg->game) {
      mvprintw(25, 57, "nmm version %s", VERSION);
      for (;;) {
	initall(sg);
	refresh();
	phaseone(sg);
	phasetwo(sg);
	if (!gameend(sg)) {
	  break;
	}
      }
    }
  } else {
    refresh();
    endwin();
    errx(ENOMEM, "Unable to allocate enough memory.");
  }
  refresh();
  endwin();
  return 0;
}
