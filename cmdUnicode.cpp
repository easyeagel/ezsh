//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdUnicode.cpp
//
//   Description:  Unicode 实现工具
//
//       Version:  1.0
//       Created:  2014年12月31日 14时40分42秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<functional>
#include<core/encode.hpp>
#include<core/utf8/utf8.h>
#include<boost/filesystem/fstream.hpp>

#include"option.hpp"
#include"optionFileSet.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class CmdUnicode:public CmdBaseT<CmdUnicode, FileSetCmdBase<>>
{
    typedef CmdBaseT<CmdUnicode, FileSetCmdBase> BaseThis;
public:
    CmdUnicode()
        :BaseThis("unicode - unicode file or dir")
    {
        static const char* opts[]={"bomAdd", "bomRemove", "valid"};
        opt_.add_options()
            (opts[0], "add BOM in file header")
            (opts[1], "remove BOM in file header")
            (opts[2], "check files utf8 encode")
        ;

        modeOnly_.add(std::begin(opts), std::end(opts));
        modeOnly_.doit(*this);
    }

    static const char* nameGet()
    {
        return "unicode";
    }

    void doit()
    {
        const auto& vm=mapGet();
        auto& files=fileGet();

        files.init(vm);

        void (CmdUnicode::* fileOne)(const FileUnit& )=nullptr;
        if(modeOnly_.oneGet()=="bomAdd")
            fileOne=&CmdUnicode::bomAdd;
        else if (modeOnly_.oneGet()=="bomRemove")
            fileOne=&CmdUnicode::bomRemove;
        else if (modeOnly_.oneGet()=="valid")
            fileOne=&CmdUnicode::valid;
        assert(fileOne);

        files.loop([this, fileOne](FileUnit&& fu)
            {
                (this->*fileOne)(fu);
                return errorBreak()==false;
            }
        );
    }

private:
    void valid(const FileUnit& file)
    {
        bf::ifstream strm;
        const auto result=bomUtf8Check(file, strm);
        if(strm.is_open()==false)
            return;

        stdOut()
            << core::WCharConverter::to(file.total.string())
            << (result.second ? ": valid" : ": invalid") << std::endl;
    }

    void bomAdd(const FileUnit& file)
    {
        bf::ifstream strm;
        const auto result=bomUtf8Check(file, strm);
        if(strm.is_open()==false)
            return;

        if(result.second==false)
        {
            stdErr() << "utf8-invalid: " << file.total << std::endl;
            return;
        }

        if(result.first==true)
        {
            stdErr() << "utf8-BOM: " << file.total << std::endl;
            return;
        }

        strm.seekg(0);

        auto newFile=file.total;
        newFile += pathCreate(".new");
        bf::ofstream out(newFile);
        out.write(reinterpret_cast<const char*>(utf8::bom), sizeof(utf8::bom));
        std::copy(std::istreambuf_iterator<char>(strm), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(out));
        bf::rename(newFile, file.total);
    }

    void bomRemove(const FileUnit& file)
    {
        bf::ifstream strm;
        const auto result=bomUtf8Check(file, strm);
        if(strm.is_open()==false)
            return;

        if(result.second==false)
        {
            stdErr() << "utf8-invalid: " << file.total << std::endl;
            return;
        }

        if(result.first==false)
        {
            stdErr() << "utf8-no-BOM: " << file.total << std::endl;
            return;
        }

        strm.seekg(3);

        auto newFile=file.total;
        newFile += pathCreate(".new");
        bf::ofstream out(newFile);
        std::copy(std::istreambuf_iterator<char>(strm), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(out));
        bf::rename(newFile, file.total);
    }

private:
    std::pair<bool, bool> bomUtf8Check(const FileUnit& file, bf::ifstream& strm)
    {
        if(!file.isExist())
        {
            stdErr() << file.total << ": notExist" << std::endl;
            return std::make_pair(false, false);
        }

        ErrorCode ec;
        if(file.isDir())
        {
            stdErr() << file.total << ": isDir" << std::endl;
            return std::make_pair(false, false);
        }

        strm.open(file.total);
        if(!strm)
        {
            stdErr() << file.total << ": openFailed" << std::endl;
            return std::make_pair(false, false);
        }

        std::istreambuf_iterator<char> const end;
        std::istreambuf_iterator<char> const strmItr(strm);
        const bool isBOM=utf8::starts_with_bom(strmItr, end);
        if(!isBOM)
            strm.seekg(0);

        auto const itr=std::istreambuf_iterator<char>(strm);
        const auto isValid=utf8::is_valid(itr, end);
        return std::make_pair(isBOM, isValid);
    }

private:
    OptionOneAndOnly modeOnly_;
};

namespace
{
static CmdRegisterT<CmdUnicode> gsRegister;
}


}

