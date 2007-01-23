/*
** c_console.cpp
** Implements the console itself
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "m_alloc.h"
#include "templates.h"
#include "p_setup.h"
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "version.h"
#include "g_game.h"
#include "c_console.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "i_video.h"
#include "i_input.h"
#include "m_swap.h"
#include "v_palette.h"
#include "v_video.h"
#include "v_text.h"
#include "w_wad.h"
#include "r_main.h"
#include "r_draw.h"
#include "sbar.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "doomstat.h"
#include "d_gui.h"
#include "v_video.h"

#include "gi.h"

#define CONSOLESIZE	16384	// Number of characters to store in console
#define CONSOLELINES 256	// Max number of lines of console text
#define LINEMASK (CONSOLELINES-1)

#define LEFTMARGIN 8
#define RIGHTMARGIN 8
#define BOTTOMARGIN 12

static void C_TabComplete (bool goForward);
static bool C_TabCompleteList ();
static bool TabbedLast;		// True if last key pressed was tab
static bool TabbedList;		// True if tab list was shown
CVAR (Bool, con_notablist, false, CVAR_ARCHIVE)

static int conback;
static BYTE conshade[256];
static bool conline;

extern int		gametic;
extern bool		automapactive;	// in AM_map.c
extern bool		advancedemo;

extern FBaseCVar *CVars;
extern FConsoleCommand *Commands[FConsoleCommand::HASH_SIZE];

int			ConCols, PhysRows;
bool		vidactive = false, gotconback = false;
bool		cursoron = false;
int			ConBottom, ConScroll, RowAdjust;
int			CursorTicker;
constate_e	ConsoleState = c_up;

static char ConsoleBuffer[CONSOLESIZE];
static char *Lines[CONSOLELINES];
static bool LineJoins[CONSOLELINES];

static int TopLine, InsertLine;
static char *BufferRover = ConsoleBuffer;

static void ClearConsole ();

struct GameAtExit
{
	GameAtExit *Next;
	char Command[1];
};

static GameAtExit *ExitCmdList;

#define SCROLLUP 1
#define SCROLLDN 2
#define SCROLLNO 0

EXTERN_CVAR (Bool, show_messages)

static unsigned int TickerAt, TickerMax;
static bool TickerPercent;
static const char *TickerLabel;

static bool TickerVisible;
static bool ConsoleDrawing;

// Buffer for AddToConsole()
static char *work = NULL;
static int worklen = 0;


struct History
{
	struct History *Older;
	struct History *Newer;
	char String[1];
};

// CmdLine[0]  = # of chars on command line
// CmdLine[1]  = cursor position
// CmdLine[2+] = command line (max 255 chars + NULL)
// CmdLine[259]= offset from beginning of cmdline to display
static BYTE CmdLine[260];

#define MAXHISTSIZE 50
static struct History *HistHead = NULL, *HistTail = NULL, *HistPos = NULL;
static int HistSize;

CVAR (Float, con_notifytime, 3.f, CVAR_ARCHIVE)
CVAR (Bool, con_centernotify, false, CVAR_ARCHIVE)
CUSTOM_CVAR (Int, con_scaletext, 0, CVAR_ARCHIVE)		// Scale notify text at high resolutions?
{
	if (self < 0) self = 0;
	if (self > 2) self = 2;
}

// Command to run when Ctrl-D is pressed at start of line
CVAR (String, con_ctrl_d, "", CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

#define NUMNOTIFIES 4
#define NOTIFYFADETIME 6

static struct NotifyText
{
	int TimeOut;
	int PrintLevel;
	FString Text;
} NotifyStrings[NUMNOTIFIES];

static int NotifyTop, NotifyTopGoal;

#define PRINTLEVELS 5
int PrintColors[PRINTLEVELS+2] = { CR_RED, CR_GOLD, CR_GRAY, CR_GREEN, CR_GREEN, CR_GOLD };

static void setmsgcolor (int index, int color);

FILE *Logfile = NULL;

void C_AddNotifyString (int printlevel, const char *source);


FIntCVar msglevel ("msg", 0, CVAR_ARCHIVE);

CUSTOM_CVAR (Int, msg0color, 6, CVAR_ARCHIVE)
{
	setmsgcolor (0, self);
}

CUSTOM_CVAR (Int, msg1color, 5, CVAR_ARCHIVE)
{
	setmsgcolor (1, self);
}

CUSTOM_CVAR (Int, msg2color, 2, CVAR_ARCHIVE)
{
	setmsgcolor (2, self);
}

CUSTOM_CVAR (Int, msg3color, 3, CVAR_ARCHIVE)
{
	setmsgcolor (3, self);
}

CUSTOM_CVAR (Int, msg4color, 3, CVAR_ARCHIVE)
{
	setmsgcolor (4, self);
}

CUSTOM_CVAR (Int, msgmidcolor, 5, CVAR_ARCHIVE)
{
	setmsgcolor (PRINTLEVELS, self);
}

CUSTOM_CVAR (Int, msgmidcolor2, 4, CVAR_ARCHIVE)
{
	setmsgcolor (PRINTLEVELS+1, self);
}

static void maybedrawnow (bool tick, bool force)
{
	if (ConsoleDrawing || !gotconback || screen->IsLocked ())
	{
		return;
	}

	if (vidactive &&
		(((tick || gameaction != ga_nothing) && ConsoleState == c_down)
		|| gamestate == GS_STARTUP))
	{
		static size_t lastprinttime = 0;
		size_t nowtime = I_GetTime(false);

		if (nowtime - lastprinttime > 1 || force)
		{
			screen->Lock (false);
			C_DrawConsole ();
			screen->Update ();
			lastprinttime = nowtime;
		}
	}
}

struct TextQueue
{
	TextQueue (bool notify, int printlevel, const char *text)
		: Next(NULL), bNotify(notify), PrintLevel(printlevel), Text(text)
	{
	}
	TextQueue *Next;
	bool bNotify;
	int PrintLevel;
	FString Text;
};

TextQueue *EnqueuedText, **EnqueuedTextTail = &EnqueuedText;

void EnqueueConsoleText (bool notify, int printlevel, const char *text)
{
	TextQueue *queued = new TextQueue (notify, printlevel, text);
	*EnqueuedTextTail = queued;
	EnqueuedTextTail = &queued->Next;
}

void DequeueConsoleText ()
{
	TextQueue *queued = EnqueuedText;

	while (queued != NULL)
	{
		TextQueue *next = queued->Next;
		if (queued->bNotify)
		{
			C_AddNotifyString (queued->PrintLevel, queued->Text);
		}
		else
		{
			AddToConsole (queued->PrintLevel, queued->Text);
		}
		delete queued;
		queued = next;
	}
	EnqueuedText = NULL;
	EnqueuedTextTail = &EnqueuedText;
}

void C_InitConsole (int width, int height, bool ingame)
{
	if ( (vidactive = ingame) )
	{
		if (!gotconback)
		{
			int i;

			conback = TexMan.CheckForTexture ("CONBACK", FTexture::TEX_MiscPatch);

			if (conback <= 0)
			{
				BYTE unremap[256];
				BYTE shadetmp[256];

				conback = TexMan.GetTexture (gameinfo.titlePage, FTexture::TEX_MiscPatch);

				FWadLump palookup = Wads.OpenLumpName ("COLORMAP");
				palookup.Seek (22*256, SEEK_CUR);
				palookup.Read (shadetmp, 256);
				memset (unremap, 0, 256);
				for (i = 0; i < 256; ++i)
				{
					unremap[GPalette.Remap[i]] = i;
				}
				for (i = 0; i < 256; ++i)
				{
					conshade[i] = GPalette.Remap[shadetmp[unremap[i]]];
				}
				conline = true;
				conshade[0] = GPalette.Remap[0];
			}
			else
			{
				for (i = 0; i < 256; ++i)
				{
					conshade[i] = i;
				}
				conline = false;
			}

			gotconback = true;
		}
	}

	int cwidth, cheight;

	if (ConFont != NULL)
	{
		cwidth = ConFont->GetCharWidth ('M');
		cheight = ConFont->GetHeight();
	}
	else
	{
		cwidth = cheight = 8;
	}
	ConCols = (width - LEFTMARGIN - RIGHTMARGIN) / cwidth;
	PhysRows = height / cheight;

	// If there is some text in the console buffer, reformat it
	// for the new resolution.
	if (TopLine != InsertLine)
	{
		// Note: Don't use new here, because we attach a handler to new in
		// i_main.cpp that calls I_FatalError if the allocation fails,
		// but we can gracefully handle such a condition here by just
		// clearing the console buffer. (OTOH, what are the chances that
		// any other memory allocations would succeed if we can't get
		// these mallocs here?)

		char *fmtBuff = (char *)malloc (CONSOLESIZE);
		char **fmtLines = (char **)malloc (CONSOLELINES*sizeof(char*)*4);
		int out = 0;

		if (fmtBuff && fmtLines)
		{
			int in;
			char *fmtpos;
			bool newline = true;

			fmtpos = fmtBuff;
			memset (fmtBuff, 0, CONSOLESIZE);

			for (in = TopLine; in != InsertLine; in = (in + 1) & LINEMASK)
			{
				size_t len = strlen (Lines[in]);

				if (fmtpos + len + 2 - fmtBuff > CONSOLESIZE)
				{
					break;
				}
				if (newline)
				{
					newline = false;
					fmtLines[out++] = fmtpos;
				}
				strcpy (fmtpos, Lines[in]);
				fmtpos += len;
				if (!LineJoins[in])
				{
					*fmtpos++ = '\n';
					fmtpos++;
					if (out == CONSOLELINES*4)
					{
						break;
					}
					newline = true;
				}
			}
		}

		ClearConsole ();

		if (fmtBuff && fmtLines)
		{
			int i;

			for (i = 0; i < out; i++)
			{
				AddToConsole (-1, fmtLines[i]);
			}
		}

		if (fmtBuff)
			free (fmtBuff);
		if (fmtLines)
			free (fmtLines);
	}
}

//==========================================================================
//
// CCMD atexit
//
//==========================================================================

CCMD (atexit)
{
	if (argv.argc() == 1)
	{
		Printf ("Registered atexit commands:\n");
		GameAtExit *record = ExitCmdList;
		while (record != NULL)
		{
			Printf ("%s\n", record->Command);
			record = record->Next;
		}
		return;
	}
	for (int i = 1; i < argv.argc(); ++i)
	{
		GameAtExit *record = (GameAtExit *)M_Malloc (
			sizeof(GameAtExit)+strlen(argv[i]));
		strcpy (record->Command, argv[i]);
		record->Next = ExitCmdList;
		ExitCmdList = record;
	}
}

//==========================================================================
//
// C_DeinitConsole
//
// Executes the contents of the atexit cvar, if any, at quit time.
// Then releases all of the console's memory.
//
//==========================================================================

void C_DeinitConsole ()
{
	GameAtExit *cmd = ExitCmdList;

	while (cmd != NULL)
	{
		GameAtExit *next = cmd->Next;
		AddCommandString (cmd->Command);
		free (cmd);
		cmd = next;
	}

	// Free command history
	History *hist = HistTail;

	while (hist != NULL)
	{
		History *next = hist->Newer;
		free (hist);
		hist = next;
	}
	HistTail = HistHead = HistPos = NULL;

	// Free cvars allocated at runtime
	FBaseCVar *var, *next, **nextp;
	for (var = CVars, nextp = &CVars; var != NULL; var = next)
	{
		next = var->m_Next;
		if (var->GetFlags() & CVAR_UNSETTABLE)
		{
			delete var;
			*nextp = next;
		}
		else
		{
			nextp = &var->m_Next;
		}
	}

	// Free alias commands. (i.e. The "commands" that can be allocated
	// at runtime.)
	for (size_t i = 0; i < countof(Commands); ++i)
	{
		FConsoleCommand *cmd = Commands[i];

		while (cmd != NULL)
		{
			FConsoleCommand *next = cmd->m_Next;
			if (cmd->IsAlias())
			{
				delete cmd;
			}
			cmd = next;
		}
	}

	// Make sure all tab commands are cleared before the memory for
	// their names is deallocated.
	C_ClearTabCommands ();

	// Free AddToConsole()'s work buffer
	if (work != NULL)
	{
		free (work);
		work = NULL;
		worklen = 0;
	}
}

static void ClearConsole ()
{
	RowAdjust = 0;
	TopLine = InsertLine = 0;
	BufferRover = ConsoleBuffer;
	memset (ConsoleBuffer, 0, CONSOLESIZE);
	memset (Lines, 0, sizeof(Lines));
	memset (LineJoins, 0, sizeof(LineJoins));
}

static void setmsgcolor (int index, int color)
{
	if ((unsigned)color >= (unsigned)NUM_TEXT_COLORS)
		color = 0;
	PrintColors[index] = color;
}

extern int DisplayWidth;

void C_AddNotifyString (int printlevel, const char *source)
{
	static enum
	{
		NEWLINE,
		APPENDLINE,
		REPLACELINE
	} addtype = NEWLINE;

	FBrokenLines *lines;
	int i, len, width;

	if ((printlevel != 128 && !show_messages) ||
		!(len = (int)strlen (source)) ||
		gamestate == GS_FULLCONSOLE ||
		gamestate == GS_DEMOSCREEN)
		return;

	if (ConsoleDrawing)
	{
		EnqueueConsoleText (true, printlevel, source);
		return;
	}

	width = con_scaletext > 1 ? DisplayWidth/2 : con_scaletext == 1 ? DisplayWidth / CleanXfac : DisplayWidth;

	if (addtype == APPENDLINE && NotifyStrings[NUMNOTIFIES-1].PrintLevel == printlevel)
	{
		FString str = NotifyStrings[NUMNOTIFIES-1].Text + source;
		lines = V_BreakLines (screen->Font, width, str);
	}
	else
	{
		lines = V_BreakLines (screen->Font, width, source);
		addtype = (addtype == APPENDLINE) ? NEWLINE : addtype;
	}

	if (lines == NULL)
		return;

	for (i = 0; lines[i].Width >= 0; i++)
	{
		if (addtype == NEWLINE)
		{
			for (int j = 0; j < NUMNOTIFIES-1; ++j)
			{
				NotifyStrings[j] = NotifyStrings[j+1];
			}
		}
		NotifyStrings[NUMNOTIFIES-1].Text = lines[i].Text;
		NotifyStrings[NUMNOTIFIES-1].TimeOut = gametic + (int)(con_notifytime * TICRATE);
		NotifyStrings[NUMNOTIFIES-1].PrintLevel = printlevel;
		addtype = NEWLINE;
	}

	V_FreeBrokenLines (lines);
	lines = NULL;

	switch (source[len-1])
	{
	case '\r':	addtype = REPLACELINE;	break;
	case '\n':	addtype = NEWLINE;		break;
	default:	addtype = APPENDLINE;	break;
	}

	NotifyTopGoal = 0;
}

static int FlushLines (const char *start, const char *stop)
{
	int i;

	for (i = TopLine; i != InsertLine; i = (i + 1) & LINEMASK)
	{
		if (Lines[i] < stop && Lines[i] + strlen (Lines[i]) > start)
		{
			Lines[i] = NULL;
		}
		else
		{
			break;
		}
	}
	if (i != TopLine)
		i = i;
	return i;
}

static void AddLine (const char *text, bool more, int len)
{
	if (BufferRover + len + 1 - ConsoleBuffer > CONSOLESIZE)
	{
		TopLine = FlushLines (BufferRover, ConsoleBuffer + CONSOLESIZE);
		BufferRover = ConsoleBuffer;
	}
	if (len >= CONSOLESIZE - 1)
	{
		text = text + len - CONSOLESIZE + 1;
		len = CONSOLESIZE - 1;
	}
	TopLine = FlushLines (BufferRover, BufferRover + len + 1);
	memcpy (BufferRover, text, len);
	BufferRover[len] = 0;
	Lines[InsertLine] = BufferRover;
	BufferRover += len + 1;
	LineJoins[InsertLine] = more;
	InsertLine = (InsertLine + 1) & LINEMASK;
	if (InsertLine == TopLine)
	{
		TopLine = (TopLine + 1) & LINEMASK;
	}
}

void AddToConsole (int printlevel, const char *text)
{
	static enum
	{
		NEWLINE,
		APPENDLINE,
		REPLACELINE
	} addtype = NEWLINE;

	char *work_p;
	char *linestart;
	FString cc('A' + char(CR_TAN));
	int size, len;
	int x;
	int maxwidth;

	if (ConsoleDrawing)
	{
		EnqueueConsoleText (false, printlevel, text);
		return;
	}

	len = (int)strlen (text);
	size = len + 3;

	if (addtype != NEWLINE)
	{
		InsertLine = (InsertLine - 1) & LINEMASK;
		if (Lines[InsertLine] == NULL)
		{
			InsertLine = (InsertLine + 1) & LINEMASK;
			addtype = NEWLINE;
		}
		else
		{
			BufferRover = Lines[InsertLine];
		}
	}
	if (addtype == APPENDLINE)
	{
		size += (int)strlen (Lines[InsertLine]);
	}
	if (size > worklen)
	{
		work = (char *)M_Realloc (work, size);
		worklen = size;
	}
	if (work == NULL)
	{
		static char oom[] = TEXTCOLOR_RED "*** OUT OF MEMORY ***";
		work = oom;
		worklen = 0;
	}
	else
	{
		if (addtype == APPENDLINE)
		{
			strcpy (work, Lines[InsertLine]);
			strcat (work, text);
		}
		else if (printlevel >= 0)
		{
			work[0] = TEXTCOLOR_ESCAPE;
			work[1] = 'A' + (printlevel == PRINT_HIGH ? CR_TAN :
						printlevel == 200 ? CR_GREEN :
						printlevel < PRINTLEVELS ? PrintColors[printlevel] :
						CR_TAN);
			cc = work[1];
			strcpy (work + 2, text);
		}
		else
		{
			strcpy (work, text);
		}
	}

	work_p = linestart = work;

	if (ConFont != NULL && screen != NULL)
	{
		x = 0;
		maxwidth = screen->GetWidth() - LEFTMARGIN - RIGHTMARGIN;

		while (*work_p)
		{
			if (*work_p == TEXTCOLOR_ESCAPE)
			{
				work_p++;
				if (*work_p == '[')
				{
					char *start = work_p;
					while (*work_p != ']' && *work_p != '\0')
					{
						work_p++;
					}
					if (*work_p != '\0')
					{
						work_p++;
					}
					cc = FString(start, work_p - start);
				}
				else if (*work_p != '\0')
				{
					cc = *work_p++;
				}
				continue;
			}
			int w = ConFont->GetCharWidth (*work_p);
			if (*work_p == '\n' || x + w > maxwidth)
			{
				AddLine (linestart, *work_p != '\n', work_p - linestart);
				if (*work_p == '\n')
				{
					x = 0;
					work_p++;
				}
				else
				{
					x = w;
				}
				if (*work_p)
				{
					linestart = work_p - 1 - cc.Len();
					linestart[0] = TEXTCOLOR_ESCAPE;
					strncpy (linestart + 1, cc, cc.Len());
				}
				else
				{
					linestart = work_p;
				}
			}
			else
			{
				x += w;
				work_p++;
			}
		}

		if (*linestart)
		{
			AddLine (linestart, true, work_p - linestart);
		}
	}
	else
	{
		while (*work_p)
		{
			if (*work_p++ == '\n')
			{
				AddLine (linestart, false, work_p - linestart - 1);
				linestart = work_p;
			}
		}
		if (*linestart)
		{
			AddLine (linestart, true, work_p - linestart);
		}
	}

	switch (text[len-1])
	{
	case '\r':	addtype = REPLACELINE;	break;
	case '\n':	addtype = NEWLINE;		break;
	default:	addtype = APPENDLINE;	break;
	}
}

/* Adds a string to the console and also to the notify buffer */
int PrintString (int printlevel, const char *outline)
{
	if (printlevel < msglevel || *outline == '\0')
	{
		return 0;
	}

	if (Logfile)
	{
		// Strip out any color escape sequences before writing to the log file
		char * copy = new char[strlen(outline)+1];
		const char * srcp = outline;
		char * dstp = copy;

		while (*srcp != 0)
		{
			if (*srcp!=0x1c)
			{
				*dstp++=*srcp++;
			}
			else
			{
				if (srcp[1]!=0) srcp+=2;
				else break;
			}
		}
		*dstp=0;

		fputs (copy, Logfile);
		delete [] copy;
//#ifdef _DEBUG
		fflush (Logfile);
//#endif
	}

	I_PrintStr (outline);

	AddToConsole (printlevel, outline);
	if (vidactive && screen && screen->Font)
	{
		C_AddNotifyString (printlevel, outline);
		maybedrawnow (false, false);
	}
	return (int)strlen (outline);
}

