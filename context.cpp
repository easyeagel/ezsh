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

#include"script.hpp"
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
    std::string replace(const Context& ctx, const ReplacePattern& rp, bool splited, StrCommandLine& dest);

private:
    typedef std::string (ContextReplace::*CallType)
        (const Context& ctx, const ReplacePattern::Operator& opt, bool splited, StrCommandLine& dest);

    //执行匹配返回
    //格式: [@match][var][default][otherMatch]...
    std::string matchReplace(const Context& ctx, const ReplacePattern::Operator& opt, bool splited, StrCommandLine& dest)
    {
        const auto& params=opt.params;
        const auto& match=ctx.stringGet(params[0].value);
        const auto psize=params.size();
        if(psize<=1)
            return result(match, splited, dest);

        const auto& defaultVal=params[1];
        if(psize==2)
            return result(std::move(defaultVal.value), splited, dest);

        std::vector<std::pair<std::string, std::string>> symble;
        for(size_t i=2; i<psize; ++i)
            symble.emplace_back(simpleSplit(params[i].value, ':'));

        auto const itr=std::find_if(symble.begin(), symble.end(),
            [&match](const std::pair<std::string, std::string>& u)
            {
                return boost::algorithm::iequals(u.first, match);
            }
        );

        if(itr==symble.end())
            return result(std::move(defaultVal.value), splited, dest);
        return result(std::move(itr->second), splited, dest);
    }

    //执行文件包含
    //格式: [@include][var/val]...
    std::string includeReplace(const Context& ctx, const ReplacePattern::Operator& opt, bool splited, StrCommandLine& dest)
    {
        std::string ret;
        for(const auto& param: opt.params)
        {
            const auto& name=
                (param.type==ReplacePattern::eLiteral) ? param.value : ctx.stringGet(param.value, param.value);
            bf::ifstream strm(Path(name).path());
            if(!strm)
                continue;

            std::string str;
            std::istreambuf_iterator<char> itr(strm);
            std::istreambuf_iterator<char> const end;
            std::copy(itr, end, std::back_inserter(str));
            boost::algorithm::replace_all(str, "\n", " ");
            if(!ret.empty() && !str.empty())
                ret += ' ';
            ret += str;
        }

        return result(ret, splited, dest);
    }

    //执行变量替换
    //格式: [@replace][var]...
    std::string replaceReplace(const Context& ctx, const ReplacePattern::Operator& opt, bool splited, StrCommandLine& dest)
    {
        std::string ret;
        for(const auto& param: opt.params)
        {
            const auto& str=ctx.stringGet(param.value, param.value);
            if(!ret.empty() && !str.empty())
                ret += ' ';
            ret += str;
        }

        return result(ret, splited, dest);
    }

    std::string result(std::string src, bool splited, StrCommandLine& dest)
    {
        if(splited==false)
            return std::move(src);

        CmdLineSeparator sep;
        auto itr=src.begin();
        const auto end=src.end();
        for(;;)
        {
            auto const status=sep(itr, end,
                [&dest](std::string::iterator s, std::string::iterator e)
                {
                    dest.emplace_back(s, e);
                }
            );

            if(status==false || itr==end)
                break;
        }

        return std::string();
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
    MD(replace, replaceReplace)
#undef MD
};

std::string ContextReplace::replace(const Context& ctx, const ReplacePattern& rp, bool splited, StrCommandLine& dest)
{
    std::string ret;
    for(const auto& op: rp.operatorsGet())
    {
        auto const itr=std::find_if(std::begin(dict_), std::end(dict_),
            [&op](const Unit& u)
            {
                return op.name==u.name;
            }
        );

        if(itr==std::end(dict_))
            continue;

        auto const tmp=(this->*itr->call)(ctx, op, splited, dest);
        if(!ret.empty() && !tmp.empty())
            ret += ' ';
        ret += tmp;
    }

    return ret;
}

VarSPtr Context::get(const std::string& name) const
{
    auto ctx=shared_from_this();
    while(ctx)
    {
        auto const itr=ctx->vars_.find(name);
        if(itr==ctx->vars_.end())
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

std::string Context::stringGet(const std::string& name, const std::string& def) const
{
    auto ptr=get(name);
    if(!ptr)
        return def;

    auto const strVal=boost::get<VarString>(ptr.get());
    if(strVal!=nullptr)
        return *strVal;

    auto const listVal=boost::get<VarList>(ptr.get());
    if(listVal->empty())
        return def;

    return listVal->front();
}

void Context::cmdlineReplace(const StrCommandLine& cmd, StrCommandLine& dest) const
{
    ErrorCode ec;
    dest.clear();
    for(const auto& s: cmd)
    {
        std::string d;
        const auto currentSize=dest.size();
        ReplacePattern::replace(ec, std::back_inserter(d), s.begin(), s.end(),
            [this, &s, &dest](ErrorCode& ec, std::back_insert_iterator<std::string> out, const ReplacePattern& rp)
            {
                ContextReplace cr;
                auto ret=cr.replace(*this, rp, rp.needSplit(), dest);
                out=std::copy(ret.begin(), ret.end(), out);
                return out;
            }
        );

        if(currentSize==dest.size())
            dest.emplace_back(std::move(d));
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

