#pragma once

// ---------------------------------------------------------------------------
// Text layer.
//   Pack format: key=value (UTF-8, BOM optional), ; and # start a comment.
//   Example: mission.36.target=Bob Lane
//   A missing key makes T() return the key itself and TF() return the
//   fallback given by the caller, so neither ever returns a null pointer.
//
// Keep this header ASCII only. It is compiled with the GBK code page, and a
// multi-byte comment placed after a declaration can swallow the line break
// and the declaration that follows.
// ---------------------------------------------------------------------------

void loadLangFile(const char* path);           // replace the whole table
int loadLangFallback(const char* path);        // fill gaps only, keep existing keys
const char* T(const char* key);                // key -> text, key when absent
const char* TF(const char* key, const char* fallback); // key -> text, fallback when absent
int getLangEntryCount();                       // number of loaded entries
const char* getLangFilePath();                 // path of the last loaded pack
void logMissingLangKeys();                     // report unresolved keys to the log
