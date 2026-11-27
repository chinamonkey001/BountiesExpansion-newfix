#pragma once

using namespace std;

void initializeLogger();
// Master switch. When disabled nothing is written at all, so a shipped
// build does not leave a log file behind.
void setLoggingEnabled(bool enabled);
void log(const char* msg);
void log(Vector3 position);
void log(std::string msg);