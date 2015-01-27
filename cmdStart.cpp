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
#else
#endif //WIN32

#include<boost/algorithm/string/predicate.hpp>

#include"option.hpp"

namespace ezsh
{

class CmdStart: public CmdBaseT<CmdStart>
{
    typedef CmdBaseT<CmdStart> BaseThis;
    typedef StrCommandLine CmdLine;
public:
    CmdStart()
        :BaseThis("start - start a program")
    {
        opt_.add_options()
            ("exist",        bp::value<std::vector<std::string>>(), "start exec, if this exist")
            ("existNot",     bp::value<std::vector<std::string>>(), "start exec, if this not exist")
            ("fileExist",    bp::value<std::vector<std::string>>(), "start exec, if this file exist")
            ("fileExistNot", bp::value<std::vector<std::string>>(), "start exec, if this file not exist")
            ("dirExist",     bp::value<std::vector<std::string>>(), "start exec, if this dir exist")
            ("dirExistNot",  bp::value<std::vector<std::string>>(), "start exec, if this dir not exist")

            ("stdOut",  bp::value<std::string>(), "pipe stdout to this file")
        ;
    }

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

        const auto& vm=mapGet();
        auto itr=vm.find("stdOut");
        if(itr!=vm.end())
            stdOut_=itr->second.as<std::string>();

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
        const auto& vm=mapGet();

        typedef std::function<bool (const bf::file_status& st)> Cond;
        struct U
        {
            const char* name;
            Cond cond;
        };

        typedef bf::file_type FT;
        const U d[]=
        {
            {"exist",        [](const bf::file_status& st){ return st.type()!=FT::file_not_found; } },
            {"existNot",     [](const bf::file_status& st){ return st.type()==FT::file_not_found; } },
            {"fileExist",    [](const bf::file_status& st){ return st.type()==FT::regular_file; } },
            {"fileExistNot", [](const bf::file_status& st){ return st.type()!=FT::regular_file; } },
            {"dirExist",     [](const bf::file_status& st){ return st.type()==FT::directory_file; } },
            {"dirExistNot",  [](const bf::file_status& st){ return st.type()!=FT::directory_file; } },
        };

        for(const auto& u: d)
        {
            auto itr=vm.find(u.name);
            if(itr!=vm.end())
            {
                const auto& files=itr->second.as<std::vector<std::string>>();
                for(const auto& file: files)
                {
                    Path path(file);
                    ErrorCode ec;
                    const auto st=bf::status(file, ec);
                    if(u.cond(st)==false)
                        return false;
                }
            }
        }

        return true;
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
    void stdIOReset(::STARTUPINFOW& info)
    {
		info.dwFlags |= STARTF_USESTDHANDLES;

		handleReset(stdIn_, ::GetStdHandle(STD_INPUT_HANDLE), info.hStdInput);
		handleReset(stdOut_, ::GetStdHandle(STD_OUTPUT_HANDLE), info.hStdOutput);
		handleReset(stdErr_, ::GetStdHandle(STD_ERROR_HANDLE),  info.hStdError);
    }

    void handleReset(const std::string& file, HANDLE src, HANDLE& dest)
    {
		if (file.empty())
		{
			HANDLE out = INVALID_HANDLE_VALUE;
			auto const prc = ::GetCurrentProcess();
			::DuplicateHandle(prc, src, prc, &out, 0, true, DUPLICATE_SAME_ACCESS);
			dest = out;
			return;
		}

        Path path(file);
        path.make_preferred();
        ::SECURITY_ATTRIBUTES att;
        std::memset(&att, 0, sizeof(att));
        att.nLength = sizeof(att);
        att.bInheritHandle = true;
        auto handle=::CreateFileW(path.native().c_str(),
            GENERIC_WRITE, FILE_SHARE_READ, &att,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr
        );

		dest = handle;
    }

    int start(const Path& exe, const CmdLine& cl)
    {
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
		std::memset(&pi, 0, sizeof(pi) );

        stdIOReset(si);

		if (!::CreateProcessW(nullptr,
			const_cast<wchar_t*>(cmd.data()),        // Command line
			nullptr,           // Process handle not inheritable
			nullptr,           // Thread handle not inheritable
			true,          // Set handle inheritance to true
			0,              // No creation flags
			nullptr,           // Use parent's environment block
			nullptr,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi )           // Pointer to PROCESS_INFORMATION structure
		) 
		{
			stdErr() << cmd << ": CreateProcess failed: " << ::GetLastError() << std::endl;
			return -1;
		}

		// Wait until child process exits.
		::WaitForSingleObject(pi.hProcess, INFINITE);

		// Close process and thread handles. 
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
		if (si.hStdOutput)
			::CloseHandle(si.hStdOutput);
		if (si.hStdError)
			::CloseHandle(si.hStdError);
		if(si.hStdInput)
			::CloseHandle(si.hStdInput);
		return 0;
    }
#else
    void stdIOReset()
    {
        if(stdOut_.empty())
            return;
        int const fd=::open(stdOut_.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
        ::dup2(fd, STDOUT_FILENO);
    }

    int start(const Path& exe, const CmdLine& cl)
    {
        auto cmd=cmdlineMake(cl);
        cmd.push_back(nullptr);
        const auto pid=::fork();
        switch(pid)
        {
            case 0://child
            {
                stdIOReset();
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
                int status=0;
                ::waitpid(pid, &status, 0);
                return status;
            }
        }
    }

#endif //__MSC_VER
private:
    std::string stdIn_;
    std::string stdOut_;
    std::string stdErr_;
    std::vector<CmdLine> cmdLines_;
};

namespace
{
    static CmdRegisterT<CmdStart> gsCmdRegister;
}

}


