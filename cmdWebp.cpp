//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdWebp.cpp
//
//   Description:  Webp 两个命令
//
//       Version:  1.0
//       Created:  2014年12月26日 08时46分42秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

extern "C"
{
    int cwebp_main(int argc, const char* argv[]);
    int dwebp_main(int argc, const char* argv[]);
}

#include<webp/decode.h>
#include<webp/encode.h>
#include<boost/filesystem.hpp>
#include<boost/algorithm/string/predicate.hpp>

#include"option.hpp"


namespace ezsh
{

namespace bf=boost::filesystem;
class CmdCWebp:public CmdBaseT<CmdCWebp>
{
    typedef CmdBaseT<CmdCWebp> BaseThis;

    struct Unit
    {
        Unit(const Path& i, const Path& b, bool s)
            :in(i), base(b), scaned(s)
        {}

        Path in;
        Path base;
        bool scaned;
    };

public:
    CmdCWebp()
        :BaseThis("cwebp - webp encode")
    {
        opt_.add_options()
            ("concurrency,c", "concurrency encode with threads")

            ("input,i",  bp::value<std::vector<std::string>>()->required(), "file or dir to encode")
            ("output,o", bp::value<std::string>()->required(), "file or dir for output")

            ("quality,q",      bp::value<unsigned>()->default_value(50), "image quality")
            ("alphaQuality,a", bp::value<unsigned>()->default_value(50), "image alpha channel quality")
        ;
        optPos_.add("input", -1);
    }

    static const char* nameGet()
    {
        return "cwebp";
    }

    void doit()
    {
        const auto& vm=mapGet();

        quality_=vm["quality"].as<unsigned>();
        alphaQuality_=vm["alphaQuality"].as<unsigned>();
        concurrency_=vm.count("concurrency") ? true : false;

        inputCollect();
        if(units_.empty())
        {
            stdErr() << "no input valid" << std::endl;
            return ecSet(EzshError::ecMake(EzshError::eParamInvalid));
        }

        const Path out=vm["output"].as<std::string>();
        const auto outIsExist=bf::exists(out);
        const auto outIsDir=bf::is_directory(out);

        //单个文件，处理情况特殊
        if(units_.size()==1 && !outIsExist)
        {
            callCWebpMain(units_[0].in, Path(out));
            return;
        }

        if(outIsExist && !outIsDir)
        {
            stdErr() << out << ": exist but not directory" << std::endl;
            return ecSet(EzshError::ecMake(EzshError::eParamInvalid));
        }

        if(!outIsExist)
            bf::create_directories(out);

        for(const auto& u: units_)
        {
            if(u.scaned)
                callCWebpMain(u.base/u.in, out/u.in);
            else
                callCWebpMain(u.in, out/u.in.filename());
        }
    }

private:
    void inputCollect()
    {
        const auto& vm=mapGet();
        for(const Path in: vm["input"].as<std::vector<std::string>>())
        {
            if(!bf::exists(in.path()))
            {
                stdErr() << in << ": not exist" << std::endl;
                continue;
            }

            if(canCWepb(in))
            {
                units_.emplace_back(Path(in), Path(), false); 
                continue;
            }

            bf::directory_iterator ditr(in.path());
            bf::directory_iterator const end;
            for(; ditr!=end; ++ditr)
            {
                const auto& path=ditr->path();
                if(canCWepb(path))
                    units_.emplace_back(path.filename(), in, true);
            }
        }
    }

    void callCWebpMain(const Path& in, Path&& out) const
    {
        out.replace_extension("webp");
        const auto& pp=out.parent_path();
        if(!bf::exists(pp))
        {
            boost::system::error_code ec;
            bf::create_directories(pp, ec);
            if(ec)
            {
                stdErr() << "mkdir:" << pp << ":error: " << ec.message() << std::endl;
                return;
            }
        }

        //task可能在this结束后再运行
        auto task=[this, out, in]() mutable
        {
			auto const outPath = out.native();
			auto const inPath = in.native();
            auto const quality=std::to_string(quality_);
            auto const alphaQuality=std::to_string(alphaQuality_);
            const char* cmdt[] =
            {
                "cwep", "-quiet",

                "-q",       quality.c_str(),
                "-alpha_q", alphaQuality.c_str(),

                "-o",       reinterpret_cast<const char*>(outPath.c_str()),
                reinterpret_cast<const char*>(inPath.c_str()),
            };

            std::vector<const char*> cmd(cmdt, cmdt+(sizeof(cmdt)/sizeof(cmdt[0])));
            cwebp_main(cmd.size(), cmd.data());
        };

        if(concurrency_==false)
            return task();

        auto& pool=TaskPool::instance();
        pool.post(task);
    }

    bool canCWepb(const Path& path) const
    {
        return bf::is_regular_file(path.path())
			&& boost::algorithm::iends_with(WCharConverter::to(path.native()), ".png");
    }

private:
    ///@todo 使用fileset执行文件选择
    bool concurrency_=false;
    unsigned quality_=50;
    unsigned alphaQuality_=50;
    std::vector<Unit> units_;
};

namespace
{
static CmdRegisterT<CmdCWebp> gsRegister;
}


}

