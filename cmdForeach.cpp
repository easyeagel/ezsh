//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdForeach.cpp
//
//   Description:  foreach 命令实现
//
//       Version:  1.0
//       Created:  2014年12月28日 13时59分25秒
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

namespace ezsh
{

class GroupHeadForeach: public GroupPointBaseT<GroupHeadForeach>
{
    class Visitor: public boost::static_visitor<>
    {
    public:
        Visitor(GroupHeadForeach& sc)
            :sc_(sc)
        {}

        void operator()(const VarString& var) const
        {
            sc_.list_.emplace_back(var);
            sc_.listPtr_.push_back(&sc_.list_);
        }

        void operator()(const VarList& var) const
        {
            sc_.listPtr_.push_back(&var);
        }

    private:
        GroupHeadForeach& sc_;
    };

    friend class Visitor;
    typedef GroupPointBaseT<GroupHeadForeach> BaseThis;
public:
    GroupHeadForeach()
        :BaseThis("foreach - foreach group begin")
    {
        opt_.add_options()
            ("loop", bp::value<std::string>()->required(), "loop var name")
            ("var",  bp::value<std::string>(), "foreach from context var")
            ("list", bp::value<std::vector<std::string>>()->multitoken(), "foreah from this list")
        ;
    }

    void doit()
    {
        if(done_)
            return ecSet(EzshError::ecMake(EzshError::eGroupDone));

        if(first_)
        {
            first_=false;
            const auto& vm=mapGet();

            loop_=vm["loop"].as<std::string>();

            auto itr=vm.find("var");
            if(itr!=vm.end())
            {
                auto const ptr=contextGet()->get(itr->second.as<std::string>());
                if(ptr)
                    boost::apply_visitor(Visitor(*this), *ptr);
            }

            itr=vm.find("list");
            if(itr!=vm.end())
                listPtr_.push_back(&itr->second.as<VarList>());

            if(listPtr_.empty())
                return ecSet(EzshError::ecMake(EzshError::eGroupDone));
        }

        auto& ptr=listPtr_.front();
        contextGet()->set(loop_, VarSPtr(new Variable(ptr->at(index_++))));
        if(ptr->size()<=index_)
        {
            index_=0;
            listPtr_.pop_front();
            if(listPtr_.empty())
                done_=true;
        }
    }

    void doDry()
    {
        BaseThis::doDry();
        return doit();
    }

    static const char* nameGet()
    {
        return "foreach";
    }

private:
    bool done_=false;
    bool first_=true;
    std::string loop_;

    size_t index_=0;
    VarList list_;
    std::deque<const VarList*> listPtr_;
};

class GroupTailForeach: public GroupPointBaseT<GroupTailForeach>
{
    typedef GroupPointBaseT<GroupTailForeach> BaseThis;
public:
    GroupTailForeach()
        :BaseThis("endforeach - foreach group end")
    {}

    void doit()
    {
    }

    static const char* nameGet()
    {
        return "endforeach";
    }

};

class CmdForeach:public CommandGroupT<CmdForeach, GroupHeadForeach, GroupTailForeach>
{
    typedef CommandGroupT<CmdForeach, GroupHeadForeach, GroupTailForeach> BaseThis;
public:
    CmdForeach(const ScriptCommand& b, const Script& s, const ScriptCommand& e)
        : BaseThis(b, s, e)
    {}

    static CommandGroupTrait::Type_t typeCheck(const std::string& str)
    {
        if(str==GroupHeadForeach::nameGet())
            return CommandGroupTrait::eHead;
        if(str==GroupTailForeach::nameGet())
            return CommandGroupTrait::eTail;
        return CommandGroupTrait::eNone;
    }
};

namespace
{
    CommandGroupRegister<CmdForeach> gsCommandGroupRegister;
}

}

