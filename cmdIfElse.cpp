//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdIfElse.cpp
//
//   Description:  if...else.. 分支支持
//
//       Version:  1.0
//       Created:  2015年01月28日 14时53分29秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"option.hpp"
#include"script.hpp"
#include"optionPredication.hpp"

namespace ezsh
{

class GroupHeadIf: public GroupPointBaseT<GroupHeadIf, PredicationCmdBase<>>
{
    typedef GroupPointBaseT<GroupHeadIf, PredicationCmdBase<>> BaseThis;
public:
    GroupHeadIf(const char* msg="if - if group begin")
        :BaseThis(msg)
    {}

    void doit()
    {
        predicationInit(mapGet());
    }

    void doDry()
    {
        BaseThis::doDry();
        return doit();
    }

    static const char* nameGet()
    {
        return "if";
    }

    bool needCall() const
    {
        return isPassed();
    }
};

class GroupHeadElseIf: public GroupHeadIf
{
public:
    GroupHeadElseIf()
        :GroupHeadIf("elseif - elseif branch of if group")
    {}

    static const char* nameGet()
    {
        return "elseif";
    }
};

class GroupHeadElse: public GroupPointBaseT<GroupHeadElse>
{
    typedef GroupPointBaseT<GroupHeadElse> BaseThis;
public:
    GroupHeadElse()
        :BaseThis("else - else branch of if group")
    {}

    void doit()
    {
    }

    static const char* nameGet()
    {
        return "else";
    }

    bool needCall() const
    {
        return true;
    }
};

class GroupTailEndIf: public GroupPointBaseT<GroupTailEndIf>
{
    typedef GroupPointBaseT<GroupTailEndIf> BaseThis;
public:
    GroupTailEndIf()
        :BaseThis("endif - if group end")
    {}

    void doit()
    {
    }

    static const char* nameGet()
    {
        return "endif";
    }
};

class CmdIfElse:public CommandGroupT<CmdIfElse, GroupHeadIf, GroupTailEndIf>
{
    typedef CommandGroupT<CmdIfElse, GroupHeadIf, GroupTailEndIf> BaseThis;
public:
    CmdIfElse(const GroupCommand::BodySet& b, const ScriptCommand& e)
        : BaseThis(b, e)
    {}

    static CommandGroupTrait::Type_t typeCheck(CommandGroupTrait::CheckStatus& status, const std::string& str)
    {
        if(status.empty())
            status.resize(4, 0);

        if(str==GroupHeadIf::nameGet())
        {
            status[0] += 1;
            return CommandGroupTrait::eHead;
        }

        if(str==GroupHeadElseIf::nameGet())
        {
            status[1] += 1;
            return CommandGroupTrait::eMiddle;
        }

        if(str==GroupHeadElse::nameGet())
        {
            auto& c=status[2];
            c += 1;
            if(c>1) //esle 只能出现一次
                return CommandGroupTrait::eNone;
            return CommandGroupTrait::eMiddle;
        }

        if(str==GroupTailEndIf::nameGet())
        {
            status[3] += 1;
            return CommandGroupTrait::eTail;
        }

        return CommandGroupTrait::eNone;
    }

    void taskDoit()
    {
        tail_.oldContextSet(this->contextGet());
        auto ctx=this->contextGet()->alloc();
        contextGet()->yield([this, &ctx]()
            {
                this->contextStart(ctx);
            }
        );
    }

    template<typename Cmd>
    bool call(const ContextSPtr& ctx, const GroupCommand::Body& body)
    {
        Cmd cmd;
        cmd.oldContextSet(this->contextGet());
        body.head.init(this->ecGet(), ctx, cmd);
        if(this->bad())
            return false;

        cmd.taskDoit();
        if(cmd.bad())
        {
            this->ecSet(cmd.ecGet());
            return false;
        }

        if(!cmd.needCall())
            return false;

        const auto& script=*body.script;
        script.execute(this->ecGet(), cmd.errorBreak(), ctx);
        return true;
    }

private:
    void contextStart(const ContextSPtr& ctx)
    {
        ctx->start([this, &ctx]()
            {
                auto itr=scBody_.begin();
                auto const end=scBody_.end();

                assert(itr!=end);
                
                //循环处理可能的 if elseif
                for(;; ++itr)
                {
                    if(itr==end)
                        break;

                    const auto& sc=itr->head;
                    if(sc.cmdGet()==GroupHeadIf::nameGet())
                    {
                        auto const called=call<GroupHeadIf>(ctx, *itr);
                        if(this->bad())
                            return contexResum();
                        if(called)
                            break;
                        continue;
                    } else if(sc.cmdGet()==GroupHeadElseIf::nameGet()) {
                        auto const called=call<GroupHeadElseIf>(ctx, *itr);
                        if(this->bad())
                            return contexResum();
                        if(called)
                            break;
                        continue;
                    } else {
                        assert(sc.cmdGet()==GroupHeadElse::nameGet());
                        auto const called=call<GroupHeadElse>(ctx, *itr);
                        if(this->bad())
                            return contexResum();
                        if(called)
                            break;
                        continue;
                    }
                }

                scTail_.init(this->ecGet(), ctx, tail_);
                if(this->bad())
                    return contexResum();

                tail_.taskDoit();
                if(tail_.bad())
                    this->ecSet(tail_.ecGet());
                return contexResum();
            }
        );
    }

    void contexResum()
    {
        core::MainServer::post([this]()
            {
                this->contextGet()->resume();
            }
        );
    }

private:
    GroupTailEndIf tail_;
};

namespace
{
    CommandGroupRegister<CmdIfElse> gsCommandGroupRegister;
}

}

