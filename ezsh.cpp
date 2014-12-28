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

#include"option.hpp"

namespace ezsh
{

void help()
{
    std::cerr <<
        "ezsh command [options]\n\n";

    for(const auto& u: CmdDict::dictGet())
    {
        const auto& create=std::get<1>(u);
        const auto& cmd=create();
        cmd->help();
    }
}
}

int main(int argc, const char* const* argv)
{
    if(argc<=1)
    {
        ezsh::help();
        return static_cast<int>(ezsh::MainReturn::eParamInvalid);
    }

    const auto create=ezsh::CmdDict::find(argv[1]);
    if(create==nullptr)
    {
        ezsh::help();
        return static_cast<int>(ezsh::MainReturn::eParamInvalid);
    }

    auto& cs=ezsh::ContextStack::instance();
    const auto& cmd=create();
    try
    {
        cmd->parse(cs.top(), argc-1, argv+1);
    } catch (const boost::program_options::error& ec) {
        std::cerr << ec.what() << std::endl;
        ezsh::help();
        return static_cast<int>(ezsh::MainReturn::eParamInvalid);
    }

    const auto ret=cmd->doit();

    auto& pool=ezsh::TaskPool::instance();
    pool.stop();

    return static_cast<int>(ret);
}

