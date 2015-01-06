﻿//  Copyright [2014] <lgb (LiuGuangBao)>
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
#include<boost/filesystem.hpp>
#include<boost/algorithm/string.hpp>
#include<boost/filesystem/fstream.hpp>

#include"option.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class CmdLineSeparator
{
    typedef std::string::iterator Itr;
public:
    template<typename Out>
    bool operator()(Itr& next, Itr end, Out&& out)
    {
        skipBlank(next, end);
        if(next==end)
            return false;

        start_=next;
        while(next!=end)
        {
            switch(*next)
            {
                case '"':
                {
                    escaped_=!escaped_;
                    break;
                }
                case ' ':
                case '\t':
                {
                    if(escaped_==false)
                    {
                        out(start_, next++);
                        return true;
                    }

                    break;
                }
                default:
                    break;
            }

            ++next;
        }

        if(escaped_)
            return false;
        out(start_, end);
        return true;
    }

    void reset()
    {
        start_=Itr();
        escaped_=false;
    }

private:
    void skipBlank(Itr& next, Itr end)
    {
        while(next!=end && std::isspace(*next))
            ++next;
    }

private:
    Itr start_;
    bool escaped_=false;
};

class Script
{
    typedef std::vector<char*> CmdLine;
    typedef std::vector<std::string> StrCmdLine;

public:
    struct ScriptUnit
    {
        std::string line;
        CmdTraitSPtr trait;
        std::vector<std::string> args;
    };

    Script(const std::string& path)
        :file_(path)
    {}

    bool load()
    {
        bf::ifstream strm(file_);
        ScriptUnit unit;
        std::string line;
        for(;;)
        {
            std::getline(strm, line);
            if(!strm)
            {
                if(unit.line.empty())
                    break;
                goto GotoUnitEnd;
            }

            boost::trim(line);
            if(line.empty() || line[0]=='#')
            {
                continue;
            } else if(line[0]=='-') {
                unit.line += " ";
                unit.line += line;
                continue;
            } else if(unit.line.empty()) {
                unit.line += line;
                continue;
            } else {
            GotoUnitEnd:
                if(tokenize(unit)==false)
                    return false;

                unit.trait=ezsh::CmdDict::find(unit.args[0]);
                if(unit.trait==nullptr)
                    return false;

                script_.emplace_back(std::move(unit));

                if(line.empty()) //文件已经结束
                    break;

                unit.line += line;
            }
        }

        return true;
    }

    bool tokenize(ScriptUnit& unit)
    {
        CmdLineSeparator sep;
        auto itr=unit.line.begin();
        const auto end=unit.line.end();
        for(;;)
        {
            bool have=false;
            const auto ret=sep(itr, end,
                [&unit, &have](std::string::iterator s, std::string::iterator e)
                {
                    *e='\0';
                    unit.args.push_back(&*s);
                    have=true;
                }
            );

            if(ret==false)
                return false;

            if(itr==end)
                return true;
        }
    }

    MainReturn execute(const ContextSPtr& context)
    {
        std::vector<char*> args;
        std::vector<std::string> strArgs;
        for(auto& u: script_)
        {
            args.clear();
            cmdlineReplace(context, u.args, strArgs);
            std::for_each(strArgs.begin(), strArgs.end(),
                [&args](const std::string& s)
                {
                    args.push_back(const_cast<char*>(s.data()));
                }
            );

            const auto& cmd=u.trait->create();
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

private:
    void cmdlineReplace(const ContextSPtr& context, const StrCmdLine& cmd, StrCmdLine& dest)
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

private:
    std::string file_;
    std::vector<ScriptUnit> script_;
};

class CmdScript:public CmdBaseT<CmdScript>
{
    typedef CmdBaseT<CmdScript> BaseThis;
public:
    CmdScript()
        :BaseThis("script - run file as ezsh script")
    {
        opt_.add_options()
            ("file,f", bp::value<std::vector<std::string>>()->required(), "files to run")
        ;
        optPos_.add("file", -1);
    }

    static const char* nameGet()
    {
        return "script";
    }

    MainReturn doit() override
    {
        namespace bf=boost::filesystem;

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
            scripts_.emplace_back(file);
            auto& s=scripts_.back();
            const auto ok=s.load();
            if(ok==false)
            {
                stdErr() << file << ": load failed" << std::endl;
                return MainReturn::eParamInvalid;
            }
        }

        for(auto& s: scripts_)
        {
            const auto ret=s.execute(contextGet());
            if(ret!=MainReturn::eGood)
                return ret;
        }

        return MainReturn::eGood;
    }

public:
    std::list<Script> scripts_;
};

namespace
{
static CmdRegisterT<CmdScript> gsRegister;
}


}