extern bool gameisdead;

int VPrintf (int printlevel, const char *format, va_list parms)
{
	if (gameisdead)
		return 0;

	FString outline;
	outline.VFormat (format, parms);
	return PrintString (printlevel, outline);
}

int STACK_ARGS Printf (int printlevel, const char *format, ...)
{
	va_list argptr;
	int count;

	va_start (argptr, format);
	count = VPrintf (printlevel, format, argptr);
	va_end (argptr);

	return count;
}

int STACK_ARGS Printf (const char *format, ...)
{
	va_list argptr;
	int count;

	va_start (argptr, format);
	count = VPrintf (PRINT_HIGH, format, argptr);
	va_end (argptr);

	return count;
}

int STACK_ARGS DPrintf (const char *format, ...)
{
	va_list argptr;
	int count;

	if (developer)
	{
		va_start (argptr, format);
		count = VPrintf (PRINT_HIGH, format, argptr);
		va_end (argptr);
		return count;
	}
	else
	{
		return 0;
	}
}

void C_FlushDisplay ()
{
	int i;

	for (i = 0; i < NUMNOTIFIES; i++)
		NotifyStrings[i].TimeOut = 0;
}

void C_AdjustBottom ()
{
	if (gamestate == GS_FULLCONSOLE || gamestate == GS_STARTUP)
		ConBottom = SCREENHEIGHT;
	else if (ConBottom > SCREENHEIGHT / 2 || ConsoleState == c_down)
		ConBottom = SCREENHEIGHT / 2;
}

