#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include "parse.h"
#include <getopt.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define rotated_x(x,y,d) ((x * cos(d*(3.14159265358979323846264338327950/180.0))) - (y * sin(d*(3.14159265358979323846264338327950/180.0))))
#define rotated_y(x,y,d) ((x * sin(d*(3.14159265358979323846264338327950/180.0))) + (y * cos(d*(3.14159265358979323846264338327950/180.0))))
#define center_x(minx, maxx) ((minx + maxx) / 2.0)
#define center_y(miny, maxy) ((miny + maxy) / 2.0)

void timer();
void setup_curses();
void unset_curses();
void background(FILE* landscape);
void initial_spawn();
void buildship(double angle);
void eraseship();
void erasethrusters();
void handle_timeout(int signal);
void blocksignal();
void unblocksignal();
void forwardthrust();
bool intersect(double Ax, double Ay, double Bx, double By, double Cx, double Cy, double Dx, double Dy);
void screenwrap();
void removebullets();
void stopthruster();


sigset_t old_mask;
FILE* sketchP;
FILE* landscape;
sigset_t block_mask_g;

double x[100], y[100];
double ox1=300, oy1=10, ox2=295, oy2=30, ox3=315, oy3=30,
       ox4=310, oy4=10, ox5=300, oy5=10, ox6=305, oy6=5, ox7=310, oy7=10;
static double x1=300, y1=10, x2=295, y2=30, x3=315, y3=30,
              x4=310, y4=10, x5=300, y5=10, x6=305, y6=5, x7=310, y7=10,
              x8=296, y8=30, x9=305, y9=38, x0=314, y0=30;
static double tx1, ty1, tx2, ty2, tx3, ty3,
              tx4, ty4, tx5, ty5, tx6, ty6, tx7, ty7,
              tx8, ty8, tx9, ty9, tx0, ty0;
double left=-10, right=10;
double deltaT=0.05;
double gravity;
double thrust;
static double xvelocity=0, yvelocity=0;
static double tilt;
static double atilt;
static int i=0, fi=0, b=0;
static int improvement = 0;
static double Ax, Ay, Bx, By, Cx, Cy, Dx, Dy;
static double yb1 = 480, yb2 = 470;
static double yb21 = 580, yb22 = 570;
static double yb31 = 780, yb32 = 770;
static double yb41 = 680, yb42 = 670;
static double yb51 = 880, yb52 = 870;
static double yb61 = 980, yb62 = 970;
static double B1, B2, B3, B4, B5, B6;
static int shot = 0, shot2 = 0, shot3 = 0, shot4 = 0, shot5 = 0, shot6 = 0;

