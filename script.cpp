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
    if(cmd.bad())
        ec=cmd.ecGet();
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

GroupCommand::GroupCommand(const CommandGroupTraitSPtr& tt)
    :trait_(tt)
{}

void GroupCommand::tailSet(ScriptCommand&& t)
{
    tail_=std::move(t);
}

void GroupCommand::bodyPush(ScriptCommand&& h, Script&& s)
{
    body_.emplace_back(Body{std::move(h), std::shared_ptr<const Script>(new Script(std::move(s)))});
}

void GroupCommand::execute(ErrorCode& ec, const ContextSPtr& context) const
{
    auto const ptr=trait_->create(body_, tail_);
    ptr->init(context);

    if(ptr->bad())
    {
        ec=ptr->ecGet();
        return ;
    }

    ptr->taskDoit();
    if(ptr->bad())
        ec=ptr->ecGet();
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
        static const std::string appendChars="-,;{}@+*";
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
            } else if(appendChars.find(line_[0])!=std::string::npos) {
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

        if(traitGroup_)
        {
            groupType_=traitGroup_->typeCheck(groupStatus_, unit_.cmdGet());
            switch(groupType_)
            {
                //找到结束命令
                case CommandGroupTrait::eTail:
                case CommandGroupTrait::eMiddle:
                    toBreak_=true;
                    return true;
                case CommandGroupTrait::eHead:
                    //ctx.messageSet("command group match error");
                    break;
                case CommandGroupTrait::eNone:
                    break;
            }
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

        GroupCommand group(u);
        for(;;)
        {
            Script script;
            ScriptLoad sl(u);
            sl.line_=std::move(line_);
            if(sl.load(ctx, script)==false)
                return false;

            if(sl.unit_.empty())
            {
                ctx.messageSet("unclosed group: " + u->head);
                return false;
            }

            switch(sl.groupType_)
            {
                case CommandGroupTrait::eTail:
                {
                    group.bodyPush(std::move(unit_), std::move(script));
                    group.tailSet(std::move(std::move(sl.unit_)));
                    spt.push(std::move(group));
                    unit_.reset();
                    if(!sl.line_.empty())
                        unit_.lineAppend(sl.line_);
                    return true;
                }
                case CommandGroupTrait::eMiddle:
                {
                    group.bodyPush(std::move(unit_), std::move(script));
                    unit_=std::move(sl.unit_);
                    if(!sl.line_.empty())
                        line_=std::move(sl.line_);
                    break;
                }
                default:
                    ctx.messageSet("unclosed group: " + u->head);
                    return false;
            }
        }
    }

private:
    bool firstLine_=true;
    bool toBreak_=false;
    std::string line_;
    ScriptCommand unit_;

    CommandGroupTraitSPtr traitGroup_;
    CommandGroupTrait::Type_t groupType_;
    CommandGroupTrait::CheckStatus groupStatus_;
};

bool Script::load(ScriptLoadContext& ctx, Script& spt)
{
    ScriptLoad sl;
    return sl.load(ctx, spt);
}

void Script::execute(ErrorCode& ec, bool errorBreak, const ContextSPtr& context) const
{
    for(auto& ptr: script_)
    {
        auto const sc=boost::get<ScriptCommand>(&ptr);
        if(sc!=nullptr)
        {
            sc->execute(ec, context);
            if(ec.bad()) //脚本执行出错，默认继续执行
            {
                if(errorBreak)
                    return;
                ec.clear();
            }
            continue;
        }

        auto const cg=boost::get<GroupCommand>(&ptr);
        if(cg!=nullptr)
        {
            cg->execute(ec, context);
            if(ec.bad())
            {
                if(errorBreak)
                    return;
                ec.clear();
            }
            continue;
        }
    }
}


}

