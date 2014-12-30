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

int main(int argc, const char* const* argv)
{
    std::string cmdName(argc<=1 ? "help" : argv[1]);
    auto trait=ezsh::CmdDict::find(cmdName);
    if(trait==nullptr)
    {
        std::cerr
            << "\n\nunkown command: "
            << argv[1] << "\n\n" << std::endl;
        argc=2;
        trait=ezsh::CmdDict::find("help");
    }
    assert(trait);

    auto& cs=ezsh::ContextStack::instance();
    const auto& cmd=trait->create();
    try
    {
        cmd->parse(argc-1, argv+1);
    } catch (const boost::program_options::error& ec) {
        std::cerr << ec.what() << std::endl;
        return static_cast<int>(ezsh::MainReturn::eParamInvalid);
    }

    auto ret=cmd->init(cs.top());
    if(ret==ezsh::MainReturn::eGood)
        ret=cmd->doit();

    ezsh::TaskPool::stop();
    return static_cast<int>(ret);
}

