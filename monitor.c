/*********************  START OF SPECIFICATIONS  ********************/
/*                                                                  */
/*  SOURCE FILE NAME:  monitor.c                                    */
/*  DESCRIPTIVE NAME:                                               */
/*      A simple minded CPU monitor (and a first attempt at a       */
/*      Presentation Manager Application).                          */
/*  COPYRIGHT:         IBM Corporation 1988                         */
/*  STATUS:            Version 1.02                                 */
/*                                                                  */
/*  DESCRIPTION AND PURPOSE:                                        */
/*           This program provides a windowed display of the        */
/*      approximate CPU utilization.                                */
/*                                                                  */
/*  EXTERNAL DEPENDENCIES:                                          */
/*          This function has dependencies on the following         */
/*      files for compilation.                                      */
/*                                                                  */
/*  EXECUTION INSTRUCTIONS:                                         */
/*          TBD.                                                    */
/*                                                                  */
/*  EXPECTED OUTPUT: A windowed graphical display of the CPU usage. */
/*                                                                  */
/*  PROGRAM AUTHOR:                                                 */
/*      Marc L. Cohen,         Dept. 39J                            */
/*                                                                  */
/*  DATE WRITTEN:                                                   */
/*      24 May 1988 Initial headers filled in.                      */
/*                                                                  */
/*  UPDATE LOG:                                                     */
/*                                                                  */
/***********************  END OF SPECIFICATIONS  ********************/

/* These include files are standard C include files.                */
#include <stdlib.h>
#include <string.h>

/* These include files are OS/2 specific.                           */
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_GPI
#define INCL_WININPUT
#define INCL_WINFRAMEMGR
#define INCL_WINERRORS
#define INCL_WINMENUS
#define INCL_WINSYS

#include <os2.h>
USHORT APIENTRY WinQueryProfileData ( HAB , PSZ , PSZ , PVOID , PUSHORT );
USHORT APIENTRY WinWriteProfileData ( HAB , PSZ , PSZ , PVOID , USHORT );

/* Define me and local defines.                                     */
#define MYNAME MONITOR

/* These include files are specific to this program.                */
#include "monitor.h"
#include "match.h"

#define INTERVAL  1000
#define STACKSIZE 350
#define MAXPOINTS 400
#define MAXPOINT  151
#define MINPOINTS  20
#define idMyFrame 1
#define NEXT_POINT 1
#define MAX_BACK  15
#define MAX_FORE  15



/********************************************************/
/* Macro to issue messages to check/uncheck menu items. */
/********************************************************/
#define CHEKMENU(item,value) (WinSendMsg( hwndOptions,\
                  MM_SETITEMATTR,\
                  MPFROM2SHORT( item, TRUE),\
                  MPFROM2SHORT( MIA_CHECKED,\
                                (value ?  MIA_CHECKED\
                                       : ~MIA_CHECKED) ) ) )

#define MAX(a,b) (((a)>(b))?(a):(b))

static char * copyright = "(C) Copyright IBM Corporation 1988 \nIBM Internal Use Only\n";

/* These are global variables within this program                   */

static ULONG    useage[MAXPOINTS+3];   /* The usage history array.      */
static SHORT    maxpoints = MAXPOINT;  /* Maximum points to use         */
static SHORT    maxpoint = MAXPOINT;   /* The maximum point recorded    */
                                       /*   in the useage array.        */
static SHORT    breakpoint = MAXPOINT; /* Point where real data starts  */
static ULONG    counter  = 0L;         /* The counter that measures the */
                                       /*   idle time.                  */
static BOOL     running  = TRUE;       /* Shutdown indicator.           */
static ULONG    interval = INTERVAL;   /* Sampling interval in msecs.   */
static HWND     hwndWindow;            /* Handle of the client window.  */
static HWND     hwndFrame;             /* Handle of the frame window.   */
static HWND     hwndMenuBar;
static HWND     hwndOptions;
static BOOL     dynamic  = FALSE;      /* Dynamic or static maximum.    */
static BOOL     frozen   = FALSE;      /* Frozen/thawed display state.  */
static UCHAR    *no      = "NO";       /* Move this outside later       */
static ULONG    ctldata = FCF_STANDARD & ~FCF_SHELLPOSITION;
                                       /* Set up for a standard window. */
