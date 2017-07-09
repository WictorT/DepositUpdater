//+------------------------------------------------------------------+
//|                                       MetaTrader WebRegistration |
//|                 Copyright © 2001-2008, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#pragma once
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
#define ANTIFLOOD_PERIOD   (60)
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
struct Group
  {
   int               id;
   char              group[64];
  };
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
struct Flooder
  {
   char              ip[16];              // IP
   time_t            lasttime;            // last time
  };
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
class CProcessor
  {
private:
   CSync             m_sync;              // synchronizer
   char              m_password[32];      // master password
   ULONG             m_ip;                // master IP
   Group            *m_groups;            // groups
   int               m_groups_total;      // count of groups
   Flooder          *m_flooders;          // flooders
   int               m_flooders_total;    // count of flooders
   int               m_flooders_max;      // max of flooders
   
public:
                     CProcessor();
                    ~CProcessor();
   //---- initializing
   void              Initialize(void);
   //---- process Telnet requests
   int               Process(const ULONG ip,char *buffer,const int size);
private:
   //---- out to server log
   void              Out(const int code,LPCSTR ip,LPCSTR msg,...) const;
   //---- get group by index
   int               MapGroup(const int index,char *group);
   //---- check by antiflood
   int               CheckFlooder(LPCSTR ip);
   //---- sort
   static int        FloodersSorts(const void *param1,const void *param2);
  };

extern CProcessor ExtProcessor;
//+------------------------------------------------------------------+
