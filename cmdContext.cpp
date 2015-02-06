//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdContext.cpp
//
//   Description:  context命令
//
//       Version:  1.0
//       Created:  2015年01月06日 11时37分57秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<boost/algorithm/string/trim.hpp>

#include"parser.hpp"
#include"option.hpp"
#include"context.hpp"

namespace ezsh
{

class CmdContext: public CmdBaseT<CmdContext>
{
    typedef CmdBaseT<CmdContext> BaseThis;
    typedef std::vector<char*> CmdLine;
public:
    CmdContext()
        :BaseThis("context - contrl current context")
    {
        opt_.add_options()
            ("set",    bp::value<std::vector<std::string>>()->multitoken(), "set context var")
            ("unset",  bp::value<std::vector<std::string>>()->multitoken(), "unset this params")
            ("setif",  bp::value<std::vector<std::string>>()->multitoken(), "set context var, if the var not exist")
            ("list",   bp::value<std::vector<std::string>>()->multitoken(), "set context list var")
            ("listif", bp::value<std::vector<std::string>>()->multitoken(), "set context list var, if the var not exist")
            ("echo",   bp::value<std::vector<std::string>>()->multitoken(), "echo this params")
        ;
    }

    static const char* nameGet()
    {
        return "context";
    }

    void doit()
    {
        const auto& vm=mapGet();

        ContextVisitor visitor(*contextGet());
        auto itr=vm.find("set");
        if(itr!=vm.end())
            visitor.setDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("setif");
        if(itr!=vm.end())
            visitor.setIfDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("list");
        if(itr!=vm.end())
            visitor.setListDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("listif");
        if(itr!=vm.end())
            visitor.setIfListDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("echo");
        if(itr!=vm.end())
            visitor.echoDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("unset");
        if(itr!=vm.end())
            visitor.unsetDo(itr->second.as<std::vector<std::string>>());
    }

    void doDry()
    {
        BaseThis::doDry();

        ContextVisitor visitor(*contextGet());

        const auto& vm=mapGet();
        auto itr=vm.find("set");
        if(itr!=vm.end())
            visitor.setDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("setif");
        if(itr!=vm.end())
            visitor.setIfDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("list");
        if(itr!=vm.end())
            visitor.setListDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("listif");
        if(itr!=vm.end())
            visitor.setIfListDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("unset");
        if(itr!=vm.end())
            visitor.unsetDo(itr->second.as<std::vector<std::string>>());
    }

private:
};

namespace
{
    static CmdRegisterT<CmdContext> gsCmdRegister;
}

}  // namespace ezsh

