#pragma once

// ---------------------------------------------------------------------------
// Shared reader for BountiesExpansion/BountiesExpansion.ini.
//
// The path is built from the game executable, so the values are found no
// matter what the process working directory happens to be at script boot.
// GetPrivateProfileIntA is (section, key, default, file) - getting that order
// wrong silently returns the default, which is how the poster switch once
// looked permanently off.
// ---------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <string.h>

inline const char* modIniPath()
{
	static char s_path[MAX_PATH];
	static bool s_ready = false;

	if (s_ready)
	{
		return s_path;
	}
	s_ready = true;
	s_path[0] = '\0';

	char exePath[MAX_PATH];
	if (GetModuleFileNameA(NULL, exePath, MAX_PATH) > 0)
	{
		char* lastSlash = strrchr(exePath, '\\');
		if (lastSlash != NULL)
		{
			lastSlash[1] = '\0';
			sprintf(s_path, "%sBountiesExpansion\\BountiesExpansion.ini", exePath);
		}
	}
	if (s_path[0] == '\0' || GetFileAttributesA(s_path) == INVALID_FILE_ATTRIBUTES)
	{
		sprintf(s_path, "BountiesExpansion/BountiesExpansion.ini");
	}
	return s_path;
}

inline int modIniInt(const char* section, const char* key, int def)
{
	return GetPrivateProfileIntA(section, key, def, modIniPath());
}

// String variant. Surrounding blanks and an optional pair of double quotes are
// trimmed, so Language = "lang_en.dat" still yields a usable file name.
inline void modIniString(const char* section, const char* key, const char* def, char* out, int outSize)
{
	if (out == NULL || outSize <= 0)
	{
		return;
	}
	out[0] = '\0';
	GetPrivateProfileStringA(section, key, def, out, (DWORD)outSize, modIniPath());

	char* s = out;
	while (*s == ' ' || *s == '\t')
	{
		s++;
	}
	int len = (int)strlen(s);
	while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n'))
	{
		s[--len] = '\0';
	}
	if (len >= 2 && s[0] == '"' && s[len - 1] == '"')
	{
		s[len - 1] = '\0';
		s++;
	}
	if (s != out)
	{
		memmove(out, s, strlen(s) + 1);
	}
}

// Which language pack to load: [Lang] File=Lang.dat (Language= is accepted as
// an alias). Packs live in the BountiesExpansion folder next to the ini, so
// switching a whole translation is a one-line edit there.
inline const char* modLangPath()
{
	static char s_path[MAX_PATH];
	static bool s_ready = false;

	if (s_ready)
	{
		return s_path;
	}
	s_ready = true;

	char name[128];
	name[0] = '\0';
	modIniString("Lang", "File", "", name, sizeof(name));
	if (name[0] == '\0')
	{
		modIniString("Lang", "Language", "", name, sizeof(name));
	}
	if (name[0] == '\0')
	{
		modIniString("General", "Language", "", name, sizeof(name));
	}
	if (name[0] == '\0')
	{
		strcpy(name, "Lang.dat");
	}

	sprintf(s_path, "BountiesExpansion/%s", name);
	return s_path;
}
