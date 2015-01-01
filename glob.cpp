
#include "glob.hpp"
#include<cstdint>
#include <cassert>
#include <stdexcept>

namespace ezsh
{
    struct glob_error : std::runtime_error
    {
        explicit glob_error(char const* error) :
            std::runtime_error(error) {}
    };

    typedef boost::string_ref::const_iterator glob_iterator;

    void static check_glob_range(glob_iterator&, glob_iterator);
    void static check_glob_escape(glob_iterator&, glob_iterator);

    bool static match_section(glob_iterator& pattern_begin, glob_iterator pattern_end,
            glob_iterator& filename_begin, glob_iterator& filename_end);
    bool static match_range(glob_iterator& pattern_begin, glob_iterator pattern_end,
            unsigned char x);

    bool static check_glob(boost::string_ref pattern)
    {
        bool is_glob = false;
        bool is_ascii = true;

        glob_iterator begin = pattern.begin();
        glob_iterator end = pattern.end();

        while (begin != end) {
            if (static_cast<uint8_t>(*begin) < 32 || static_cast<uint8_t>(*begin) > 127)
                is_ascii = false;

            switch(*begin) {
                case '\\':
                    check_glob_escape(begin, end);
                    break;

                case '[':
                    check_glob_range(begin, end);
                    is_glob = true;
                    break;

                case ']':
                    throw glob_error("uneven square brackets");

                case '?':
                    is_glob = true;
                    ++begin;
                    break;

                case '*':
                    is_glob = true;
                    ++begin;

                    if (begin != end && *begin == '*') {
                        throw glob_error("'**' not supported");
                    }
                    break;

                default:
                    ++begin;
            }
        }

        if (is_glob && !is_ascii)
            throw glob_error("invalid character, globs are ascii only");

        return is_glob;
    }

    void static check_glob_range(glob_iterator& begin, glob_iterator end)
    {
        assert(begin != end && *begin == '[');
        ++begin;

        if (*begin == ']')
            throw glob_error("empty range");

        while (begin != end) {
            switch (*begin) {
                case '\\':
                    ++begin;

                    if (begin == end) {
                        throw glob_error("trailing escape");
                    }
                    else if (*begin == '\\' || *begin == '/') {
                        throw glob_error("contains escaped slash");
                    }

                    ++begin;
                    break;
                case '[':
                    // TODO: Allow?
                    throw glob_error("nested square brackets");
                case ']':
                    ++begin;
                    return;
                case '/':
                    throw glob_error("slash in square brackets");
                default:
                    ++begin;
            }
        }

        throw glob_error("uneven square brackets");
    }

    void static check_glob_escape(glob_iterator& begin, glob_iterator end)
    {
        assert(begin != end && *begin == '\\');

        ++begin;

        if (begin == end) {
            throw glob_error("trailing escape");
        }
        else if (*begin == '\\' || *begin == '/') {
            throw glob_error("contains escaped slash");
        }

        ++begin;
    }

    bool static glob(boost::string_ref const& pattern,
            boost::string_ref const& filename)
    {
        // If there wasn't this special case then '*' would match an
        // empty string.
        if (filename.empty()) return pattern.empty();

        glob_iterator pattern_it = pattern.begin();
        glob_iterator pattern_end = pattern.end();

        glob_iterator filename_it = filename.begin();
        glob_iterator filename_end = filename.end();

        if (!match_section(pattern_it, pattern_end, filename_it, filename_end))
            return false;

        while (pattern_it != pattern_end) {
            assert(*pattern_it == '*');
            ++pattern_it;

            if (pattern_it == pattern_end) return true;

            // TODO: Error?
            if (*pattern_it == '*') return false;

            while (true) {
                if (filename_it == filename_end) return false;
                if (match_section(pattern_it, pattern_end, filename_it, filename_end))
                    break;
                ++filename_it;
            }
        }

        return filename_it == filename_end;
    }