static char *option_names[] = {
    "MENU"
   ,"INTERVAL"
   ,"SMOOTH"
   ,"SLIDE"
   ,"POINTS"
   ,"FOREGROUND"
   ,"BACKGROUND"
   ,"FILL"
   ,"ICON"
   ,"DYNAMIC"
   ,NULL};
#define OPT_MENU      0
#define OPT_INTERVAL  1
#define OPT_SMOOTH    2
#define OPT_SLIDE     3
#define OPT_POINTS    4
#define OPT_FOREG     5
#define OPT_BACKG     6
#define OPT_FILL      7
#define OPT_ICON      8
#define OPT_DYNAMIC   9

static char *color_names[] = {
    "WHITE"
   ,"BLACK"
   ,"BACKGROUND"
   ,"BLUE"
   ,"RED"
   ,"PINK"
   ,"GREEN"
   ,"CYAN"
   ,"YELLOW"
   ,"NEUTRAL"
   ,"DARKGRAY"
   ,"DARKBLUE"
   ,"DARKRED"
   ,"DARKPINK"
   ,"DARKGREEN"
   ,"DARKCYAN"
   ,"BROWN"
   ,"PALEGRAY"
   ,"WINDOWSTATICTEXT"
   ,"SCROLLBAR"
   ,"SYSBACKGROUND"
   ,"ACTIVETITLE"
   ,"INACTIVETITLE"
   ,"MENU"
   ,"WINDOW"
   ,"WINDOWFRAME"
   ,"MENUTEXT"
   ,"WINDOWTEXT"
   ,"TITLETEXT"
   ,"ACTIVEBORDER"
   ,"INACTIVEBORDER"
   ,"APPWORKSPACE"
   ,"HELPBACKGROUND"
   ,"HELPTEXT"
   ,"HELPHILITE"
   ,NULL};

static COLOR colors[] = {
    CLR_WHITE
   ,CLR_BLACK
   ,CLR_BACKGROUND
   ,CLR_BLUE
   ,CLR_RED
   ,CLR_PINK
   ,CLR_GREEN
   ,CLR_CYAN
   ,CLR_YELLOW
   ,CLR_NEUTRAL
   ,CLR_DARKGRAY
   ,CLR_DARKBLUE
   ,CLR_DARKRED
   ,CLR_DARKPINK
   ,CLR_DARKGREEN
   ,CLR_DARKCYAN
   ,CLR_BROWN
   ,CLR_PALEGRAY
   ,SYSCLR_WINDOWSTATICTEXT
   ,SYSCLR_SCROLLBAR
   ,SYSCLR_BACKGROUND
   ,SYSCLR_ACTIVETITLE
   ,SYSCLR_INACTIVETITLE
   ,SYSCLR_MENU
   ,SYSCLR_WINDOW
   ,SYSCLR_WINDOWFRAME
   ,SYSCLR_MENUTEXT
   ,SYSCLR_WINDOWTEXT
   ,SYSCLR_TITLETEXT
   ,SYSCLR_ACTIVEBORDER
   ,SYSCLR_INACTIVEBORDER
   ,SYSCLR_APPWORKSPACE
   ,SYSCLR_HELPBACKGROUND
   ,SYSCLR_HELPTEXT
   ,SYSCLR_HELPHILITE
   };


  /*******************************************************************/
  /*                                                                 */
  /*  This structure holds all of the personal preference data that  */
  /*  is stored across invocations of the program.                   */
  /*                                                                 */
  /*******************************************************************/

static struct INITDATA {
       COLOR    background;            /* Background color.             */
       COLOR    line_color;            /* Graph line color.             */
       BOOL     smoothed;              /* Smooth/normal graph state.    */
       SWP      wp;                    /* Window size/position.         */
       BOOL     slide;                 /* Sliding vs. jump graph.       */
       BOOL     fill;                  /* Fill below the graph.         */
       } init,saveinit;



static UCHAR    *program_name = "CPU Utilization Monitor";