void C_NewModeAdjust ()
{
	C_InitConsole (SCREENWIDTH, SCREENHEIGHT, true);
	C_FlushDisplay ();
	C_AdjustBottom ();
}

void C_Ticker ()
{
	static int lasttic = 0;

	if (lasttic == 0)
		lasttic = gametic - 1;

	if (ConsoleState != c_up)
	{
		if (ConsoleState == c_falling)
		{
			ConBottom += (gametic - lasttic) * (SCREENHEIGHT*2/25);
			if (ConBottom >= SCREENHEIGHT / 2)
			{
				ConBottom = SCREENHEIGHT / 2;
				ConsoleState = c_down;
			}
		}
		else if (ConsoleState == c_rising)
		{
			ConBottom -= (gametic - lasttic) * (SCREENHEIGHT*2/25);
			if (ConBottom <= 0)
			{
				ConsoleState = c_up;
				ConBottom = 0;
			}
		}
	}

	if (--CursorTicker <= 0)
	{
		cursoron ^= 1;
		CursorTicker = C_BLINKRATE;
	}

	lasttic = gametic;

	if (NotifyTopGoal > NotifyTop)
	{
		NotifyTop++;
	}
	else if (NotifyTopGoal < NotifyTop)
	{
		NotifyTop--;
	}
}

