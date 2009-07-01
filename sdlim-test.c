/*
    sdlim-test: Prints Unicode characters with SDL_ttf as they are typed.
    Copyright (C) 2009 Simos Xenitellis

    Based on showfont.c by Sam Lantinga, slouken@libsdl.org
    Copyright (C) 1997-2009 Sam Lantinga

    Based on checkkeys.c by SDL authors (part of SDL distribution).

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/* A simple program to test the Input Method support in the SDL library (1.3+) */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include "SDL.h"
#include "SDL_ttf.h"

#define DEFAULT_PTSIZE	18
#define DEFAULT_TEXT	"ϻ"
#define DEFAULT_FONT    "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf"
#define NUM_COLORS      65536

static void cleanup(int exitcode)
{
	TTF_Quit();
	SDL_Quit();
	exit(exitcode);
}

static void
print_modifiers(void)
{
    int mod;
    printf(" modifiers:");
    mod = SDL_GetModState();
    if (!mod) {
        printf(" (none)");
        return;
    }
    if (mod & KMOD_LSHIFT)
        printf(" LSHIFT");
    if (mod & KMOD_RSHIFT)
        printf(" RSHIFT");
    if (mod & KMOD_LCTRL)
        printf(" LCTRL");
    if (mod & KMOD_RCTRL)
        printf(" RCTRL");
    if (mod & KMOD_LALT)
        printf(" LALT");
    if (mod & KMOD_RALT)
        printf(" RALT");
    if (mod & KMOD_LGUI)
        printf(" LGUI");
    if (mod & KMOD_RGUI)
        printf(" RGUI");
    if (mod & KMOD_NUM)
        printf(" NUM");
    if (mod & KMOD_CAPS)
        printf(" CAPS");
    if (mod & KMOD_MODE)
        printf(" MODE");
}

static void
PrintKey(SDL_keysym * sym, int pressed)
{
    /* Print the keycode, name and state */
    if (sym->sym) {
        printf("Key %s:  scancode %d = %s, keycode 0x%08X = %s ",
               pressed ? "pressed " : "released",
               sym->scancode,
               SDL_GetScancodeName(sym->scancode),
               sym->sym, SDL_GetKeyName(sym->sym));
    } else {
        printf("Unknown Key (scancode %d = %s) %s ",
               sym->scancode,
               SDL_GetScancodeName(sym->scancode),
               pressed ? "pressed" : "released");
    }

    /* Print the translated character, if one exists */
    if (sym->unicode) {
        /* Is it a control-character? */
        if (sym->unicode < ' ') {
            printf(" (^%c)", sym->unicode + '@');
        } else {
            printf(" (%c)", sym->unicode);
        }
    }
    print_modifiers();
    printf("\n");
}

