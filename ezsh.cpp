//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  ezsh.cpp
//
//   Description:  主入口
//
//       Version:  1.0
//       Created:  2014年12月25日 09时36分06秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<vector>
#include<string>
#include<clocale>
#include<core/encode.hpp>
#include<core/server.hpp>

#include"option.hpp"
#include"parser.hpp"

namespace ezsh
{

//假设命令行参数都是utf8编码
void myMain(int& ret, StrCommandLine& arg)
{
    auto& cs=ContextStack::instance();
    const auto topCtx=cs.top();
    topCtx->start([&ret, &arg]()
        {
            if(arg.size()<=1)
                arg.push_back("help");

            const std::string& cmdName=arg[1];
            auto trait=CmdDict::find(cmdName);
            if(trait==nullptr)
            {
                std::cerr
                    << "\n\nunkown command: "
                    << arg[1] << "\n\n" << std::endl;
                arg.resize(2);
                arg[1]="help";
                trait=CmdDict::find("help");
            }
            assert(trait);

            const auto& cmd=trait->create();
            try
            {
                arg.assign(arg.begin()+1, arg.end());
                cmd->parse(StrCommandLine(arg));
            } catch (const boost::program_options::error& ec) {
                std::cerr << ec.what() << std::endl;
                ret=static_cast<int>(EzshError::eParamInvalid);
                core::IOServer::stop();
                return;
            }

            auto& cs=ContextStack::instance();
            cmd->init(cs.top());
            if(cmd->good())
                cmd->taskDoit();

            core::IOServer::stop();
            ret=cmd->ecGet().value();
        }
    );
}

}

#ifdef _MSC_VER

//Windows系统使用wchar_t，并把其转换为utf8
int wmain(int argc, const wchar_t* argv[])
{
    std::setlocale(LC_CTYPE, "");

    ezsh::Environment::instance();

    std::vector<std::string> arg;
    for(int i=0; i<argc; ++i)
        arg.emplace_back(core::WCharConverter::to(argv[i], std::wcslen(argv[i])));

    int ret=0;
    auto& ms=core::MainServer::instance();
    ms.post([&arg, &ret]()
        {
            ezsh::myMain(ret, arg);
        }
    );

    ms.start();

    return ret;
}

#else

//非Windows系统假设默认为utf8
int main(int argc, char* argv[])
{
    ezsh::test::patternReplaceTest();


    std::setlocale(LC_CTYPE, "");
    ezsh::Environment::instance();

    int ret=0;
    auto& ms=core::MainServer::instance();
    ms.post([argc, argv, &ret]()
        {
            ezsh::StrCommandLine arg;
            for(int i=0; i<argc; ++i)
                arg.push_back(argv[i]);
            ezsh::myMain(ret, arg);
        }
    );

    ms.start();

    return ret;
}

#endif



