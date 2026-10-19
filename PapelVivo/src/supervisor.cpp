#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include "workflow.hpp"
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
 wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);auto base=std::filesystem::path(exe).parent_path();
 auto name=L"Local\\PapelVivo.Stop."+std::to_wstring(GetCurrentProcessId());HANDLE stop=CreateEventW(nullptr,TRUE,FALSE,name.c_str());
 HANDLE job=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
 if(!stop||!job||!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))return 2;
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::wstring cmd=pv::quoteArg((base/L"PapelVivo.Core.exe").wstring());for(int i=1;i<argc;i++)cmd+=L" "+pv::quoteArg(argv[i]);LocalFree(argv);
 SetEnvironmentVariableW(L"PAPELVIVO_STOP_EVENT",name.c_str());STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};auto core=base/L"PapelVivo.Core.exe";
 if(!CreateProcessW(core.c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED,nullptr,base.c_str(),&si,&pi)){MessageBoxW(nullptr,L"PapelVivo.Core.exe não encontrado. Reinstale o pacote completo. / Core missing. Reinstall the full package.",L"Papel Vivo",MB_ICONERROR);return 3;}
 if(!AssignProcessToJobObject(job,pi.hProcess)){TerminateProcess(pi.hProcess,4);return 4;}ResumeThread(pi.hThread);CloseHandle(pi.hThread);
 HANDLE handles[]{pi.hProcess,stop};auto result=WaitForMultipleObjects(2,handles,FALSE,INFINITE);
 if(result==WAIT_OBJECT_0+1 && WaitForSingleObject(pi.hProcess,3000)==WAIT_TIMEOUT){TerminateJobObject(job,5);WaitForSingleObject(pi.hProcess,1000);}
 DWORD code=0;GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hProcess);CloseHandle(stop);CloseHandle(job);return code==STILL_ACTIVE?5:int(code);
}
