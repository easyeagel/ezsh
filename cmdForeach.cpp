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

class GroupBeginForeach: public GroupPointBase
{
    class Visitor: public boost::static_visitor<>
    {
    public:
        Visitor(GroupBeginForeach& sc)
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
        GroupBeginForeach& sc_;
    };

    friend class Visitor;
public:
    GroupBeginForeach(const ScriptCommand& sc)
        :GroupPointBase(sc, "foreach - foreach group begin")
    {
        opt_.add_options()
            ("loop", bp::value<std::string>()->required(), "loop var name")
            ("var",  bp::value<std::string>(), "foreach from context var")
            ("list", bp::value<std::vector<std::string>>()->multitoken(), "foreah from this list")
        ;
    }

    MainReturn doit()
    {
        if(done_)
            return MainReturn::eGroupDone;

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
                return MainReturn::eGroupDone;
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

        return MainReturn::eGood;
    }

private:
    bool done_=false;
    bool first_=true;
    std::string loop_;

    size_t index_=0;
    VarList list_;
    std::deque<const VarList*> listPtr_;
};

class CmdForeach:public CommandGroup<CmdForeach>
{
    typedef CommandGroup<CmdForeach> BaseThis;
public:
    CmdForeach(const ScriptCommand& b, const Script& s, const ScriptCommand& e)
        : BaseThis(s)
        , begin_(b)
        , end_(e, "endforeach - foreach group end")
    {}

    static const char* beginGet()
    {
        return "foreach";
    }

    static const char* endGet()
    {
        return "endforeach";
    }

    typedef GroupPointBase GroupEnd;
    typedef GroupBeginForeach GroupBegin;

    GroupBegin& groupBeginGet()
    {
        return begin_;
    }

    GroupEnd& groupEndGet()
    {
        return end_;
    }

private:
    GroupBegin begin_;
    GroupEnd   end_;
};

namespace
{
    CommandGroupRegister<CmdForeach> gsCommandGroupRegister;
}

}

