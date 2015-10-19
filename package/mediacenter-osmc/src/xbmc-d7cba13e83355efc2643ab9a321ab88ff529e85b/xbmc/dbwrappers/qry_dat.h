/**********************************************************************
 * Copyright (c) 2004, Leo Seib, Hannover
 *
 * Project:Dataset C++ Dynamic Library
 * Module: FieldValue class and result sets classes header file
 * Author: Leo Seib      E-Mail: leoseib@web.de
 * Begin: 5/04/2002
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **********************************************************************/

#ifndef _QRYDAT_H
#define _QRYDAT_H

#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <stdint.h>

namespace dbiplus {

enum fType { 
	ft_String,
	ft_Boolean,
	ft_Char,
	ft_WChar,
	ft_WideString,
	ft_Short,
	ft_UShort,
	ft_Int,
	ft_UInt,
	ft_Float,
	ft_Double,
	ft_LongDouble,
  ft_Int64,
	ft_Object
    };



class field_value {
private:
  fType field_type;
  std::string str_value;
  union {
    bool   bool_value;
    char   char_value;
    short  short_value;
    unsigned short ushort_value;
    int   int_value;
    unsigned int uint_value;
    float  float_value;
    double double_value;
    int64_t int64_value;
    void   *object_value;
  } ;

  bool is_null;

public:
  field_value();
  field_value(const char *s);
  field_value(const bool b);
  field_value(const char c);
  field_value(const short s);
  field_value(const unsigned short us);
  field_value(const int l);
  field_value(const unsigned int ul);
  field_value(const float f);
  field_value(const double d);
  field_value(const int64_t i);
  field_value(const field_value & fv);
  ~field_value();

  fType get_fType() const {return field_type;}
  bool get_isNull() const {return is_null;}
  std::string get_asString() const;
  bool get_asBool() const;
  char get_asChar() const;
  short get_asShort() const;
  unsigned short get_asUShort() const;
  int get_asInt() const;
  unsigned int get_asUInt() const;
  float get_asFloat() const;
  double get_asDouble() const;
  int64_t get_asInt64() const;

  field_value& operator= (const char *s)
    {set_asString(s); return *this;}
  field_value& operator= (const std::string &s)
    {set_asString(s); return *this;}
  field_value& operator= (const bool b)
    {set_asBool(b); return *this;}
  field_value& operator= (const short s)
    {set_asShort(s); return *this;}
  field_value& operator= (const unsigned short us)
    {set_asUShort(us); return *this;}
  field_value& operator= (const int l)
    {set_asInt(l); return *this;}
  field_value& operator= (const unsigned int l)
    {set_asUInt(l); return *this;}
  field_value& operator= (const float f)
    {set_asFloat(f); return *this;}
  field_value& operator= (const double d)
    {set_asDouble(d); return *this;}
  field_value& operator= (const int64_t i)
    {set_asInt64(i); return *this;}
  field_value& operator= (const field_value & fv);
  
  //class ostream;
  friend std::ostream& operator<< (std::ostream& os, const field_value &fv)
  {switch (fv.get_fType()) {
    case ft_String: {
      return os << fv.get_asString();
      break;
    }
    case ft_Boolean:{
      return os << fv.get_asBool();
      break;     
    }
    case ft_Char: {
      return os << fv.get_asChar();
      break;
    }
    case ft_Short: {
      return os << fv.get_asShort();
      break;
    }
    case ft_UShort: {
      return os << fv.get_asUShort();
      break;
    }
    case ft_Int: {
      return os << fv.get_asInt();
      break;
    }
    case ft_UInt: {
      return os << fv.get_asUInt();
      break;
    }
    case ft_Float: {
      return os << fv.get_asFloat();
      break;
    }
    case ft_Double: {
      return os << fv.get_asDouble();
      break;
    }
    case ft_Int64: {
      return os << fv.get_asInt64();
      break;
    }
    default: {
      return os;
      break;
    }
  }
  }

  void set_isNull(){is_null=true;}
  void set_asString(const char *s);
  void set_asString(const std::string & s);
  void set_asBool(const bool b);
  void set_asChar(const char c);
  void set_asShort(const short s);
  void set_asUShort(const unsigned short us);
  void set_asInt(const int l);
  void set_asUInt(const unsigned int l);
  void set_asFloat(const float f);
  void set_asDouble(const double d);
  void set_asInt64(const int64_t i);

  fType get_field_type();
  std::string gft();
};

struct field_prop {
  std::string name,display_name;
  fType type;
  std::string field_table; //?
  bool read_only;
  unsigned int field_len;
  unsigned int field_flags;
  int idx;
};

struct field {
  field_prop props;
  field_value val;
}; 


typedef std::vector<field> Fields;
typedef std::vector<field_value> sql_record;
typedef std::vector<field_prop> record_prop;
typedef std::vector<sql_record*> query_data;
typedef field_value variant;

//typedef Fields::iterator fld_itor;
typedef sql_record::iterator rec_itor;
typedef record_prop::iterator recprop_itor;
typedef query_data::iterator qry_itor;

class result_set
{
public:
  result_set()
  {
  };
  ~result_set()
  {
    clear();
  };
  void clear()
  {
    for (unsigned int i = 0; i < records.size(); i++)
      if (records[i])
        delete records[i];
    records.clear();
    record_header.clear();
  };

  record_prop record_header;
  query_data records;
};

} // namespace

#endif