/* Prototypes of all function calls                             */

MRESULT EXPENTRY MyWindowProc(HWND,USHORT,MPARAM,MPARAM);
void APIENTRY IdleLoopThread(void);
void APIENTRY MonitorLoopThread(void);
void cdecl main(int, char * *, char * *);


  /***********************************************************************/
  /*                                                                     */
  /*                This is the IdleLoopThread.                          */
  /*                                                                     */
  /* It sets itself to an IDLE class priority and falls into a loop that */
  /* counts forever.                                                     */
  /*                                                                     */
  /***********************************************************************/

void APIENTRY IdleLoopThread()
{
   if (DosSetPrty(PRTYS_THREAD,PRTYC_IDLETIME,-31,0)) {
      WinPostMsg(hwndFrame,WM_CLOSE,0L,0L);
   } else {
      while (running) {
         counter++;
      } /* endwhile */
   } /* endif */
   DosExit(EXIT_THREAD,0);
}

  /********************************************************************/
  /*                                                                  */
  /*                This is the MonitorLoopThread.                    */
  /*                                                                  */
  /* It sets a recurrent timer and then calculates statistics on each */
  /* timer pop and sends the free time to the window procedure.       */
  /*                                                                  */
  /********************************************************************/


void APIENTRY MonitorLoopThread()
{
   ULONG  last = 0L;         /* The counter value at last check         */
   PGINFOSEG pgseg  = NULL;  /* A far pointer for the Global Info Seg   */
                             /* We will get the time from here.         */
   PLINFOSEG lgseg  = NULL;  /* A pointer to the Local Info Seg         */
   ULONG  maxcount = 0L;     /* The maximum change seen so far          */
   LONG   current;           /* The change in counter value this time   */
   ULONG  last_msecs;        /* The millisecond value at our last check */
   ULONG  current_msecs;     /* The millisec. value now.                */
   ULONG  delta_msec;        /* The number of millisecs between checks  */
   USHORT max_times = 0;     /* Time since maximum seen                 */
   ULONG  sub_max = 0;       /* Secondary maximum                       */

   if (DosSetPrty(PRTYS_THREAD,PRTYC_TIMECRITICAL,+31,0)) {
      WinPostMsg(hwndFrame,WM_CLOSE,0L,0L);
   } else if (DosGetInfoSeg(&SELECTOROF(pgseg),&SELECTOROF(lgseg))) {
      WinPostMsg(hwndFrame,WM_CLOSE,0L,0L);
   } else {
      last_msecs = pgseg->msecs;
      while (running) {
         DosSleep(interval);
         current_msecs = pgseg->msecs;
         current = counter - last;
         last = counter;
         delta_msec = current_msecs - last_msecs;
         last_msecs = current_msecs;
         current = (current * 1000) / delta_msec;
         if (running) {
            WinPostMsg(hwndWindow,WM_USER+NEXT_POINT
                      ,MPFROMLONG(current)
                      ,MPFROMLONG(pgseg->time));
         } /* endif */
      } /* endwhile */
   } /* endif */
   DosExit(EXIT_THREAD,0);
}

   /*******************************************************************/
   /*                                                                 */
   /* This routine process the command line arguments to check for    */
   /* any requests to modify the default or saved values.             */
   /*                                                                 */
   /*******************************************************************/
