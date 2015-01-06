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
            ("set",  bp::value<std::vector<std::string>>(), "set context var")
            ("echo", bp::value<std::vector<std::string>>(), "echo this params")
        ;
    }

    static const char* nameGet()
    {
        return "context";
    }

    MainReturn doit()
    {
        const auto& vm=mapGet();

        auto itr=vm.find("set");
        if(itr!=vm.end())
            setDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("echo");
        if(itr!=vm.end())
            echoDo(itr->second.as<std::vector<std::string>>());

        return MainReturn::eGood;
    }

private:
    void setDo(const std::vector<std::string>& sets)
    {
        for(const auto& s: sets)
        {
            std::string key;
            std::string val;
            auto const pos=s.find('=');
            if(pos==std::string::npos)
            {
                key=s;
            } else {
                key=s.substr(0, pos);
                val=s.substr(pos+1);
            }

            boost::algorithm::trim(key);
            boost::algorithm::trim(val);

            contextGet()->set(key, VarSPtr(new Variable(val)));
        }
    }

    void echoDo(const std::vector<std::string>& echos)
    {
        for(const auto& s: echos)
            stdOut() << s;
        stdOut() << std::endl;
    }
};

namespace
{
    static CmdRegisterT<CmdContext> gsCmdRegister;
}

}  // namespace ezsh