int main(int argc, char *argv[])
{
	SDL_Surface *screen;
	TTF_Font *font;
	SDL_Surface *text, *temp;
	int ptsize;
	int i, done;
	int rdiff, gdiff, bdiff;
	SDL_Color colors[NUM_COLORS];
	SDL_Color white = { 0xFF, 0xFF, 0xFF, 0 };
	SDL_Color black = { 0x00, 0x00, 0x00, 0 };
	SDL_Color *forecol;
	SDL_Color *backcol;
	SDL_Rect dstrect;
	SDL_Event event;
	int rendersolid;
	int renderstyle;
	enum {
		RENDER_LATIN1,
		RENDER_UTF8,
		RENDER_UNICODE
	} rendertype;
	char *message, string[256];

	ptsize = 288;
	/* Look for special rendering types */
	rendersolid = 0;
	renderstyle = TTF_STYLE_NORMAL;
	rendertype = RENDER_UTF8;
	/* Default is black and white */
	forecol = &black;
	backcol = &white;

	/* Initialize SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		return(2);
	}

	/* Initialize the TTF library */
	if ( TTF_Init() < 0 ) {
		fprintf(stderr, "Couldn't initialize TTF: %s\n",SDL_GetError());
		SDL_Quit();
		return(2);
	}

	/* Open the font file */
	font = TTF_OpenFont(DEFAULT_FONT, ptsize);
	if ( font == NULL ) {
		fprintf(stderr, "Couldn't load %d pt font from %s: %s\n",
					ptsize, DEFAULT_FONT, SDL_GetError());
		cleanup(2);
	}
	TTF_SetFontStyle(font, renderstyle);

	/* Set a 400x300x16 video mode */
	screen = SDL_SetVideoMode(400, 300, 16, SDL_SWSURFACE);
	if ( screen == NULL ) {
		fprintf(stderr, "Couldn't set 400x300x16 video mode: %s\n",
							SDL_GetError());
		cleanup(2);
	}

	/* Set a palette that is good for the foreground colored text */
	rdiff = backcol->r - forecol->r;
	gdiff = backcol->g - forecol->g;
	bdiff = backcol->b - forecol->b;
	for ( i=0; i<NUM_COLORS; ++i ) {
		colors[i].r = forecol->r + (i*rdiff)/4;
		colors[i].g = forecol->g + (i*gdiff)/4;
		colors[i].b = forecol->b + (i*bdiff)/4;
	}
	SDL_SetColors(screen, colors, 0, NUM_COLORS);

	/* Clear the background to background color */
	SDL_FillRect(screen, NULL,
			SDL_MapRGB(screen->format, backcol->r, backcol->g, backcol->b));
	SDL_Flip(screen);

	/* Enable Unicode keyboard translation */
	if (SDL_EnableUNICODE(1))
		puts("Unicode keyboard translation is already enabled");
	else
		puts("Enabled Unicode keyboard translation");

	/* Render and center the message */
	message = DEFAULT_TEXT;

	switch (rendertype) {

	    case RENDER_UTF8:
		if ( rendersolid ) {
			text = TTF_RenderUTF8_Solid(font,message,*forecol);
		} else {
			text = TTF_RenderUTF8_Shaded(font,message,*forecol,*backcol);
		}
		break;

	    default:
		text = NULL; /* This shouldn't happen */
		break;
	}
	if ( text == NULL ) {
		fprintf(stderr, "Couldn't render text: %s\n", SDL_GetError());
		TTF_CloseFont(font);
		cleanup(2);
	}
	dstrect.x = (screen->w - text->w)/2;
	dstrect.y = (screen->h - text->h)/2;
	dstrect.w = text->w;
	dstrect.h = text->h;

	/* Blit the text surface */
	if ( SDL_BlitSurface(text, NULL, screen, &dstrect) < 0 ) {
		fprintf(stderr, "Couldn't blit text to display: %s\n", 
								SDL_GetError());
		TTF_CloseFont(font);
		cleanup(2);
	}
	SDL_Flip(screen);

	/* Set the text colorkey and convert to display format */
	if ( SDL_SetColorKey(text, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0) < 0 ) {
		fprintf(stderr, "Warning: Couldn't set text colorkey: %s\n",
								SDL_GetError());
	}
	temp = SDL_DisplayFormat(text);
	if ( temp != NULL ) {
		SDL_FreeSurface(text);
		text = temp;
	}

	/* Wait for a keystroke, and blit text on mouse press */
	done = 0;
	while ( ! done ) {
		if ( SDL_WaitEvent(&event) < 0 ) {
			fprintf(stderr, "SDL_PullEvent() error: %s\n",
								SDL_GetError());
			done = 1;
			continue;
		}
		switch (event.type) {
			case SDL_QUIT:
				done = 1;
				break;

			case SDL_KEYDOWN:
				PrintKey(&event.key.keysym, 1);
				break;

			case SDL_KEYUP:
				PrintKey(&event.key.keysym, 0);
				printf("\n");
				break;

			case SDL_VIDEOEXPOSE:
				break;

			case SDL_TEXTINPUT:
				printf("Printing character %s\n", event.text.text);
				SDL_FillRect(screen, NULL, 0xFFFFFF);
				text = TTF_RenderUTF8_Solid(font,event.text.text,*forecol);
				dstrect.x = (screen->w - text->w)/2;
				dstrect.y = (screen->h - text->h)/2;
				dstrect.w = text->w;
				dstrect.h = text->h;
				if ( SDL_BlitSurface(text, NULL, screen,
							&dstrect) == 0 ) {
					SDL_Flip(screen);
				} else {
					fprintf(stderr,
					"Couldn't blit text to display: %s\n", 
								SDL_GetError());
				}
				
			default:
				break;
		}
	}
	SDL_FreeSurface(text);
	TTF_CloseFont(font);
	cleanup(0);

	/* Not reached, but fixes compiler warnings */
	return 0;
}
