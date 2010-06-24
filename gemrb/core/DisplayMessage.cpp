/* GemRB - Infinity Engine Emulator
* Copyright (C) 2003-2005 The GemRB Project
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*
*/

#include "DisplayMessage.h"

#include "strrefs.h"

#include "Interface.h"
#include "Label.h"
#include "TableMgr.h"
#include "TextArea.h"

GEM_EXPORT DisplayMessage * displaymsg;

static int strref_table[STRREF_COUNT];

#define PALSIZE 8
static Color ActorColor[PALSIZE];
static const char* DisplayFormatName = "[color=%lX]%s - [/color][p][color=%lX]%s[/color][/p]";
static const char* DisplayFormatAction = "[color=%lX]%s - [/color][p][color=%lX]%s %s[/color][/p]";
static const char* DisplayFormat = "[/color][p][color=%lX]%s[/color][/p]";
static const char* DisplayFormatValue = "[/color][p][color=%lX]%s: %d[/color][/p]";
static const char* DisplayFormatNameString = "[color=%lX]%s - [/color][p][color=%lX]%s: %s[/color][/p]";

DisplayMessage::DisplayMessage(void) {
	ReadStrrefs();
}

bool DisplayMessage::ReadStrrefs()
{
	int i;
	memset(strref_table,-1,sizeof(strref_table) );
	AutoTable tab("strings");
	if (!tab) {
		return false;
	}
	for(i=0;i<STRREF_COUNT;i++) {
		strref_table[i]=atoi(tab->QueryField(i,0));
	}
	return true;
}

void DisplayMessage::DisplayString(const char* Text, Scriptable *target) const
{
	Label *l = core->GetMessageLabel();
	if (l) {
		l->SetText(Text, 0);
	}
	TextArea *ta = core->GetMessageTextArea();
	if (ta) {
		ta->AppendText( Text, -1 );
	} else {
		if(target) {
			char *tmp = strdup(Text);

			target->DisplayHeadText(tmp);
		}
	}
}

ieStrRef DisplayMessage::GetStringReference(int stridx) const
{
	return strref_table[stridx];
}

bool DisplayMessage::HasStringReference(int stridx) const
{
	return strref_table[stridx] != -1;
}

unsigned int DisplayMessage::GetSpeakerColor(const char *&name, const Scriptable *&speaker) const
{
	unsigned int speaker_color;

	if(!speaker) return 0;
	switch (speaker->Type) {
		case ST_ACTOR:
			name = speaker->GetName(-1);
			core->GetPalette( ((Actor *) speaker)->GetStat(IE_MAJOR_COLOR) & 0xFF, PALSIZE, ActorColor );
			speaker_color = (ActorColor[4].r<<16) | (ActorColor[4].g<<8) | ActorColor[4].b;
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			name = core->GetString( ((InfoPoint *) speaker)->DialogName );
			speaker_color = 0xc0c0c0;
			break;
		default:
			name = "";
			speaker_color = 0x800000;
			break;
	}
	return speaker_color;
}


//simply displaying a constant string
void DisplayMessage::DisplayConstantString(int stridx, unsigned int color, Scriptable *target) const
{
	if (stridx<0) return;
	const char* text = core->GetString( strref_table[stridx], IE_STR_SOUND );
	DisplayString(text, color, target);
}

void DisplayMessage::DisplayString(int stridx, unsigned int color, ieDword flags) const
{
	if (stridx<0) return;
	const char* text = core->GetString( stridx, flags);
	DisplayString(text, color, NULL);
}

void DisplayMessage::DisplayString(const char *text, unsigned int color, Scriptable *target) const
{
	if (!text) return;
	int newlen = (int)(strlen( DisplayFormat) + strlen( text ) + 12);
	char* newstr = ( char* ) malloc( newlen );
	snprintf( newstr, newlen, DisplayFormat, color, text );
	DisplayString( newstr, target );
	free( newstr );
}

// String format is
// blah : whatever
void DisplayMessage::DisplayConstantStringValue(int stridx, unsigned int color, ieDword value) const
{
	if (stridx<0) return;
	char* text = core->GetString( strref_table[stridx], IE_STR_SOUND );
	int newlen = (int)(strlen( DisplayFormat ) + strlen( text ) + 28);
	char* newstr = ( char* ) malloc( newlen );
	snprintf( newstr, newlen, DisplayFormatValue, color, text, (int) value );
	core->FreeString( text );
	DisplayString( newstr );
	free( newstr );
}

// String format is
// <charname> - blah blah : whatever
void DisplayMessage::DisplayConstantStringNameString(int stridx, unsigned int color, int stridx2, const Scriptable *actor) const
{
	unsigned int actor_color;
	const char *name;

	if (stridx<0) return;
	actor_color = GetSpeakerColor(name, actor);
	char* text = core->GetString( strref_table[stridx], IE_STR_SOUND );
	char* text2 = core->GetString( strref_table[stridx2], IE_STR_SOUND );
	int newlen = (int)(strlen( DisplayFormat ) + strlen(name) + strlen( text ) + strlen(text2) + 18);
	char* newstr = ( char* ) malloc( newlen );
	if (strlen(text2)) {
		snprintf( newstr, newlen, DisplayFormatNameString, actor_color, name, color, text, text2 );
	} else {
		snprintf( newstr, newlen, DisplayFormatName, color, name, color, text );
	}
	core->FreeString( text );
	core->FreeString( text2 );
	DisplayString( newstr );
	free( newstr );
}

// String format is
// <charname> - blah blah
void DisplayMessage::DisplayConstantStringName(int stridx, unsigned int color, const Scriptable *speaker) const
{
	if (stridx<0) return;
	if(!speaker) return;

	const char* text = core->GetString( strref_table[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	DisplayStringName(text, color, speaker);
}

void DisplayMessage::DisplayConstantStringAction(int stridx, unsigned int color, const Scriptable *attacker, const Scriptable *target) const
{
	unsigned int attacker_color;
	const char *name1;
	const char *name2;

	if (stridx<0) return;

	GetSpeakerColor(name2, target);
	attacker_color = GetSpeakerColor(name1, attacker);

	char* text = core->GetString( strref_table[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	int newlen = (int)(strlen( DisplayFormatAction ) + strlen( name1 ) +
		+ strlen( name2 ) + strlen( text ) + 18);
	char* newstr = ( char* ) malloc( newlen );
	snprintf( newstr, newlen, DisplayFormatAction, attacker_color, name1, color,
		text, name2);
	core->FreeString( text );
	DisplayString( newstr );
	free( newstr );
}

void DisplayMessage::DisplayStringName(int stridx, unsigned int color, const Scriptable *speaker, ieDword flags) const
{
	if (stridx<0) return;

	const char* text = core->GetString( stridx, flags);
	DisplayStringName(text, color, speaker);
}

void DisplayMessage::DisplayStringName(const char *text, unsigned int color, const Scriptable *speaker) const
{
	unsigned int speaker_color;
	const char *name;

	if (!text) return;
	speaker_color = GetSpeakerColor(name, speaker);

	int newlen = (int)(strlen( DisplayFormatName ) + strlen( name ) +
		+ strlen( text ) + 18);
	char* newstr = ( char* ) malloc( newlen );
	snprintf( newstr, newlen, DisplayFormatName, speaker_color, name, color, text );
	DisplayString( newstr );
	free( newstr );
}