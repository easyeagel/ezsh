//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdRemove.cpp
//
//   Description:  删除命令
//
//       Version:  1.0
//       Created:  2014年12月25日 10时44分35秒
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

class CmdRemove:public CmdBaseT<CmdRemove, FileSetCmdBase<>>
{
    typedef CmdBaseT<CmdRemove, FileSetCmdBase> BaseThis;
public:
    CmdRemove()
        :BaseThis("remove - remove file or dir")
    {}

    static const char* nameGet()
    {
        return "remove";
    }

    void doit()
    {
        auto& files=fileGet();
        files.init(mapGet());
        files.scan();
        for(auto& file: files.setGet())
        {
            if(file.done)
                continue;

            fileRemove(file);
            if(errorBreak())
                return;
        }
    }

private:
    void fileRemove(const FileUnit& file)
    {
        if(!file.isExist())
        {
            errorSet(EzshError::ecMake(EzshError::eParamNotExist));
            errorReport() << ": not exist: " << file.total << std::endl;
            return;
        }

        ErrorCode ec;
        if(!file.isDir())
        {
            bf::remove(file.total, ec);
        } else if(fileGet().isRecursive()) {
            bf::remove_all(file.total, ec);
            fileGet().subtreeDone(file);
        } else {
            bf::remove(file.total, ec);
        }

        if(ec.good())
        {
            file.doneSet();
            return;
        }

        errorSet(EzshError::ecMake(EzshError::eParamInvalid));
        errorReport() << ": " << ec.message() << ": "<< file.total << std::endl;
    }
};

namespace
{
static CmdRegisterT<CmdRemove> gsRegister;
}


}