void process_args(int argc, char **argv)
{
   USHORT option;
   BOOL no;
   char *arg_ptr;
   int temp;

   while (--argc) {
      if (strnicmp("no",arg_ptr = *++argv,2)) {
         no = FALSE;
      } else {
         no = TRUE;
         arg_ptr += 2;
      }
      if (NO_MATCH == (option = match(option_names,arg_ptr))) {
         /* Do something good here! */
         /* Until then, ignore option! */
      } else {
         switch (option) {
            case OPT_MENU:
               if (no) {
                  ctldata &= ~FCF_MENU;
               } else {
                  ctldata |= FCF_MENU;
               }
               break;
            case OPT_ICON:
               if (no) {
                  ctldata &= ~FCF_ICON;
               } else {
                  ctldata |= FCF_ICON;
               } /* endif */
               break;
            case OPT_INTERVAL:
               interval = atol(*++argv);
               argc--;
               break;
            case OPT_DYNAMIC:
               dynamic = !no;
               break;
            case OPT_SMOOTH:
               init.smoothed = !no;
               break;
            case OPT_SLIDE:
               init.slide = !no;
               break;
            case OPT_POINTS:
               temp = atoi(*++argv);
               if (temp>MINPOINTS && temp<=MAXPOINTS) {
                  maxpoints = temp;
                  breakpoint = temp;
                  maxpoint  = maxpoints;
               }
               argc--;
               break;
            case OPT_FOREG:
               if (NO_MATCH != (temp = match(color_names,*++argv))) {
                  init.line_color = colors[temp];
               }
               argc--;
               break;
            case OPT_BACKG:
               if (NO_MATCH != (temp = match(color_names,*++argv))) {
                  init.background = colors[temp];
               }
               argc--;
               break;
            case OPT_FILL:
               init.fill = !no;
               break;

            default: /* This is bad, shouldn't get here! */
               break;
         }
      }
   }
   return;
}


   /*******************************************************************/
   /*                                                                 */
   /*                  Here is the main program.                      */
   /* It sets up the PM environment and enter the message loop until  */
   /* terminated.                                                     */
   /*                                                                 */
   /*******************************************************************/

void cdecl main(argc, argv, envp)
   int argc;
   char **argv;
   char **envp;
{
   UCHAR  stack1[STACKSIZE]; /* The stack for the idle loop thread.     */
   UCHAR  stack2[STACKSIZE]; /* The stack for the monitor thread.       */
   HMQ    hMsgQ;             /* The PM message Q handle.                */
   QMSG   qMsg;              /* A message structure to hold the message */
                             /*   currently being processed.            */


   TID    IdleTID;           /* The thread ID for the idle loop thread. */
   TID    MonitorTID;        /* The thread ID for the monitor thread.   */
   USHORT i;                 /* Just used for clearing the storage array*/
   HAB    hAB;               /* Anchor block handle.                    */
   MENUITEM Mi;



   if (!(hAB = WinInitialize(NULL))) {  /* Initialize the PM interface.  */
      exit(1);                          /* Error in WinInitialize!       */
   } /* endif */
                                        /* Start up the message queue    */
   if (!(hMsgQ = WinCreateMsgQueue(hAB,0))) {
      exit(1);                          /* Error creating message queue! */
   } /* endif */

   WinRegisterClass(hAB                 /* Register the Window class     */
                   ,"MonitorWindow"
                   ,MyWindowProc
                   ,CS_SIZEREDRAW
                   ,0);

   for (i = 0; i<=maxpoints; i++) {     /* Initialize the storage array. */
      useage[i] = 0xffffffffl;
   } /* endfor */

   /* Get our saved profile data                */
   i = sizeof init;
   if (!WinQueryProfileData(hAB,program_name
                ,"Monitor window data"
                ,&init
                ,&i) ) {
      init.background = CLR_DARKGREEN;  /* No data, so initialize to    */
      init.line_color = CLR_BLUE;       /*   default values.            */
      init.smoothed   = FALSE;
      init.wp.x       = 100;
      init.wp.y       = 100;
      init.wp.cx      = 220;
      init.wp.cy      = 134;
      init.fill       = FALSE;
   } else if (i < (sizeof init)) {
      init.fill       = FALSE;
   } /* endif */
   saveinit = init;                     /* Save the initial values to   */
                                        /*   check against later.       */

   process_args(argc,argv);



   hwndFrame = WinCreateStdWindow       /* And create the window.        */
                      (HWND_DESKTOP
                      ,0L
                      ,&ctldata
                      ,"MonitorWindow"
                      ,""
                      ,0L
                      ,NULL
                      ,ID_MONITOR
                      ,&hwndWindow);

   if ((NULL == hwndFrame) || (NULL == hwndWindow)) {
      /* Make sure the window was created.      */
      exit(1);
   } /* endif */

   WinSetWindowPos(hwndFrame            /* Position the window where it  */
                  ,HWND_TOP             /* was last time.                */
                  ,init.wp.x
                  ,init.wp.y
                  ,init.wp.cx
                  ,init.wp.cy
                  ,SWP_SIZE | SWP_MOVE | SWP_ACTIVATE | SWP_SHOW);

   /* Get infor for modifying menu items with check marks.      */
   hwndMenuBar = WinWindowFromID( hwndFrame, FID_MENU    );
   WinSendMsg( hwndMenuBar
             , MM_QUERYITEM
             , MPFROM2SHORT( IDMOPTIONS, FALSE)
             ,(MPARAM)(PSZ)&Mi
             );
   hwndOptions = Mi.hwndSubMenu;
   CHEKMENU(IDMSMOOTH,init.smoothed);
   CHEKMENU(IDMSLIDE,init.slide);
   CHEKMENU(IDMFILL,init.fill);

   /* Start the idle loop thread.            */

   if (DosCreateThread((void far *)IdleLoopThread
                           ,&IdleTID
                           ,stack1+STACKSIZE-2)) {
      WinPostMsg(hwndWindow,WM_QUIT,0L,0L);  /* Quit if it failed.       */


   } else if (DosCreateThread((void far *)MonitorLoopThread
                           ,&MonitorTID      /* Start the monitor thread.*/
                           ,stack2+STACKSIZE-2)) {
      WinPostMsg(hwndWindow,WM_QUIT,0L,0L);  /* Quit if it failed.       */
   } /* endif */


   /* This is the main message loop. Stay here until told to quit       */

   while (WinGetMsg(hAB,(PQMSG)&qMsg,(HWND)NULL,0,0)) {
      WinDispatchMsg(hAB,(PQMSG)&qMsg);
   } /* endwhile */


   /* If the preference data has changed, then save it. */

   WinQueryWindowPos(hwndFrame,&init.wp);
   if (memcmp(&init,&saveinit,sizeof init)) {
      WinWriteProfileData(hAB,program_name
                         ,"Monitor window data"
                         ,&init
                         ,sizeof init);
   } /* endif */

   WinDestroyWindow(hwndFrame);
   WinDestroyMsgQueue(hMsgQ);
   WinTerminate(hAB);
   DosExit(EXIT_PROCESS,0);
}

   /**********************************************************************/
   /*                                                                    */
   /*                  This is the window procedure.                     */
   /* It is the heart of the user interface. All work related to the     */
   /* input and output of the program are located here. All work         */
   /* requests are presented as messages. Currently, this procedure only */
   /* handles WM_PAINT, WM_CLOSE, WM_COMMAND and NEXT_POINT messages.    */
   /*                                                                    */
   /**********************************************************************/

