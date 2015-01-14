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
            ("file,f", bp::value<std::vector<std::string>>()->required(), "files to run")
            ("set",    bp::value<std::vector<std::string>>()->multitoken(), "set var with value")
        ;
        optPos_.add("file", -1);
    }

    static const char* nameGet()
    {
        return "script";
    }

    MainReturn doit()
    {
        varSet();

        const auto& vm=mapGet();
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
            return MainReturn::eParamInvalid;

        for(const auto& file: files)
        {
            scripts_.emplace_back();
            auto& s=scripts_.back();
            bf::ifstream strm(Path(file).path());
            if(!strm)
                continue;

            const auto ok=Script::load(strm, s);
            if(ok==false)
            {
                stdErr() << file << ": load failed" << std::endl;
                return MainReturn::eParamInvalid;
            }
        }

        for(auto& s: scripts_)
        {
            const auto ret=s.execute(contextGet());
            if(ret.bad())
                return ret;
        }

        return MainReturn::eGood;
    }

private:
    void varSet()
    {
        const auto& vm=mapGet();
        const auto itr=vm.find("set");
        if(itr==vm.end())
            return;

        const auto& sets=itr->second.as<std::vector<std::string>>();
        for(const auto& s: sets)
        {
            auto const p=simpleSplit(s, '=');
            contextGet()->set(p.first, p.second);
        }
    }

private:
    std::list<Script> scripts_;
};

namespace
{
static CmdRegisterT<CmdScript> gsRegister;
}


}

