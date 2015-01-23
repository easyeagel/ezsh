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
            ("details", "show details info")
        ;
    }

    static const char* nameGet()
    {
        return "list";
    }

    void doit()
    {
        auto& files=fileGet();
        const auto& vm=mapGet();

        details_=(vm.count("details")>0);

        files.init(vm);
        files.loop([this](const FileUnit& u)
            {
                filePrint(u);
                return errorBreak()==false;
            }
        );
    }

private:
    void filePrint(const FileUnit& u)
    {
        if(!u.isExist())
        {
            errorSet(EzshError::ecMake(EzshError::eParamNotExist));
            errorReport() << ": not exist" << u.total << std::endl;
            return;
        }

        if(details_==false)
        {
            stdOut() << u.total << std::endl;
            return;
        }

        std::tm tm;
#ifndef _MSC_VER
        ::localtime_r(&u.ctime, &tm);
#else
        ::localtime_s(&tm, &u.ctime);
#endif

        stdOut()
            << tm << ' '
            << std::right << std::setfill(L' ') << std::setw(12) << u.size << ' '
            << u.total << std::endl;
    }

private:
    //时间，尺寸，
    bool details_=false;
};

namespace
{
static CmdRegisterT<CmdList> gsRegister;
}


}