int main(int argc, char* argv[]) {
  char opt;
  char *foption=NULL, *toption=NULL, *goption=NULL, *ioption=NULL;
  atilt = 90;
  tilt=atilt*(3.14159265358979323846264338327950/180.0);
  if (argc == 8) {
    while ((opt = getopt(argc, argv, "t:f:g:i::")) != -1) {
      switch (opt) {
        case 'f':
          foption = optarg;
          landscape = fopen(foption, "r");
          if (landscape==NULL) {
            fprintf(stderr, "Could not open file %s\n", foption);
            exit(1);
          }
          break;
        case 't':
          toption = optarg;
          break;
        case 'g':
          goption = optarg;
          break;
        case 'i':
          ioption = 0;
          break;
        default:
          break;
      }
    }
    if (ioption == 0) {
      improvement = 1;
    }
  }
  else if (argc == 7 && improvement == 0) {
    while ((opt = getopt(argc, argv, "t:f:g:")) != -1) {
      switch (opt) {
        case 'f':
          foption = optarg;
          landscape = fopen(foption, "r");
          if (landscape==NULL) {
            fprintf(stderr, "Could not open file %s\n", foption);
            exit(1);
          }
          break;
        case 't':
          toption = optarg;
          break;
        case 'g':
          goption = optarg;
          break;
        default:
          break;
      }
    }
  }
  else if (argc != 7 && improvement == 0) {
    fprintf(stderr, "Need flags -g gravity, -f landscape.txt, -t thrust\n");
    exit(1);
  }
  gravity = strtod(goption, NULL);
  if (gravity < 0 || gravity > 20) {
    fprintf(stderr, "gravity < 0, > 20 not allowed \n");
    exit(1);
  }
  thrust = strtod(toption, NULL);
  if (thrust > 0 || thrust < -20) {
    fprintf(stderr, "thrust > 0, < -20 not allowed \n");
    exit(1);
  }
  setup_curses();
  move(5, 0);
  printw("Press any key to start. To safely land, you must land on a flat surface\nand the vertical velocity must be lower than 20. Use the arrow keys to\nrotate the lander, space key for thrust, and 'q' if you wish to quit the\ngame.\n");
  if (improvement == 1) {
    printw("The enemy has thruster deactivating bullets. Get hit by one of those and\nyou'll be kissing the ground face first.");
  }
  refresh();
  int c = getch();
  if (improvement == 1) {
    //system("xdg-open http://www.youtube.com/watch?v=kxopViU98Xo &");
  }
  nodelay(stdscr, true);
  erase();
  sketchP = popen("java -jar Sketchpad.jar", "w");
  background(landscape);
  initial_spawn();
  move(5, 10);
  printw("Press arrow keys to rotate, space key for thrust, 'q' to quit.");
  refresh();
  c = getch();
  timer();
  while(1)
  {
    if (b==1) {
      move(8, 10);
      printw("CRASHED!");
      removebullets();
      if (improvement == 1) {
        eraseship();
        erasethrusters();
        double Ex = (Ax+Bx)/2;
        double Ey = ((Ay+By)/2);
        double Exc = 0;
        double left = 7, checkleft = 0;
        while (Ey != ((Ay+By)/2) + 21) {
          int A = rand() % 3;
          if (A == 1) {
            fprintf(sketchP, "setColor 255 0 0\n");
          }
          if (A==2) {
            fprintf(sketchP, "setColor 255 165 0\n");
          }
          if (A==3) {
            fprintf(sketchP, "setColor 255 255 0\n");
          }
          fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround((Ex)+Exc+left), lround(Ey+left), lround(Ex+Exc), lround( ((Ay+By)/2) +30));
          fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround((Ex)-Exc+left), lround(Ey+left), lround(Ex-Exc), lround( ((Ay+By)/2) +30));
          Ey++;
          Exc++;
          if (checkleft == 0) {
            left--;
            if (left <= -1) {
              checkleft=1;
            }
          }
          if (checkleft == 1) {
            left++;
            if (left >= 8) {
              checkleft=0;
            }
          }
        }
      }
    }
    if (b==2) {
      move(8, 10);
      printw("LANDED!");
      removebullets();
    }
    if (c != ERR)
    {
      erase();
      move(5,10);
      printw("Press arrow keys to rotate, space key for thrust, 'q' to quit.");
      move(6, 10);
      if ((c == ' ') && (b==0)) {
        printw("space key pressed");
        forwardthrust();
      }
      else if ((c == KEY_LEFT) && (b==0)) {
        printw("left key pressed");
        tilt=tilt-(10*(3.14159265358979323846264338327950/180.0));
        buildship(left);
        atilt=atilt-10;
      }
      else if ((c == KEY_RIGHT) && (b==0)) {
        printw("right key pressed");
        tilt=tilt+(10*(3.14159265358979323846264338327950/180.0));
        buildship(right);
        atilt=atilt+10;
      }
      else if (c == 'q') {
	fprintf(sketchP, "end\n");
        if (improvement == 1) {
          //system("pkill firefox");
        }
        break;
      }
      refresh();
    }
    c = getch();
  }

  unset_curses();

  exit(EXIT_SUCCESS);
}

void initial_spawn() { 
  blocksignal();
  if (improvement == 1) {
    fprintf(sketchP, "setColor 165 42 42\n");
  }
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(ox1), lround(oy1), lround(ox2), lround(oy2));
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(ox2), lround(oy2), lround(ox3), lround(oy3));
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(ox3), lround(oy3), lround(ox4), lround(oy4));
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(ox4), lround(oy4), lround(ox5), lround(oy5));
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(ox5), lround(oy5), lround(ox6), lround(oy6));
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(ox6), lround(oy6), lround(ox7), lround(oy7));
  unblocksignal();
  fflush(sketchP);
}
void eraseshoot1() {
  blocksignal();
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(B1), lround(yb1), lround(B1), lround(yb2));
  unblocksignal();
  fflush(sketchP);
}
void eraseshoot2() {
  blocksignal();
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(B2), lround(yb21), lround(B2), lround(yb22));
  unblocksignal();
  fflush(sketchP);
}
void eraseshoot3() {
  blocksignal();
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(B3), lround(yb31), lround(B3), lround(yb32));
  unblocksignal();
  fflush(sketchP);
}
void eraseshoot4() {
  blocksignal();
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(B4), lround(yb41), lround(B4), lround(yb42));
  unblocksignal();
  fflush(sketchP);
}
void eraseshoot5() {
  blocksignal();
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(B5), lround(yb51), lround(B5), lround(yb52));
  unblocksignal();
  fflush(sketchP);
}
void eraseshoot6() {
  blocksignal();
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(B6), lround(yb61), lround(B6), lround(yb62));
  unblocksignal();
  fflush(sketchP);
}

