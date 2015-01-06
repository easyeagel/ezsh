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
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#else
#endif //WIN32

#include<boost/algorithm/string/predicate.hpp>

#include"option.hpp"

namespace ezsh
{

class CmdStart: public CmdBaseT<CmdStart>
{
    typedef CmdBaseT<CmdStart> BaseThis;
    typedef std::vector<char*> CmdLine;
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
        ;
    }

    void parse(int ac, char* av[]) override
    {
        char** const avEnd=av+ac;
        for(;;)
        {
            auto itr=std::find(av, avEnd, std::string("--"));
            cmdLines_.emplace_back(av, itr);
            if(itr==avEnd)
                break;
            *itr=nullptr;
            av=itr+1;
            if(av==avEnd)
                throw bp::error("no exec to start");
        }

        if(cmdLines_.size()<=1)
            throw bp::error("no exec to start");

        auto& self=cmdLines_[0];
        return BaseThis::parse(self.size(), self.data());
    }

    static const char* nameGet()
    {
        return "start";
    }

    MainReturn doit()
    {
        if(check()==false)
            return MainReturn::eGood;

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
                return MainReturn::eParamInvalid;
            }

            start(path, cmd);
        }
        return MainReturn::eGood;
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
                    boost::system::error_code ec;
                    const auto st=bf::status(file, ec);
                    if(u.cond(st)==false)
                        return false;
                }
            }
        }

        return true;
    }

    std::string exeFind(const std::string& file)
    {
		std::string adjustFile = file;
#ifdef WIN32
		return adjustFile;
#else
		//相对路径不查找
		const char* st[] = { ".", "/" };
		for (const auto s : st)
		{
			if (boost::algorithm::starts_with(adjustFile, s))
				return adjustFile;
		}

		const auto& env = Environment::instance();
		return env.pathFile(adjustFile);
#endif //WIN32
    }

#ifdef WIN32
    int start(const std::string& exe, CmdLine& cl)
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
            cmd += WCharConverter::from(cl[i]);
            cmd += L"\" ";
        }

		::STARTUPINFOW si;
		::PROCESS_INFORMATION pi;

		std::memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		std::memset(&pi, 0, sizeof(pi) );

		if (!::CreateProcessW(nullptr,
			const_cast<wchar_t*>(cmd.data()),        // Command line
			nullptr,           // Process handle not inheritable
			nullptr,           // Thread handle not inheritable
			false,          // Set handle inheritance to FALSE
			0,              // No creation flags
			nullptr,           // Use parent's environment block
			nullptr,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi )           // Pointer to PROCESS_INFORMATION structure
		) 
		{
			stdErr() << "CreateProcess failed: " << ::GetLastError() << std::endl;
			return -1;
		}

		// Wait until child process exits.
		::WaitForSingleObject(pi.hProcess, INFINITE);

		// Close process and thread handles. 
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);

		return 0;
    }
#else
    int start(const std::string& exe, CmdLine& cmd)
    {
        cmd.push_back(nullptr);

        const auto pid=::fork();
        switch(pid)
        {
            case 0://child
            {
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
    std::vector<CmdLine> cmdLines_;
};

namespace
{
    static CmdRegisterT<CmdStart> gsCmdRegister;
}

}


