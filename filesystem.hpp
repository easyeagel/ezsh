//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  filesystem.hpp
//
//   Description:  文件系统支持
//
//       Version:  1.0
//       Created:  2015年01月01日 22时14分08秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<boost/filesystem.hpp>
#include<boost/filesystem/fstream.hpp>

#include<core/encode.hpp>

namespace ezsh
{

namespace bf=boost::filesystem;

namespace details
{
    template<typename Char>
    struct PathCreate;

    template<>
    struct PathCreate<char>
    {
        static bf::path doit(const std::string& s)
        {
            return bf::path(s);
        }

        static bf::path doit(const std::wstring& s)
        {
            return bf::path(core::WCharConverter::to(s));
        }
    };

    template<>
    struct PathCreate<wchar_t>
    {
        static bf::path doit(const std::string& s)
        {
            return bf::path(core::WCharConverter::from(s));
        }

        static bf::path doit(const std::wstring& s)
        {
            return bf::path(s);
        }
    };
}

static inline bf::path pathCreate(const std::string& s)
{
    return details::PathCreate<bf::path::value_type>::doit(s);
}

static inline bf::path pathCreate(const std::wstring& s)
{
    return details::PathCreate<bf::path::value_type>::doit(s);
}

class Path: public bf::path
{
public:
    Path()=default;

    Path(const std::string& s)
        :bf::path(pathCreate(s).generic_string<bf::path::string_type>())
    {}

    Path(const std::wstring& s)
        :bf::path(pathCreate(s).generic_string<bf::path::string_type>())
    {}

    Path(const bf::path& path)
        :bf::path(path.generic_string<bf::path::string_type>())
    {}

    Path(const char* s)
        :bf::path(pathCreate(s).generic_string<bf::path::string_type>())
    {}

    Path(const wchar_t* s)
        :bf::path(pathCreate(s).generic_string<bf::path::string_type>())
    {}

    void generalize()
    {
        *this=this->generic_string<bf::path::string_type>();
    }

    bf::path& path()
    {
        return static_cast<bf::path&>(*this);
    }

    const bf::path& path() const
    {
        return static_cast<const bf::path&>(*this);
    }

    bool isChild(const Path& parent) const
    {
        auto b=begin();
        auto const e=end();
        auto pb=parent.begin();
        auto const pe=parent.end();

        while(b!=e && pb!=pe && *b==*pb)
        {
            ++b;
            ++pb;
        }

        auto const ret=(pb==pe && b!=e);
        return ret;
    }

};

}  // namespace ezsh


