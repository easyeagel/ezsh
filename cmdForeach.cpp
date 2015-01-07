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
public:
    GroupBeginForeach(ScriptCommand&& sc)
        :GroupPointBase(std::move(sc), "foreach - foreach group begin")
    {
        opt_.add_options()
            ("loop", bp::value<std::string>()->required(), "loop var name")
            ("var",  bp::value<std::string>(), "foreach from context var")
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

            auto itr=vm.find("var");
            if(itr!=vm.end())
                varPtr_=contextGet()->get(itr->second.as<std::string>());

            itr=vm.find("loop");
            assert(itr!=vm.end());
            loop_=itr->second.as<std::string>();

            varstr_=boost::get<VarString>(varPtr_.get());
            varlist_=boost::get<VarList>(varPtr_.get());
        }

        if(varstr_!=nullptr && !varstr_->empty())
        {
            done_=true;
            contextGet()->set(loop_, VarSPtr(new Variable(*varstr_)));
            return MainReturn::eGood;
        }

        if(varlist_!=nullptr && !varlist_->empty())
        {
            contextGet()->set(loop_, VarSPtr(new Variable(varlist_->at(index_++))));
            done_=(varlist_->size()<=index_);
            return MainReturn::eGood;
        }

        return MainReturn::eGroupDone;
    }

private:
    bool done_=false;
    bool first_=true;
    std::string loop_;

    size_t index_=0;
    VarSPtr varPtr_;

    VarList* varlist_=nullptr;
    VarString* varstr_=nullptr;
};

class CmdForeach:public CommandGroup<CmdForeach>
{
    typedef CommandGroup<CmdForeach> BaseThis;
public:
    CmdForeach(ScriptCommand&& b, Script&& s, ScriptCommand&& e)
        : BaseThis(std::move(s))
        , begin_(std::move(b))
        , end_(std::move(e), "endforeach - foreach group end")
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

