//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdStart.cpp
//
//   Description:  其它程序启动命令
//
//       Version:  1.0
//       Created:  2015年01月05日 15时20分56秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#ifndef WIN32
    #include<fcntl.h>
    #include<sys/stat.h>
    #include<sys/types.h>
    #include<sys/wait.h>
    #include<unistd.h>
    #include<boost/asio/posix/stream_descriptor.hpp>
#else
    #include<core/msvc.hpp>
    #include<boost/asio/windows/stream_handle.hpp>
#endif //WIN32

#include<core/pipe.hpp>
#include<boost/algorithm/string/predicate.hpp>

#include"option.hpp"
#include"optionPredication.hpp"

namespace ezsh
{

class CmdStart: public CmdBaseT<CmdStart, PredicationCmdBase<>>
{
    typedef CmdBaseT<CmdStart, PredicationCmdBase<>> BaseThis;
    typedef StrCommandLine CmdLine;
public:
    CmdStart()
        :BaseThis("start - start a program")
        ,stdOutStream_(core::MainServer::get())
        ,stdErrStream_(core::MainServer::get())
    {}

    void parse(StrCommandLine&& cl) override
    {
        auto av=cl.begin();
        auto const avEnd=cl.end();
        for(;;)
        {
            auto itr=std::find(av, avEnd, "--");
            StrCommandLine tmp(std::distance(av, itr));
            if(tmp.empty())
                throw bp::error("cmdline error for start");

            std::move(av, itr, tmp.begin());
            cmdLines_.emplace_back(std::move(tmp));
            if(itr==avEnd)
                break;

            av=itr+1;
            if(av==avEnd)
                throw bp::error("no exec to start");
        }

        if(cmdLines_.size()<=1)
            throw bp::error("no exec to start");

        auto& self=cmdLines_[0];
        return BaseThis::parse(std::move(self));
    }

    static const char* nameGet()
    {
        return "start";
    }

    void doit()
    {
        if(check()==false)
            return ecSet(EzshError::ecMake(EzshError::eParamInvalid));

        const size_t count=cmdLines_.size();
        for(size_t i=1; i<count; ++i)
        {
            auto& cmd=cmdLines_[i];
            const auto& exec=cmd[0];

            const auto& path=exeFind(exec);
            if(path.empty())
            {
                start(exec, cmd);
                stdErr() << exec << ": not exist or not executable" << std::endl;
                return ecSet(EzshError::ecMake(EzshError::eParamInvalid));
            }

            start(path, cmd);
        }
    }

    void doDry()
    {
        BaseThis::doDry();
        const size_t count=cmdLines_.size();
        for(size_t i=1; i<count; ++i)
        {
            auto& cmd=cmdLines_[i];
            stdOut() << "\t--";
            for(const auto& c: cmd)
                stdOut() << " " << c;
            stdOut() << std::endl;
        }
    }

private:
    bool check()
    {
        predicationInit(mapGet());
        return isPassed();
    }

    Path exeFind(const Path& file)
    {
        if(!file.parent_path().empty())
            return file;

#ifdef WIN32
        std::vector<Path> adjustFile;
        const auto& ext = file.extension();
        if (ext == "exe" || ext == "bat" || ext == "cmd")
        {
            adjustFile.emplace_back(file);
        } else {
            adjustFile.emplace_back(Path(file).concat(".exe"));
            adjustFile.emplace_back(Path(file).concat(".bat"));
            adjustFile.emplace_back(Path(file).concat(".cmd"));
        }
#else
        std::vector<Path> adjustFile = { file };
#endif //WIN32
        const auto& env = Environment::instance();
        for(const auto& p: adjustFile)
        {
            auto const path=env.pathFile(p);
            if(!path.empty())
                return path;
        }

#ifdef WIN32
        return file;
#else
        return Path();
#endif //WIN32
    }

#ifdef WIN32
    typedef boost::asio::windows::stream_handle Stream;
    void stdIOReset(::STARTUPINFOW& info)
    {
        info.dwFlags |= STARTF_USESTDHANDLES;

        handleReset(::GetStdHandle(STD_INPUT_HANDLE), info.hStdInput);

        info.hStdOutput=stdOut_->writeGet();
        info.hStdError=stdErr_->writeGet();
    }

    void handleReset(HANDLE src, HANDLE& dest)
    {
        HANDLE out = INVALID_HANDLE_VALUE;
        auto const prc = ::GetCurrentProcess();
        ::DuplicateHandle(prc, src, prc, &out, 0, true, DUPLICATE_SAME_ACCESS);
        dest = out;
    }

