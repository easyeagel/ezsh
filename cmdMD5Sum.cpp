//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdMD5Sum.cpp
//
//   Description:  MD5Sum 计算
//
//       Version:  1.0
//       Created:  2015年04月03日 13时31分23秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<core/md5sum.hpp>
#include<boost/filesystem/fstream.hpp>

#include"option.hpp"
#include"optionFileSet.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class CmdMD5Sum:public CmdBaseT<CmdMD5Sum, FileSetCmdBase<>>
{
    typedef CmdBaseT<CmdMD5Sum, FileSetCmdBase> BaseThis;
public:
    CmdMD5Sum()
        :BaseThis("md5sum - md5sum file or dir")
    {}

    static const char* nameGet()
    {
        return "md5sum";
    }

    void doit()
    {
        const auto& vm=mapGet();
        auto& files=fileGet();

        files.init(vm);

        files.loop([this](FileUnit&& fu)
            {
                if(fu.isExist()==false || fu.isDir())
                    return false;

                bf::ifstream in(fu.total.path(), std::ios::in | std::ios::binary);
                if(!in)
                    return false;

                core::MD5Compute md5;
                std::vector<char> buf(4*1024);
                size_t totalSize = 0;
                for(;;)
                {
                    in.read(buf.data(), buf.size());
                    auto const count=static_cast<int>(in.gcount());
                    if (count <= 0)
                        break;
                    totalSize += count;
                    md5.append(reinterpret_cast<const uint8_t*>(buf.data()), count);
                }
                md5.finish();

                const auto& md5sum=md5.sumGet();
                stdOut() << fu.self << " " << md5sum.toString() << std::endl;

                return errorBreak()==false;
            }
        );
    }

};

namespace
{
static CmdRegisterT<CmdMD5Sum> gsRegister;
}


}

