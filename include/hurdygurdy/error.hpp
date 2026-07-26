#pragma once

StringView getError();

void setError(StringView error);

template<typename... Ts> requires (sizeof...(Ts) > 0)
void setError(StringView errorFmt, Ts... args);

void logError();