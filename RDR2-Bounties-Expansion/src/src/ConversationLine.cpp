#include "Main.h";

ConversationLine::ConversationLine(Ped speaker, char* speechLine)
{
	this->speaker = speaker;
	this->speechLineName = speechLine;
}

bool ConversationLine::isPlaying()
{
	return (AUDIO::IS_ANY_SPEECH_PLAYING(speaker));
}

void ConversationLine::play()
{
	before();
	playAmbientSpeech(speaker, speechLineName);
	after();
}

void ConversationLine::playSync()
{
	before();
	playAmbientSpeech(speaker, speechLineName);
	// Bounded: a stuck speech handle would otherwise park the script forever.
	int playGuard = 0;
	while (isPlaying() && playGuard < 60)
	{
		WAIT(100);
		playGuard++;
	}
	after();
}

Ped ConversationLine::getSpeaker()
{
	return speaker;
}

void ConversationLine::before()
{
}

void ConversationLine::after()
{
}