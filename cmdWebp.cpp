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

#include<core/server.hpp>

#include"output.hpp"
#include"option.hpp"
#include"fileset.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;
class CmdCWebp:public CmdBaseT<CmdCWebp, FileSetCmdBase<OutPutCmdBase<>>>
{
    typedef CmdBaseT<CmdCWebp, FileSetCmdBase<OutPutCmdBase<>>> BaseThis;
public:
    CmdCWebp()
        :BaseThis("cwebp - webp encode")
    {
        opt_.add_options()
            ("concurrency,c", "concurrency encode with threads")
            ("quality,q",      bp::value<unsigned>()->default_value(50), "image quality")
            ("alphaQuality,a", bp::value<unsigned>()->default_value(50), "image alpha channel quality")
        ;
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

        auto& files=fileGet();
        files.init(vm);
        files.scan();

        auto& output=outGet();
        output.init(vm);

        counter_=files.setGet().size();
        contextGet()->yield([&, this]()
            {
                const auto& o=outGet();
                const auto& s=fileGet().setGet();
                for(const auto& u: s)
                {
                    FileUnit dest;
                    o.rewrite(u, dest);
                    callCWebpMain(u, dest);
                }
            }
        );
    }

private:
    void callCWebpMain(const FileUnit& in, const FileUnit& out) const
    {
        //task可能在this结束后再运行
        auto task=[this, out, in]() mutable
        {
			auto const outPath = out.total.native();
			auto const inPath = in.total.native();
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

            if(--counter_==0)
                contextGet()->resume();
        };

        if(concurrency_==false)
            return task();

        auto& pool=core::IOServer::instance();
        pool.post(task);
    }

private:
    bool concurrency_=false;
    unsigned quality_=50;
    unsigned alphaQuality_=50;
    mutable std::atomic<size_t> counter_;
};

namespace
{
static CmdRegisterT<CmdCWebp> gsRegister;
}


}

