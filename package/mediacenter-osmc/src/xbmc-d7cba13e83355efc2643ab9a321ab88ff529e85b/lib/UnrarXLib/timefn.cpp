#include "rar.hpp"

RarTime::RarTime()
{
  Reset();
}

#ifdef _WIN_32
RarTime& RarTime::operator =(FILETIME &ft)
{
  FILETIME lft,zft;
  FileTimeToLocalFileTime(&ft,&lft);
  SYSTEMTIME st;
  FileTimeToSystemTime(&lft,&st);
  rlt.Year=st.wYear;
  rlt.Month=st.wMonth;
  rlt.Day=st.wDay;
  rlt.Hour=st.wHour;
  rlt.Minute=st.wMinute;
  rlt.Second=st.wSecond;
  rlt.wDay=st.wDayOfWeek;
  rlt.yDay=rlt.Day-1;
  for (int I=1;I<rlt.Month;I++)
  {
    static int mdays[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    rlt.yDay+=mdays[I-1];
  }
  if (rlt.Month>2 && IsLeapYear(rlt.Year))
    rlt.yDay++;

  st.wMilliseconds=0;
  SystemTimeToFileTime(&st,&zft);
  rlt.Reminder=lft.dwLowDateTime-zft.dwLowDateTime;
  return(*this);
}


void RarTime::GetWin32(FILETIME *ft)
{
  SYSTEMTIME st;
  st.wYear=rlt.Year;
  st.wMonth=rlt.Month;
  st.wDay=rlt.Day;
  st.wHour=rlt.Hour;
  st.wMinute=rlt.Minute;
  st.wSecond=rlt.Second;
  st.wMilliseconds=0;
  FILETIME lft;
  SystemTimeToFileTime(&st,&lft);
  lft.dwLowDateTime+=rlt.Reminder;
  if (lft.dwLowDateTime<rlt.Reminder)
    lft.dwHighDateTime++;
  LocalFileTimeToFileTime(&lft,ft);
}
#endif


#if defined(_UNIX) || defined(_EMX)
RarTime& RarTime::operator =(time_t ut)
{
  struct tm t;
  localtime_r(&ut, &t);

  rlt.Year=t.tm_year+1900;
  rlt.Month=t.tm_mon+1;
  rlt.Day=t.tm_mday;
  rlt.Hour=t.tm_hour;
  rlt.Minute=t.tm_min;
  rlt.Second=t.tm_sec;
  rlt.Reminder=0;
  rlt.wDay=t.tm_wday;
  rlt.yDay=t.tm_yday;
  return(*this);
}


time_t RarTime::GetUnix()
{
  struct tm t;

  t.tm_sec=rlt.Second;
  t.tm_min=rlt.Minute;
  t.tm_hour=rlt.Hour;
  t.tm_mday=rlt.Day;
  t.tm_mon=rlt.Month-1;
  t.tm_year=rlt.Year-1900;
  t.tm_isdst=-1;
  return(mktime(&t));
}
#endif


Int64 RarTime::GetRaw()
{
  if (!IsSet())
    return(0);
#ifdef _WIN_32
  FILETIME ft;
  GetWin32(&ft);
  return(int32to64(ft.dwHighDateTime,ft.dwLowDateTime));
#elif defined(_UNIX) || defined(_EMX)
  time_t ut=GetUnix();
  return(int32to64(0,ut)*10000000+rlt.Reminder);
#else
  return(0);
#endif
}


#ifndef SFX_MODULE
void RarTime::SetRaw(Int64 RawTime)
{
#ifdef _WIN_32
  FILETIME ft;
  ft.dwHighDateTime=int64to32(RawTime>>32);
  ft.dwLowDateTime=int64to32(RawTime);
  *this=ft;
#elif defined(_UNIX) || defined(_EMX)
  time_t ut=int64to32(RawTime/10000000);
  *this=ut;
  rlt.Reminder=int64to32(RawTime%10000000);
#endif
}
#endif


bool RarTime::operator == (RarTime &rt)
{
  return(rlt.Year==rt.rlt.Year && rlt.Month==rt.rlt.Month &&
         rlt.Day==rt.rlt.Day && rlt.Hour==rt.rlt.Hour &&
         rlt.Minute==rt.rlt.Minute && rlt.Second==rt.rlt.Second &&
         rlt.Reminder==rt.rlt.Reminder);
}


bool RarTime::operator < (RarTime &rt)
{
  return(GetRaw()<rt.GetRaw());
}


bool RarTime::operator <= (RarTime &rt)
{
  return(*this<rt || *this==rt);
}


bool RarTime::operator > (RarTime &rt)
{
  return(GetRaw()>rt.GetRaw());
}


bool RarTime::operator >= (RarTime &rt)
{
  return(*this>rt || *this==rt);
}


uint RarTime::GetDos()
{
  uint DosTime=(rlt.Second/2)|(rlt.Minute<<5)|(rlt.Hour<<11)|
               (rlt.Day<<16)|(rlt.Month<<21)|((rlt.Year-1980)<<25);
  return(DosTime);
}


void RarTime::SetDos(uint DosTime)
{
  rlt.Second=(DosTime & 0x1f)*2;
  rlt.Minute=(DosTime>>5) & 0x3f;
  rlt.Hour=(DosTime>>11) & 0x1f;
  rlt.Day=(DosTime>>16) & 0x1f;
  rlt.Month=(DosTime>>21) & 0x0f;
  rlt.Year=(DosTime>>25)+1980;
  rlt.Reminder=0;
}


#if !defined(GUI) || !defined(SFX_MODULE)
void RarTime::GetText(char *DateStr,bool FullYear)
{
  if (FullYear)
    sprintf(DateStr,"%02u-%02u-%u %02u:%02u",rlt.Day,rlt.Month,rlt.Year,rlt.Hour,rlt.Minute);
  else
    sprintf(DateStr,"%02u-%02u-%02u %02u:%02u",rlt.Day,rlt.Month,rlt.Year%100,rlt.Hour,rlt.Minute);
}
#endif


#ifndef SFX_MODULE
void RarTime::SetIsoText(char *TimeText)
{
  int Field[6];
  memset(Field,0,sizeof(Field));
  for (int DigitCount=0;*TimeText!=0;TimeText++)
    if (isdigit(*TimeText))
    {
      unsigned int FieldPos=DigitCount<4 ? 0:(DigitCount-4)/2+1;
      if (FieldPos<sizeof(Field)/sizeof(Field[0]))
        Field[FieldPos]=Field[FieldPos]*10+*TimeText-'0';
      DigitCount++;
    }
  rlt.Second=Field[5];
  rlt.Minute=Field[4];
  rlt.Hour=Field[3];
  rlt.Day=Field[2]==0 ? 1:Field[2];
  rlt.Month=Field[1]==0 ? 1:Field[1];
  rlt.Year=Field[0];
  rlt.Reminder=0;
}
#endif


#ifndef SFX_MODULE
void RarTime::SetAgeText(char *TimeText)
{
  uint Seconds=0,Value=0;
  for (int I=0;TimeText[I]!=0;I++)
  {
    int Ch=TimeText[I];
    if (isdigit(Ch))
      Value=Value*10+Ch-'0';
    else
    {
      switch(toupper(Ch))
      {
        case 'D':
          Seconds+=Value*24*3600;
          break;
        case 'H':
          Seconds+=Value*3600;
          break;
        case 'M':
          Seconds+=Value*60;
          break;
        case 'S':
          Seconds+=Value;
          break;
      }
      Value=0;
    }
  }
  SetCurrentTime();
  Int64 RawTime=GetRaw();
  SetRaw(RawTime-int32to64(0,Seconds)*10000000);
}
#endif


#ifndef SFX_MODULE
void RarTime::SetCurrentTime()
{
#ifdef _WIN_32
  FILETIME ft;
  SYSTEMTIME st;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st,&ft);
  *this=ft;
#else
  time_t st;
  time(&st);
  *this=st;
#endif
}
#endif


#if !defined(SFX_MODULE) && !defined(_WIN_CE)
const char *GetMonthName(int Month)
{
#ifdef SILENT
  return("");
#else
  static MSGID MonthID[]={
         MMonthJan,MMonthFeb,MMonthMar,MMonthApr,MMonthMay,MMonthJun,
         MMonthJul,MMonthAug,MMonthSep,MMonthOct,MMonthNov,MMonthDec
  };
  return(St(MonthID[Month]));
#endif
}
#endif


bool IsLeapYear(int Year)
{
  return((Year&3)==0 && (Year%100!=0 || Year%400==0));
}
