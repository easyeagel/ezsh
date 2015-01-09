﻿//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  script.cpp
//
//   Description:  脚本基本组件
//
//       Version:  1.0
//       Created:  2015年01月06日 17时15分37秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"script.hpp"
#include<boost/algorithm/string.hpp>

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
        end_=next;
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
                    out(start_, end_);
                    skipBlank(next, end);
                    start_=next;
                    end_=next;
                    break;
                }
                default:
                {
                    if(end_!=next)
                    {
                        *end_++=*next++;
                    } else {
                        ++end_;
                        ++next;
                    }
                    break;
                }
            }
        }

        if(ret==false)
            return false;

        if(start_!=end_)
            out(start_, end_);
        return true;
    }

    void reset()
    {
        start_=Itr();
        end_=start_;
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

            *end_++=*next++;
        }

        ret=false;
    }

private:
    Itr start_;
    Itr end_;
    bool ret=true;
};

bool ScriptCommand::tokenize()
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

MainReturn ScriptCommand::execute(const ContextSPtr& context) const
{
    const auto& cmd=traitGet()->create();
    return execute(context, *cmd);
}

MainReturn ScriptCommand::execute(const ContextSPtr& context, CmdBase& cmd) const
{
    CommandLine args;
    StrCommandLine strArgs;
    context->cmdlineReplace(cmdlineGet(), strArgs);
    std::for_each(strArgs.begin(), strArgs.end(),
        [&args](const std::string& s)
        {
            args.push_back(const_cast<char*>(s.data()));
        }
    );

    try
    {
        cmd.parse(args.size(), const_cast<char**>(args.data()));
    } catch (const boost::program_options::error& ec) {
        std::cerr << ec.what() << std::endl;
        return MainReturn::eParamInvalid;
    }

    auto ret=cmd.init(context);
    if(ret.good())
        ret=cmd.doit();
    return ret;
}

GroupCommand::GroupCommand(ScriptCommand&& h, ScriptCommand&& t, Script&& b, const CommandGroupTraitSPtr& tt)
    : head_(std::move(h)), tail_(std::move(t))
    , body_(std::shared_ptr<Script>(new Script(std::move(b))))
    , trait_(tt)
{}

MainReturn GroupCommand::execute(const ContextSPtr& context) const
{
    auto const ptr=trait_->create(head_, *body_, tail_);
    return ptr->execute(context);
}

class ScriptLoad
{
public:
    ScriptLoad()=default;
    ScriptLoad(const std::string& end)
        :end_(end)
    {}

    bool load(std::istream& strm, Script& spt)
    {
        for(;;)
        {
            if(firstLine_)
            {
                firstLine_=false;
                if(line_.empty())
                    std::getline(strm, line_);
            } else {
                std::getline(strm, line_);
            }

            if(!strm)
            {
                if(unit_.empty())
                    break;
                return unitDoit(strm, spt);
            }

            boost::trim(line_);
            if(line_.empty() || line_[0]=='#')
            {
                continue;
            } else if(line_[0]=='-') {
                unit_.lineAppend(line_);
                continue;
            } else if(unit_.empty()) {
                unit_.lineAppend(line_);
                continue;
            } else {
                if(unitDoit(strm, spt)==false)
                    return false;
                if(toBreak_)
                    break;
            }
        }

        return true;
    }

    bool unitDoit(std::istream& strm, Script& spt)
    {
        if(unit_.tokenize()==false)
            return false;

        auto trait=CmdDict::find(unit_.cmdGet());
        if(trait!=nullptr)
        {
            unit_.traitSet(trait);

            spt.push(std::move(unit_));
            unit_.reset(); //MSVC 没有正确实现移到构造

            //或达到文件结尾
            if(!strm)
                toBreak_=true;

            unit_.lineAppend(line_);
            return true;
        }

        //找到结束命令
        if(unit_.cmdGet()==end_)
        {
            toBreak_=true;
            return true;
        }

        return groupLoad(strm, spt);
    }

    const std::string& lastLineGet() const
    {
        return line_;
    }

    bool groupLoad(std::istream& strm, Script& spt)
    {
        const auto& gd=CommandGroupDict::instance();
        auto const u=gd.find(unit_.cmdGet());
        if(u==nullptr)
            return false;

        Script group;
        ScriptLoad sl(u->tail);
        sl.line_=std::move(line_);
        if(sl.load(strm, group)==false)
            return false;

        spt.push(GroupCommand(std::move(unit_), std::move(sl.unit_), std::move(group), u));
        unit_.reset();
        if(!sl.line_.empty())
            unit_.lineAppend(sl.line_);

        return true;
    }

private:
    bool firstLine_=true;
    bool toBreak_=false;
    std::string end_;
    std::string line_;
    ScriptCommand unit_;
};

bool Script::load(std::istream& strm, Script& spt)
{
    ScriptLoad sl;
    return sl.load(strm, spt);
}

MainReturn Script::execute(const ContextSPtr& context) const
{
    for(auto& ptr: script_)
    {
        auto const sc=boost::get<ScriptCommand>(&ptr);
        if(sc!=nullptr)
        {
            const auto ret=sc->execute(context);
            if(ret.bad())
                return ret;
            continue;
        }

        auto const cg=boost::get<GroupCommand>(&ptr);
        if(cg!=nullptr)
        {
            const auto ret=cg->execute(context);
            if(ret.bad())
                return ret;
            continue;
        }
    }

    return MainReturn::eGood;
}


}

