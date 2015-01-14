//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdList.cpp
//
//   Description:  列出指定文件
//
//       Version:  1.0
//       Created:  2015年01月03日 14时25分48秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<ctime>

#include"option.hpp"
#include"fileset.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class CmdList:public CmdBaseT<CmdList, FileSetCmdBase<>>
{
    typedef CmdBaseT<CmdList, FileSetCmdBase> BaseThis;
public:
    CmdList()
        :BaseThis("list - list file or dir")
    {
        opt_.add_options()
            ("noError", "do not report error")
        ;
    }

    static const char* nameGet()
    {
        return "list";
    }

    MainReturn doit()
    {
        auto& files=fileGet();
        const auto& vm=mapGet();

        files.init(vm);
        const bool noError=vm.count("noError") ? true : false;
        for(const auto& file: files.setGet())
        {
            if(!file.isExist() && !noError)
            {
                stdErr() << file.total << ": notExist" << std::endl;
                continue;
            }

            filePrint(file);
        }

        files.scan([this](const FileUnit& u)
            {
                filePrint(u);
            }
        );

        return MainReturn::eGood;
    }

private:
    void filePrint(const FileUnit& u)
    {
        std::tm tm;
#ifndef _MSC_VER
        ::localtime_r(&u.ctime, &tm);
#else
        ::localtime_s(&tm, &u.ctime);
#endif
        stdOut() << tm << " " << u.total << std::endl;
    }

};

namespace
{
static CmdRegisterT<CmdList> gsRegister;
}


}

