// Copyright 1992-98, Be Incorporated, All Rights Reserved.
//
// send comments/suggestions/feedback to pavel@be.com
//

#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
#include <Debug.h>
#include <Dragger.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>

#include <stdlib.h>
#include <string.h>

#include "CDButton.h"
#include "CDPanel.h"
#include "iconfile.h"

const char *app_signature = "application/x-vnd.Be-CDButton";
// the application signature used by the replicant to find the supporting
// code

// CDButton can be used as a standalone application, as a simple replicant
// installed into any replicant shell or as a special Deskbar-compatible
// replicant.

CDButton::CDButton(BRect frame, const char *name, int devicefd,
	uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags),
		engine(devicefd)
{
	SetViewColor(200, 200, 200);
	BRect rect(Bounds());
	rect.OffsetTo(B_ORIGIN);
	rect.top = rect.bottom - 7;
	rect.left = rect.right - 7;
	AddChild(new BDragger(rect, this, 0));
	segments = new BBitmap(BRect(0, 0, 64 - 1, 8 - 1), B_COLOR_8_BIT);
	segments->SetBits(LCDsmall64x8_raw, 64*8, 0, B_COLOR_8_BIT);
}

static int
FindFirstCPPlayerDevice()
{
	// simple CD lookup, only works with a single CD, for multiple CD support
	// need to get more fancy
	return CDEngine::FindCDPlayerDevice();
}

CDButton::CDButton(BMessage *message)
	:	BView(message),
		engine(FindFirstCPPlayerDevice())
{
	segments = new BBitmap(BRect(0, 0, 64 - 1, 8 - 1), B_COLOR_8_BIT);
	segments->SetBits(LCDsmall64x8_raw, 64*8, 0, B_COLOR_8_BIT);
}


CDButton::~CDButton()
{
	delete segments;
}

// archiving overrides
CDButton *
CDButton::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "CDButton"))
		return NULL;
	return new CDButton(data);
}

status_t 
CDButton::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	data->AddString("add_on", app_signature);
	return B_NO_ERROR;
}


void 
CDButton::Draw(BRect rect)
{
	BView::Draw(rect);
	int32 trackNum = engine.TrackStateWatcher()->GetTrack();
	
	// draw the track number
	float offset = BlitDigit(this, BPoint(2, 7),
		trackNum >= 0 ? trackNum / 10 + 1 : 0,
		BRect(0, 0, 4, 7), segments, ViewColor());

	BlitDigit(this, BPoint(offset + 2, 7),
		trackNum >= 0 ? trackNum % 10 + 1 : 0,
		BRect(0, 0, 4, 7), segments, ViewColor());
	
	BRect playStateRect(Bounds());
	playStateRect.OffsetBy(3, 1);
	playStateRect.right = playStateRect.left + 6;
	playStateRect.bottom = playStateRect.top + 4;
		
	SetDrawingMode(B_OP_COPY);
	const rgb_color color = {80, 80, 80, 0};

	// draw the play or pause icons
	SetHighColor(color);
	if (engine.PlayStateWatcher()->GetState() == kPlaying) {
		StrokeLine(BPoint(playStateRect.left, playStateRect.top), 
			BPoint(playStateRect.left, playStateRect.bottom));
		StrokeLine(BPoint(playStateRect.left + 1, playStateRect.top), 
			BPoint(playStateRect.left + 1, playStateRect.bottom));
		
		StrokeLine(BPoint(playStateRect.left + 4, playStateRect.top), 
			BPoint(playStateRect.left + 4, playStateRect.bottom));
		StrokeLine(BPoint(playStateRect.left + 5, playStateRect.top), 
			BPoint(playStateRect.left + 5, playStateRect.bottom));
	} else {
		StrokeLine(BPoint(playStateRect.left, playStateRect.top), 
			BPoint(playStateRect.left, playStateRect.bottom));
		StrokeLine(BPoint(playStateRect.left, playStateRect.top), 
			BPoint(playStateRect.right, playStateRect.top + 2));
		StrokeLine(BPoint(playStateRect.left, playStateRect.top + 1), 
			BPoint(playStateRect.right, playStateRect.top + 2));
		StrokeLine(BPoint(playStateRect.left, playStateRect.top + 2), 
			BPoint(playStateRect.right, playStateRect.top + 2));
		StrokeLine(BPoint(playStateRect.left, playStateRect.top + 3), 
			BPoint(playStateRect.right, playStateRect.top + 2));
		StrokeLine(BPoint(playStateRect.left, playStateRect.bottom), 
			BPoint(playStateRect.right, playStateRect.top + 2));
	}
}

