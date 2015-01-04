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

#include"option.hpp"
#include"fileset.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class CmdList:public CmdBaseT<CmdList>
{
    typedef CmdBaseT<CmdList> BaseThis;
public:
    CmdList()
        :BaseThis("list - list file or dir")
    {
        components_.push_back(FileSet::componentGet());
    }

    static const char* nameGet()
    {
        return "list";
    }

    MainReturn doit() override
    {
        const auto& vm=mapGet();

        files_.init(vm);

        for(const auto& file: files_.setGet())
        {
            if(!file.isExist())
            {
                stdErr() << file.total << ": notExist" << std::endl;
                continue;
            }

            stdOut() << file.total << std::endl;
        }

        files_.scan([this](const FileSet::FileUnit& u)
            {
                stdOut() << u.total << std::endl;
            }
        );

        return MainReturn::eGood;
    }

private:
    FileSet files_;
};

namespace
{
static CmdRegisterT<CmdList> gsRegister;
}


}

