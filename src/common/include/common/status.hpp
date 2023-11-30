#pragma once

enum class SearchStatus {
    found_match,
    found_swap,
    found_hole,
    found_nohole,
};

enum class ErrorType {
    // We want !error to imply an ok status
    ok = 0,
    e_unknown,
    e_oom,
    e_notfound,
    e_nohole,
};
