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
}

class ReplacePattern
{
public:
    enum Type
    {
        eList,
        eValue,
        eLiteral,
    };

    struct Param
    {
        Type type;
        std::string value;
    };

    struct Operator
    {
        std::string name;
        std::vector<Param> params;
    };

    static const xpr::sregex& regexGet()
    {
        return xpr::gsReplacePattern;
    };

    void init(const xpr::smatch& what);

    const std::vector<Operator>& operatorsGet() const
    {
        return operators_;
    }

    bool needSplit() const
    {
        return needSplit_;
    }

private:
    bool needSplit_=false;
    std::vector<Operator> operators_;
};

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



