//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  context.cpp
//
//   Description:  运行环境实现
//
//       Version:  1.0
//       Created:  2014年12月28日 14时04分03秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"context.hpp"

#include"parser.hpp"
#include"filesystem.hpp"
#include<boost/xpressive/xpressive.hpp>
#include<boost/algorithm/string/replace.hpp>
#include<boost/algorithm/string/predicate.hpp>

namespace ezsh
{

class ContextReplace
{
public:
    std::string replace(const std::string& val, const std::vector<std::string>& pattern);

private:
    typedef std::string (ContextReplace::*CallType)(const std::string& val, const std::vector<std::string>& pattern);

    //执行匹配返回
    //格式: [val][math][default][otherMath]
    std::string matchReplace(const std::string& val, const std::vector<std::string>& pattern)
    {
        const auto& macth=val.empty() ? pattern[0] : val;
        const auto psize=pattern.size();
        if(psize<=2)
            return val;

        const auto& defaultVal=pattern[2];
        if(psize==3)
            return defaultVal;

        std::vector<std::pair<std::string, std::string>> symble;
        for(size_t i=3; i<psize; ++i)
            symble.emplace_back(simpleSplit(pattern[i], ':'));

        auto const itr=std::find_if(symble.begin(), symble.end(),
            [&val](const std::pair<std::string, std::string>& u)
            {
                return boost::algorithm::iequals(u.first, val);
            }
        );

        if(itr==symble.end())
            return defaultVal;
        return itr->second;
    }

    //执行文件包含
    //格式: [val][include]
    std::string includeReplace(const std::string& val, const std::vector<std::string>& pattern)
    {
        const auto& name=val.empty() ? pattern[0] : val;
        bf::ifstream strm(Path(name).path());
        if(!strm)
            return val;

        std::string ret;
        std::istreambuf_iterator<char> itr(strm);
        std::istreambuf_iterator<char> const end;
        std::copy(itr, end, std::back_inserter(ret));
        return boost::algorithm::replace_all_copy(ret, "\n", " ");
    }

private:
    struct Unit
    {
        std::string name;
        CallType call;
    };

    static  Unit dict_[];
};

ContextReplace::Unit ContextReplace::dict_[]=
{
#define MD(CppName, CppCall) {#CppName, &ContextReplace::CppCall},
    MD(match,   matchReplace)
    MD(include, includeReplace)
#undef MD
};

std::string ContextReplace::replace(const std::string& val, const std::vector<std::string>& pattern)
{
    auto const itr=std::find_if(std::begin(dict_), std::end(dict_),
        [&pattern](const Unit& u)
        {
            return pattern[1]==u.name;
        }
    );

    if(itr==std::end(dict_))
        return val;
    return (this->*itr->call)(val, pattern);
}

VarSPtr Context::get(const std::string& name) const
{
    auto ctx=shared_from_this();
    while(ctx)
    {
        auto const itr=ctx->vars_.find(name);
        if(itr==vars_.end())
        {
            ctx=ctx->frontGet();
            continue;
        }

        return itr->second;
    }

    return nullptr;
}

std::string Context::stringGet(const std::string& name) const
{
    auto ptr=get(name);
    if(!ptr)
        return std::string();

    auto const strVal=boost::get<VarString>(ptr.get());
    if(strVal!=nullptr)
        return *strVal;

    auto const listVal=boost::get<VarList>(ptr.get());
    if(listVal->empty())
        return std::string();

    return listVal->front();
}

void Context::stringReplace(const std::string& str, std::string& dest) const
{
    xpr::regex_replace(std::back_inserter(dest), str.begin(), str.end(), xpr::gsReplacePattern,
        [this](const xpr::smatch& match) -> std::string
        {
            std::vector<std::string> result;
            xpr::replacePattern(match, std::back_inserter(result));

            const auto& name=result[0];
            std::string value=stringGet(name);

            if(result.size()==1)
                return std::move(value);

            ContextReplace cr;
            return cr.replace(value, result);
        }
    );
}

void Context::cmdlineReplace(const StrCommandLine& cmd, StrCommandLine& dest) const
{
    const auto size=cmd.size();
    dest.resize(size);

    for(size_t i=0; i<size; ++i)
    {
        auto& d=dest[i];
        const auto& s=cmd[i];

        d.clear();
        stringReplace(s, d);
    }
}

namespace
{
    struct Unit
    {
        const char* name;
        const char* value;
    };

    //预定义全局变量
    static Unit gsDict[]=
    {
#define MD(CppName, CppValue) {"g:" #CppName, #CppValue},
#ifdef WIN32
        MD(osType, windows)
#elif defined(__linux__)
        MD(osType, linux)
#elif defined(__APPLE__)
        MD(osType, macos)
#endif
#undef MD
    };
}

ContextStack::ContextStack()
{
    ContextSPtr ptr(new Context);
    for(const auto& u: gsDict)
        ptr->set(u.name, VarSPtr(new Variable(u.value)));
    stack_.emplace_back(ptr);
}

}

