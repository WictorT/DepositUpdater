//+------------------------------------------------------------------+
//|                                       MetaTrader WebRegistration |
//|                 Copyright © 2001-2008, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#include "stdafx.h"
#include "Processor.h"
//---- Plugin description
PluginInfo        ExtPluginInfo={"WebRegistration",160,"MetaQuotes Software Corp.",{0}};
//---- Server interface
CServerInterface *ExtServer    =NULL;
//+------------------------------------------------------------------+
//| DLL entry point                                                  |
//+------------------------------------------------------------------+
BOOL APIENTRY DllMain(HANDLE hModule,DWORD  ul_reason_for_call,LPVOID /*lpReserved*/)
  {
   char tmp[256],*cp;
//----
   switch(ul_reason_for_call)
     {
      case DLL_PROCESS_ATTACH:
        //---- create configuration file name
        GetModuleFileName((HMODULE)hModule,tmp,sizeof(tmp)-1);
        if((cp=strrchr(tmp,'.'))!=NULL) *cp=0;
        //---- add .ini
        strcat(tmp,".ini");
        //---- load configuration
        ExtConfig.Load(tmp);
        break;
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
        break;
     }
//----
   return(TRUE);
  }
//+------------------------------------------------------------------+
//| About, must be present always!                                   |
//+------------------------------------------------------------------+
void APIENTRY MtSrvAbout(PluginInfo *info)
  {
   if(info!=NULL) memcpy(info,&ExtPluginInfo,sizeof(PluginInfo));
  }
//+------------------------------------------------------------------+
//| Set server interface point                                       |
//+------------------------------------------------------------------+
int APIENTRY MtSrvStartup(CServerInterface *server)
  {
//---- check version
   if(server==NULL)                        return(FALSE);
   if(server->Version()!=ServerApiVersion) return(FALSE);
//---- store link to server
   ExtServer=server;
//---- initialize processor
   ExtProcessor.Initialize();
//---- ok
   return(TRUE);
  }
//+------------------------------------------------------------------+
//| Standard configuration functions                                 |
//+------------------------------------------------------------------+
int APIENTRY MtSrvPluginCfgAdd(const PluginCfg *cfg)
  {
   int res=ExtConfig.Add(0,cfg);
   ExtProcessor.Initialize();
   return(res);
  }
int APIENTRY MtSrvPluginCfgSet(const PluginCfg *values,const int total)
  {
   int res=ExtConfig.Set(values,total);
   ExtProcessor.Initialize();
   return(res);
  }
int APIENTRY MtSrvPluginCfgDelete(LPCSTR name)
  {
   int res=ExtConfig.Delete(name);
   ExtProcessor.Initialize();
   return(res);
  }
int APIENTRY MtSrvPluginCfgGet(LPCSTR name,PluginCfg *cfg)      { return ExtConfig.Get(name,cfg);   }
int APIENTRY MtSrvPluginCfgNext(const int index,PluginCfg *cfg) { return ExtConfig.Next(index,cfg); }
int APIENTRY MtSrvPluginCfgTotal()                              { return ExtConfig.Total();         }
//+------------------------------------------------------------------+
//| Telnet                                                           |
//+------------------------------------------------------------------+
int APIENTRY MtSrvTelnet(const ULONG ip,char *buf,const int maxlen)
  {
   return ExtProcessor.Process(ip,buf,maxlen);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
