//  Copyright [2015] <lgb (LiuGuangBao)>
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

void ScriptCommand::execute(ErrorCode& ec, const ContextSPtr& context) const
{
    const auto& cmd=traitGet()->create();
    return execute(ec, context, *cmd);
}

void ScriptCommand::execute(ErrorCode& ec, const ContextSPtr& context, CmdBase& cmd) const
{
    init(ec, context, cmd);
    if(ec.bad())
        return;
    cmd.taskDoit();
}

void ScriptCommand::init(ErrorCode& ec, const ContextSPtr& context, CmdBase& cmd) const
{
    StrCommandLine args;
    context->cmdlineReplace(ec, cmdlineGet(), args);
    if(ec.bad())
        return;

    try
    {
        cmd.parse(std::move(args));
    } catch (const boost::program_options::error& e) {
        std::cerr << e.what() << std::endl;
        ec=EzshError::ecMake(EzshError::eParamInvalid);
        return;
    }

    cmd.init(context);
    if(cmd.bad())
        ec=cmd.ecGet();
}

GroupCommand::GroupCommand(ScriptCommand&& h, ScriptCommand&& t, Script&& b, const CommandGroupTraitSPtr& tt)
    : head_(std::move(h)), tail_(std::move(t))
    , body_(std::shared_ptr<Script>(new Script(std::move(b))))
    , trait_(tt)
{}

void GroupCommand::execute(ErrorCode& ec, const ContextSPtr& context) const
{
    auto const ptr=trait_->create(head_, *body_, tail_);
    ptr->init(context);

    if(ptr->bad())
    {
        ec=ptr->ecGet();
        return ;
    }

    ptr->taskDoit();
    if(ptr->bad())
    {
        ec=ptr->ecGet();
        return ;
    }
}

class ScriptLoad
{
public:
    ScriptLoad()=default;
    ScriptLoad(const CommandGroupTraitSPtr& trait)
        :traitGroup_(trait)
    {}

    bool load(ScriptLoadContext& ctx, Script& spt)
    {
        for(;;)
        {
            if(firstLine_)
            {
                firstLine_=false;
                if(line_.empty())
                    ctx.getline(line_);
            } else {
                ctx.getline(line_);
            }

            if(ctx.bad())
            {
                if(unit_.empty())
                    break;
                return unitDoit(ctx, spt);
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
                if(unitDoit(ctx, spt)==false)
                    return false;
                if(toBreak_)
                    break;
            }
        }

        return true;
    }

    bool unitDoit(ScriptLoadContext& ctx, Script& spt)
    {
        if(unit_.tokenize()==false)
        {
            ctx.messageSet("cmline tokenize failed");
            return false;
        }

        auto trait=CmdDict::find(unit_.cmdGet());
        if(trait!=nullptr)
        {
            unit_.traitSet(trait);

            spt.push(std::move(unit_));
            unit_.reset(); //MSVC 没有正确实现移到构造

            //或达到文件结尾
            if(ctx.bad())
                toBreak_=true;

            unit_.lineAppend(line_);
            return true;
        }

        //找到结束命令
        if(traitGroup_ && traitGroup_->typeCheck(unit_.cmdGet())==CommandGroupTrait::eTail)
        {
            toBreak_=true;
            return true;
        }

        return groupLoad(ctx, spt);
    }

    const std::string& lastLineGet() const
    {
        return line_;
    }

    bool groupLoad(ScriptLoadContext& ctx, Script& spt)
    {
        const auto& gd=CommandGroupDict::instance();
        auto const u=gd.find(unit_.cmdGet());
        if(u==nullptr)
        {
            ctx.messageSet("unkown command: " + unit_.cmdGet());
            return false;
        }

        Script group;
        ScriptLoad sl(u);
        sl.line_=std::move(line_);
        if(sl.load(ctx, group)==false)
            return false;

        if(sl.unit_.empty() || u->typeCheck(sl.unit_.cmdGet())!=CommandGroupTrait::eTail)
        {
            ctx.messageSet("unclosed group: " + u->head);
            return false;
        }

        spt.push(GroupCommand(std::move(unit_), std::move(sl.unit_), std::move(group), u));
        unit_.reset();
        if(!sl.line_.empty())
            unit_.lineAppend(sl.line_);

        return true;
    }

private:
    bool firstLine_=true;
    bool toBreak_=false;
    std::string line_;
    ScriptCommand unit_;
    CommandGroupTraitSPtr traitGroup_;
};

bool Script::load(ScriptLoadContext& ctx, Script& spt)
{
    ScriptLoad sl;
    return sl.load(ctx, spt);
}

void Script::execute(ErrorCode& ec, const ContextSPtr& context) const
{
    for(auto& ptr: script_)
    {
        auto const sc=boost::get<ScriptCommand>(&ptr);
        if(sc!=nullptr)
        {
            sc->execute(ec, context);
            if(ec.bad())
                return;
            continue;
        }

        auto const cg=boost::get<GroupCommand>(&ptr);
        if(cg!=nullptr)
        {
            cg->execute(ec, context);
            if(ec.bad())
                return;
            continue;
        }
    }
}


}

