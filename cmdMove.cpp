//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdMove.cpp
//
//   Description:  移动文件命令
//
//       Version:  1.0
//       Created:  2015年02月06日 14时10分33秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"optionOutPut.hpp"
#include"optionFileSet.hpp"

namespace ezsh
{

class CmdMove:public CmdBaseT<CmdMove, FileSetCmdBase<OutPutCmdBase<>>>
{
    typedef CmdBaseT<CmdMove, FileSetCmdBase> BaseThis;
    typedef std::ostreambuf_iterator<char> OutItr;
public:
    CmdMove()
        :BaseThis("move - move file or dir")
    {
        opt_.add_options()
            ("force", "force overwrite if file exist")
        ;
    }

    static const char* nameGet()
    {
        return "move";
    }

    void doit()
    {
        const auto& vm=mapGet();

        force_ = (vm.count("force")>0);

        auto& files=fileGet();
        files.init(vm);
        files.scan();

        auto& output=outGet();
        output.init(vm);

        FileUnit out;
        size_t count=0;
        for(auto& file: files.setGet())
        {
            if(file.done)
                continue;

            ++count;
            output.rewrite(file, out);
            fileOne(file, out);
        }

        if(count>0)
            return;

        errorSet(EzshError::ecMake(EzshError::eParamNotExist));
        errorReport() << ": no file copied, maybe need --fsRecursive" << std::endl;
        return;
    }

private:
    void fileOne(const FileUnit& in, const FileUnit& out)
    {
        if(!in.isExist())
        {
            errorSet(EzshError::ecMake(EzshError::eParamNotExist));
            errorReport() << ": not exist: " << in.total << std::endl;
            return;
        }

        ErrorCode ec;
        bf::rename(in.total, out.total, ec);

        if(ec)
        {
            errorSet(EzshError::ecMake(EzshError::eOperationFailed));
            errorReport() << ": " << in.total  << " -> " << out.total << ": " << ec.message() << std::endl;
            return;
        }

        in.doneSet();
    }

private:
    bool force_=false;
};

namespace
{
static CmdRegisterT<CmdMove> gsRegister;
}


}