static void C_DrawNotifyText ()
{
	bool center = (con_centernotify != 0.f);
	int i, line, lineadv, color, j, skip;
	bool canskip;
	
	if (gamestate == GS_FULLCONSOLE || gamestate == GS_DEMOSCREEN/* || menuactive != MENU_Off*/)
		return;

	line = NotifyTop;
	skip = 0;
	canskip = true;

	lineadv = SmallFont->GetHeight ();
	if (con_scaletext == 1)
	{
		lineadv *= CleanYfac;
	}

	BorderTopRefresh = screen->GetPageCount ();

	for (i = 0; i < NUMNOTIFIES; i++)
	{
		if (NotifyStrings[i].TimeOut == 0)
			continue;

		j = NotifyStrings[i].TimeOut - gametic;
		if (j > 0)
		{
			if (!show_messages && NotifyStrings[i].PrintLevel != 128)
				continue;

			fixed_t alpha;

			if (j < NOTIFYFADETIME)
			{
				alpha = OPAQUE * j / NOTIFYFADETIME;
			}
			else
			{
				alpha = OPAQUE;
			}

			if (NotifyStrings[i].PrintLevel >= PRINTLEVELS)
				color = CR_UNTRANSLATED;
			else
				color = PrintColors[NotifyStrings[i].PrintLevel];

			if (con_scaletext == 1)
			{
				if (!center)
					screen->DrawText (color, 0, line, NotifyStrings[i].Text,
						DTA_CleanNoMove, true, DTA_Alpha, alpha, TAG_DONE);
				else
					screen->DrawText (color, (SCREENWIDTH -
						SmallFont->StringWidth (NotifyStrings[i].Text)*CleanXfac)/2,
						line, NotifyStrings[i].Text, DTA_CleanNoMove, true,
						DTA_Alpha, alpha, TAG_DONE);
			}
			else if (con_scaletext == 0)
			{
				if (!center)
					screen->DrawText (color, 0, line, NotifyStrings[i].Text,
						DTA_Alpha, alpha, TAG_DONE);
				else
					screen->DrawText (color, (SCREENWIDTH -
						SmallFont->StringWidth (NotifyStrings[i].Text))/2,
						line, NotifyStrings[i].Text,
						DTA_Alpha, alpha, TAG_DONE);
			}
			else
			{
				if (!center)
					screen->DrawText (color, 0, line, NotifyStrings[i].Text,
						DTA_VirtualWidth, screen->GetWidth() / 2, 
						DTA_VirtualHeight, screen->GetHeight() / 2,
						DTA_KeepRatio, true,
						DTA_Alpha, alpha, TAG_DONE);
				else
					screen->DrawText (color, (screen->GetWidth() / 2 -
						SmallFont->StringWidth (NotifyStrings[i].Text))/2,
						line, NotifyStrings[i].Text,
						DTA_VirtualWidth, screen->GetWidth() / 2, 
						DTA_VirtualHeight, screen->GetHeight() / 2,
						DTA_KeepRatio, true,
						DTA_Alpha, alpha, TAG_DONE);
			}
			line += lineadv;
			canskip = false;
		}
		else
		{
			if (canskip)
			{
				NotifyTop += lineadv;
				line += lineadv;
				skip++;
			}
			NotifyStrings[i].TimeOut = 0;
		}
	}
	if (canskip)
	{
		NotifyTop = NotifyTopGoal;
	}
}

void C_InitTicker (const char *label, unsigned int max, bool showpercent)
{
	TickerPercent = showpercent;
	TickerMax = max;
	TickerLabel = label;
	TickerAt = 0;
	maybedrawnow (true, false);
}

void C_SetTicker (unsigned int at, bool forceUpdate)
{
	TickerAt = at > TickerMax ? TickerMax : at;
	maybedrawnow (true, TickerVisible ? forceUpdate : false);
}

