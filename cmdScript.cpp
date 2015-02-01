//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdScript.cpp
//
//   Description:  脚本命令实现
//
//       Version:  1.0
//       Created:  2014年12月28日 13时59分48秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<cctype>
#include<boost/filesystem/fstream.hpp>

#include"parser.hpp"
#include"script.hpp"
#include"option.hpp"
#include"filesystem.hpp"

namespace ezsh
{

class CmdScript:public CmdBaseT<CmdScript>
{
    typedef CmdBaseT<CmdScript> BaseThis;
public:
    CmdScript()
        :BaseThis("script - run file as ezsh script")
    {
        opt_.add_options()
            ("dry", "dry run for debug")
            ("file,f", bp::value<std::vector<std::string>>()->required(), "files to run")
            ("set",    bp::value<std::vector<std::string>>()->multitoken(), "set var with value")
            ("list",   bp::value<std::vector<std::string>>()->multitoken(), "set context list var")
        ;
        optPos_.add("file", -1);
    }

    static const char* nameGet()
    {
        return "script";
    }

    void doit()
    {
        contextVisit();

        const auto& vm=mapGet();
        if(vm.count("dry")>0)
            CmdBase::setDry();

        const auto& files=vm["file"].as<std::vector<std::string>>();

        bool isExist=true;
        for(const auto& file: files)
        {
            const bool exist=bf::exists(file);
            if(exist==false)
            {
                stdErr() << file << ": not exist" << std::endl;
                isExist=false;
                continue;
            }
        }

        if(isExist==false)
            return ecSet(EzshError::ecMake(EzshError::eParamInvalid));

        for(const auto& file: files)
        {
            scripts_.emplace_back();
            auto& s=scripts_.back();

            ScriptLoadContext ctx;
            ctx.push(file);
            if(!ctx.good())
                continue;

            const auto ok=Script::load(ctx, s);
            if(ok==false)
            {
                stdErr() << ctx.messageGet() << std::endl;
                return ecSet(EzshError::ecMake(EzshError::eParamInvalid));
            }
        }

        for(auto& s: scripts_)
        {
            s.execute(ecGet(), contextGet());
            if(bad())
                return;
        }
    }

private:
    void contextVisit()
    {
        const auto& vm=mapGet();
        ContextVisitor visitor(*contextGet());

        auto itr=vm.find("set");
        if(itr!=vm.end())
            visitor.setDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("list");
        if(itr!=vm.end())
            visitor.setListDo(itr->second.as<std::vector<std::string>>());
    }

private:
    std::list<Script> scripts_;
};

namespace
{
static CmdRegisterT<CmdScript> gsRegister;
}


}