void buildship(double angle) {

  double max_xt1=MAX(MAX(x1,x2), MAX(x2,x3));
  double max_xt2=MAX(MAX(x3,x4), MAX(x4,x5));
  double max_x=MAX(max_xt1,max_xt2);
  if (improvement == 1) {
    double max_xt3=MAX(MAX(x5,x6), MAX(x6,x7));
    max_x=MAX(MAX(max_xt1,max_xt2), MAX(max_xt2,max_xt3));
  }

  double max_yt1=MAX(MAX(y1,y2), MAX(y2,y3));
  double max_yt2=MAX(MAX(y3,y4), MAX(y4,y5));
  double max_y=MAX(max_yt1,max_yt2);
  if (improvement == 1) {
    double max_yt3=MAX(MAX(y5,y6), MAX(y6,y7));
    max_y=MAX(MAX(max_yt1,max_yt2), MAX(max_yt2,max_yt3));
  }

  double min_xt1=MIN(MIN(x1,x2), MIN(x2,x3));
  double min_xt2=MIN(MIN(x3,x4), MIN(x4,x5));
  double min_x=MIN(min_xt1,min_xt2);
  if (improvement == 1) {
    double min_xt3=MIN(MIN(x5,x6), MIN(x6,x7));
    min_x=MIN(MIN(min_xt1,min_xt2), MIN(min_xt2,min_xt3));
  }

  double min_yt1=MIN(MIN(y1,y2), MIN(y2,y3));
  double min_yt2=MIN(MIN(y3,y4), MIN(y4,y5));
  double min_y=MIN(min_yt1,min_yt2);
  if (improvement == 1) {
    double min_yt3=MIN(MIN(y5,y6), MIN(y6,y7));
    min_y=MIN(MIN(min_yt1,min_yt2), MIN(min_yt2,min_yt3));
  }

  double oxc = center_x(min_x, max_x);
  double oyc = center_y(min_y, max_y);
  double xc = center_x(min_x, max_x);
  double yc = center_y(min_y, max_y);
  double txc;
  double tyc;

  txc = rotated_x(xc, yc, angle);
  tyc = rotated_y(xc, yc, angle);
  xc=txc;
  yc=tyc;
  eraseship();
  tx1 = rotated_x(x1, y1, angle) + oxc - xc;
  tx2 = rotated_x(x2, y2, angle) + oxc - xc;
  tx3 = rotated_x(x3, y3, angle) + oxc - xc;
  tx4 = rotated_x(x4, y4, angle) + oxc - xc;
  tx5 = rotated_x(x5, y5, angle) + oxc - xc;
  tx6 = rotated_x(x6, y6, angle) + oxc - xc;
  tx7 = rotated_x(x7, y7, angle) + oxc - xc;
  tx8 = rotated_x(x8, y8, angle) + oxc - xc;
  tx9 = rotated_x(x9, y9, angle) + oxc - xc;
  tx0 = rotated_x(x0, y0, angle) + oxc - xc;

  ty1 = rotated_y(x1, y1, angle) + oyc - yc;
  ty2 = rotated_y(x2, y2, angle) + oyc - yc;
  ty3 = rotated_y(x3, y3, angle) + oyc - yc;
  ty4 = rotated_y(x4, y4, angle) + oyc - yc;
  ty5 = rotated_y(x5, y5, angle) + oyc - yc;
  ty6 = rotated_y(x6, y6, angle) + oyc - yc;
  ty7 = rotated_y(x7, y7, angle) + oyc - yc;
  ty8 = rotated_y(x8, y8, angle) + oyc - yc;
  ty9 = rotated_y(x9, y9, angle) + oyc - yc;
  ty0 = rotated_y(x0, y0, angle) + oyc - yc;
  erasethrusters();
  x1=tx1;
  x2=tx2;
  x3=tx3;
  x4=tx4;
  x5=tx5;
  x6=tx6;
  x7=tx7;
  x8=tx8;
  x9=tx9;
  x0=tx0;
  y1=ty1;
  y2=ty2;
  y3=ty3;
  y4=ty4;
  y5=ty5;
  y6=ty6;
  y7=ty7;
  y8=ty8;
  y9=ty9;
  y0=ty0;
  blocksignal();
  if (improvement == 1) {
    fprintf(sketchP, "setColor 165 42 42\n");
  }
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(x1), lround(y1), lround(x2), lround(y2));
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(x2), lround(y2), lround(x3), lround(y3));
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(x3), lround(y3), lround(x4), lround(y4));
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(x4), lround(y4), lround(x5), lround(y5));
  if (improvement == 1) {
    fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(x5), lround(y5), lround(x6), lround(y6));
    fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(x6), lround(y6), lround(x7), lround(y7));
  }
  unblocksignal();
  fflush(sketchP);
}

