//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  parser.hpp
//
//   Description:  通用解析装置
//
//       Version:  1.0
//       Created:  2015年01月09日 10时17分31秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<boost/xpressive/xpressive.hpp>
#include<boost/algorithm/string/trim.hpp>

namespace ezsh
{

namespace xpr
{
    using namespace boost::xpressive;
    
    //变量名或宏名
    extern sregex gsVarName;
    extern sregex gsMacroName;

    //替换模式
    extern sregex gsNotComma;
    extern sregex gsNotCommaStr;
    extern sregex gsReplacePattern;

    template<typename OutItr>
    inline void replacePattern(const smatch& what, OutItr itr)
    {
        auto begin = what.nested_results().begin();
        auto end   = what.nested_results().end();

        sregex_id_filter_predicate name(gsNotCommaStr.regex_id());

        std::for_each(
            boost::make_filter_iterator(name, begin, end),
            boost::make_filter_iterator(name, end, end),
            [&itr](const smatch& m)
            {
                *itr=m.str();
            }
        );
    }

}

static inline std::pair<std::string, std::string> simpleSplit(const std::string& str, char del='=')
{
    std::string key, val;
    auto const n=str.find(del);
    if(n==std::string::npos)
    {
        key=str;
    } else {
        key=str.substr(0, n);
        val=str.substr(n+1);
    }

    boost::algorithm::trim(key);
    boost::algorithm::trim(val);

    return std::make_pair(key, val);
}

}



