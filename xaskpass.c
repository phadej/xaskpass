/*
   xaskpass - a simplistic replacement for *-askpass.

   Usage -- with default prompt:
   xaskpass

   Usage -- with a custom prompt:
   xaskpass 'Prompt goes here'

   Writes the passphrase the user enters, followed by a newline, to
   stdout, unless the input is cancelled by pressing ESC. Returns 0 on
   success, 1 on cancel or error.


   Copyright (c) 2005 Juho Salminen
   Copyright (c) 2009 Oleg Grenrus

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PROGNAME "xaskpass"
#define WINDOW_CLASS "Xaskpass"

#define MAX_PASSPHRASE_LEN 2048
#define FONT "-*-fixed-medium-r-*-*-10-*-*-*-*-*-iso8859-15"
#define DIALOG_TITLE "SSH Authentication Passphrase Request"
#define DEFAULT_PROMPT "Please enter your authentication passphrase:"
#define MARGIN_W 16
#define MARGIN_H 16

char passphrase[MAX_PASSPHRASE_LEN + 1];

/* A local getprogname() implementation to improve portablity */
const char *getprogname()
{
	static char progname[] = PROGNAME;
	return progname;
}


int main(int argc, char *argv[])
{
	/* X connection */
	Display *disp;
	Window root_w;

	/* xaskpass main window */
	Window w;
	GC gc;

	XTextProperty wp;
	XEvent e;

	/* font */
	XFontStruct *font_info;
	char *font_name;

	/* the passphrase prompt */
	char *prompt;
	int prompt_len;

	/* passphrase input counter */
	int cp;

	/* the return value of the program */
	int retval;

	/* a quit flag */
	int quit;

	/* prompt size and position */
	int textw, texth, textx, texty;

	/* window size */
	int winw, winh;

	/* screen size */
	int screenw, screenh;

	/* black and white color */
	int c_black, c_white;

	/* the window title */
	char *title;

	/* storage for a single keypress */
	char s[2];

	/* class hints */
	XClassHint class_hints;
	class_hints.res_name = PROGNAME;
	class_hints.res_class = WINDOW_CLASS;

	disp = XOpenDisplay(NULL);
	if (!disp) {
		fprintf(stderr, "%s: unable to open display", getprogname());
		return 1;
	}

	/* Extract prompt from command line or use the default. */
	if (argc == 2) {
		prompt = argv[1];
	} else {
		prompt = DEFAULT_PROMPT;
	}
	prompt_len = strlen(prompt);


	/* Load the font */
	font_name = FONT;
	font_info = XLoadQueryFont(disp, font_name);
	if (!font_info) {
		fprintf(stderr, "%s: failed to load font '%s'", getprogname(), font_name);
		return 1;
	}

	/* Get the colors */
	c_black = BlackPixel(disp, DefaultScreen(disp));
	c_white = WhitePixel(disp, DefaultScreen(disp));

	/* Calculate the size of the prompt */
	textw = XTextWidth(font_info, prompt, prompt_len);
	texth = font_info->ascent + font_info->descent;

	/* Calculate the position of the prompt */
	textx = MARGIN_W;
	texty = MARGIN_H + font_info->ascent;

	/* Calculate the size of the window */
	winw = textw + 2 * MARGIN_W;
	winh = texth + 2 * MARGIN_H;

	/* Get the root window */
	root_w = DefaultRootWindow(disp);

	/* Get the size of the screen */
	screenw = DisplayWidth(disp, DefaultScreen(disp));
	screenh = DisplayWidth(disp, DefaultScreen(disp));

	/* Create the xaskpass window */
	w = XCreateSimpleWindow(disp, root_w,
			screenw / 2 - winw / 2, screenh / 3 - winh / 2,
			winw, winh, 2, c_black, c_white);

	/* Hints */
	XSetClassHint(disp, w, &class_hints);

	/* Transient */
	XSetTransientForHint(disp, w, root_w);

	/* Want MapNotify, Expose and KeyPress events */
	XSelectInput(disp, w, StructureNotifyMask | ExposureMask | KeyPressMask);

	/* Map the window */
	XMapWindow(disp, w);

	/* Sync */
	XSync(disp, False);

	/* Set the window title */
	title = DIALOG_TITLE;

	if (XStringListToTextProperty(&title, 1, &wp) != 0) {
		XSetWMName(disp, w, &wp);
		XFree(wp.value);
	}

	/* Create the graphical context */
	gc = XCreateGC(disp, w, 0, NULL);
	if (!gc) {
		fprintf(stderr, "%s: XCreateGC failed", getprogname());
		return 1;
	}

	retval = 1;
	quit = 0;

	/* Passphrase length is initially 0 */
	cp = 0;

	/* Event loop */
	while (!quit) {
		XNextEvent(disp, &e);

		switch (e.type) {
			case MapNotify:
				/* Grab the keyboard */
				XSync(disp, False);
				if (XGrabKeyboard(disp, w, True, GrabModeAsync,
							GrabModeAsync, CurrentTime) != GrabSuccess) {
					fprintf(stderr, "%s: unable to grab keyboard", getprogname());
					return 1;
				}
				break;

			case Expose:
				/* Draw the prompt */
				XSetFont(disp, gc, font_info->fid);
				XSetForeground(disp, gc, c_black);
				XDrawString(disp, w, gc, textx, texty, prompt, prompt_len);
				XSync(disp, False);
				break;

			case KeyPress:
				/* Lookup the key */
				if (XLookupString(&e.xkey, s, sizeof(s), NULL, NULL)) {
					if (s[0] == 8 || s[0] == 0x7f) {
						/* Backspace or DEL; erase the last character */
						if (cp > 0) {
							passphrase[--cp] = '\0';
						}

					} else if (s[0] == 10 || s[0] == 13) {
						/* Return; print the passphrase to stdout and return success */
						printf("%s\n", passphrase);
						retval = 0;
						quit = 1;

					} else if (s[0] == 27) {
						/* ESC; return failure */
						quit = 1;

					} else if (s[0] >= 32) {
						/* Not a control character; add to the passphrase */
						if (cp < MAX_PASSPHRASE_LEN) {
							passphrase[cp++] = s[0];
							passphrase[cp] = '\0';
						}
					}
				}
				break;
		}
	}
	/* Zero the passphrase from the memory */
	for (cp = 0; cp < MAX_PASSPHRASE_LEN; cp++) {
		passphrase[cp] = '\0';
	}

	/* Ungrab the keyboard */
	XUngrabKeyboard(disp, CurrentTime);

	/* Return */
	return retval;
}
