//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdCopy.cpp
//
//   Description:  复制文件或目录
//
//       Version:  1.0
//       Created:  2015年01月18日 09时50分41秒
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

class CmdCopy:public CmdBaseT<CmdCopy, FileSetCmdBase<OutPutCmdBase<>>>
{
    typedef CmdBaseT<CmdCopy, FileSetCmdBase> BaseThis;
    typedef std::ostreambuf_iterator<char> OutItr;
public:
    CmdCopy()
        :BaseThis("copy - copy file or dir")
    {
        opt_.add_options()
            ("force", "force overwrite if file exist")
        ;
    }

    static const char* nameGet()
    {
        return "copy";
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
            if(file.done || (!file.scaned && file.isDir()))
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
        switch(in.status.type())
        {
            case bf::file_type::directory_file:
            {
                if(out.isDir())
                    break;
                bf::copy_directory(in.total, out.total, ec);
                break;
            }
            case bf::file_type::regular_file:
            {
                if(force_)
                    bf::copy_file(in.total, out.total, bf::copy_option::overwrite_if_exists, ec);
                else
                    bf::copy_file(in.total, out.total, ec);
                break;
            }
            default:
            {
                bf::copy(in.total, out.total, ec);
                break;
            }
        }

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
static CmdRegisterT<CmdCopy> gsRegister;
}


}