MRESULT EXPENTRY MyWindowProc(HWND hWnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
   HPS      hPS;                        /* Handle to the presentation   */
                                        /*   used for PAINTing.         */
   RECTL    rect;                       /* Rectangle for background     */
   USHORT   shift_size;                 /* Temporary variable for       */
                                        /*   shifting data in the       */
                                        /*   useage array.              */
   SHORT    loop_var;                   /* Temporary for loop control.  */
   ULONG    xsize,ysize;                /* Use for window sizes.        */
static   ULONG    max = 0;
static   POINTL   points[MAXPOINTS+3];  /* Storage for data to graph.   */
static   POINTL beginpt = {0,0};
static   POINTL endpt = {0,0};

   switch (msg) {

   case WM_USER+NEXT_POINT:             /* This message holds the next  */
                                        /*   data point to graph.       */
      if (maxpoint<maxpoints) {
         maxpoint++;
      } else if (init.slide) {
         if (breakpoint>=0) breakpoint--;
         for (loop_var=0; loop_var<maxpoints; loop_var++) {
            useage[loop_var] = useage[loop_var+1];
         } /* endfor */
      } else {
         shift_size = maxpoint/3;
         breakpoint -= (breakpoint>=shift_size) ? shift_size : (breakpoint+1);
         for (loop_var=0; loop_var<maxpoints-shift_size; loop_var++) {
            useage[loop_var] = useage[loop_var+shift_size];
         } /* endfor */
         maxpoint = maxpoints-shift_size;
      } /* endif */
      useage[maxpoint] = LONGFROMMP(mp1);
      if (!frozen) {
         WinInvalidateRegion(hWnd,NULL,FALSE);
      } /* endif */
      if (!dynamic) {
         max = MAX(max,LONGFROMMP(mp1));
      } /* endif */
      break;

   case WM_PAINT:                       /* This message requests a      */
                                        /*   re-drawing of the window.  */
      hPS = WinBeginPaint(hWnd,(HPS)NULL,&rect);

      WinQueryWindowRect(hWnd,&rect);
      if (dynamic) {
         max = 0;
         for (loop_var = breakpoint+1; loop_var<=maxpoint; loop_var++) {
            max = MAX(useage[loop_var],max);
         } /* endfor */
      } /* endif */
      if (!max) max = 1;
      WinFillRect(hPS,&rect,init.background);
      endpt.x = rect.xRight - rect.xLeft + 1;
      xsize = endpt.x * 1000 / (maxpoints);
      ysize = rect.yTop   - rect.yBottom - 1;

      for (loop_var = 0; loop_var<=maxpoint; loop_var++) {
         points[loop_var].x = loop_var * xsize / 1000;
         points[loop_var].y = ( (loop_var > breakpoint)
                              ? ((max - useage[loop_var])* ysize /max)
                              : 0);
      } /* endfor */
      endpt.x = points[maxpoint].x;

      GpiSetColor(hPS,init.line_color);
      if (maxpoint>1) {
         if (init.fill) {
            GpiBeginArea(hPS,BA_BOUNDARY);
            GpiMove(hPS,&beginpt);
            GpiLine(hPS,points);
         } else {
            GpiMove(hPS,points);
         }
         if (init.smoothed) {
            GpiPolyFillet(hPS,(ULONG)(maxpoint+1),points);
         } else {
            GpiPolyLine(hPS,(ULONG)(maxpoint+1),points);
         } /* endif */
         if (init.fill) {
            GpiLine(hPS,&endpt);
            GpiEndArea(hPS);
         }
      } /* endif */
      WinEndPaint(hPS);
      break;

   case WM_CLOSE:                       /* This message is a shutdown   */
                                        /*   request.                   */
      running = FALSE;
      WinPostMsg(hWnd,WM_QUIT,0L,0L);
      break;

   case WM_COMMAND:                     /* This message is a command    */
                                        /*   from the menu or an        */
                                        /*   accelerator key.           */
      switch (SHORT1FROMMP(mp1)) {

      case IDMEXITNOW:                  /* Another request to exit.     */
         WinPostMsg(hWnd,WM_CLOSE,0L,0L);
         break;

      case IDMBACKG:                    /* Cycle to next background color*/
         init.background++;
         if (init.background>MAX_BACK) {
            init.background = 0;
         } /* endif */
         break;

      case IDMFOREG:                    /* Cycle to next foreground color*/
         init.line_color++;
         if (init.line_color>MAX_FORE) {
            init.line_color = 0;
         } /* endif */
         break;

      case IDMSMOOTH:                   /* Flip between smooth and      */
                                        /*   simple graph.              */
         init.smoothed = !init.smoothed;
         CHEKMENU(IDMSMOOTH,init.smoothed);
         break;

      case IDMSLIDE:                    /* Flip between sliding and     */
                                        /*   shifting graph.            */
         init.slide = !init.slide;
         CHEKMENU(IDMSLIDE,init.slide);
         break;

      case IDMFILL:                     /* Flip between fill and simple */
                                        /*   graph.                     */
         init.fill = !init.fill;
         CHEKMENU(IDMFILL,init.fill);
         break;

      case IDMFREEZE:                   /* Flip between frozen and      */
                                        /*   thawed display.            */
         frozen = !frozen;
         CHEKMENU(IDMFREEZE,frozen);
         break;

      default:
         return ( WinDefWindowProc(hWnd,msg,mp1,mp2) );
      } /* endswitch */
      break;

   default:
      return ( WinDefWindowProc(hWnd,msg,mp1,mp2) );
   } /* endswitch */

   return FALSE;
}

