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
    bool operator()(Itr& next, Itr const end, Out&& out)
    {
        skipBlank(next, end);
        if(next==end)
            return false;

        start_=next;
        tail_=next;
        while(next!=end)
        {
            switch(*next)
            {
                case '"':
                {
                    nextColon(next, end);
                    break;
                }
                case ' ':
                case '\t':
                {
                    out(start_, tail_);
                    skipBlank(next, end);
                    start_=next;
                    tail_=next;
                    break;
                }
                case '\\':
                {
                    ++next;
                    if(next==end)
                        return false;
                    *tail_++=escape(*next++);
                    continue;
                }
                default:
                {
                    if(tail_!=next)
                    {
                        *tail_++=*next++;
                    } else {
                        ++tail_;
                        ++next;
                    }
                    break;
                }
            }
        }

        if(ret==false)
            return false;

        if(start_!=tail_)
            out(start_, tail_);
        return true;
    }

    void reset()
    {
        start_=Itr();
        tail_=start_;
        ret=true;
    }

private:
    void skipBlank(Itr& next, Itr const end)
    {
        while(next!=end && std::isspace(*next))
            ++next;
    }

    void nextColon(Itr& next, Itr const end)
    {
        ++next;
        while(next!=end)
        {
            if(*next=='"')
            {
                ++next;
                return;
            }

            if(*next=='\\')
            {
                ++next;
                *tail_++=escape(*next++);
                continue;
            }

            *tail_++=*next++;
        }

        ret=false;
    }

    char escape(char c) const
    {
        switch(c)
        {
            case 'n': return '\n';
            case 't': return '\t';
            default: return c;
        }
    }

private:
    Itr start_;
    Itr tail_;
    bool ret=true;
};

class Script;
class ScriptCommand;
typedef std::function<std::unique_ptr<TaskBase>(const ScriptCommand&, const Script&, const ScriptCommand& )> CommandGroupCreate;

struct CommandGroupTrait
{
    enum Type_t{eNone, eHead, eMiddle, eTail};
    typedef std::function<Type_t (const std::string& )> TypeCheck;

    std::string head;
    TypeCheck typeCheck;
    CommandGroupCreate create;
};
typedef std::shared_ptr<CommandGroupTrait> CommandGroupTraitSPtr;

class ScriptLoadContext
{
public:
    struct Unit
    {
        Unit(const std::string& f)
            :file(f), lineNumber(0), stream(new bf::ifstream(Path(f).path()))
        {}

        Unit(Unit&& u)
            : file(std::move(u.file))
            , lineNumber(u.lineNumber)
            , stream(std::move(u.stream))
        {}

        std::string file;
        size_t lineNumber;
        std::unique_ptr<bf::ifstream> stream;
    };

    void push(const std::string& f)
    {
        stack_.push(Unit(f));
    }

    bool getline(std::string& line)
    {
        auto& u=stack_.top();
        std::getline(*u.stream, line);
        const bool ret=static_cast<bool>(*u.stream);
        if(ret)
            u.lineNumber += 1;
        return ret;
    }

    bool good() const
    {
        return static_cast<bool>(*stack_.top().stream);
    }

    bool bad() const
    {
        return !good();
    }

    void messageSet(const std::string& msg)
    {
        message_=msg;
    }

    const std::string messageGet() const
    {
        auto& u=stack_.top();
        return u.file + ":" + std::to_string(u.lineNumber) + ":" + message_;
    }

private:
    std::string message_;
    std::stack<Unit> stack_;
};

class ScriptCommand
{
public:
    bool tokenize();

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

    void init(ErrorCode& ec, const ContextSPtr& context, CmdBase& cmd) const;

    void execute(ErrorCode& ec, const ContextSPtr& context) const;
    void execute(ErrorCode& ec, const ContextSPtr& context, CmdBase& cmd) const;

private:
    std::string line_;
    CmdTraitSPtr trait_;
    StrCommandLine args_;
};

class GroupCommand
{
public:
    GroupCommand(ScriptCommand&& h, ScriptCommand&& t, Script&& b, const CommandGroupTraitSPtr& tt);
    void execute(ErrorCode& ec, const ContextSPtr& context) const;

private:
    ScriptCommand head_;
    ScriptCommand tail_;
    std::shared_ptr<const Script> body_;
    CommandGroupTraitSPtr trait_;
};