void C_DrawConsole ()
{
	static int oldbottom = 0;
	int lines, left, offset;

	left = LEFTMARGIN;
	lines = (ConBottom-ConFont->GetHeight()*2)/ConFont->GetHeight();
	if (-ConFont->GetHeight() + lines*ConFont->GetHeight() > ConBottom - ConFont->GetHeight()*7/2)
	{
		offset = -ConFont->GetHeight()/2;
		lines--;
	}
	else
	{
		offset = -ConFont->GetHeight();
	}

	if ((ConBottom < oldbottom) &&
		(gamestate == GS_LEVEL || gamestate == GS_TITLELEVEL) &&
		(viewwindowx || viewwindowy) &&
		viewactive)
	{
		BorderNeedRefresh = screen->GetPageCount ();
	}

	oldbottom = ConBottom;

	if (ConsoleState == c_up)
	{
		C_DrawNotifyText ();
		return;
	}
	else if (ConBottom)
	{
		int visheight, realheight;
		FTexture *conpic = TexMan[conback];

		visheight = ConBottom;
		realheight = (visheight * conpic->GetHeight()) / SCREENHEIGHT;

		screen->DrawTexture (conpic, 0, visheight - screen->GetHeight(),
			DTA_DestWidth, screen->GetWidth(),
			DTA_DestHeight, screen->GetHeight(),
			DTA_Translation, conshade,
			DTA_Masked, false,
			TAG_DONE);
		if (conline && visheight < screen->GetHeight())
		{
			screen->Clear (0, visheight, screen->GetWidth(), visheight+1, 0);
		}

		if (ConBottom >= 12)
		{
			screen->SetFont (ConFont);
			screen->DrawText (CR_ORANGE, SCREENWIDTH - 8 -
				ConFont->StringWidth ("v" DOTVERSIONSTR),
				ConBottom - ConFont->GetHeight() - 4,
				"v" DOTVERSIONSTR, TAG_DONE);
			if (TickerMax)
			{
				char tickstr[256];
				const int tickerY = ConBottom - ConFont->GetHeight() - 4;
				size_t i;
				int tickend = ConCols - SCREENWIDTH / 90 - 6;
				int tickbegin = 0;

				if (TickerLabel)
				{
					tickbegin = (int)strlen (TickerLabel) + 2;
					sprintf (tickstr, "%s: ", TickerLabel);
				}
				if (tickend > 256 - ConFont->GetCharWidth(0x12))
					tickend = 256 - ConFont->GetCharWidth(0x12);
				tickstr[tickbegin] = 0x10;
				memset (tickstr + tickbegin + 1, 0x11, tickend - tickbegin);
				tickstr[tickend + 1] = 0x12;
				tickstr[tickend + 2] = ' ';
				if (TickerPercent)
				{
					sprintf (tickstr + tickend + 3, "%d%%", Scale (TickerAt, 100, TickerMax));
				}
				else
				{
					tickstr[tickend+3] = 0;
				}
				screen->DrawText (CR_BROWN, LEFTMARGIN, tickerY, tickstr, TAG_DONE);

				// Draw the marker
				i = LEFTMARGIN+5+tickbegin*8 + Scale (TickerAt, (SDWORD)(tickend - tickbegin)*8, TickerMax);
				screen->DrawChar (CR_ORANGE, (int)i, tickerY, 0x13, TAG_DONE);

				TickerVisible = true;
			}
			else
			{
				TickerVisible = false;
			}
		}

		// Apply palette blend effects
		if (StatusBar != NULL)
		{
			player_t *player = StatusBar->CPlayer;
			if (player->camera != NULL && player->camera->player != NULL)
			{
				player = player->camera->player;
			}
			if (player->BlendA != 0 && (gamestate == GS_LEVEL || gamestate == GS_TITLELEVEL))
			{
				screen->Dim (PalEntry ((unsigned char)(player->BlendR*255), (unsigned char)(player->BlendG*255), (unsigned char)(player->BlendB*255)),
					player->BlendA, 0, ConBottom, screen->GetWidth(), screen->GetHeight() - ConBottom);
				SB_state = screen->GetPageCount ();
				BorderNeedRefresh = screen->GetPageCount ();
			}
		}
	}

	if (menuactive != MENU_Off)
	{
		screen->SetFont (SmallFont);
		return;
	}

	if (lines > 0)
	{
		int bottomline = ConBottom - ConFont->GetHeight()*2 - 4;
		int pos = (InsertLine - 1) & LINEMASK;
		int i;

		screen->SetFont (ConFont);

		ConsoleDrawing = true;

		for (i = RowAdjust; i; i--)
		{
			if (pos == TopLine)
			{
				RowAdjust = RowAdjust - i;
				break;
			}
			else
			{
				pos = (pos - 1) & LINEMASK;
			}
		}
		pos++;
		do
		{
			pos = (pos - 1) & LINEMASK;
			if (Lines[pos] != NULL)
			{
				screen->DrawText (CR_TAN, LEFTMARGIN, offset + lines * ConFont->GetHeight(),
					Lines[pos], TAG_DONE);
			}
			lines--;
		} while (pos != TopLine && lines > 0);

		ConsoleDrawing = false;
		DequeueConsoleText ();

		if (ConBottom >= 20)
		{
			if (gamestate != GS_STARTUP)
			{
				// Make a copy of the command line, in case an input event is handled
				// while we draw the console and it changes.
				CmdLine[2+CmdLine[0]] = 0;
				FString command((char *)&CmdLine[2+CmdLine[259]]);
				int cursorpos = CmdLine[1] - CmdLine[259];

				screen->DrawChar (CR_ORANGE, left, bottomline, '\x1c', TAG_DONE);
				screen->DrawText (CR_ORANGE, left + ConFont->GetCharWidth(0x1c), bottomline,
					command, TAG_DONE);

				if (cursoron)
				{
					screen->DrawChar (CR_YELLOW, left + ConFont->GetCharWidth(0x1c) + cursorpos * ConFont->GetCharWidth(0xb),
						bottomline, '\xb', TAG_DONE);
				}
			}
			if (RowAdjust && ConBottom >= ConFont->GetHeight()*7/2)
			{
				// Indicate that the view has been scrolled up (10)
				// and if we can scroll no further (12)
				screen->DrawChar (CR_GREEN, 0, bottomline, pos == TopLine ? 12 : 10, TAG_DONE);
			}
		}
	}
	screen->SetFont (SmallFont);
}

void C_FullConsole ()
{
	if (demoplayback)
		G_CheckDemoStatus ();
	D_QuitNetGame ();
	advancedemo = false;
	ConsoleState = c_down;
	HistPos = NULL;
	TabbedLast = false;
	TabbedList = false;
	if (gamestate != GS_STARTUP)
	{
		gamestate = GS_FULLCONSOLE;
		level.music = NULL;
		S_Start ();
		P_FreeLevelData ();
		V_SetBlend (0,0,0,0);
	}
	else
	{
		C_AdjustBottom ();
	}
}