    int start(const Path& exe, const CmdLine& cl)
    {
        stdOut_.reset(new core::Pipe());
        stdErr_.reset(new core::Pipe());

        Path path(exe);
        path.make_preferred();

        std::wstring cmd;
        cmd += L'"';
        cmd += path.native();
        cmd += L"\" ";
        for(size_t i=1; i<cl.size(); ++i)
        {
            cmd += L'"';
            cmd += core::WCharConverter::from(cl[i]);
            cmd += L"\" ";
        }

        ::STARTUPINFOW si;
        ::PROCESS_INFORMATION pi;

        std::memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        std::memset(&pi, 0, sizeof(pi));

        stdIOReset(si);

        if (!::CreateProcessW(nullptr,
            const_cast<wchar_t*>(cmd.data()),  // Command line
            nullptr,       // Process handle not inheritable
            nullptr,       // Thread handle not inheritable
            true,          // Set handle inheritance to true
            0,             // No creation flags
            nullptr,       // Use parent's environment block
            nullptr,       // Use parent's starting directory 
            &si,           // Pointer to STARTUPINFO structure
            &pi )          // Pointer to PROCESS_INFORMATION structure
        ) 
        {
            stdErr() << cmd << ": CreateProcess failed: " << ::GetLastError() << std::endl;
            return -1;
        }

        stdIOParent();

        // Wait until child process exits.
        ::WaitForSingleObject(pi.hProcess, INFINITE);

        // Close process and thread handles. 
        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);
        return 0;
    }
#else
    typedef boost::asio::posix::stream_descriptor Stream;
    void stdIOChild()
    {
        //::dup2(stdIn_.readGet(), STDIN_FILENO);
        //stdIn_.writeClose();

        ::dup2(stdOut_->writeGet(), STDOUT_FILENO);
        stdOut_->readClose();

        ::dup2(stdErr_->writeGet(), STDERR_FILENO);
        stdErr_->readClose();
    }

    int start(const Path& exe, const CmdLine& cl)
    {
        stdOut_.reset(new core::Pipe());
        stdErr_.reset(new core::Pipe());

        auto cmd=cmdlineMake(cl);
        cmd.push_back(nullptr);
        const auto pid=::fork();
        switch(pid)
        {
            case 0://child
            {
                stdIOChild();
                ::execv(exe.c_str(), cmd.data());
                ::perror("execv");
                ::exit(EXIT_FAILURE);
            }
            case -1://error
            {
                stdErr() << "fork error" << std::endl;
                return -1;
            }
            default:
            {
                stdIOParent();
                int status=0;
                ::waitpid(pid, &status, 0);
                return status;
            }
        }
    }

#endif //__MSC_VER

    void stdIOParent()
    {
        //关闭不需要的端口
        //stdIn_.readClose();
        stdOut_->writeClose();
        stdErr_->writeClose();

        //暂停当前协程，并异步转发数据
        contextGet()->yield([this]()
            {
                this->stdIOTransfer();
            }
        );
    }

    void stdIOTransfer()
    {
        //当前对象可能执行多个本地命令
        streamCount_ = 2;

        if (stdOutStream_.is_open())
            stdOutStream_.close();
        stdOutStream_.assign(stdOut_->readReleaseGet());
        stdOutBuffer_.resize(4*1024);
        stdRead(stdOutStream_, stdOutBuffer_, stdOut());

        if (stdErrStream_.is_open())
            stdErrStream_.close();
        stdErrStream_.assign(stdErr_->readReleaseGet());
        stdErrBuffer_.resize(4*1024);
        stdRead(stdErrStream_, stdErrBuffer_, stdErr());
    }

    void stdRead(Stream& strm, std::vector<char>& buf, StdStream& out)
    {
        strm.async_read_some(boost::asio::buffer(buf),
            [this, &strm, &out, &buf](const boost::system::error_code& ec, std::size_t nb)
            {
                if(ec)
                {
                    if(--streamCount_==0)
                        contextGet()->resume();
                    return;
                }

                out.write(core::mbstowcs(buf.data(), nb));
                stdRead(strm, buf, out);
            }
        );
    }

private:
    //core::Pipe stdIn_;
    std::unique_ptr<core::Pipe> stdOut_;
    std::unique_ptr<core::Pipe> stdErr_;

    int streamCount_=2;

    Stream stdOutStream_;
    std::vector<char> stdOutBuffer_;

    Stream stdErrStream_;
    std::vector<char> stdErrBuffer_;

    std::vector<CmdLine> cmdLines_;
};

namespace
{
    static CmdRegisterT<CmdStart> gsCmdRegister;
}

}