void
CDButton::MessageReceived(BMessage *message)
{
	if (message->what == B_ABOUT_REQUESTED)
		// This responds to invoking the About CDButton from the replicants'
		// menu
		(new BAlert("About CDButton", "CDButton (Replicant)\n"
			"  Brought to you by the folks at Be.\n\n"
			"  Use CDButton -deskbar from Terminal to install CDButton into the Deskbar.\n\n"
			"  Copyright Be Inc., 1997-1998","OK"))->Go();
	else if (!Observer::HandleObservingMessages(message))
	// just support observing messages
		BView::MessageReceived(message);
}

void
CDButton::NoticeChange(Notifier *)
{
	// respond to a notice by forcig a redraw
	Invalidate();
}

void 
CDButton::AttachedToWindow()
{
	// start observing
	engine.AttachedToLooper(Window());
	StartObserving(engine.TrackStateWatcher());
	StartObserving(engine.PlayStateWatcher());
}

void 
CDButton::Pulse()
{
	// pulse the CD engine
	engine.DoPulse();
}

void
CDButton::MouseDown(BPoint point)
{
	for (int32 count = 0; count < 20; count++) {
		uint32 mouseButtons;
		BPoint where;
		GetMouse(&where, &mouseButtons, true);
		
		// handle mouse click/press in the CDButton icon
		
		if (!mouseButtons) {
			// handle a few modifier click cases
			if (modifiers() & B_CONTROL_KEY)
				// Control - click ejects the CD
				engine.Eject();
			else if ((modifiers() & (B_COMMAND_KEY | B_OPTION_KEY)) == (B_COMMAND_KEY | B_OPTION_KEY))
				// Command/Alt - Option/Win click skips to previous track
				engine.SkipOneBackward();

			else if (modifiers() & B_COMMAND_KEY)
				// Command/Alt - click skips to next track
				engine.SkipOneForward();
			else
				// button just clicked, play, pause, eject or skip
				engine.PlayOrPause();
			return;
		}
		snooze(10000);
	}
	ConvertToScreen(&point);
	// button pressed, show the whole panel
	(new CDPanelWindow(point, &engine))->Show();
//	do not call inherited here, we do not wan't to block by tracking the button
}

// Optionally the CDButton can get installed into the Deskbar using the
// replicant technology.
//
// This only works on PR2 if you do not have the mail daemon running
// On R3 however this feature is fully supported. The following code
// illustrates how to use it.

CDButtonApplication::CDButtonApplication(bool install)
	:	BApplication(app_signature)
{
	if (install) {
		// install option specified, install self into the Deskbar
		BRect buttonRect(0, 0, 16, 16);
		CDButton *replicant = new CDButton(buttonRect, "CD", FindFirstCPPlayerDevice());
		BMessage archiveMsg(B_ARCHIVED_OBJECT);
		// archive an instance of CDButton into a message
		replicant->Archive(&archiveMsg);
		
		// send archived replicant to the deskbar
		BMessenger messenger("application/x-vnd.Be-TSKB", -1, NULL);
		messenger.SendMessage(&archiveMsg);

		// commit suicide
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	// Just run CDButton as a regular application/replicant hatch
	// show a window with the CD button
	BRect windowRect(100, 100, 120, 120);
	BWindow *window = new BWindow(windowRect, "", B_TITLED_WINDOW, 
			B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE);
	BRect buttonRect(window->Bounds());
	buttonRect.InsetBy(2, 2);
	
	BView *button = new CDButton(buttonRect, "CD", FindFirstCPPlayerDevice());
	window->AddChild(button);
	window->Show();
}


main(int, char **argv)
{
	if (argv[1] && strcmp(argv[1], "-deskbar") != 0) {
		// print a simple usage string
		printf(	"# %s (c) 1997-98 Be Inc.\n"
				"# Usage: %s [-deskbar]\n",
			 argv[0], argv[0]);
		return 0;
	}

	bool install = argv[1] && (strcmp(argv[1], "-deskbar") == 0);
	(new CDButtonApplication(install))->Run();

	return 0;
}
