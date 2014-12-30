
#include <boost/utility/string_ref.hpp>

namespace ezsh
{

class Glob
{
public:
    static bool valid(boost::string_ref const& pattern);

    template<typename Itr>
    static Itr invalidFind(Itr b, Itr e)
    {
        for(; b!=e; ++b)
        {
            if(!valid(b))
                return b;
        }

        return e;
    }

    static bool match(boost::string_ref const& pattern, boost::string_ref const& filename);
};

}