void C_ToggleConsole ()
{
	if (gamestate == GS_DEMOSCREEN || demoplayback)
	{
		gameaction = ga_fullconsole;
	}
	else if (!chatmodeon && (ConsoleState == c_up || ConsoleState == c_rising) && menuactive == MENU_Off)
	{
		ConsoleState = c_falling;
		HistPos = NULL;
		TabbedLast = false;
		TabbedList = false;
	}
	else if (gamestate != GS_FULLCONSOLE && gamestate != GS_STARTUP)
	{
		ConsoleState = c_rising;
		C_FlushDisplay ();
	}
}

void C_HideConsole ()
{
	if (gamestate != GS_FULLCONSOLE)
	{
		ConsoleState = c_up;
		ConBottom = 0;
		HistPos = NULL;
	}
}

static void makestartposgood ()
{
	int n;
	int pos = CmdLine[259];
	int curs = CmdLine[1];
	int len = CmdLine[0];

	n = pos;

	if (pos >= len)
	{ // Start of visible line is beyond end of line
		n = curs - ConCols + 2;
	}
	if ((curs - pos) >= ConCols - 2)
	{ // The cursor is beyond the visible part of the line
		n = curs - ConCols + 2;
	}
	if (pos > curs)
	{ // The cursor is in front of the visible part of the line
		n = curs;
	}
	if (n < 0)
		n = 0;
	CmdLine[259] = n;
}

static bool C_HandleKey (event_t *ev, BYTE *buffer, int len)
{
	int i;
	int data1 = ev->data1;

	switch (ev->subtype)
	{
	default:
		return false;

	case EV_GUI_Char:
		// Add keypress to command line
		if (buffer[0] < len)
		{
			if (buffer[1] == buffer[0])
			{
				buffer[buffer[0] + 2] = ev->data1;
			}
			else
			{
				char *c, *e;

				e = (char *)&buffer[buffer[0] + 1];
				c = (char *)&buffer[buffer[1] + 2];

				for (; e >= c; e--)
					*(e + 1) = *e;

				*c = ev->data1;
			}
			buffer[0]++;
			buffer[1]++;
			makestartposgood ();
			HistPos = NULL;
		}
		TabbedLast = false;
		TabbedList = false;
		break;

	case EV_GUI_WheelUp:
	case EV_GUI_WheelDown:
		if (!(ev->data3 & GKM_SHIFT))
		{
			data1 = GK_PGDN + ev->subtype - EV_GUI_WheelDown;
		}
		else
		{
			data1 = GK_DOWN + ev->subtype - EV_GUI_WheelDown;
		}
		// Intentional fallthrough

	case EV_GUI_KeyDown:
	case EV_GUI_KeyRepeat:
		switch (data1)
		{
		case '\t':
			// Try to do tab-completion
			C_TabComplete ((ev->data3 & GKM_SHIFT) ? false : true);
			break;

		case GK_PGUP:
			if (ev->data3 & (GKM_SHIFT|GKM_CTRL))
			{ // Scroll console buffer up one page
				RowAdjust += (SCREENHEIGHT-4) /
					((gamestate == GS_FULLCONSOLE || gamestate == GS_STARTUP) ? ConFont->GetHeight() : ConFont->GetHeight()*2) - 3;
			}
			else if (RowAdjust < CONSOLELINES)
			{ // Scroll console buffer up
				if (ev->subtype == EV_GUI_WheelUp)
				{
					RowAdjust += 3;
				}
				else
				{
					RowAdjust++;
				}
			}
			break;

		case GK_PGDN:
			if (ev->data3 & (GKM_SHIFT|GKM_CTRL))
			{ // Scroll console buffer down one page
				const int scrollamt = (SCREENHEIGHT-4) /
					((gamestate == GS_FULLCONSOLE || gamestate == GS_STARTUP) ? ConFont->GetHeight() : ConFont->GetHeight()*2) - 3;
				if (RowAdjust < scrollamt)
				{
					RowAdjust = 0;
				}
				else
				{
					RowAdjust -= scrollamt;
				}
			}
			else if (RowAdjust > 0)
			{ // Scroll console buffer down
				if (ev->subtype == EV_GUI_WheelDown)
				{
					RowAdjust = MAX (0, RowAdjust - 3);
				}
				else
				{
					RowAdjust--;
				}
			}
			break;

		case GK_HOME:
			if (ev->data3 & GKM_CTRL)
			{ // Move to top of console buffer
				RowAdjust = CONSOLELINES;
			}
			else
			{ // Move cursor to start of line
				buffer[1] = buffer[len+4] = 0;
			}
			break;

		case GK_END:
			if (ev->data3 & GKM_CTRL)
			{ // Move to bottom of console buffer
				RowAdjust = 0;
			}
			else
			{ // Move cursor to end of line
				buffer[1] = buffer[0];
				makestartposgood ();
			}
			break;

		case GK_LEFT:
			// Move cursor left one character
			if (buffer[1])
			{
				buffer[1]--;
				makestartposgood ();
			}
			break;

		case GK_RIGHT:
			// Move cursor right one character
			if (buffer[1] < buffer[0])
			{
				buffer[1]++;
				makestartposgood ();
			}
			break;

		case '\b':
			// Erase character to left of cursor
			if (buffer[0] && buffer[1])
			{
				char *c, *e;

				e = (char *)&buffer[buffer[0] + 2];
				c = (char *)&buffer[buffer[1] + 2];

				for (; c < e; c++)
					*(c - 1) = *c;
				
				buffer[0]--;
				buffer[1]--;
				if (buffer[len+4])
					buffer[len+4]--;
				makestartposgood ();
			}
			TabbedLast = false;
			TabbedList = false;
			break;

		case GK_DEL:
			// Erase character under cursor
			if (buffer[1] < buffer[0])
			{
				char *c, *e;

				e = (char *)&buffer[buffer[0] + 2];
				c = (char *)&buffer[buffer[1] + 3];

				for (; c < e; c++)
					*(c - 1) = *c;

				buffer[0]--;
				makestartposgood ();
			}
			TabbedLast = false;
			TabbedList = false;
			break;

		case GK_UP:
			// Move to previous entry in the command history
			if (HistPos == NULL)
			{
				HistPos = HistHead;
			}
			else if (HistPos->Older)
			{
				HistPos = HistPos->Older;
			}

			if (HistPos)
			{
				strcpy ((char *)&buffer[2], HistPos->String);
				buffer[0] = buffer[1] = (BYTE)strlen ((char *)&buffer[2]);
				buffer[len+4] = 0;
				makestartposgood();
			}

			TabbedLast = false;
			TabbedList = false;
			break;

		case GK_DOWN:
			// Move to next entry in the command history
			if (HistPos && HistPos->Newer)
			{
				HistPos = HistPos->Newer;
			
				strcpy ((char *)&buffer[2], HistPos->String);
				buffer[0] = buffer[1] = (BYTE)strlen ((char *)&buffer[2]);
			}
			else
			{
				HistPos = NULL;
				buffer[0] = buffer[1] = 0;
			}
			buffer[len+4] = 0;
			makestartposgood();
			TabbedLast = false;
			TabbedList = false;
			break;

		case 'X':
			if (ev->data3 & GKM_CTRL)
			{
				buffer[1] = buffer[0] = 0;
				TabbedLast = TabbedList = false;
			}
			break;

		case 'D':
			if (ev->data3 & GKM_CTRL && buffer[0] == 0)
			{ // Control-D pressed on an empty line
				int replen = (int)strlen (con_ctrl_d);

				if (replen == 0)
					break;	// Replacement is empty, so do nothing

				if (replen > len)
					replen = len;

				memcpy (&buffer[2], con_ctrl_d, replen);
				buffer[0] = buffer[1] = replen;
			}
			else
			{
				break;
			}
			// Intentional fall-through for command(s) added with Ctrl-D

		case '\r':
			// Execute command line (ENTER)

			buffer[2 + buffer[0]] = 0;

			for (i = 0; i < buffer[0] && isspace(buffer[2+i]); ++i)
			{
			}
			if (i == buffer[0])
			{
				 // Command line is empty, so do nothing to the history
			}
			else if (HistHead && stricmp (HistHead->String, (char *)&buffer[2]) == 0)
			{
				// Command line was the same as the previous one,
				// so leave the history list alone
			}
			else
			{
				// Command line is different from last command line,
				// or there is nothing in the history list,
				// so add it to the history list.

				History *temp = (History *)M_Malloc (sizeof(struct History) + buffer[0]);

				strcpy (temp->String, (char *)&buffer[2]);
				temp->Older = HistHead;
				if (HistHead)
				{
					HistHead->Newer = temp;
				}
				temp->Newer = NULL;
				HistHead = temp;

				if (!HistTail)
				{
					HistTail = temp;
				}

				if (HistSize == MAXHISTSIZE)
				{
					HistTail = HistTail->Newer;
					free (HistTail->Older);
					HistTail->Older = NULL;
				}
				else
				{
					HistSize++;
				}
			}
			HistPos = NULL;
			Printf (127, TEXTCOLOR_WHITE "]%s\n", &buffer[2]);
			buffer[0] = buffer[1] = buffer[len+4] = 0;
			AddCommandString ((char *)&buffer[2]);
			TabbedLast = false;
			TabbedList = false;
			break;
		
		case '`':
		case GK_ESCAPE:
			// Close console and clear command line. But if we're in the
			// fullscreen console mode, there's nothing to fall back on
			// if it's closed, so open the main menu instead.
			if (gamestate == GS_STARTUP)
			{
				return false;
			}
			else if (gamestate == GS_FULLCONSOLE)
			{
				C_DoCommand ("menu_main");
			}
			else
			{
				buffer[0] = buffer[1] = buffer[len+4] = 0;
				HistPos = NULL;
				C_ToggleConsole ();
			}
			break;

		case 'C':
		case 'V':
			TabbedLast = false;
			TabbedList = false;
			if (ev->data3 & GKM_CTRL)
			{
				if (data1 == 'C')
				{ // copy to clipboard
					if (buffer[0] > 0)
					{
						buffer[2 + buffer[0]] = 0;
						I_PutInClipboard ((char *)&buffer[2]);
					}
					break;
				}
				else
				{ // paste from clipboard
					char *clip = I_GetFromClipboard ();
					if (clip != NULL)
					{
						strtok (clip, "\r\n\b");
						int cliplen = (int)strlen (clip);

						cliplen = MIN(len, cliplen);
						if (buffer[0] + cliplen > len)
						{
							cliplen = len - buffer[0];
						}

						if (cliplen > 0)
						{
							if (buffer[1] < buffer[0])
							{
								memmove (&buffer[2 + buffer[1] + cliplen],
										 &buffer[2 + buffer[1]], buffer[0] - buffer[1]);
							}
							memcpy (&buffer[2 + buffer[1]], clip, cliplen);
							buffer[0] += cliplen;
							buffer[1] += cliplen;
							makestartposgood ();
							HistPos = NULL;
						}
						delete[] clip;
					}
					break;
				}
			}
			break;
		}
	}
	// Ensure that the cursor is always visible while typing
	CursorTicker = C_BLINKRATE;
	cursoron = 1;
	return true;
}

