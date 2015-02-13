//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdText.cpp
//
//   Description:  基本文件处理命令
//
//       Version:  1.0
//       Created:  2015年02月10日 14时34分07秒
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

#include<functional>
#include<boost/algorithm/string/replace.hpp>

namespace ezsh
{

class CmdText:public CmdBaseT<CmdText, FileSetCmdBase<OutPutCmdBase<>>>
{
    typedef CmdBaseT<CmdText, FileSetCmdBase> BaseThis;
    typedef std::ostreambuf_iterator<char> OutItr;
    typedef std::function<void (ErrorCode& , const FileUnit& , const FileUnit& )> Funtor;
public:
    CmdText()
        :BaseThis("text - text process (replace,search..) for file")
    {
        opt_.add_options()
            ("search",      bp::value<std::string>(), "search string in files")
            ("regexSearch", bp::value<std::string>(), "regex search in files")
            ("replace",     bp::value<std::string>(), "replace string with string in files")
            ("regexReplace",bp::value<std::string>(), "replace regex with string in files")
        ;

        oneAndOnly_.add({"search", "regexSearch", "replace", "regexReplace"});
        oneAndOnly_.doit(*this);
    }

    static const char* nameGet()
    {
        return "text";
    }

    void doit()
    {
        const auto& vm=mapGet();

        auto& files=fileGet();
        files.init(vm);
        files.scan();

        auto& output=outGet();
        output.init(vm);

        funtorInit();

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
        errorReport() << ": no file processed, maybe need --fsRecursive" << std::endl;
        return;
    }

private:
    void fileOne(const FileUnit& in, const FileUnit& out)
    {
        ErrorCode ec;
        switch(in.status.type())
        {
            case bf::file_type::regular_file:
            {
                funtor_(ec, in, out);
                if(ec.bad())
                {
                    errorSet(ec);
                    errorReport() << ": " << in.total  << " -> " << out.total << ": " << ec.message() << std::endl;
                    return;
                }

                in.doneSet();
                return;
            }
            default:
            {
                errorSet(EzshError::ecMake(EzshError::eParamInvalid));
                errorReport() << ": ivalid: " << in.total << std::endl;
                return;
            }
        }
    }

    void funtorInit()
    {
        const std::string& one=oneAndOnly_.oneGet();   
        struct U
        {
            std::string name;
            Funtor fun;
        };

        static U dict[]=
        {
#define MD(CppMethod)\
    { #CppMethod, [this](ErrorCode& ec, const FileUnit& in, const FileUnit& out){ return CppMethod(ec, in, out); } },

            MD(search)
            MD(replace)

#undef MD
        };

        const auto itr=std::find_if(std::begin(dict), std::end(dict), [&one](const U& u){ return u.name==one; });
        assert(itr!=std::end(dict));
        funtor_=itr->fun;
    }

    void search(ErrorCode& ec, const FileUnit& in, const FileUnit&) const
    {
        bf::ifstream strm(in.total.path());
        if(!strm)
        {
            ec=EzshError::ecMake(EzshError::eParamInvalid);
            return;
        }

        const auto& vm=mapGet();
        std::string substr=vm.find("search")->second.as<std::string>();
        std::string line;

        size_t count=0;
        while(std::getline(strm, line))
        {
            ++count;
            if(line.find(substr)!=std::string::npos)
                stdOut() << in.total << ":" << count << ": " << line << std::endl;
        }
    }

    void replace(ErrorCode& ec, const FileUnit& in, const FileUnit& out) const
    {
        bf::ifstream strm(in.total.path());
        if(!strm)
        {
            ec=EzshError::ecMake(EzshError::eParamInvalid);
            return;
        }

        Path outPath;
        bf::ofstream outStrm;
        if(in.total==out.total)
        {
            outPath=out.total;
            outPath += ".new";
            outStrm.open(outPath.path());
        } else {
            outStrm.open(out.total);
        }

        const auto& vm=mapGet();
        std::string substr=vm.find("replace")->second.as<std::string>();
        std::string from, to;
        auto const pos=substr.find('=');
        if(pos==std::string::npos)
        {
            from=substr;
        } else {
            from=substr.substr(0, pos);
            to=substr.substr(pos+1);
        }

        if(from.empty())
        {
            ec=EzshError::ecMake(EzshError::eParamInvalid);
            return;
        }

        std::string line;
        while(std::getline(strm, line))
        {
            boost::algorithm::replace_all(line, from, to);
            outStrm << line << '\n';
        }

        if(in.total==out.total)
            bf::rename(outPath, out.total, ec);
    }

private:
    Funtor funtor_;
    OptionOneAndOnly oneAndOnly_;
};

namespace
{
static CmdRegisterT<CmdText> gsRegister;
}


}

