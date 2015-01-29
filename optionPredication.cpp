//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  optionPredication.cpp
//
//   Description:  前置条件实现
//
//       Version:  1.0
//       Created:  2015年01月29日 10时01分13秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"optionPredication.hpp"

namespace ezsh
{

void Predication::Component::options(bp::options_description& opt, bp::positional_options_description& )
{
    opt.add_options()
        ("exist",        bp::value<std::vector<std::string>>(), "retrun true, if this exist")
        ("existNot",     bp::value<std::vector<std::string>>(), "retrun true, if this not exist")
        ("fileExist",    bp::value<std::vector<std::string>>(), "retrun true, if this file exist")
        ("fileExistNot", bp::value<std::vector<std::string>>(), "retrun true, if this file not exist")
        ("dirExist",     bp::value<std::vector<std::string>>(), "retrun true, if this dir exist")
        ("dirExistNot",  bp::value<std::vector<std::string>>(), "retrun true, if this dir not exist")
    ;
}

void Predication::Component::longHelp (std::ostream& strm)
{
    bp::options_description opt;
    bp::positional_options_description pos;
    options(opt, pos);
    strm << "predication - do a test before run a command\n";
    strm << opt << std::endl;
}

void Predication::Component::shortHelp(std::ostream& strm)
{
    strm << "  *predication - predication test, \"option predication\" for details" << std::endl;
}

void Predication::config(CmdBase& cmd)
{
    cmd.componentPush(componentGet());
}

void Predication::init(const bp::variables_map& vm)
{
    typedef std::function<bool (const bf::file_status& st)> Cond;
    struct U
    {
        const char* name;
        Cond cond;
    };

    typedef bf::file_type FT;
    const U d[]=
    {
        {"exist",        [](const bf::file_status& st){ return st.type()!=FT::file_not_found; } },
        {"existNot",     [](const bf::file_status& st){ return st.type()==FT::file_not_found; } },
        {"fileExist",    [](const bf::file_status& st){ return st.type()==FT::regular_file; } },
        {"fileExistNot", [](const bf::file_status& st){ return st.type()!=FT::regular_file; } },
        {"dirExist",     [](const bf::file_status& st){ return st.type()==FT::directory_file; } },
        {"dirExistNot",  [](const bf::file_status& st){ return st.type()!=FT::directory_file; } },
    };

    for(const auto& u: d)
    {
        auto itr=vm.find(u.name);
        if(itr!=vm.end())
        {
            const auto& files=itr->second.as<std::vector<std::string>>();
            for(const auto& file: files)
            {
                Path path(file);
                ErrorCode ec;
                const auto st=bf::status(file, ec);
                if(u.cond(st)==false)
                {
                    isPassed_=false;
                    return;
                }
            }
        }
    }

    isPassed_=true;
}

namespace
{
    static ezsh::OptionRegisterT<ezsh::Predication> gsOptionPredication;
}

}  // namespace ezsh