bool C_Responder (event_t *ev)
{
	if (ev->type != EV_GUI_Event ||
		ConsoleState == c_up ||
		ConsoleState == c_rising ||
		menuactive != MENU_Off)
	{
		return false;
	}

	return C_HandleKey (ev, CmdLine, 255);
}

CCMD (history)
{
	struct History *hist = HistTail;

	while (hist)
	{
		Printf ("   %s\n", hist->String);
		hist = hist->Newer;
	}
}

CCMD (clear)
{
	C_FlushDisplay ();
	ClearConsole ();
}

CCMD (echo)
{
	int last = argv.argc()-1;
	for (int i = 1; i <= last; ++i)
	{
		strbin (argv[i]);
		Printf ("%s%s", argv[i], i!=last ? " " : "\n");
	}
}

/* Printing in the middle of the screen */

CVAR (Float, con_midtime, 3.f, CVAR_ARCHIVE)

static const char bar1[] = TEXTCOLOR_RED "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
						  "\36\36\36\36\36\36\36\36\36\36\36\36\37" TEXTCOLOR_TAN "\n";
static const char bar2[] = TEXTCOLOR_RED "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
						  "\36\36\36\36\36\36\36\36\36\36\36\36\37" TEXTCOLOR_GREEN "\n";
static const char bar3[] = TEXTCOLOR_RED "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
						  "\36\36\36\36\36\36\36\36\36\36\36\36\37" TEXTCOLOR_NORMAL "\n";
static const char logbar[] = "\n<------------------------------->\n";

void C_MidPrint (const char *msg)
{
	if (StatusBar == NULL)
		return;

	if (msg)
	{
		AddToConsole (-1, bar1);
		AddToConsole (-1, msg);
		AddToConsole (-1, bar3);
		if (Logfile)
		{
			fputs (logbar, Logfile);
			fputs (msg, Logfile);
			fputs (logbar, Logfile);
			fflush (Logfile);
		}

		StatusBar->AttachMessage (new DHUDMessage (msg, 1.5f, 0.375f, 0, 0,
			(EColorRange)PrintColors[PRINTLEVELS], con_midtime), MAKE_ID('C','N','T','R'));
	}
	else
	{
		StatusBar->DetachMessage (MAKE_ID('C','N','T','R'));
	}
}

