#include <cctype> // For tolower()
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <vector>

#include <time.h>

#include "../common/logger.hpp"
#include "../common/status.hpp"
#include "../common/types.hpp"

#include "../sequential/sequential.hpp"

enum class HashTableOperation {
    insert,
    search,
    remove,
};

struct Trace {
    HashTableOperation op;
    KeyType key;
    ValueType value;
};

std::string
clean_text(std::string raw_text)
{
    std::string text = raw_text;

    // Convert all whitespace, punctuation (adjacent to whitespace), and dashes to a single ' '
    // N.B. We do not remove punctuation within text because these are contractions
    //      e.g. "can't", "won't", etc.
    std::regex whitespace("\\s+");
    text = std::regex_replace(text, whitespace, " ");
    std::regex punctuation("(([:space:][:punct:]+[:space:]?)|([:space:]?[:punct:]+[:space:]))", std::regex::basic);
    text = std::regex_replace(text, punctuation, " ");
    std::cout << text;
    std::regex dashes("[:space:]*[-]+[:space:]*)");
    text = std::regex_replace(text, punctuation, " ");


    // Lower-case everything
    for (char &c : text) {
        c = tolower(c);
        // if (ispunct(c) || iscntrl(c)) { c = ' '; }


    }
    return text;
}

KeyType
hash_str_to_KeyType(char *str)
{
    // TODO
    return 0;
}

std::vector<Trace>
generate_trace_from_text(std::string text_source)
{
    // TODO
    // 1. Open text file
    //  - Normalize words (make all lowercase, convert all white space to ' ', remove punctuation, etc.)
    // 2. Create hash on word (i.e. terminated by any white space or EOF)
    std::vector<Trace> traces;
    std::ifstream istrm(text_source);
    std::stringstream buffer;
    buffer << istrm.rdbuf();
    std::string text = clean_text(buffer.str());

    // TODO insert word into trace
    return traces;
}

void
run_sequential_perf_test()
{
    clock_t start, end;
    start = clock();
    constexpr size_t num_elem = 1000000;
    SequentialRobinHoodHashTable a;

    for (uint64_t kv = 0; kv < num_elem; ++kv) {
        a.insert(kv, kv);
    }

    for (unsigned i = 0; i < 100; ++i) {
        for (uint64_t kv = 0; kv < num_elem; ++kv) {
            volatile std::optional<ValueType> r = a.search(kv);
        }
    }

    for (uint64_t kv = 0; kv < num_elem; ++kv) {
        a.remove(kv);
    }
    end = clock();
    std::cout << "Time in sec: " << ((double)(end - start)) / CLOCKS_PER_SEC << std::endl;
}

int main() {
    // run_sequential_perf_test();

    generate_trace_from_text("../traces/book-war-and-peace.txt");

    return 0;
}