void eraseship() {
  blocksignal();
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(x1), lround(y1), lround(x2), lround(y2));
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(x2), lround(y2), lround(x3), lround(y3));
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(x3), lround(y3), lround(x4), lround(y4));
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(x4), lround(y4), lround(x5), lround(y5));
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(x5), lround(y5), lround(x6), lround(y6));
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(x6), lround(y6), lround(x7), lround(y7));
  unblocksignal();
  erasethrusters();
  fflush(sketchP);
}

void erasethrusters() {
  blocksignal();
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(x8), lround(y8), lround(x9), lround(y9));
  fprintf(sketchP, "eraseSegment %ld %ld %ld %ld \n", lround(x9), lround(y9), lround(x0), lround(y0));
  unblocksignal();
  fflush(sketchP);
}

void background(FILE* landscape) {
  i=0;
  char line_storage[100];
  long xnew = 0, ynew = 0, xold = 0, yold = 0;
  while (fgets(line_storage, sizeof(line_storage), landscape) != NULL) {
    if (sscanf(line_storage, "%lf%lf", &x[i], &y[i]) == 2) {
      if (i == 0){ 
        xold = x[i];
        yold = y[i];
        xnew = x[i];
        ynew = y[i];
      }
      else
      { 
        xold = x[i-1];
        yold = y[i-1];
        xnew = x[i];
        ynew = y[i];
      }
      if (improvement == 1) {
        fprintf(sketchP, "setColor 10 255 100\n");
      }
      fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(xold), lround(yold), lround(xnew), lround(ynew));
      i++;
    }
  }
  fi=i;
  fflush(sketchP);
}

void setup_curses() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, true);
}

void unset_curses() {
  keypad(stdscr, false);
  nodelay(stdscr, false);
  nocbreak();
  echo();
  endwin();
}

void timer() {
  blocksignal();
  sigemptyset(&block_mask_g); 
  sigaddset(&block_mask_g, SIGALRM); 
  struct sigaction handler;
  handler.sa_handler = handle_timeout;
  sigemptyset(&handler.sa_mask);
  handler.sa_flags = 0;
  if (sigaction(SIGALRM, &handler, NULL) == -1)
    exit(EXIT_FAILURE);
  struct itimerval timer;
    struct timeval time_delay;
    time_delay.tv_sec = 0.05;
    time_delay.tv_usec = 0;
    time_delay.tv_usec = 50000;
    timer.it_interval = time_delay;

    struct timeval start;
    start.tv_sec = 0;
    start.tv_usec = 0;
    start.tv_usec = 50000;
    timer.it_value = start;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
      exit(EXIT_FAILURE);
}

