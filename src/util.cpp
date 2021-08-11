#include "util.hpp"
#if _WIN32
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

#include <iostream>

namespace util {
	auto isTTY() -> bool {
		return ISATTY(FILENO(stdout));
	}
	split_range::split_range(std::string_view sv, std::string_view delim) : 
		text(sv),
		delim(delim)
	{
	}

	split_range::iterator::iterator(const split_range& parent) :
		text(parent.text),
		delim(parent.delim)
	{
		end = text.find(delim,0);
		if(end == std::string_view::npos){
			end = text.size();
		}
	}


	auto split_range::begin() const -> split_range::iterator {
		iterator ret(*this);
		return ret;
	}
	auto split_range::end() const -> split_range::iterator {
		iterator ret(*this);
		ret.end_flag = true;
		return ret;
	}
	auto split(std::string_view text, std::string_view delim) -> split_range {
		return split_range(text,delim);
	}
}
