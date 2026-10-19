#pragma once
#include <windows.h>
#include <filesystem>
#include <fstream>
#include "json.hpp"
struct LocalWorker {
 HANDLE process=nullptr,job=nullptr;std::filesystem::path dir;ULONGLONG started=0;
 ~LocalWorker(){stop();}
 void stop(){if(job){TerminateJobObject(job,2);CloseHandle(job);job=nullptr;}if(process){WaitForSingleObject(process,3000);CloseHandle(process);process=nullptr;}}
 bool busy()const{return process!=nullptr;}
 bool start(const std::filesystem::path&base,const std::filesystem::path&folder,const nlohmann::json&request){
  stop();dir=folder;std::filesystem::create_directories(dir);{std::ofstream f(dir/L"job.json");f<<request.dump();}
  auto exe=base/L"python/python.exe";auto cmd=L"\""+exe.wstring()+L"\" -I -B \""+(base/L"tools/remove_background.py").wstring()+L"\" --job \""+(dir/L"job.json").wstring()+L"\" --result \""+(dir/L"result.json").wstring()+L"\"";
  job=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;if(!job||!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))return false;
  SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};HANDLE log=CreateFileW((dir/L"worker.log").c_str(),GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);HANDLE nul=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr);
  STARTUPINFOW si{};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdInput=nul;si.hStdOutput=si.hStdError=log;PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),cmd.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,base.c_str(),&si,&pi)){if(log!=INVALID_HANDLE_VALUE)CloseHandle(log);if(nul!=INVALID_HANDLE_VALUE)CloseHandle(nul);return false;}
  if(log!=INVALID_HANDLE_VALUE)CloseHandle(log);if(nul!=INVALID_HANDLE_VALUE)CloseHandle(nul);
  process=pi.hProcess;if(!AssignProcessToJobObject(job,process)){TerminateProcess(process,2);CloseHandle(pi.hThread);stop();return false;}ResumeThread(pi.hThread);CloseHandle(pi.hThread);started=GetTickCount64();return true;
 }
 int poll(){if(!process)return 0;if(GetTickCount64()-started>600000){stop();return -1;}if(WaitForSingleObject(process,0)==WAIT_TIMEOUT)return 1;DWORD code=3;GetExitCodeProcess(process,&code);return code==0?2:-1;}
 nlohmann::json result(){std::ifstream f(dir/L"result.json");return nlohmann::json::parse(f);}
};
