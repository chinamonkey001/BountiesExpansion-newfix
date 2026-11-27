#include "Main.h"

// ---------------------------------------------------------------------------
// 语言表。std::map 的节点在插入后地址稳定，因此 T() 返回的 c_str() 可以长期
// 持有（只要不再调用 loadLangFile 重新加载）。
// ---------------------------------------------------------------------------

static std::map<std::string, std::string> g_lang;
static std::string g_langPath;

// Keys the script asked for but the pack does not have. Reported once in a
// while so a translated pack can be completed instead of silently falling
// back to the Chinese text baked into the code.
static std::set<std::string> g_missing;

static std::string langTrim(const std::string& s)
{
	size_t b = s.find_first_not_of(" \t\r\n");
	if (b == std::string::npos)
	{
		return std::string();
	}
	size_t e = s.find_last_not_of(" \t\r\n");
	return s.substr(b, e - b + 1);
}

// 允许在值里写 \n 表示换行
static std::string langUnescape(const std::string& s)
{
	std::string out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == '\\' && i + 1 < s.size())
		{
			char n = s[i + 1];
			if (n == 'n') { out.push_back('\n'); ++i; continue; }
			if (n == 't') { out.push_back('\t'); ++i; continue; }
			if (n == '\\') { out.push_back('\\'); ++i; continue; }
		}
		out.push_back(s[i]);
	}
	return out;
}

void loadLangFile(const char* path)
{
	g_lang.clear();
	g_langPath = path ? path : "";

	if (!path)
	{
		return;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		log(std::string("lang file not found, using built-in defaults: ").append(path));
		return;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string data = buffer.str();
	file.close();

	// 去掉 UTF-8 BOM
	if (data.size() >= 3 &&
		(unsigned char)data[0] == 0xEF &&
		(unsigned char)data[1] == 0xBB &&
		(unsigned char)data[2] == 0xBF)
	{
		data.erase(0, 3);
	}

	size_t pos = 0;
	int loaded = 0;
	int skipped = 0;

	while (pos <= data.size())
	{
		size_t nl = data.find('\n', pos);
		std::string line = (nl == std::string::npos)
			? data.substr(pos)
			: data.substr(pos, nl - pos);

		if (nl == std::string::npos)
		{
			pos = data.size() + 1;
		}
		else
		{
			pos = nl + 1;
		}

		line = langTrim(line);
		if (line.empty() || line[0] == ';' || line[0] == '#')
		{
			continue;
		}

		size_t eq = line.find('=');
		if (eq == std::string::npos)
		{
			skipped++;
			continue;
		}

		std::string key = langTrim(line.substr(0, eq));
		std::string value = langUnescape(line.substr(eq + 1));
		if (key.empty())
		{
			skipped++;
			continue;
		}

		g_lang[key] = value;
		loaded++;
	}

	std::stringstream msg;
	msg << "lang loaded: " << loaded << " entries (skipped " << skipped << ") from " << path;
	log(msg.str());
}

int loadLangFallback(const char* path)
{
	// Fills the gaps left by the pack that was just loaded. A translation
	// usually misses a few keys, and without this the script would drop back
	// to the English defaults baked into the code.
	if (!path || !path[0])
	{
		return 0;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		return 0;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string data = buffer.str();
	file.close();

	if (data.size() >= 3 &&
		(unsigned char)data[0] == 0xEF &&
		(unsigned char)data[1] == 0xBB &&
		(unsigned char)data[2] == 0xBF)
	{
		data.erase(0, 3);
	}

	int filled = 0;
	size_t pos = 0;

	while (pos <= data.size())
	{
		size_t nl = data.find('\n', pos);
		std::string line = (nl == std::string::npos)
			? data.substr(pos)
			: data.substr(pos, nl - pos);

		if (nl == std::string::npos)
		{
			pos = data.size() + 1;
		}
		else
		{
			pos = nl + 1;
		}

		line = langTrim(line);
		if (line.empty() || line[0] == ';' || line[0] == '#')
		{
			continue;
		}

		size_t eq = line.find('=');
		if (eq == std::string::npos)
		{
			continue;
		}

		std::string key = langTrim(line.substr(0, eq));
		if (key.empty() || g_lang.find(key) != g_lang.end())
		{
			continue;
		}

		g_lang[key] = langUnescape(line.substr(eq + 1));
		filled++;
	}

	if (filled > 0)
	{
		std::stringstream msg;
		msg << "lang fallback: filled " << filled << " missing entries from " << path;
		log(msg.str());
	}

	return filled;
}

const char* T(const char* key)
{
	if (!key || !key[0])
	{
		return "";
	}

	std::map<std::string, std::string>::const_iterator it = g_lang.find(std::string(key));
	if (it == g_lang.end())
	{
		g_missing.insert(std::string(key));
		return key;
	}
	return it->second.c_str();
}

const char* TF(const char* key, const char* fallback)
{
	if (!key || !key[0])
	{
		return fallback ? fallback : "";
	}

	std::map<std::string, std::string>::const_iterator it = g_lang.find(std::string(key));
	if (it == g_lang.end())
	{
		g_missing.insert(std::string(key));
		return fallback ? fallback : key;
	}
	return it->second.c_str();
}

void logMissingLangKeys()
{
	// Report only what has not been reported yet: a pack usually misses a
	// handful of keys, and the list grows while missions run.
	static size_t s_reported = 0;
	if (g_missing.size() <= s_reported)
	{
		return;
	}
	s_reported = g_missing.size();

	std::stringstream msg;
	msg << "lang: " << g_missing.size() << " key(s) missing from " << g_langPath << ":";

	int shown = 0;
	std::set<std::string>::const_iterator it;
	for (it = g_missing.begin(); it != g_missing.end(); ++it)
	{
		if (shown >= 16)
		{
			msg << " ...";
			break;
		}
		msg << " " << *it;
		shown++;
	}
	log(msg.str());
	g_missing.clear();
}

int getLangEntryCount()
{
	return (int)g_lang.size();
}

const char* getLangFilePath()
{
	return g_langPath.c_str();
}
