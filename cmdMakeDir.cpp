//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdMakeDir.cpp
//
//   Description:  实现一个简单的mkdir命令
//
//       Version:  1.0
//       Created:  2015年01月13日 10时08分41秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"option.hpp"
#include"optionFileSet.hpp"

namespace ezsh
{

class CmdMakeDir:public CmdBaseT<CmdMakeDir>
{
    typedef CmdBaseT<CmdMakeDir> BaseThis;
public:
    CmdMakeDir()
        :BaseThis("mkdir - make dir")
    {
        opt_.add_options()
            ("parents,p", "no error if existing, make parent directories as needed")
            ("dir,d", bp::value<std::vector<std::string>>()->required(), "dirs to make")
        ;
        optPos_.add("dir", -1);
    }

    static const char* nameGet()
    {
        return "mkdir";
    }

    void doit()
    {
        const auto& vm=mapGet();
        const bool parents=(vm.count("parents")>0) ? true : false;
        const auto& dirs=vm["dir"].as<std::vector<std::string>>();
        for(const auto& dir: dirs)
        {
            ErrorCode ec;
            if(parents)
            {
                bf::create_directories(Path(dir).path(), ec);
                continue;
            }

            bf::create_directory(Path(dir).path(), ec);
            if(ec)
            {
                stdErr()
                    << nameGet() << ":"
                    << dir << ":" << ec.message()
                    << std::endl;
                ecSet(EzshError::ecMake(EzshError::eParamInvalid));
            }
        }
    }

};

namespace
{
static CmdRegisterT<CmdMakeDir> gsRegister;
}


}

