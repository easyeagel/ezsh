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

#include<utf8.h>
#include<functional>
#include<boost/filesystem/fstream.hpp>

#include"encode.hpp"
#include"option.hpp"
#include"fileset.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class CmdUnicode:public CmdBaseT<CmdUnicode>
{
    typedef CmdBaseT<CmdUnicode> BaseThis;
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

        components_.push_back(FileSet::componentGet());
        modeOnly_.add(std::begin(opts), std::end(opts));
        modeOnly_.doit(*this);
    }

    static const char* nameGet()
    {
        return "unicode";
    }

    MainReturn doit() override
    {
        const auto& vm=mapGet();

        files_.init(vm);

        void (CmdUnicode::* fileOne)(const FileSet::FileUnit& )=nullptr;
        if(modeOnly_.oneGet()=="bomAdd")
            fileOne=&CmdUnicode::bomAdd;
        else if (modeOnly_.oneGet()=="bomRemove")
            fileOne=&CmdUnicode::bomRemove;
        else if (modeOnly_.oneGet()=="valid")
            fileOne=&CmdUnicode::valid;
        assert(fileOne);

        for(const auto& file: files_.setGet())
            (this->*fileOne)(file);

        files_.scan([this, fileOne](FileSet::FileUnit&& fu)
            {
                (this->*fileOne)(fu);
            }
        );

        return MainReturn::eGood;
    }

private:
    void valid(const FileSet::FileUnit& file)
    {
        bf::ifstream strm;
        const auto result=bomUtf8Check(file, strm);
        if(!strm)
            return;

        context_->stdCOut()
            << WCharConverter::to(file.total.string())
            << (result.second ? ": valid" : ": invalid") << std::endl;
    }

    void bomAdd(const FileSet::FileUnit& file)
    {
        bf::ifstream strm;
        const auto result=bomUtf8Check(file, strm);
        if(!strm)
            return;

        if(result.second==false)
        {
            contextGet()->stdCErr() << "utf8-invalid: " << file.total << std::endl;
            return;
        }

        if(result.first==true)
        {
            contextGet()->stdCErr() << "utf8-BOM: " << file.total << std::endl;
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

    void bomRemove(const FileSet::FileUnit& file)
    {
        bf::ifstream strm;
        const auto result=bomUtf8Check(file, strm);
        if(!strm)
            return;

        if(result.second==false)
        {
            contextGet()->stdCErr() << "utf8-invalid: " << file.total << std::endl;
            return;
        }

        if(result.first==false)
        {
            contextGet()->stdCErr() << "utf8-no-BOM: " << file.total << std::endl;
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
    std::pair<bool, bool> bomUtf8Check(const FileSet::FileUnit& file, bf::ifstream& strm)
    {
        if(!file.isExist())
        {
            context_->stdCErr() << WCharConverter::to(file.total.string()) << ": notExist" << std::endl;
            return std::make_pair(false, false);
        }

        boost::system::error_code ec;
        if(file.isDir())
            return std::make_pair(false, false);

        strm.open(file.total);
        if(!strm)
            return std::make_pair(false, false);

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
    FileSet files_;
    OptionOneAndOnly modeOnly_;
};

namespace
{
static CmdRegisterT<CmdUnicode> gsRegister;
}


}

