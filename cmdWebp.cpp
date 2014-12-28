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
class CmdCWebp:public OptBaseT<CmdCWebp>
{
    typedef OptBaseT<CmdCWebp> BaseThis;

    struct Unit
    {
        Unit(const bf::path& i, const bf::path& b, bool s)
            :in(i), base(b), scaned(s)
        {}

        bf::path in;
        bf::path base;
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

    MainReturn doit() override
    {
        const auto& vm=mapGet();

        quality_=vm["quality"].as<unsigned>();
        alphaQuality_=vm["alphaQuality"].as<unsigned>();
        concurrency_=vm.count("concurrency") ? true : false;

        inputCollect();
        if(units_.empty())
        {
            context_->stdCErr() << "no input valid" << std::endl;
            return MainReturn::eParamInvalid;
        }

        const bf::path out=vm["output"].as<std::string>();
        const auto outIsExist=bf::exists(out);
        const auto outIsDir=bf::is_directory(out);

        //单个文件，处理情况特殊
        if(units_.size()==1 && !outIsExist)
        {
            callCWebpMain(units_[0].in, bf::path(out));
            return MainReturn::eGood;
        }

        if(outIsExist && !outIsDir)
        {
            context_->stdCErr() << out << ": exist but not directory" << std::endl;
            return MainReturn::eParamInvalid;
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
        
        return MainReturn::eGood;
    }

private:
    void inputCollect()
    {
        const auto& vm=mapGet();
        for(const auto& in: vm["input"].as<std::vector<std::string>>())
        {
            if(!bf::exists(in))
            {
                context_->stdCErr() << in << ": not exist" << std::endl;
                continue;
            }

            if(canCWepb(in))
            {
                units_.emplace_back(bf::path(in), bf::path(), false); 
                continue;
            }

            bf::directory_iterator ditr(in);
            bf::directory_iterator const end;
            for(; ditr!=end; ++ditr)
            {
                const auto& path=ditr->path();
                if(canCWepb(path))
                    units_.emplace_back(path.filename(), in, true);
            }
        }
    }

    void callCWebpMain(const bf::path& in, bf::path&& out) const
    {
        out.replace_extension("webp");
        const auto& pp=out.parent_path();
        if(!bf::exists(pp))
        {
            boost::system::error_code ec;
            bf::create_directories(pp, ec);
            if(ec)
            {
                context_->stdCErr() << "mkdir:" << pp << ":error: " << ec.message() << std::endl;
                return;
            }
        }

        //task可能在this结束后再运行
        auto task=[out, in,
             quality=std::to_string(quality_),
             alphaQuality=std::to_string(alphaQuality_)]() mutable
        {
			auto const outPath = convert(out.native());
			auto const inPath = convert(in.native());
            const char* cmdt[] =
            {
                "cwep", "-quiet",

                "-q",       quality.c_str(),
                "-alpha_q", alphaQuality.c_str(),

                "-o",       outPath.c_str(),
                inPath.c_str(),
            };

            std::vector<const char*> cmd(cmdt, cmdt+(sizeof(cmdt)/sizeof(cmdt[0])));
            cwebp_main(cmd.size(), cmd.data());
        };

        if(concurrency_==false)
            return task();

        auto& pool=TaskPool::instance();
        pool.post(task);
    }

    bool canCWepb(const bf::path& path) const
    {
        return bf::is_regular_file(path)
            && boost::algorithm::iends_with(path.string(), ".png");
    }

	static std::string convert(const std::string& s)
	{
		return s;
	}

	static std::string convert(const std::wstring& s)
	{
		if (s.empty())
			return std::string();

		std::string ret(s.size()*16, '\0');
#ifdef _MSC_VER
		size_t const destlen = ::WideCharToMultiByte(CP_ACP, 0, s.c_str(), s.size()*sizeof(wchar_t),
			const_cast<char*>(ret.data()), ret.size(),
			nullptr, nullptr);
#else
		size_t const destlen = ::wcstombs(const_cast<char*>(ret.data()), s.c_str(), ret.size());
#endif
		if (destlen == static_cast<size_t>(-1))
			ret.clear();
		else
			ret.resize(destlen);
		return std::move(ret);
	}

private:
    bool concurrency_=false;
    unsigned quality_=50;
    unsigned alphaQuality_=50;
    std::vector<Unit> units_;
};

namespace
{
static OptRegisterT<CmdCWebp> gsRegister;
}


}

