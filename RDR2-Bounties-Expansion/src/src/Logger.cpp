#include "Main.h";
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>

const char* const LOG_FILE = "BountiesExpansion.log";

ofstream file;
static bool loggingEnabled = false;

void setLoggingEnabled(bool enabled)
{
	loggingEnabled = enabled;
}

void initializeLogger()
{
	if (!loggingEnabled)
	{
		return;
	}

	file.open(LOG_FILE, std::ios_base::out);
	file.close();
}

void log(const char* msg)
{
	if (!loggingEnabled)
	{
		return;
	}

	time_t nowTimestamp = std::time(0);
	tm* now = localtime(&nowTimestamp);
	stringstream txt;

	file.open(LOG_FILE, std::ios_base::app);
	if (file.is_open())
	{
		txt << "[" << now->tm_mday << "/" << now->tm_mon + 1 << "/" << now->tm_year + 1900 << " "
			<< std::setfill('0') << std::setw(2) << now->tm_hour << ":"
			<< std::setw(2) << now->tm_min << ":"
			<< std::setw(2) << now->tm_sec << "] " << msg;
		file << txt.str().c_str() << "\n";
		file.close();
	}
}

void log(Vector3 pos)
{
	std::stringstream output;
	output << pos.x << ", " << pos.y << ", " << pos.z;
	log(output.str().c_str());
}

void log(std::string msg)
{
	log(msg.c_str());
}
