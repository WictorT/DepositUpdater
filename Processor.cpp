//+------------------------------------------------------------------+
//|                                       MetaTrader WebRegistration |
//|                 Copyright © 2001-2008, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#include "stdafx.h"
#include "Processor.h"
//---- Link to our server interface
extern CServerInterface *ExtServer;
//---- Our Telnet processor
CProcessor               ExtProcessor;
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
CProcessor::CProcessor() : m_ip(0),         m_groups(NULL),     m_groups_total(0),
                           m_flooders(NULL),m_flooders_total(0),m_flooders_max(0)
  {
   m_password[0]=0;
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
CProcessor::~CProcessor()
  {
//---- lock
   m_sync.Lock();
//---- delete all groups and flooders
   if(m_groups  !=NULL) { delete[] m_groups;   m_groups  =NULL; }
   if(m_flooders!=NULL) { delete[] m_flooders; m_flooders=NULL; }
//---- set all to zero
   m_groups_total=m_flooders_total=m_flooders_max=0;
//---- unlock
   m_sync.Unlock();
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void CProcessor::Initialize(void)
  {
   char        buffer[1024];
   Group      *temp;
   int         i,groups_max;
   PluginCfg   cfg;
//---- lock
   m_sync.Lock();
//---- get master password
   ExtConfig.GetString(1,"Master Password",m_password,sizeof(m_password)-1,"password");
//---- get master IP
   ExtConfig.GetString(2,"Master IP",      buffer,    sizeof(buffer)-1,    "127.0.0.1");
//---- conver IP to number format
   m_ip=inet_addr(buffer);
//---- get groups map
   groups_max=m_groups_total;
//---- parse groups
   for(i=0,m_groups_total=0;;i++)
     {
      //---- prepare query
      _snprintf(buffer,sizeof(buffer)-1,"Group %d",i+1);
      //---- try to receive group
      if(ExtConfig.Get(buffer,&cfg)==FALSE) break;
      //---- check
      if(cfg.value[0]==0) continue;
      //---- check space
      if(m_groups==NULL || m_groups_total>=groups_max)
        {
         //---- allocate new buffer
         if((temp=new Group[m_groups_total+1024])==NULL)
           {
            m_sync.Unlock();
            Out(CmdAtt,"WebRegistration","not enough memory for groups [%d]",m_groups_total+1024);
            return;
           }
         //---- copy all from old buffer to new and delete old
         if(m_groups!=NULL)
           {
            memcpy(temp,m_groups,sizeof(Group)*m_groups_total);
            delete[] m_groups;
           }
         //---- set new buffer
         m_groups  =temp;
         groups_max=m_groups_total+1024;
        }
      //---- add group
      m_groups[m_groups_total].id=i+1;
      COPY_STR(m_groups[m_groups_total].group,cfg.value);
      //---- increment groups total
      m_groups_total++;
     }
//---- if groups not exists then create samples
   if(m_groups_total==0)
     {
      //---- add first example
      COPY_STR(cfg.name, "Group 1");
      COPY_STR(cfg.value,"demoforex");
      ExtConfig.Add(0,&cfg);
      //---- add second example
      COPY_STR(cfg.name, "Group 2");
      COPY_STR(cfg.value,"");
      ExtConfig.Add(0,&cfg);
     }
//---- unlock
   m_sync.Unlock();
  }
//+------------------------------------------------------------------+
//| Parser Telnet requests                                           |
//+------------------------------------------------------------------+
int CProcessor::Process(const ULONG ip,char *buffer,const int size)
  {
   UserRecord user ={0};
   ConGroup   group={0};
   char       group_name[32]={0};
   char       temp[256];
   int        i;
   double     deposit=0;
//---- lock
   m_sync.Lock();
//---- checks
   if(ExtServer==NULL || buffer==NULL || size<1 || ip!=m_ip || m_groups==NULL) 
     { 
      m_sync.Unlock();
      return(0);
     }
//---- check command
   if(memcmp(buffer,"NEWACCOUNT",10)!=0)
     { 
      m_sync.Unlock(); 
      return(0); 
     }
//---- receive master password and check it
   if(GetStrParam(buffer,"MASTER=",temp,sizeof(temp)-1)==FALSE || strcmp(temp,m_password)!=0)
     {
      m_sync.Unlock();
      return _snprintf(buffer,size-1,"ERROR\r\ninvalid data\r\nend\r\n");
     }
//---- unlock
   m_sync.Unlock();
//---- receive IP
   if(GetStrParam(buffer,"IP=",temp,sizeof(temp)-1)==FALSE) 
      return _snprintf(buffer,size-1,"ERROR\r\ninvalid data\r\nend\r\n");
//---- receive group id and map it
   if(GetIntParam(buffer,"GROUP=",&i)==FALSE || (MapGroup(i,group_name))==FALSE)
      return _snprintf(buffer,size-1,"ERROR\r\ninvalid data\r\nend\r\n");
//---- receive group overview
   if(ExtServer->GroupsGet(group_name,&group)==FALSE)
      return _snprintf(buffer,size-1,"ERROR\r\ninvalid data\r\nend\r\n");
//---- prepare user record
   user.enable                =TRUE;
   user.enable_change_password=TRUE;
   user.leverage              =group.default_leverage;
   user.user_color            =0xff000000;
   COPY_STR(user.group,group_name);
   GetStrParam(buffer,"NAME=",          user.name,             sizeof(user.name)-1);
   GetStrParam(buffer,"PASSWORD=",      user.password,         sizeof(user.password)-1);
   GetStrParam(buffer,"INVESTOR=",      user.password_investor,sizeof(user.password_investor)-1);
   GetStrParam(buffer,"EMAIL=",         user.email,            sizeof(user.email)-1);
   GetStrParam(buffer,"COUNTRY=",       user.country,          sizeof(user.country)-1);
   GetStrParam(buffer,"STATE=",         user.state,            sizeof(user.state)-1);
   GetStrParam(buffer,"CITY=",          user.city,             sizeof(user.city)-1);
   GetStrParam(buffer,"ADDRESS=",       user.address,          sizeof(user.address)-1);
   GetStrParam(buffer,"COMMENT=",       user.comment,          sizeof(user.comment)-1);
   GetStrParam(buffer,"PHONE=",         user.phone,            sizeof(user.phone)-1);
   GetStrParam(buffer,"PHONE_PASSWORD=",user.password_phone,   sizeof(user.password_phone)-1);
   GetStrParam(buffer,"STATUS=",        user.status,           sizeof(user.status)-1);
   GetStrParam(buffer,"ZIPCODE=",       user.zipcode,          sizeof(user.zipcode)-1);
   GetStrParam(buffer,"ID=",            user.id,               sizeof(user.id)-1);
   GetIntParam(buffer,"LEVERAGE=",     &user.leverage);
   GetIntParam(buffer,"AGENT=",        &user.agent_account);
   GetIntParam(buffer,"SEND_REPORTS=", &user.send_reports);
   GetFltParam(buffer,"DEPOSIT=",      &deposit);
//---- checks
   if(user.leverage     <1)         user.leverage     =0;
   if(user.agent_account<1)         user.agent_account=0;
   if(user.send_reports!=0)         user.send_reports =TRUE;
   if(deposit           <0)         deposit           =0;
   if(deposit           >100000000) deposit           =100000000;
//---- check default deposit
   if(group.default_deposit>0) deposit=group.default_deposit;
//---- check complexity of password
   if(CheckPassword(user.password)==FALSE)
      return _snprintf(buffer,size-1,"ERROR\r\nsimple password\r\nend\r\n");
//---- check user record
   if(user.name[0]==0 || user.leverage<1 || user.agent_account<0)
      return _snprintf(buffer,size-1,"ERROR\r\ninvalid data\r\nend\r\n");
//---- check IP by antiflood
   if(CheckFlooder(temp)==FALSE)
      return _snprintf(buffer,size-1,
             "ERROR\r\nIP is blocked. Please wait %d secs and try again.\r\nend\r\n",
             ANTIFLOOD_PERIOD);
//---- creating new account
   if(ExtServer->ClientsAddUser(&user)==FALSE)
      return _snprintf(buffer,size-1,"ERROR\r\naccount create failed\r\nend\r\n");
//---- checking is demo
   if(strncmp(user.group,"demo",4)==0 && deposit>0)
     {
      //---- trying process deposit operation
      ExtServer->ClientsChangeBalance(user.login,&group,deposit,"");
     }
//---- write answer
   return _snprintf(buffer,size-1,"OK\r\nLOGIN=%d\r\nend\r\n",user.login);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void CProcessor::Out(const int code,LPCSTR ip,LPCSTR msg,...) const
  {
   char buffer[1024];
//---- check
   if(ExtServer==NULL || msg==NULL) return;
//---- format string
   va_list arg_ptr;
   va_start(arg_ptr,msg);
   _vsnprintf(buffer,sizeof(buffer)-1,msg,arg_ptr);
   va_end(arg_ptr);
//---- out to server log
   ExtServer->LogsOut(code,ip,buffer);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
int CProcessor::MapGroup(const int index,char *group)
  {
//---- check
   if(index<1 || group==NULL) return(FALSE);
//---- lock
   m_sync.Lock();
//---- check
   if(index>m_groups_total || m_groups==NULL) 
     {
      m_sync.Unlock();
      return(FALSE);
     }
//---- return group name by index
   strcpy(group,m_groups[index-1].group);
//---- ok
   m_sync.Unlock();
   return(TRUE);
  }

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
int CProcessor::CheckFlooder(LPCSTR ip)
  {
   Flooder *temp,flooder;
   time_t   currtime;
   int      i;
//---- check
   if(ExtServer==NULL || ip==NULL) return(FALSE);
//---- get current time
   currtime=ExtServer->TradeTime();
//---- lock
   m_sync.Lock();
//---- refresh flooders list
   if(m_flooders!=NULL && m_flooders_total>0)
     {
      //---- check: may be somebody can delete
      for(i=0,temp=m_flooders;i<m_flooders_total;i++,temp++)
        {
         //---- we should delete the oldest flooders
         if(currtime-temp->lasttime>ANTIFLOOD_PERIOD*5)
           {
            //---- delete from buffer
            memmove(temp,temp+1,sizeof(Flooder)*(m_flooders_total-i-1));
            //---- correct counters
            m_flooders_total--; i--; temp--;
           }
        }
     }
//---- try find IP
   if((temp=(Flooder *)bsearch(ip,m_flooders,m_flooders_total,sizeof(Flooder),FloodersSorts))==NULL)
     {
      //---- check space
      if(m_flooders==NULL || m_flooders_total>=m_flooders_max)
        {
         //---- allocate new buffer
         if((temp=new Flooder[m_flooders_total+1024])==NULL)
           {
            m_sync.Unlock();
            Out(CmdAtt,"WebRegistration","not enough memory for flooders [%d]",m_flooders_total+1024);
            return(FALSE);
           }
         //---- copy all from old buffer to new and delete old buffer
         if(m_flooders!=NULL)
           {
            memcpy(temp,m_flooders,sizeof(Flooder)*m_flooders_total);
            delete[] m_flooders;
           }
         //---- set new buffer
         m_flooders    =temp;
         m_flooders_max=m_flooders_total+1024;
        }
      //---- prepare flooder
      flooder.lasttime=currtime;
      COPY_STR(flooder.ip,ip);
      //---- insert flooder in the list
      if(insert(m_flooders,&flooder,m_flooders_total,sizeof(Flooder),FloodersSorts)!=NULL)
         m_flooders_total++;
      //---- all right
      m_sync.Unlock();
      return(TRUE);
     }
//---- check: may be IP is blocked?
   if((currtime-temp->lasttime)<ANTIFLOOD_PERIOD)
     { 
      m_sync.Unlock(); 
      return(FALSE); 
     }
//---- set last time
   temp->lasttime=currtime;
//---- ok
   m_sync.Unlock();
   return(TRUE);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
int CProcessor::FloodersSorts(const void *param1,const void *param2)
  {
   return strcmp(((Flooder *)param1)->ip,((Flooder *)param2)->ip);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