    bool static match_section(glob_iterator& pattern_begin, glob_iterator pattern_end,
            glob_iterator& filename_begin, glob_iterator& filename_end)
    {
        glob_iterator pattern_it = pattern_begin;
        glob_iterator filename_it = filename_begin;

        while (pattern_it != pattern_end && *pattern_it != '*') {
            if (filename_it == filename_end) return false;

            switch(*pattern_it) {
                case '*':
                    assert(false);
                    return false;
                case '[':
                    if (!match_range(pattern_it, pattern_end, *filename_it))
                        return false;
                    ++filename_it;
                    break;
                case '?':
                    ++pattern_it;
                    ++filename_it;
                    break;
                case '\\':
                    ++pattern_it;
                    if (pattern_it == pattern_end) return false;
                    BOOST_FALLTHROUGH;
                default:
                    if (*pattern_it != *filename_it) return false;
                    ++pattern_it;
                    ++filename_it;
            }
        }

        if (pattern_it == pattern_end && filename_it != filename_end)
            return false;

        pattern_begin = pattern_it;
        filename_begin = filename_it;
        return true;
    }

    bool static match_range(glob_iterator& pattern_begin, glob_iterator pattern_end,
            unsigned char x)
    {
        assert(pattern_begin != pattern_end && *pattern_begin == '[');
        ++pattern_begin;
        if (pattern_begin == pattern_end) return false;

        bool invert_match = false;
        bool matched = false;

        if (*pattern_begin == '^') {
            invert_match = true;
            ++pattern_begin;
            if (pattern_begin == pattern_end) return false;
        }

        // Search for a match
        while (true) {
            unsigned char first = *pattern_begin;
            ++pattern_begin;
            if (first == ']') break;
            if (pattern_begin == pattern_end) return false;

            if (first == '\\') {
                first = *pattern_begin;
                ++pattern_begin;
                if (pattern_begin == pattern_end) return false;
            }

            if (*pattern_begin != '-') {
                matched = matched || (first == x);
            }
            else {
                ++pattern_begin;
                if (pattern_begin == pattern_end) return false;

                unsigned char second = *pattern_begin;
                ++pattern_begin;
                if (second == ']') {
                    matched = matched || (first == x) || (x == '-');
                    break;
                }
                if (pattern_begin == pattern_end) return false;

                if (second == '\\') {
                    second = *pattern_begin;
                    ++pattern_begin;
                    if (pattern_begin == pattern_end) return false;
                }

                // TODO: What if second < first?
                matched = matched || (first <= x && x <= second);
            }
        }

        return invert_match != matched;
    }

#if 0
    std::size_t static find_glob_char(boost::string_ref pattern,
            std::size_t pos)
    {
        // Weird style is because boost::string_ref's find_first_of
        // doesn't take a position argument.
        std::size_t removed = 0;

        while (true) {
            pos = pattern.find_first_of("[]?*\\");
            if (pos == boost::string_ref::npos) return pos;
            if (pattern[pos] != '\\') return pos + removed;
            pattern.remove_prefix(pos + 2);
            removed += pos + 2;
        }
    }

    std::string static glob_unescape(boost::string_ref pattern)
    {
        std::string result;

        while (true) {
            std::size_t pos = pattern.find("\\");
            if (pos == boost::string_ref::npos) {
                result.append(pattern.data(), pattern.size());
                break;
            }

            result.append(pattern.data(), pos);
            ++pos;
            if (pos < pattern.size()) {
                result += pattern[pos];
                ++pos;
            }
            pattern.remove_prefix(pos);
        }

        return result;
    }
#endif


    bool Glob::valid(boost::string_ref const& pattern)
    {
        try
        {
            auto const ret=check_glob(pattern);
            return ret;
        } catch(glob_error& ge) {
            return false;
        }
    }

    bool Glob::match(boost::string_ref const& pattern, boost::string_ref const& filename)
    {
        return glob(pattern, filename);
    }
}

