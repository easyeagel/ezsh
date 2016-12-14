//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdTextpp.cpp
//
//   Description:  text Preprocessor，文本预处理器
//
//       Version:  1.0
//       Created:  2015年01月07日 16时10分50秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<boost/algorithm/string.hpp>
#include<boost/xpressive/xpressive.hpp>

#include"option.hpp"
#include"parser.hpp"
#include"optionOutPut.hpp"
#include"optionFileSet.hpp"

namespace ezsh
{

class TextppContext
{
public:

private:
    std::map<std::string, std::string> dict_;
};

class CmdTextpp:public CmdBaseT<CmdTextpp, FileSetCmdBase<OutPutCmdBase<>>>
{
    typedef CmdBaseT<CmdTextpp, FileSetCmdBase> BaseThis;
    typedef std::ostreambuf_iterator<char> OutItr;

    class Format
    {
    public:
        Format(const CmdTextpp* c)
            :cmd_(c)
        {}

        template<typename Itr>
        Itr operator()(ErrorCode& ec, Itr out, const ReplacePattern& what)
        {
            cmd_->matchOne(ec, what, out);
            return out;
        }

        private:
            const CmdTextpp* cmd_;
    };

    friend class Format;

public:
    CmdTextpp()
        :BaseThis("textpp - textpp file or dir")
    {
        opt_.add_options()
            ("define,D",  bp::value<std::vector<std::string>>(), "define a macro")
            ("defFile,F", bp::value<std::vector<std::string>>(), "define a macro from many files")
        ;
    }

    static const char* nameGet()
    {
        return "textpp";
    }

    void doit()
    {
        const auto& vm=mapGet();

        optionDoit();
        if(bad() && errorBreak())
            return;

        auto& files=fileGet();
        files.init(vm);

        auto& output=outGet();
        output.init(vm);

        FileUnit outPath;
        for(const auto& file: files.setGet())
        {
            if(!file.isExist() || file.isDir())
            {
                errorReport() << ": not exist or is dir: " << file.total << std::endl;
                continue;
            }

            bf::ifstream strm(file.total.path());
            if(!strm)
            {
                errorSet(EzshError::ecMake(EzshError::eParamInvalid));
                errorReport() << ": open failed: " << file.total << std::endl;
                if(errorBreak())
                    return;
                continue;
            }

            output.rewrite(file, outPath);
            if(outPath.total.empty())
            {
                OutItr out(std::cout);
                fileOne(strm, out);
            } else {
                bf::ofstream ostrm(outPath.total.path());
                if(!ostrm)
                {
                    errorSet(EzshError::ecMake(EzshError::eParamInvalid));
                    errorReport() << ": open failed: " << outPath.total << std::endl;
                    if(errorBreak())
                        return;
                    continue;
                }
                OutItr out(ostrm);
                fileOne(strm, out);
            }
        }
    }

private:
    void optionDoit()
    {
        const auto& vm=mapGet();
        auto itr=vm.find("define");
        if(itr!=vm.end())
        {
            const auto& macros=itr->second.as<std::vector<std::string>>();
            for(const auto& m: macros)
                lineSplit(m);
        }

        itr=vm.find("defFile");
        if(itr!=vm.end())
        {
            std::string line;
            const auto& files=itr->second.as<std::vector<std::string>>();
            for(const auto& f: files)
            {
                bf::ifstream strm(Path(f).path());
                if(!strm)
                {
                    errorSet(EzshError::ecMake(EzshError::eParamInvalid));
                    errorReport() << ": open failed: " << f << std::endl;
                    if(errorBreak())
                        return;
                    continue;
                }

                while(std::getline(strm, line))
                    lineSplit(line);
            }
        }
    }

    void lineSplit(const std::string& line)
    {
        auto const pair=simpleSplit(line);
        const auto& key=pair.first;
        const auto& val=pair.second;
        if(!xpr::regex_match(key.begin(), key.end(), xpr::gsMacroName))
        {
            errorReport() << "warning:notInvalidMacroName: " << key << std::endl;
            return ;
        }

        dict_[key]=val;
    }

    template<typename Itr>
    void fileOne(std::istream& strm, Itr& out) const
    {
        ErrorCode ec;
        std::istreambuf_iterator<char> itr(strm);
        std::istreambuf_iterator<char> const end;
        PatternReplace rp;
        out=rp.replace(ec, out, itr, end, Format(this));
    }

    template<typename Itr>
    void matchOne(ErrorCode& ec, const ReplacePattern& match, Itr& out) const
    {
        for(const auto& opt: match.operatorsGet())
        {
            if(opt.name=="replace")
            {
                macroReplace(opt, out);
                continue;
            }

            if(opt.name=="include")
            {
                includeReplace(opt, out);
                continue;
            }

            if(opt.name=="split")
            {
                splitReplace(opt, out);
                continue;
            }

            ec=EzshError::ecMake(EzshError::eUnkownTextppOperator, opt.name);
        }
    }

    template<typename Itr>
    void macroReplace(const ReplacePattern::Operator& opt, Itr& out) const
    {
        for(const auto& param: opt.params)
        {
            auto const itr=dict_.find(param.value);
            if(itr!=dict_.end())
            {
                auto const& s=itr->second;
                std::copy(s.begin(), s.end(), out);
                continue;
            }
        }
    }

    template<typename Itr>
    void includeReplace(const ReplacePattern::Operator& opt, Itr& out) const
    {
        for(const auto& param: opt.params)
        {
            Path path=simpleReplace(param.value);
            bf::ifstream strm(path.path());
            if(!strm)
                continue;
            fileOne(strm, out);
        }
    }

    template<typename Itr>
    void splitReplace(const ReplacePattern::Operator& opt, Itr& out) const
    {
        for(const auto& param: opt.params)
        {
            auto str=simpleReplace(param.value);
            std::vector<std::string> rlt;
            boost::algorithm::split(rlt, str, boost::algorithm::is_any_of(", "), boost::algorithm::token_compress_on);
            for(auto& s: rlt)
            {
                std::copy(s.begin(), s.end(), out);
                *out++ =  ',';
            }
        }
    }

    std::string simpleReplace(const std::string& str) const
    {
        std::string out;
        xpr::regex_replace(std::back_inserter(out), str.begin(), str.end(), xpr::gsMacroName,
            [this](const xpr::smatch& what) -> std::string
            {
                auto const itr=dict_.find(what.str());
                if(itr==dict_.end())
                    return what.str();
                return itr->second;
            }
        );
        return std::move(out);
    }

private:
    std::map<std::string, std::string> dict_;
};

namespace
{
static CmdRegisterT<CmdTextpp> gsRegister;
}


}

