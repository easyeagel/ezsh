//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  script.hpp
//
//   Description:  脚本相关的基本功能
//
//       Version:  1.0
//       Created:  2015年01月06日 16时02分10秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<set>
#include<string>
#include<vector>

#include"option.hpp"
#include"context.hpp"

namespace ezsh
{

class CmdLineSeparator
{
    typedef std::string::iterator Itr;
public:
    template<typename Out>
    bool operator()(Itr& next, Itr end, Out&& out)
    {
        skipBlank(next, end);
        if(next==end)
            return false;

        start_=next;
        while(next!=end)
        {
            switch(*next)
            {
                case '"':
                {
                    escaped_=!escaped_;
                    break;
                }
                case ' ':
                case '\t':
                {
                    if(escaped_==false)
                    {
                        out(start_, next++);
                        return true;
                    }

                    break;
                }
                default:
                    break;
            }

            ++next;
        }

        if(escaped_)
            return false;
        out(start_, end);
        return true;
    }

    void reset()
    {
        start_=Itr();
        escaped_=false;
    }

private:
    void skipBlank(Itr& next, Itr end)
    {
        while(next!=end && std::isspace(*next))
            ++next;
    }

private:
    Itr start_;
    bool escaped_=false;
};

class CommandGroupDict
{
    struct Unit
    {
        std::string begin;
        std::string end;

        bool operator<(const Unit& o) const
        {
            return begin<o.begin;
        }
    };

    typedef std::set<Unit> Dict;

    CommandGroupDict()=default;
public:

    const CommandGroupDict& instance()
    {
        static CommandGroupDict gs;
        return gs;
    }

    const Unit* find(const std::string& begin) const
    {
        const auto itr=sets_.find(Unit{begin, std::string()});
        if(itr==sets_.end())
            return nullptr;
        return &*itr;
    }

    void insert(const std::string& b, const std::string& e)
    {
        sets_.insert(Unit{b, e});
    }

private:
    Dict sets_;
};

template<typename G>
struct CommandGroupRegister
{
    CommandGroupRegister()
    {
        auto& d=const_cast<CommandGroupDict&>(CommandGroupDict::instance());
        d.insert(G::beginGet(), G::endGet());
    }
};

typedef std::vector<char*> CommandLine;
typedef std::vector<std::string> StrCommandLine;

class ScriptCommand
{
public:
    bool tokenize()
    {
        CmdLineSeparator sep;
        auto itr=line_.begin();
        const auto end=line_.end();
        for(;;)
        {
            bool have=false;
            const auto ret=sep(itr, end,
                [this, &have](std::string::iterator s, std::string::iterator e)
                {
                    *e='\0';
                    args_.push_back(&*s);
                    have=true;
                }
            );

            if(ret==false)
                return false;

            if(itr==end)
                return true;
        }
    }

    void lineAppend(const std::string& s)
    {
        line_ += ' ';
        line_ += s;
    }

    const StrCommandLine& cmdlineGet() const
    {
        return args_;
    }

    bool empty() const
    {
        return line_.empty();
    }

    bool init()
    {
        if(tokenize()==false)
            return false;

        trait_=CmdDict::find(args_[0]);
        if(trait_==nullptr)
            return false;

        return true;
    }

    const CmdTraitSPtr& traitGet() const
    {
        return trait_;
    }

private:
    std::string line_;
    CmdTraitSPtr trait_;
    StrCommandLine args_;
};

class CommandGroupBase
{
public:
    virtual MainReturn doit(const ContextSPtr& context) = 0;

    virtual ~CommandGroupBase()
    {}
};

class Script
{
    typedef std::unique_ptr<ScriptCommand> ScriptCommandUPtr;
    typedef std::unique_ptr<CommandGroupBase> CommandGroupBaseUPtr;
    typedef boost::variant<ScriptCommandUPtr, CommandGroupBaseUPtr> Unit;
public:
    typedef std::vector<Unit> Container;

    Script()=default;
    Script(Container&& s)
        :script_(std::move(s))
    {}

    static bool load(const std::string& file, Script& spt);
    MainReturn execute(const ContextSPtr& context);

private:
    void cmdlineReplace(const ContextSPtr& context, const StrCommandLine& cmd, StrCommandLine& dest);

private:
    Container script_;
};

template<typename Obj, typename Base=CommandGroupBase>
class CommandGroup: public Base
{
public:
    CommandGroup(Script&& s)
        :script_(std::move(s))
    {}

    MainReturn doit(const ContextSPtr& )
    {
        return MainReturn::eGood;
    }

private:
    Script script_;
};



}  //namespace ezsh