class Script
{
    class Unit: public boost::variant<ScriptCommand, GroupCommand>
    {
        typedef boost::variant<ScriptCommand, GroupCommand> BaseThis;
    public:
		Unit(ScriptCommand&& sc)
            :BaseThis(std::move(sc))
        {}

		Unit(GroupCommand&& gc)
			:BaseThis(std::move(gc))
		{}


    };
public:
    typedef std::vector<Unit> Container;

    Script()=default;
    Script(Container&& s)
        :script_(std::move(s))
    {}

    static bool load(ScriptLoadContext& ctx, Script& spt);

    void push(Unit&& u)
    {
        script_.emplace_back(std::move(u));
    }

    void execute(ErrorCode& ec, const ContextSPtr& context) const;

private:
    Container script_;
};

template<typename Obj, typename Head, typename Tail, typename Base=TaskBase>
class CommandGroupT: public Base
{
    Obj& objGet()
    {
        return static_cast<Obj&>(*this);
    }

    const Obj& objGet() const
    {
        return static_cast<Obj&>(*this);
    }

    typedef Base BaseThis;
public:
    CommandGroupT(const ScriptCommand& b, const Script& s, const ScriptCommand& e)
        :scHead_(b), script_(s), scTail_(e)
    {}

    void taskDoit()
    {
        head_.oldContextSet(this->contextGet());

        tail_.oldContextSet(this->contextGet());

        auto ctx=this->contextGet()->alloc();

        scHead_.init(this->ecGet(), ctx, head_);
        if(this->bad())
            return;

        scTail_.init(this->ecGet(), ctx, tail_);
        if(this->bad())
            return;

        for(;;)
        {
            head_.taskDoit();
            if(head_.bad())
            {
                if(head_.ecGet()==EzshError::ecMake(EzshError::eGroupDone))
                    break;
                this->ecSet(head_.ecGet());
                return;
            }

            scriptGet().execute(this->ecGet(), ctx);
            if(this->bad())
                return;
        }

        tail_.taskDoit();
        if(tail_.bad())
            this->ecSet(tail_.ecGet());
    }

    static std::unique_ptr<TaskBase> create(const ScriptCommand& b, const Script& s, const ScriptCommand& e)
    {
        return std::unique_ptr<TaskBase>(new Obj(b, s, e));
    }

    const Script& scriptGet() const
    {
        return script_;
    }

    static const char* headGet()
    {
        return Head::nameGet();
    }

    static const char* tailGet()
    {
        return Tail::nameGet();
    }

private:
    const ScriptCommand& scHead_;
    const Script& script_;
    const ScriptCommand& scTail_;

protected:
    Head head_;
    Tail tail_;
};

class CommandGroupDict
{
    typedef std::map<std::string, CommandGroupTraitSPtr> Dict;

    CommandGroupDict()=default;
public:

    static const CommandGroupDict& instance()
    {
        static CommandGroupDict gs;
        return gs;
    }

    CommandGroupTraitSPtr find(const std::string& head) const
    {
        const auto itr=dict_.find(head);
        if(itr==dict_.end())
            return nullptr;
        return itr->second;
    }

    void insert(const std::string& b, CommandGroupTrait::TypeCheck&& check, CommandGroupCreate&& c)
    {
        dict_[b]=CommandGroupTraitSPtr(new CommandGroupTrait{b, std::move(check), std::move(c)});
    }

private:
    Dict dict_;
};

template<typename G>
struct CommandGroupRegister
{
    CommandGroupRegister()
    {
        auto& d=const_cast<CommandGroupDict&>(CommandGroupDict::instance());
        d.insert(G::headGet(), &G::typeCheck, &G::create);
    }
};

template<typename Obj>
class GroupPointBaseT: public CmdBaseT<Obj>
{
    typedef CmdBaseT<Obj> BaseThis;
public:
    GroupPointBaseT(const char* msg)
        :BaseThis(msg)
    {}

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
};


}  //namespace ezsh