void handle_timeout(int signal) {
  static int called = 0;
  double xnew = 0, ynew = 0, xold = 0, yold = 0;
  called++;
  if (signal == SIGALRM)
  {
    eraseship();
    erasethrusters();
    if (improvement == 1) {
      screenwrap();
    }
    struct itimerval timer;
    if (getitimer(ITIMER_REAL, &timer) == -1)
      exit(EXIT_FAILURE);
    if (atilt>=(360)) {
      atilt=atilt-(360);
    }
    if (atilt<=(0)) {
      atilt=atilt+(360);
    }
    blocksignal();
    if (improvement == 1) {
      if (shot == 0) {
        B1 = rand() % 640;
        shot = 1;
      }
      eraseshoot1();
      double tyb1 = yb1 - 2;
      double tyb2 = yb2 - 2;
      fprintf(sketchP, "setColor 0 0 0\n");
      fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(B1), lround(tyb1), lround(B1), lround(tyb2));
      yb1 = tyb1;
      yb2 = tyb2;
      if (yb2 < 0) {
        shot = 0;
        yb1 = 480; 
        yb2 = 470;
      }
      if (shot2 == 0) {
        B2 = rand() % 640;
        shot2 = 1;
      }
      eraseshoot2();
      double tyb21 = yb21 - 2;
      double tyb22 = yb22 - 2;
      fprintf(sketchP, "setColor 0 0 0\n");
      fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(B2), lround(tyb21), lround(B2), lround(tyb22));
      yb21 = tyb21;
      yb22 = tyb22;
      if (yb22 < 0) {
        shot2 = 0;
        yb21 = 480; 
        yb22 = 470;
      }
      if (shot3 == 0) {
        B3 = rand() % 640;
        shot3 = 1;
      }
      eraseshoot3();
      double tyb31 = yb31 - 2;
      double tyb32 = yb32 - 2;
      fprintf(sketchP, "setColor 0 0 0\n");
      fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(B3), lround(tyb31), lround(B3), lround(tyb32));
      yb31 = tyb31;
      yb32 = tyb32;
      if (yb32 < 0) {
        shot3 = 0;
        yb31 = 480; 
        yb32 = 470;
      }
      if (shot4 == 0) {
        B4 = rand() % 640;
        shot4 = 1;
      }
      eraseshoot4();
      double tyb41 = yb41 - 2;
      double tyb42 = yb42 - 2;
      fprintf(sketchP, "setColor 0 0 0\n");
      fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(B4), lround(tyb41), lround(B4), lround(tyb42));
      yb41 = tyb41;
      yb42 = tyb42;
      if (yb42 < 0) {
        shot4 = 0;
        yb41 = 480; 
        yb42 = 470;
      }
      if (shot5 == 0) {
        B5 = rand() % 640;
        shot5 = 1;
      }
      eraseshoot5();
      double tyb51 = yb51 - 2;
      double tyb52 = yb52 - 2;
      fprintf(sketchP, "setColor 0 0 0\n");
      fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(B5), lround(tyb51), lround(B5), lround(tyb52));
      yb51 = tyb51;
      yb52 = tyb52;
      if (yb52 < 0) {
        shot5 = 0;
        yb51 = 480; 
        yb52 = 470;
      }
      if (shot6 == 0) {
        B6 = rand() % 640;
        shot6 = 1;
      }
      eraseshoot6();
      double tyb61 = yb61 - 2;
      double tyb62 = yb62 - 2;
      fprintf(sketchP, "setColor 0 0 0\n");
      fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(B6), lround(tyb61), lround(B6), lround(tyb62));
      yb61 = tyb61;
      yb62 = tyb62;
      if (yb62 < 0) {
        shot6 = 0;
        yb61 = 480; 
        yb62 = 470;
      }
    }
    ty1=(y1+(yvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    ty2=(y2+(yvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    ty3=(y3+(yvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    ty4=(y4+(yvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    ty5=(y5+(yvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    ty6=(y6+(yvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    ty7=(y7+(yvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    ty8=(y8+(yvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    ty9=(y9+(yvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    ty0=(y0+(yvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));

    tx1=(x1+(xvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    tx2=(x2+(xvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    tx3=(x3+(xvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    tx4=(x4+(xvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    tx5=(x5+(xvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    tx6=(x6+(xvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    tx7=(x7+(xvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    tx8=(x8+(xvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    tx9=(x9+(xvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));
    tx0=(x0+(xvelocity * deltaT) + (1/2 * gravity * deltaT*deltaT));

    erasethrusters();
    if (improvement == 1) {
      fprintf(sketchP, "setColor 165 42 42\n");
    }
    fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(tx1), lround(ty1), lround(tx2), lround(ty2));
    fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(tx2), lround(ty2), lround(tx3), lround(ty3));
    fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(tx3), lround(ty3), lround(tx4), lround(ty4));
    fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(tx4), lround(ty4), lround(tx5), lround(ty5));
    if (improvement == 1) {
      fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(tx5), lround(ty5), lround(tx6), lround(ty6));
      fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(tx6), lround(ty6), lround(tx7), lround(ty7));
    }
    unblocksignal();
    y1=ty1;
    y2=ty2;
    y3=ty3;
    y4=ty4;
    y5=ty5;
    y6=ty6;
    y7=ty7;

    x1=tx1;
    x2=tx2;
    x3=tx3;
    x4=tx4;
    x5=tx5;
    x6=tx6;
    x7=tx7;

    y8=ty8;
    y9=ty9;
    y0=ty0;
    x8=tx8;
    x9=tx9;
    x0=tx0;

    Ax=x1; Ay=y1; Bx=x2, By=y2;
    if (improvement == 1) {
      Cx=B1; Cy=yb1; Dx=B1; Dy=yb2;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B2; Cy=yb21; Dx=B2; Dy=yb22;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B3; Cy=yb31; Dx=B3; Dy=yb32;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B4; Cy=yb41; Dx=B4; Dy=yb42;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B5; Cy=yb51; Dx=B5; Dy=yb52;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B6; Cy=yb61; Dx=B6; Dy=yb62;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
    }
    i=0;
    while (i!=fi) {
      if (i == 0) { 
        xold = (x[i]);
        yold = (y[i]);
        xnew = (x[i]);
        ynew = (y[i]);
      }
      else { 
        xold = (x[i-1]);
        yold = (y[i-1]);
        xnew = (x[i]);
        ynew = (y[i]);
      }
      Cx = xold;
      Cy = yold;
      Dx = xnew;
      Dy = ynew;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        if ((Cy==Dy) && (yvelocity<20) && (lround(atilt) == 90)) {
          b=2;
          timer.it_value.tv_usec = 0;
          timer.it_interval.tv_usec = 0;
        }
        else {
          b=1;
          timer.it_value.tv_usec = 0;
          timer.it_interval.tv_usec = 0;
        }
      }
      i++;
    }
    Ax=x2; Ay=y2; Bx=x3, By=y3;
    if (improvement == 1) {
      Cx=B1; Cy=yb1; Dx=B1; Dy=yb2;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B2; Cy=yb21; Dx=B2; Dy=yb22;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B3; Cy=yb31; Dx=B3; Dy=yb32;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B4; Cy=yb41; Dx=B4; Dy=yb42;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B5; Cy=yb51; Dx=B5; Dy=yb52;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B6; Cy=yb61; Dx=B6; Dy=yb62;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
    }
    i=0;
    while (i!=fi) {
      if (i == 0) { 
        xold = (x[i]);
        yold = (y[i]);
        xnew = (x[i]);
        ynew = (y[i]);
      }
      else { 
        xold = (x[i-1]);
        yold = (y[i-1]);
        xnew = (x[i]);
        ynew = (y[i]);
      }
      Cx = xold;
      Cy = yold;
      Dx = xnew;
      Dy = ynew;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        if ((Cy==Dy) && (yvelocity<20) && (lround(atilt) == 90)) {
          b=2;
          timer.it_value.tv_usec = 0;
          timer.it_interval.tv_usec = 0;
        }
        else {
          b=1;
          timer.it_value.tv_usec = 0;
          timer.it_interval.tv_usec = 0;
        }
      }
      i++;
    }
    Ax=x3; Ay=y3; Bx=x4, By=y4;
    if (improvement == 1) {
      Cx=B1; Cy=yb1; Dx=B1; Dy=yb2;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B2; Cy=yb21; Dx=B2; Dy=yb22;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B3; Cy=yb31; Dx=B3; Dy=yb32;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B4; Cy=yb41; Dx=B4; Dy=yb42;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B5; Cy=yb51; Dx=B5; Dy=yb52;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B6; Cy=yb61; Dx=B6; Dy=yb62;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
    }
    i=0;
    while (i!=fi) {
      if (i == 0) { 
        xold = (x[i]);
        yold = (y[i]);
        xnew = (x[i]);
        ynew = (y[i]);
      }
      else { 
        xold = (x[i-1]);
        yold = (y[i-1]);
        xnew = (x[i]);
        ynew = (y[i]);
      }
      Cx = xold;
      Cy = yold;
      Dx = xnew;
      Dy = ynew;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        if ((Cy==Dy) && (yvelocity<20) && (lround(atilt) == 90)) {
          b=2;
          timer.it_value.tv_usec = 0;
          timer.it_interval.tv_usec = 0;
        }
        else {
          b=1;
          timer.it_value.tv_usec = 0;
          timer.it_interval.tv_usec = 0;
        }
      }
      i++;
    }
    Ax=x4; Ay=y4; Bx=x5, By=y5;
    if (improvement == 1) {
      Cx=B1; Cy=yb1; Dx=B1; Dy=yb2;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B2; Cy=yb21; Dx=B2; Dy=yb22;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B3; Cy=yb31; Dx=B3; Dy=yb32;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B4; Cy=yb41; Dx=B4; Dy=yb42;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B5; Cy=yb51; Dx=B5; Dy=yb52;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
      Cx=B6; Cy=yb61; Dx=B6; Dy=yb62;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        stopthruster();
      }
    }
    i=0;
    while (i!=fi) {
      if (i == 0) { 
        xold = (x[i]);
        yold = (y[i]);
        xnew = (x[i]);
        ynew = (y[i]);
      }
      else { 
        xold = (x[i-1]);
        yold = (y[i-1]);
        xnew = (x[i]);
        ynew = (y[i]);
      }
      Cx = xold;
      Cy = yold;
      Dx = xnew;
      Dy = ynew;
      if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
        if ((Cy==Dy) && (yvelocity<20) && (lround(atilt) == 90)) {
          b=2;
          timer.it_value.tv_usec = 0;
          timer.it_interval.tv_usec = 0;
        }
        else {
          b=1;
          timer.it_value.tv_usec = 0;
          timer.it_interval.tv_usec = 0;
        }
      }
      i++;
    }
    if (improvement == 1) {
      Ax=x5; Ay=y5; Bx=x6, By=y6;
      if (improvement == 1) {
        Cx=B1; Cy=yb1; Dx=B1; Dy=yb2;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
        Cx=B2; Cy=yb21; Dx=B2; Dy=yb22;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
        Cx=B3; Cy=yb31; Dx=B3; Dy=yb32;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
        Cx=B4; Cy=yb41; Dx=B4; Dy=yb42;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
        Cx=B5; Cy=yb51; Dx=B5; Dy=yb52;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
        Cx=B6; Cy=yb61; Dx=B6; Dy=yb62;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
      }
      i=0;
      while (i!=fi) {
        if (i == 0) { 
          xold = (x[i]);
          yold = (y[i]);
          xnew = (x[i]);
          ynew = (y[i]);
        }
        else { 
          xold = (x[i-1]);
          yold = (y[i-1]);
          xnew = (x[i]);
          ynew = (y[i]);
        }
        Cx = xold;
        Cy = yold;
        Dx = xnew;
        Dy = ynew;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          if ((Cy==Dy) && (yvelocity<20) && (lround(atilt) == 90)) {
            b=2;
            timer.it_value.tv_usec = 0;
            timer.it_interval.tv_usec = 0;
          }
          else {
            b=1;
            timer.it_value.tv_usec = 0;
            timer.it_interval.tv_usec = 0;
          }
        }
        i++;
      }
    }
    if (improvement == 1) {
      if (improvement == 1) {
        Ax=x6; Ay=y6; Bx=x7, By=y7;
        Cx=B1; Cy=yb1; Dx=B1; Dy=yb2;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
        Cx=B2; Cy=yb21; Dx=B2; Dy=yb22;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
        Cx=B3; Cy=yb31; Dx=B3; Dy=yb32;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
        Cx=B4; Cy=yb41; Dx=B4; Dy=yb42;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
        Cx=B5; Cy=yb51; Dx=B5; Dy=yb52;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
        Cx=B6; Cy=yb61; Dx=B6; Dy=yb62;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          stopthruster();
        }
      }
      i=0;
      while (i!=fi) {
        if (i == 0) { 
          xold = (x[i]);
          yold = (y[i]);
          xnew = (x[i]);
          ynew = (y[i]);
        }
        else { 
          xold = (x[i-1]);
          yold = (y[i-1]);
          xnew = (x[i]);
          ynew = (y[i]);
        }
        Cx = xold;
        Cy = yold;
        Dx = xnew;
        Dy = ynew;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          if ((Cy==Dy) && (yvelocity<20) && (lround(atilt) == 90)) {
            b=2;
            timer.it_value.tv_usec = 0;
            timer.it_interval.tv_usec = 0;
          }
          else {
            b=1;
            timer.it_value.tv_usec = 0;
            timer.it_interval.tv_usec = 0;
          }
        }
        i++;
      }
    }
    if (yvelocity > 120) {
      Ax=x8; Ay=y8; Bx=x9, By=y9;
      i=0;
      while (i!=fi) {
        if (i == 0) { 
          xold = (x[i]);
          yold = (y[i]);
          xnew = (x[i]);
          ynew = (y[i]);
        }
        else { 
          xold = (x[i-1]);
          yold = (y[i-1]);
          xnew = (x[i]);
          ynew = (y[i]);
        }
        Cx = xold;
        Cy = yold;
        Dx = xnew;
        Dy = ynew;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          if ((Cy==Dy) && (yvelocity<20) && (lround(atilt) == 90)) {
            b=2;
            timer.it_value.tv_usec = 0;
            timer.it_interval.tv_usec = 0;
          }
          else {
            b=1;
            timer.it_value.tv_usec = 0;
            timer.it_interval.tv_usec = 0;
          }
        }
        i++;
      }
    }

    if (yvelocity > 120) {
      Ax=x9; Ay=y9; Bx=x0, By=y0;
      i=0;
      while (i!=fi) {
        if (i == 0) { 
          xold = (x[i]);
          yold = (y[i]);
          xnew = (x[i]);
          ynew = (y[i]);
        }
        else { 
          xold = (x[i-1]);
          yold = (y[i-1]);
          xnew = (x[i]);
          ynew = (y[i]);
        }
        Cx = xold;
        Cy = yold;
        Dx = xnew;
        Dy = ynew;
        if (intersect(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy)) {
          if ((Cy==Dy) && (yvelocity<20) && (lround(atilt) == 90)) {
            b=2;
            timer.it_value.tv_usec = 0;
            timer.it_interval.tv_usec = 0;
          }
          else {
            b=1;
            timer.it_value.tv_usec = 0;
            timer.it_interval.tv_usec = 0;
          }
        }
        i++;
      }
    }
    yvelocity = yvelocity + (gravity * deltaT);
    if (improvement == 1) {
      move(7, 10);
      printw("Vertical Velocity: %lf", yvelocity);
    }
    fflush(sketchP);

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
      exit(EXIT_FAILURE);
  }
}

void removebullets() {
  eraseshoot1();
  eraseshoot2();
  eraseshoot3();
  eraseshoot4();
  eraseshoot5();
  eraseshoot6();
}
void forwardthrust() {
  double xA = thrust * cos(tilt);
  double yA = gravity + (thrust * sin(tilt));
  if (-thrust <= gravity) {
    yA = (thrust * sin(tilt));
  }
  blocksignal();
  if (improvement == 1) {
    fprintf(sketchP, "setColor 255 100 0\n");
  }
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(tx8), lround(ty8), lround(tx9), lround(ty9));
  fprintf(sketchP, "drawSegment %ld %ld %ld %ld \n", lround(tx9), lround(ty9), lround(tx0), lround(ty0));
  unblocksignal();
  xvelocity = xvelocity + (xA * deltaT);
  if (-thrust <= gravity) {
    yvelocity = yvelocity + ((yA * deltaT));
  }
  else {
    yvelocity = yvelocity + (2.5*(yA * deltaT));
  }
  fflush(sketchP);
}

void blocksignal() {
  if (sigprocmask(SIG_BLOCK, &block_mask_g, &old_mask) < 0)
    exit(EXIT_FAILURE);
}

void unblocksignal() {
  if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0)
    exit(EXIT_FAILURE);
}

void screenwrap() {
  if ((x2 > 639) && (x3 > 639)) {
    x1=x1-639;
    x2=x2-639;
    x3=x3-639;
    x4=x4-639;
    x5=x5-639;
    x6=x6-639;
    x7=x7-639;
    x8=x8-639;
    x9=x9-639;
    x0=x0-639;
  }
  if ((x2 < -1) && (x3 < -1)) {
    x1=x1+639;
    x2=x2+639;
    x3=x3+639;
    x4=x4+639;
    x5=x5+639;
    x6=x6+639;
    x7=x7+639;
    x8=x8+639;
    x9=x9+639;
    x0=x0+639;
  }
  if ((y4 < -5) || (y5 < -5) || (y1 < -5)) {
    y1=y1+5;
    y2=y2+5;
    y3=y3+5;
    y4=y4+5;
    y5=y5+5;
    y6=y6+5;
    y7=y7+5;
    y8=y8+5;
    y9=y9+5;
    y0=y0+5;
    yvelocity = 0;
  }
}

void stopthruster() {
  b = 4;
  yvelocity = (gravity + 100);
}

bool intersect(
double Ax, double Ay,
double Bx, double By,
double Cx, double Cy,
double Dx, double Dy) 
{
  double  distAB, theCos, theSin, newX, ABpos ;

  if (((Ax==Bx) && (Ay==By)) || ((Cx==Dx) && (Cy==Dy))) return FALSE;

  if (((Ax==Cx) && (Ay==Cy)) || ((Bx==Cx) && (By==Cy))
  ||  ((Ax==Dx) && (Ay==Dy)) || ((Bx==Dx) && (By==Dy))) {
    return FALSE; }

  Bx-=Ax; By-=Ay;
  Cx-=Ax; Cy-=Ay;
  Dx-=Ax; Dy-=Ay;

  distAB=sqrt(Bx*Bx+By*By);

  theCos=Bx/distAB;
  theSin=By/distAB;
  newX=Cx*theCos+Cy*theSin;
  Cy  =Cy*theCos-Cx*theSin; Cx=newX;
  newX=Dx*theCos+Dy*theSin;
  Dy  =Dy*theCos-Dx*theSin; Dx=newX;

  if ((Cy<0. && Dy<0.) || (Cy>=0. && Dy>=0.)) return FALSE;

  ABpos=Dx+(Cx-Dx)*Dy/(Dy-Cy);

  if (ABpos<0. || ABpos>distAB) return FALSE;

  return TRUE;}

//How to compile:
//gcc -Wall -std=c99 lander.c -lcurses -lm -g
