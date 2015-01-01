﻿//  Copyright [2014] <lgb (LiuGuangBao)>
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

#include"encode.hpp"
#include"option.hpp"

namespace ezsh
{

//假设命令行参数都是utf8编码
int myMain(int argc, const char* argv[])
{
    std::string cmdName(argc<=1 ? "help" : argv[1]);
    auto trait=CmdDict::find(cmdName);
    if(trait==nullptr)
    {
        std::cerr
            << "\n\nunkown command: "
            << argv[1] << "\n\n" << std::endl;
        argc=2;
        trait=CmdDict::find("help");
    }
    assert(trait);

    auto& cs=ContextStack::instance();
    const auto& cmd=trait->create();
    try
    {
        cmd->parse(argc-1, argv+1);
    } catch (const boost::program_options::error& ec) {
        std::cerr << ec.what() << std::endl;
        return static_cast<int>(MainReturn::eParamInvalid);
    }

    auto ret=cmd->init(cs.top());
    if(ret==MainReturn::eGood)
        ret=cmd->doit();

    TaskPool::stop();
    return static_cast<int>(ret);
}

}

#ifdef _MSC_VER

//Windows系统使用wchar_t，并把其转换为utf8
int wmain(int argc, const wchar_t* argv[])
{
    std::vector<std::string> args;
    for(int i=0; i<argc; ++i)
		args.emplace_back(ezsh::WCharConverter::to(argv[i], std::wcslen(argv[i])));

    std::vector<const char*> argData;
    for(const auto& arg: args)
        argData.push_back(arg.data());

    return ezsh::myMain(argData.size(), argData.data());
}

#else

//非Windows系统假设默认为utf8
int main(int argc, const char* argv[])
{
    return ezsh::myMain(argc, argv);
}

#endif