void C_MidPrintBold (const char *msg)
{
	if (msg)
	{
		AddToConsole (-1, bar2);
		AddToConsole (-1, msg);
		AddToConsole (-1, bar3);
		if (Logfile)
		{
			fputs (logbar, Logfile);
			fputs (msg, Logfile);
			fputs (logbar, Logfile);
			fflush (Logfile);
		}

		StatusBar->AttachMessage (new DHUDMessage (msg, 1.5f, 0.375f, 0, 0,
			(EColorRange)PrintColors[PRINTLEVELS+1], con_midtime), MAKE_ID('C','N','T','R'));
	}
	else
	{
		StatusBar->DetachMessage (MAKE_ID('C','N','T','R'));
	}
}

/****** Tab completion code ******/

struct TabData
{
	int UseCount;
	FName TabName;

	TabData()
	: UseCount(0)
	{
	}

	TabData(const char *name)
	: UseCount(1), TabName(name)
	{
	}

	TabData(const TabData &other)
	: UseCount(other.UseCount), TabName(other.TabName)
	{
	}
};

static TArray<TabData> TabCommands (TArray<TabData>::NoInit);
static int TabPos;				// Last TabCommand tabbed to
static int TabStart;			// First char in CmdLine to use for tab completion
static int TabSize;				// Size of tab string

static bool FindTabCommand (const char *name, int *stoppos, int len)
{
	FName aname(name);
	unsigned int i;
	int cval = 1;

	for (i = 0; i < TabCommands.Size(); i++)
	{
		if (TabCommands[i].TabName == aname)
		{
			*stoppos = i;
			return true;
		}
		cval = strnicmp (TabCommands[i].TabName.GetChars(), name, len);
		if (cval >= 0)
			break;
	}

	*stoppos = i;

	return (cval == 0);
}

void C_AddTabCommand (const char *name)
{
	int pos;

	if (FindTabCommand (name, &pos, INT_MAX))
	{
		TabCommands[pos].UseCount++;
	}
	else
	{
		TabData tab(name);
		TabCommands.Insert (pos, tab);
	}
}

void C_RemoveTabCommand (const char *name)
{
	FName aname(name, true);

	if (aname == NAME_None)
	{
		return;
	}
	for (unsigned int i = 0; i < TabCommands.Size(); ++i)
	{
		if (TabCommands[i].TabName == aname)
		{
			if (--TabCommands[i].UseCount == 0)
			{
				TabCommands.Delete(i);
			}
			break;
		}
	}
}

void C_ClearTabCommands ()
{
	TabCommands.Clear();
}

static int FindDiffPoint (FName name1, const char *str2)
{
	const char *str1 = name1.GetChars();
	int i;

	for (i = 0; tolower(str1[i]) == tolower(str2[i]); i++)
		if (str1[i] == 0 || str2[i] == 0)
			break;

	return i;
}

static void C_TabComplete (bool goForward)
{
	int i;
	int diffpoint;

	if (!TabbedLast)
	{
		bool cancomplete;

		// Skip any spaces at beginning of command line
		if (CmdLine[2] == ' ')
		{
			for (i = 0; i < CmdLine[0]; i++)
				if (CmdLine[2+i] != ' ')
					break;

			TabStart = i + 2;
		}
		else
		{
			TabStart = 2;
		}

		if (TabStart == CmdLine[0] + 2)
			return;		// Line was nothing but spaces

		TabSize = CmdLine[0] - TabStart + 2;

		if (!FindTabCommand ((char *)(CmdLine + TabStart), &TabPos, TabSize))
			return;		// No initial matches

		// Show a list of possible completions, if more than one.
		if (TabbedList || con_notablist)
		{
			cancomplete = true;
		}
		else
		{
			cancomplete = C_TabCompleteList ();
			TabbedList = true;
		}

		if (goForward)
		{ // Position just before the list of completions so that when TabPos
		  // gets advanced below, it will be at the first one.
			--TabPos;
		}
		else
		{ // Find the last matching tab, then go one past it.
			while (++TabPos < (int)TabCommands.Size())
			{
				if (FindDiffPoint (TabCommands[TabPos].TabName, (char *)(CmdLine + TabStart)) < TabSize)
				{
					break;
				}
			}
		}
		TabbedLast = true;
		if (!cancomplete)
		{
			return;
		}
	}

	if ((goForward && ++TabPos == (int)TabCommands.Size()) ||
		(!goForward && --TabPos < 0))
	{
		TabbedLast = false;
		CmdLine[0] = CmdLine[1] = TabSize;
	}
	else
	{
		diffpoint = FindDiffPoint (TabCommands[TabPos].TabName, (char *)(CmdLine + TabStart));

		if (diffpoint < TabSize)
		{
			// No more matches
			TabbedLast = false;
			CmdLine[0] = CmdLine[1] = TabSize + TabStart - 2;
		}
		else
		{		
			strcpy ((char *)(CmdLine + TabStart), TabCommands[TabPos].TabName.GetChars());
			CmdLine[0] = CmdLine[1] = (BYTE)strlen ((char *)(CmdLine + 2)) + 1;
			CmdLine[CmdLine[0] + 1] = ' ';
		}
	}

	makestartposgood ();
}

static bool C_TabCompleteList ()
{
	int nummatches, i;
	size_t maxwidth;
	int commonsize = INT_MAX;

	nummatches = 0;
	maxwidth = 0;

	for (i = TabPos; i < (int)TabCommands.Size(); ++i)
	{
		if (FindDiffPoint (TabCommands[i].TabName, (char *)(CmdLine + TabStart)) < TabSize)
		{
			break;
		}
		else
		{
			if (i > TabPos)
			{
				// This keeps track of the longest common prefix for all the possible
				// completions, so we can fill in part of the command for the user if
				// the longest common prefix is longer than what the user already typed.
				int diffpt = FindDiffPoint (TabCommands[i-1].TabName, TabCommands[i].TabName.GetChars());
				if (diffpt < commonsize)
				{
					commonsize = diffpt;
				}
			}
			nummatches++;
			maxwidth = MAX (maxwidth, strlen (TabCommands[i].TabName.GetChars()));
		}
	}
	if (nummatches > 1)
	{
		size_t x = 0;
		maxwidth += 3;
		Printf (TEXTCOLOR_BLUE "Completions for %s:\n", CmdLine+2);
		for (i = TabPos; nummatches > 0; ++i, --nummatches)
		{
			Printf ("%-*s", int(maxwidth), TabCommands[i].TabName.GetChars());
			x += maxwidth;
			if (x > ConCols - maxwidth)
			{
				x = 0;
				Printf ("\n");
			}
		}
		if (x != 0)
		{
			Printf ("\n");
		}
		// Fill in the longest common prefix, if it's longer than what was typed.
		if (TabSize != commonsize)
		{
			TabSize = commonsize;
			strncpy ((char *)CmdLine + TabStart, TabCommands[TabPos].TabName.GetChars(), commonsize);
			CmdLine[0] = TabStart + commonsize - 2;
			CmdLine[1] = CmdLine[0];
		}
		return false;
	}
	return true;
}
