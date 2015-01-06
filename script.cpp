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


bool Script::load(const std::string& file, Script& spt)
{
    bf::ifstream strm(file);
    ScriptCommand unit;
    std::string line;
    for(;;)
    {
        std::getline(strm, line);
        if(!strm)
        {
            if(unit.empty())
                break;
            goto GotoUnitEnd;
        }

        boost::trim(line);
        if(line.empty() || line[0]=='#')
        {
            continue;
        } else if(line[0]=='-') {
            unit.lineAppend(line);
            continue;
        } else if(unit.empty()) {
            unit.lineAppend(line);
            continue;
        } else {
        GotoUnitEnd:
            if(unit.init()==false)
                return false;

            spt.script_.emplace_back(ScriptCommandUPtr(new ScriptCommand(std::move(unit))));

            if(line.empty()) //文件已经结束
                break;

            unit.lineAppend(line);
        }
    }

    return true;
}

MainReturn Script::execute(const ContextSPtr& context)
{
    CommandLine args;
    StrCommandLine strArgs;
    for(auto& ptr: script_)
    {
        auto& u=*boost::get<ScriptCommandUPtr>(ptr);
        args.clear();
        cmdlineReplace(context, u.cmdlineGet(), strArgs);
        std::for_each(strArgs.begin(), strArgs.end(),
            [&args](const std::string& s)
            {
                args.push_back(const_cast<char*>(s.data()));
            }
        );

        const auto& cmd=u.traitGet()->create();
        try
        {
            cmd->parse(args.size(), const_cast<char**>(args.data()));
        } catch (const boost::program_options::error& ec) {
            std::cerr << ec.what() << std::endl;
            return MainReturn::eParamInvalid;
        }

        auto ret=cmd->init(context);
        if(ret==ezsh::MainReturn::eGood)
            ret=cmd->doit();

        if(ret!=ezsh::MainReturn::eGood)
            return ret;
    }

    return MainReturn::eGood;
}

void Script::cmdlineReplace(const ContextSPtr& context, const StrCommandLine& cmd, StrCommandLine& dest)
{
    const auto sz=cmd.size();
    if(dest.size()!=sz)
        dest.resize(sz);

    for(size_t i=0; i<sz; ++i)
    {
        const auto& s=cmd[i];
        auto& d=dest[i];
        d.clear();
        context->replace(s, d);
    }
}


}

