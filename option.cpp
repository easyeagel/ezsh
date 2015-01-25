//  Copyright [2014] <lgb (LiuGuangBao)>
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
    :opt_(msg), baseOpt_("option for all command")
{
    if(ne)
        return;

    baseOpt_.add_options()
        ("error", bp::value<int>()->default_value(eErrorReport),
             "error attitude:\n0: ignore, 1: quiet, 2: report, 3: break")

        ("stdIn",    bp::value<std::string>(), "stdin will redirect to file or context")
        ("stdOut",   bp::value<std::string>(), "stdout will write to file or context")
        ("stdError", bp::value<std::string>(), "stderr will write to file or context")
        ("stdOutAppend",   bp::value<std::string>(), "stdout will append to file or context")
        ("stdErrorAppend", bp::value<std::string>(), "stderr will append to file or context")
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

    optAll_.add(opt_);
    optAll_.add(baseOpt_);
    optAll_.add(optComponents);
    parser.options(optAll_);

    parser.positional(optPos_);
    parser.style(bp::command_line_style::default_style);

    bp::store(parser.run(), vm_);
    bp::notify(vm_);

    for(auto& call: afterParseCalls_)
        call(vm_);
}

static void stdRedirect(CmdBase& cmd, StdStream& stdStrm, Context& ctx, const std::string& out, bool app=false)
{
    auto const old=stdStrm.get();
    switch(out[0])
    {
        case '+':
        {
            std::shared_ptr<std::wostringstream> ptr(new std::wostringstream);
            stdStrm.set(ptr.get());
            ctx.commandFinishCall([ptr, old, out, &stdStrm](Context& ctx) mutable
                {
                    VarList var;
                    std::string line;
                    std::istringstream strm(core::WCharConverter::to(ptr->str()));
                    while(std::getline(strm, line))
                        var.emplace_back(line);
                    ctx.set(out.substr(1), VarSPtr(new Variable(std::move(var))));
                    stdStrm.set(old);
                }
            );
            break;
        }
        case '*':
        {
            std::shared_ptr<std::wostringstream> ptr(new std::wostringstream);
            stdStrm.set(ptr.get());
            ctx.commandFinishCall([ptr, old, out, &stdStrm](Context& ctx) mutable
                {
                    ctx.set(out.substr(1), VarSPtr(new Variable(core::WCharConverter::to(ptr->str()))));
                    stdStrm.set(old);
                }
            );
            break;
        }
        default:
        {
            const auto mode=std::ios_base::out
                | (app ? std::ios_base::app : std::ios_base::trunc);
            std::shared_ptr<bf::wofstream> ptr(new bf::wofstream(Path(out).path(), mode));
            if(!*ptr)
            {
                cmd.errorSet(EzshError::ecMake(EzshError::eParamInvalid));
                cmd.stdErr() << "open file failed: " << out << std::endl;
                return;
            }

            core::utf8Enable(*ptr);
            stdStrm.set(ptr.get());
            ctx.commandFinishCall([ptr, old, out, &stdStrm](Context& ) mutable
                {
                    ptr->close();
                    stdStrm.set(old);
                }
            );
            break;
        }
    }
}

void CmdBase::init(const ContextSPtr& context)
{
    TaskBase::init(context);

    errorAtt_=static_cast<ErrorAttitude_t>(vm_["error"].as<int>());
    if(errorAtt_<eErrorIgnore || errorAtt_>eErrorBreak)
        errorAtt_=eErrorReport;

    if(errorAtt_<=eErrorQuiet)
        stdErr().quietSet();

    auto itr=vm_.find("stdOut");
    if(itr!=vm_.end())
    {
        const auto& out=itr->second.as<std::string>();
        stdRedirect(*this, context->stdOut(), *context, out);
    }

    itr=vm_.find("stdError");
    if(itr!=vm_.end())
    {
        const auto& out=itr->second.as<std::string>();
        stdRedirect(*this, context->stdErr(), *context, out);
    }

    itr=vm_.find("stdOutAppend");
    if(itr!=vm_.end())
    {
        const auto& out=itr->second.as<std::string>();
        stdRedirect(*this, context->stdOut(), *context, out, true);
    }

    itr=vm_.find("stdErrorAppend");
    if(itr!=vm_.end())
    {
        const auto& out=itr->second.as<std::string>();
        stdRedirect(*this, context->stdErr(), *context, out, true);
    }

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
		std::string key = core::WCharConverter::to(std::wstring(str, equ));
		boost::algorithm::to_lower(key);
		maps_[key] = core::WCharConverter::to(std::wstring(equ + 1, strend));
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
			return std::move(core::WCharConverter::to(path.native()));
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

        std::cerr << baseOpt_ << std::endl;

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

