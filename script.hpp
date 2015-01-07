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

#include<cctype>

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
            const auto ret=sep(itr, end,
                [this](std::string::iterator s, std::string::iterator e)
                {
                    args_.emplace_back(s, e);
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
        if(!line_.empty())
            line_ += ' ';
        line_ += s;
    }

    const StrCommandLine& cmdlineGet() const
    {
        return args_;
    }

    const std::string& cmdGet() const
    {
        return args_[0];
    }

    bool empty() const
    {
        return line_.empty();
    }

    const CmdTraitSPtr& traitGet() const
    {
        return trait_;
    }

    void traitSet(const CmdTraitSPtr& t)
    {
        trait_=t;
    }

	void reset()
	{
		line_.clear();
		trait_.reset();
		args_.clear();
	}

    MainReturn execute(const ContextSPtr& context);
    MainReturn execute(const ContextSPtr& context, CmdBase& cmd);
private:
    void cmdlineReplace(const ContextSPtr& context, const StrCommandLine& cmd, StrCommandLine& dest);

private:
    std::string line_;
    CmdTraitSPtr trait_;
    StrCommandLine args_;
};

class CommandGroupBase;
typedef std::shared_ptr<CommandGroupBase> CommandGroupBaseSPtr;

class Script
{
    typedef boost::variant<ScriptCommand, CommandGroupBaseSPtr> Unit;
public:
    typedef std::vector<Unit> Container;

    Script()=default;
    Script(Container&& s)
        :script_(std::move(s))
    {}

    typedef std::function<bool (std::istream&, ScriptCommand& )> BreakCond;

    static bool load(std::istream& strm, Script& spt);

    void push(Unit&& u)
    {
        script_.emplace_back(std::move(u));
    }

    MainReturn execute(const ContextSPtr& context);


private:
    Container script_;
};

class GroupPointBase: public CmdBase
{
public:
    GroupPointBase(ScriptCommand&& sc, const char* msg)
        :CmdBase(msg), sc_(std::move(sc))
    {}

    MainReturn execute(const ContextSPtr& ctx)
    {
        return sc_.execute(ctx, *this);
    }

    MainReturn doit() override
    {
        return MainReturn::eGood;
    }

    void oldContextSet(const ContextSPtr& old)
    {
        oldCtx_=old;
    }

    const ContextSPtr& oldContextGet() const
    {
        return oldCtx_;
    }

private:
    ContextSPtr oldCtx_;
    ScriptCommand sc_;
};

class CommandGroupBase
{
public:
    CommandGroupBase(Script&& s)
        :script_(std::move(s))
    {}

    virtual MainReturn execute(const ContextSPtr& context) = 0;

    virtual ~CommandGroupBase()
    {}

    Script& scriptGet()
    {
        return script_;
    }

    const Script& scriptGet() const
    {
        return script_;
    }

private:
    Script script_;
};

template<typename Obj, typename Base=CommandGroupBase>
class CommandGroup: public Base
{
    Obj& objGet()
    {
        return static_cast<Obj&>(*this);
    }

    const Obj& objGet() const
    {
        return static_cast<Obj&>(*this);
    }
public:
    CommandGroup(Script&& s)
        :Base(std::move(s))
    {}

    MainReturn execute(const ContextSPtr& context)
    {
        auto& b=objGet().groupBeginGet();
        b.oldContextSet(context);
        auto& e=objGet().groupEndGet();
        e.oldContextSet(context);

        auto ctx=context->alloc();

        for(;;)
        {
            auto ret=b.execute(ctx);
            if(ret.bad())
                return ret;

            if(ret.get()==MainReturn::eGroupDone)
                break;

            ret=this->scriptGet().execute(ctx);
            if(ret.bad())
                return ret;
        }

        return e.execute(ctx);
    }

    static std::unique_ptr<CommandGroupBase> create(ScriptCommand&& b, Script&& s, ScriptCommand&& e)
    {
        return std::unique_ptr<CommandGroupBase>(new Obj(std::move(b), std::move(s), std::move(e)));
    }

};

class CommandGroupDict
{
    typedef std::function<std::unique_ptr<CommandGroupBase>(ScriptCommand&&, Script&&, ScriptCommand&& )> GroupCreate;
    struct Unit
    {
        std::string begin;
        std::string end;
        GroupCreate create;

        bool operator<(const Unit& o) const
        {
            return begin<o.begin;
        }
    };

    typedef std::set<Unit> Dict;

    CommandGroupDict()=default;
public:

    static const CommandGroupDict& instance()
    {
        static CommandGroupDict gs;
        return gs;
    }

    const Unit* find(const std::string& begin) const
    {
        const auto itr=sets_.find(Unit{begin, std::string(), GroupCreate()});
        if(itr==sets_.end())
            return nullptr;
        return &*itr;
    }

    void insert(const std::string& b, const std::string& e, GroupCreate&& c)
    {
        sets_.insert(Unit{b, e, c});
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
        d.insert(G::beginGet(), G::endGet(), &G::create);
    }
};





}  //namespace ezsh



