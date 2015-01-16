﻿//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  option.cpp
//
//   Description:  选项实现
//
//       Version:  1.0
//       Created:  2014年12月26日 14时03分30秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<cstdlib>
#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string/case_conv.hpp>

#include"option.hpp"
#include"filesystem.hpp"

namespace ezsh
{
bool CmdBase::dry_=false;

CmdBase::CmdBase(const char* msg, bool ne)
    :opt_(msg)
{
    if(ne)
        return;

    opt_.add_options()
        ("error", bp::value<int>()->default_value(eErrorReport),
             "error attitude:\n0: ignore, 1: quiet, 2: report, 3: break")
    ;
}

void CmdBase::help(std::ostream& strm)
{
    strm << opt_ << std::endl;
    for(auto& c: components_)
        c->shortHelp(strm);
}

void CmdBase::parse(StrCommandLine&& cl)
{
    bp::options_description optComponents;
    for(auto& c: components_)
        c->options(optComponents, optPos_);

    const auto cmd=cmdlineMake(cl);
    bp::command_line_parser parser(cmd.size(), cmd.data());
    if(optComponents.options().empty())
    {
        parser.options(opt_);
    } else {
        optAll_.add(opt_);
        optAll_.add(optComponents);
        parser.options(optAll_);
    }
    parser.positional(optPos_);
    parser.style(bp::command_line_style::default_style);

    bp::store(parser.run(), vm_);
    bp::notify(vm_);

    errorAtt_=static_cast<ErrorAttitude_t>(vm_["error"].as<int>());
    if(errorAtt_<eErrorIgnore || errorAtt_>eErrorBreak)
        errorAtt_=eErrorReport;

    for(auto& call: afterParseCalls_)
        call(vm_);
}



Environment::Environment()
{
#ifdef WIN32
    const wchar_t* env=::GetEnvironmentStringsW();
	if (env==nullptr)
		return;

    for(;;)
    {
		if (*env == 0)
			break;

        const wchar_t* str=env;
        const wchar_t* strend=str+std::wcslen(str);
        env=strend+1;

        auto const equ=std::find(str, strend, L'=');
        if(equ==strend || equ==str)
            continue;
		std::string key = WCharConverter::to(std::wstring(str, equ));
		boost::algorithm::to_lower(key);
		maps_[key] = WCharConverter::to(std::wstring(equ + 1, strend));
    }

    if(maps_.empty())
        return;

    const std::string paths=maps_["path"];
	if (paths.empty())
		return;


    boost::algorithm::split(paths_, paths,
        [](char c){ return c==';'; },
        boost::algorithm::token_compress_mode_type::token_compress_on
    );
#else
    const char* const* env=::environ;
    for(; *env!=nullptr; ++env)
    {
        const char* str=*env;
        const char* strend=str+std::strlen(str);
        auto const equ=std::find(str, strend, '=');
        if(equ==strend)
            continue;
        maps_[std::string(str, equ)]=std::string(equ+1, strend);
    }

    const std::string paths=maps_["PATH"];
    if(paths.empty())
        return;

    boost::algorithm::split(paths_, paths,
        [](char c){ return c==':'; },
        boost::algorithm::token_compress_mode_type::token_compress_on
    );
#endif  //WIN32

}

Path Environment::pathFile(const Path& file) const
{
    for(const auto& p: paths_)
    {
        const Path path=Path(p)/file;
        boost::system::error_code ec;
        const auto st=bf::status(path, ec);
        if(!ec && st.type()!=bf::file_type::file_not_found)
			return std::move(WCharConverter::to(path.native()));
    }

    return Path();
}

}  // namespace ezsh

//实现命令help，option
namespace ezsh
{

class CmdHelp:public CmdBaseT<CmdHelp>
{
    typedef CmdBaseT<CmdHelp> BaseThis;
public:
    CmdHelp()
        :BaseThis("help - show help message")
    {
        opt_.add_options()
            ("long", "show long help")
            ("cmd",  bp::value<std::vector<std::string>>(), "show only this command")
        ;
        optPos_.add("cmd", -1);
    }

    static const char* nameGet()
    {
        return "help";
    }

    void doit()
    {
        std::cerr <<
            "ezsh command [options]\n\n";

        const auto& vm=mapGet();
        const auto itr=vm.find("cmd");
        if(itr!=vm.end())
        {
            const auto& cmds=itr->second.as<std::vector<std::string>>();
            for(const auto& c: cmds)
            {
                const auto w=CmdDict::find(c);
                if(w==nullptr)
                {
                    std::cerr << "unkonw command: " << c << std::endl;
                    continue;
                }

                const auto& tmp=w->create();
                tmp->help(std::cerr);
                std::cerr << std::endl;
            }

            return;
        }

        for(const auto& u: CmdDict::dictGet())
        {
            const auto& trait=std::get<1>(u);
            const auto& cmd=trait->create();
            cmd->help(std::cerr);
            std::cerr << std::endl;
        }

    }
};

class CmdOption:public CmdBaseT<CmdOption>
{
    typedef CmdBaseT<CmdOption> BaseThis;
public:
    CmdOption()
        :BaseThis("option - show option message")
    {
        opt_.add_options()
            ("option", bp::value<std::vector<std::string>>(), "show only option")
        ;
        optPos_.add("option", -1);
    }

    static const char* nameGet()
    {
        return "option";
    }

    void doit()
    {
        std::cerr << "valid options list: " << std::endl;
        const auto& opts=OptionDict::dictGet();
        for(const auto& opt: opts)
            opt.second->longHelp(std::cerr);
    }
};

namespace
{
static CmdRegisterT<CmdHelp> gsHelp;
static CmdRegisterT<CmdOption> gsOption;
}


}

