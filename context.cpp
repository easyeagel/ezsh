﻿//  Copyright [2014] <lgb (LiuGuangBao)>
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
#include<boost/algorithm/string/split.hpp>
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

    //执行变量分隔
    //格式: [@split][var]...
    std::string splitReplace(const Context& ctx, const ReplacePattern::Operator& opt, bool /* splited */, StrCommandLine& dest)
    {
        for(const auto& param: opt.params)
        {
            const auto& str=ctx.stringGet(param.value, param.value);
            if(str.empty())
                continue;

            std::vector<std::string> tmp;
            boost::algorithm::split(tmp, str, boost::algorithm::is_any_of(",;"), boost::algorithm::token_compress_on);
            for(auto& s: tmp)
            {
                boost::algorithm::trim(s);
                dest.emplace_back(std::move(s));
            }
        }

        return std::string();
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
    MD(include, includeReplace)
    MD(match,   matchReplace)
    MD(replace, replaceReplace)
    MD(split, splitReplace)
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
    return stringGet(name, std::string());
}

void Context::setif(const std::string& name, const VarSPtr& val)
{
    auto ptr=get(name);
    if(ptr)
        return;
    set(name, val);
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

    std::string ret;
    for(auto& l: *listVal)
    {
        ret += l;
        ret += ' ';
    }

    ret.resize(ret.size()-1);
    return std::move(ret);
}

void ContextVisitor::setDo(const std::vector<std::string>& sets)
{
    for(const auto& s: sets)
    {
        const auto& p=simpleSplit(s, '=');
        ctx_.set(p.first, VarSPtr(new Variable(p.second)));
    }
}

void ContextVisitor::setIfDo(const std::vector<std::string>& sets)
{
    for(const auto& s: sets)
    {
        const auto& p=simpleSplit(s, '=');
        ctx_.setif(p.first, VarSPtr(new Variable(p.second)));
    }
}

void ContextVisitor::setListDo(const std::vector<std::string>& sets)
{
    for(const auto& s: sets)
    {
        const auto& p=simpleSplit(s, '=');
        VarList list;
        simpleSplit(p.second, [&list](std::string&& v)
            {
                list.emplace_back(std::move(v));
            }, ',');

        ctx_.set(p.first, VarSPtr(new Variable(list)));
    }
}

void ContextVisitor::setIfListDo(const std::vector<std::string>& sets)
{
    for(const auto& s: sets)
    {
        const auto& p=simpleSplit(s, '=');
        VarList list;
        simpleSplit(p.second, [&list](std::string&& v)
            {
                list.emplace_back(std::move(v));
            }, ',');

        ctx_.setif(p.first, VarSPtr(new Variable(list)));
    }
}

void ContextVisitor::unsetDo(const std::vector<std::string>& sets)
{
    for(const auto& s: sets)
        ctx_.unset(s);
}

void ContextVisitor::echoDo(const std::vector<std::string>& echos)
{
    if(echos.empty())
    {
        ctx_.stdOut() << std::endl;
        return;
    }

    const auto size=echos.size()-1;
    for(size_t i=0; i<size; ++i)
        ctx_.stdOut() << echos[i] << ' ';
    ctx_.stdOut() << echos.back() << std::endl;
}

void Context::cmdlineReplace(ErrorCode& ec, const StrCommandLine& cmd, StrCommandLine& dest) const
{
    dest.clear();
    for(const auto& s: cmd)
    {
        std::string d;
        const auto currentSize=dest.size();
        PatternReplace pr;
        pr.replace(ec, std::back_inserter(d), s.begin(), s.end(),
            [this, &s, &dest](ErrorCode& , std::back_insert_iterator<std::string> out, const ReplacePattern& rp)
            {
                ContextReplace cr;
                auto ret=cr.replace(*this, rp, rp.needSplit(), dest);
                out=std::copy(ret.begin(), ret.end(), out);
                return out;
            }
        );

        if(ec.bad())
            return;

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

