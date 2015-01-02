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

#include"encode.hpp"

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
            return bf::path(ezsh::WCharConverter::to(s));
        }
    };

    template<>
    struct PathCreate<wchar_t>
    {
        static bf::path doit(const std::string& s)
        {
            return bf::path(ezsh::WCharConverter::from(s));
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

}  // namespace ezsh


