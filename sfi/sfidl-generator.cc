/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <glib-extra.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <vector>
#include <map>
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"

using namespace Sfidl;
using namespace std;

class CodeGenerator {
protected:
  const Parser& parser;
  const Options& options;

  string makeNamespaceSubst (const string& name);
  string makeLowerName (const string& name, char seperator = '_');
  string makeUpperName (const string& name);
  string makeMixedName (const string& name);
  string makeLMixedName (const string& name);

  CodeGenerator(const Parser& parser) : parser (parser), options (*Options::the()) {
  }

public:
  virtual void run () = 0;
};

class CodeGeneratorC : public CodeGenerator {
protected:
  
  void printInfoStrings (const string& name, const map<string,string>& infos);
  void printProcedure (const MethodDef& mdef, const string& className = "");

  string makeParamSpec (const ParamDef& pdef);
  string makeGTypeName (const string& name);
  string createTypeCode (const string& type, const string& name, int model);
  
public:
  CodeGeneratorC(const Parser& parser) : CodeGenerator(parser) {
  }
  void run ();
};

string CodeGenerator::makeNamespaceSubst (const string& name)
{
  if(name.substr (0, options.namespaceCut.length ()) == options.namespaceCut)
    return options.namespaceAdd + name.substr (options.namespaceCut.length ());
  else
    return name; /* pattern not matched */
}

string CodeGenerator::makeLowerName (const string& name, char seperator)
{
  bool lastupper = true, upper = true, lastunder = true;
  string::const_iterator i;
  string result;
  string sname = makeNamespaceSubst (name);
  
  for(i = sname.begin(); i != sname.end(); i++)
    {
      lastupper = upper;
      upper = isupper(*i);
      if (!lastupper && upper && !lastunder)
	{
	  result += seperator;
	  lastunder = true;
	}
      if(*i == ':' || *i == '_')
	{
	  if(!lastunder)
	    {
	      result += seperator;
	      lastunder = true;
	    }
	}
      else
	{
	  result += tolower(*i);
	  lastunder = false;
	}
    }
  return result;
}

string CodeGenerator::makeUpperName (const string& name)
{
  string lname = makeLowerName (name);
  string result;
  string::const_iterator i;
  
  for(i = lname.begin(); i != lname.end(); i++)
    result += toupper(*i);
  return result;
}

string CodeGenerator::makeMixedName (const string& name)
{
  bool lastupper = true, upper = true, lastunder = true;
  string::const_iterator i;
  string result;
  string sname = makeNamespaceSubst (name);
  
  for(i = sname.begin(); i != sname.end(); i++)
    {
      lastupper = upper;
      upper = isupper(*i);
      if (!lastupper && upper && !lastunder)
	{
	  lastunder = true;
	}
      if(*i == ':' || *i == '_')
	{
	  if(!lastunder)
	    {
	      lastunder = true;
	    }
	}
      else
	{
	  if(lastunder)
	    result += toupper (*i);
	  else
	    result += tolower (*i);
	  lastunder = false;
	}
    }
  return result;
}

string CodeGenerator::makeLMixedName (const string& name)
{
  string result = makeMixedName (name);

  if (!result.empty()) result[0] = tolower(result[0]);
  return result;
}

string CodeGeneratorC::makeGTypeName(const string& name)
{
  return makeUpperName (NamespaceHelper::namespaceOf (name)
                      + "::Type" + NamespaceHelper::nameOf(name));
}

string CodeGeneratorC::makeParamSpec(const ParamDef& pdef)
{
  string pspec;
  
  if (parser.isEnum (pdef.type))
    {
      pspec = "sfidl_pspec_Choice";
      if (pdef.args == "")
	pspec += "_default (\"" + pdef.name + "\",";
      else
	pspec += " (\"" + pdef.name + "\"," + pdef.args + ",";
      pspec += makeLowerName (pdef.type) + "_values)";
    }
  else if (parser.isRecord (pdef.type))
    {
      pspec = "sfidl_pspec_BoxedRec";
      if (pdef.args == "")
	pspec += "_default (\"" + pdef.name + "\",";
      else
	pspec += " (\"" + pdef.name + "\"," + pdef.args + ",";
      pspec += makeLowerName (pdef.type) + "_fields)";
    }
  else if (parser.isSequence (pdef.type))
    {
      const SequenceDef& sdef = parser.findSequence (pdef.type);
      pspec = "sfidl_pspec_BoxedSeq";
      if (pdef.args == "")
	pspec += "_default (\"" + pdef.name + "\",";
      else
	pspec += " (\"" + pdef.name + "\"," + pdef.args + ",";
      pspec += makeLowerName (pdef.type) + "_content)";
    }
  else
    {
      pspec = "sfidl_pspec_" + pdef.pspec;
      if (pdef.args == "")
	pspec += "_default (\"" + pdef.name + "\")";
      else
	pspec += " (\"" + pdef.name + "\"," + pdef.args + ")";
    }
  return pspec;
}

void CodeGeneratorC::printInfoStrings (const string& name, const map<string,string>& infos)
{
  printf("static const gchar *%s[] = {\n", name.c_str());

  map<string,string>::const_iterator ii;
  for (ii = infos.begin(); ii != infos.end(); ii++)
    printf("  \"%s=%s\",\n", ii->first.c_str(), ii->second.c_str());

  printf("  NULL,\n");
  printf("};\n");
}

#define MODEL_ARG         1
#define MODEL_RET         2
#define MODEL_ARRAY       3
#define MODEL_FREE        4
#define MODEL_COPY        5
#define MODEL_NEW         6
#define MODEL_FROM_VALUE  7
#define MODEL_TO_VALUE    8
#define MODEL_VCALL       9
#define MODEL_VCALL_ARG   10
#define MODEL_VCALL_CONV  11
#define MODEL_VCALL_CFREE 12
#define MODEL_VCALL_RET   13
#define MODEL_VCALL_RCONV 14

string CodeGeneratorC::createTypeCode(const string& type, const string &name, int model)
{
  g_return_val_if_fail (model != MODEL_ARG || model != MODEL_RET ||
                        model != MODEL_ARRAY || name == "", "bad");
  g_return_val_if_fail (model != MODEL_FREE || model != MODEL_COPY || model != MODEL_NEW ||
                        model != MODEL_FROM_VALUE || model != MODEL_TO_VALUE || name != "", "bad");

  if (parser.isRecord (type) || parser.isSequence (type))
    {
      if (model == MODEL_ARG)         return makeMixedName (type)+"*";
      if (model == MODEL_RET)         return makeMixedName (type)+"*";
      if (model == MODEL_ARRAY)       return makeMixedName (type)+"**";
      if (model == MODEL_FREE)        return makeLowerName (type)+"_free ("+name+")";
      if (model == MODEL_COPY)        return makeLowerName (type)+"_copy_shallow ("+name+")";
      if (model == MODEL_NEW)         return name + " = " + makeLowerName (type)+"_new ()";

      if (parser.isSequence (type))
      {
	if (model == MODEL_TO_VALUE)
	  return "sfi_value_seq (" + makeLowerName (type)+"_to_seq ("+name+"))";
	if (model == MODEL_FROM_VALUE) 
	  return makeLowerName (type)+"_from_seq (sfi_value_get_seq ("+name+"))";
	if (model == MODEL_VCALL) 
	  return "sfi_glue_vcall_seq";
	if (model == MODEL_VCALL_ARG) 
	  return "'Q', "+name+",";
	if (model == MODEL_VCALL_CONV) 
	  return makeLowerName (type)+"_to_seq ("+name+")";
	if (model == MODEL_VCALL_CFREE) 
	  return "sfi_seq_unref ("+name+")";
	if (model == MODEL_VCALL_RET) 
	  return "SfiSeq*";
	if (model == MODEL_VCALL_RCONV) 
	  return makeLowerName (type)+"_from_seq ("+name+")";
      }
      else
      {
	if (model == MODEL_TO_VALUE)   
	  return "sfi_value_rec (" + makeLowerName (type)+"_to_rec ("+name+"))";
	if (model == MODEL_FROM_VALUE)
	  return makeLowerName (type)+"_from_rec (sfi_value_get_rec ("+name+"))";
	if (model == MODEL_VCALL) 
	  return "sfi_glue_vcall_rec";
	if (model == MODEL_VCALL_ARG) 
	  return "'R', "+name+",";
	if (model == MODEL_VCALL_CONV) 
	  return makeLowerName (type)+"_to_rec ("+name+")";
	if (model == MODEL_VCALL_CFREE) 
	  return "sfi_rec_unref ("+name+")";
	if (model == MODEL_VCALL_RET) 
	  return "SfiRec*";
	/* FIXME: this does change ownership - no longer GC'ed */
	if (model == MODEL_VCALL_RCONV) 
	  return makeLowerName (type)+"_from_rec ("+name+")";
      }
    }
  else if (parser.isEnum (type))
    {
      if (model == MODEL_ARG)         return makeMixedName (type);
      if (model == MODEL_RET)         return makeMixedName (type);
      if (model == MODEL_ARRAY)       return makeMixedName (type) + "*";
      if (model == MODEL_FREE)        return "";
      if (model == MODEL_COPY)        return name;
      if (model == MODEL_NEW)         return "";
      if (options.generateBoxedTypes)
	{
	  if (model == MODEL_TO_VALUE)
	    return "sfi_value_choice_genum ("+name+", "+makeGTypeName(type)+")";
	  if (model == MODEL_FROM_VALUE) 
	    return "sfi_choice2enum (sfi_value_get_choice ("+name+"), "+makeGTypeName(type)+")";
	}
      else /* client code */
	{
	  if (model == MODEL_TO_VALUE)
	    return "sfi_value_choice (" + makeLowerName (type) + "_to_choice ("+name+"))";
	  if (model == MODEL_FROM_VALUE) 
	    return makeLowerName (type) + "_from_choice (sfi_value_get_choice ("+name+"))";
	}
      if (model == MODEL_VCALL)       return "sfi_glue_vcall_choice";
      if (model == MODEL_VCALL_ARG)   return "'c', "+makeLowerName (type)+"_to_choice ("+name+"),";
      if (model == MODEL_VCALL_CONV)  return "";
      if (model == MODEL_VCALL_CFREE) return "";
      if (model == MODEL_VCALL_RET)   return makeMixedName (type);
      if (model == MODEL_VCALL_RCONV) return makeLowerName (type)+"_from_choice ("+name+")";
    }
  else if (parser.isClass (type) || type == "Proxy")
    {
      /*
       * FIXME: we're currently not using the type of the proxy anywhere
       * it might for instance be worthwile being able to ensure that if
       * we're expecting a "SfkServer" object, we will have one
       */
      if (model == MODEL_ARG)         return "SfiProxy";
      if (model == MODEL_RET)         return "SfiProxy";
      if (model == MODEL_ARRAY)       return "SfiProxy*";
      if (model == MODEL_FREE)        return "";
      if (model == MODEL_COPY)        return name;
      if (model == MODEL_NEW)         return "";
      if (model == MODEL_TO_VALUE)    return "sfi_value_proxy ("+name+")";
      if (model == MODEL_FROM_VALUE)  return "sfi_value_get_proxy ("+name+")";
      if (model == MODEL_VCALL)       return "sfi_glue_vcall_proxy";
      if (model == MODEL_VCALL_ARG)   return "'p', "+name+",";
      if (model == MODEL_VCALL_CONV)  return "";
      if (model == MODEL_VCALL_CFREE) return "";
      if (model == MODEL_VCALL_RET)   return "SfiProxy";
      if (model == MODEL_VCALL_RCONV) return name;
    }
  else if (type == "String")
    {
      if (model == MODEL_ARG)         return "gchar*";
      if (model == MODEL_RET)         return "gchar*";
      if (model == MODEL_ARRAY)       return "gchar**";
      if (model == MODEL_FREE)        return "g_free (" + name + ")";
      if (model == MODEL_COPY)        return "g_strdup (" + name + ")";;
      if (model == MODEL_NEW)         return "";
      if (model == MODEL_TO_VALUE)    return "sfi_value_string ("+name+")";
      // FIXME: do we want sfi_value_dup_string?
      if (model == MODEL_FROM_VALUE)  return "g_strdup (sfi_value_get_string ("+name+"))";
      if (model == MODEL_VCALL)       return "sfi_glue_vcall_string";
      if (model == MODEL_VCALL_ARG)   return "'s', "+name+",";
      if (model == MODEL_VCALL_CONV)  return "";
      if (model == MODEL_VCALL_CFREE) return "";
      if (model == MODEL_VCALL_RET)   return "gchar*";
      if (model == MODEL_VCALL_RCONV) return name;
    }
  else if (type == "BBlock")
    {
      if (model == MODEL_ARG)         return "SfiBBlock*";
      if (model == MODEL_RET)         return "SfiBBlock*";
      if (model == MODEL_ARRAY)       return "SfiBBlock**";
      if (model == MODEL_FREE)        return "sfi_bblock_unref (" + name + ")";
      if (model == MODEL_COPY)        return "sfi_bblock_ref (" + name + ")";;
      if (model == MODEL_NEW)         return name + " = sfi_bblock_new ()";
      if (model == MODEL_TO_VALUE)    return "sfi_value_bblock ("+name+")";
      if (model == MODEL_FROM_VALUE)  return "sfi_bblock_ref (sfi_value_get_bblock ("+name+"))";
      if (model == MODEL_VCALL)       return "sfi_glue_vcall_bblock";
      if (model == MODEL_VCALL_ARG)   return "'B', "+name+",";
      if (model == MODEL_VCALL_CONV)  return "";
      if (model == MODEL_VCALL_CFREE) return "";
      if (model == MODEL_VCALL_RET)   return "SfiBBlock*";
      if (model == MODEL_VCALL_RCONV) return name;
    }
  else if (type == "FBlock")
    {
      if (model == MODEL_ARG)         return "SfiFBlock*";
      if (model == MODEL_RET)         return "SfiFBlock*";
      if (model == MODEL_ARRAY)       return "SfiFBlock**";
      if (model == MODEL_FREE)        return "sfi_fblock_unref (" + name + ")";
      if (model == MODEL_COPY)        return "sfi_fblock_ref (" + name + ")";;
      if (model == MODEL_NEW)         return name + " = sfi_fblock_new ()";
      if (model == MODEL_TO_VALUE)    return "sfi_value_fblock ("+name+")";
      if (model == MODEL_FROM_VALUE)  return "sfi_fblock_ref (sfi_value_get_fblock ("+name+"))";
      if (model == MODEL_VCALL)       return "sfi_glue_vcall_fblock";
      if (model == MODEL_VCALL_ARG)   return "'F', "+name+",";
      if (model == MODEL_VCALL_CONV)  return "";
      if (model == MODEL_VCALL_CFREE) return "";
      if (model == MODEL_VCALL_RET)   return "SfiFBlock*";
      if (model == MODEL_VCALL_RCONV) return name;
    }
  else if (type == "PSpec")
    {
      /* FIXME: review this for correctness */
      if (model == MODEL_ARG)         return "GParamSpec*";
      if (model == MODEL_RET)         return "GParamSpec*";
      if (model == MODEL_ARRAY)       return "GParamSpec**";
      if (model == MODEL_FREE)        return "sfi_pspec_unref (" + name + ")";
      if (model == MODEL_COPY)        return "sfi_pspec_ref (" + name + ")";;
      /* no new: users of this need to be knowing to initialize things themselves */
      if (model == MODEL_NEW)         return "";
      if (model == MODEL_TO_VALUE)    return "sfi_value_pspec ("+name+")";
      if (model == MODEL_FROM_VALUE)  return "sfi_pspec_ref (sfi_value_get_pspec ("+name+"))";
      if (model == MODEL_VCALL)       return "sfi_glue_vcall_pspec";
      if (model == MODEL_VCALL_ARG)   return "'?', "+name+",";
      if (model == MODEL_VCALL_CONV)  return "";
      if (model == MODEL_VCALL_CFREE) return "";
      if (model == MODEL_VCALL_RET)   return "SfiPSpec*";
      if (model == MODEL_VCALL_RCONV) return name;
    }
  else if (type == "Rec")
    {
      /* FIXME: review this for correctness */
      if (model == MODEL_ARG)         return "SfiRec*";
      if (model == MODEL_RET)         return "SfiRec*";
      if (model == MODEL_ARRAY)       return "SfiRec**";
      if (model == MODEL_FREE)        return "sfi_rec_unref (" + name + ")";
      if (model == MODEL_COPY)        return "sfi_rec_ref (" + name + ")";;
      if (model == MODEL_NEW)         return name + " = sfi_rec_new ()";
      if (model == MODEL_TO_VALUE)    return "sfi_value_rec ("+name+")";
      if (model == MODEL_FROM_VALUE)  return "sfi_rec_ref (sfi_value_get_rec ("+name+"))";
      if (model == MODEL_VCALL)       return "sfi_glue_vcall_rec";
      if (model == MODEL_VCALL_ARG)   return "'?', "+name+",";
      if (model == MODEL_VCALL_CONV)  return "";
      if (model == MODEL_VCALL_CFREE) return "";
      if (model == MODEL_VCALL_RET)   return "SfiRec*";
      if (model == MODEL_VCALL_RCONV) return name;
    }
  else
    {
      string sfi = (type == "void") ? "" : "Sfi"; /* there is no such thing as an SfiVoid */

      if (model == MODEL_ARG)         return sfi + type;
      if (model == MODEL_RET)         return sfi + type;
      if (model == MODEL_ARRAY)       return sfi + type + "*";
      if (model == MODEL_FREE)        return "";
      if (model == MODEL_COPY)        return name;
      if (model == MODEL_NEW)         return "";
      if (model == MODEL_TO_VALUE)    return "sfi_value_" + makeLowerName(type) + " ("+name+")";
      if (model == MODEL_FROM_VALUE)  return "sfi_value_get_" + makeLowerName(type) + " ("+name+")";
      if (model == MODEL_VCALL)       return "sfi_glue_vcall_" + makeLowerName(type);
      if (model == MODEL_VCALL_ARG)
	{
	  if (type == "Real")	      return "'r', "+name+",";
	  if (type == "Bool")	      return "'b', "+name+",";
	  if (type == "Int")	      return "'i', "+name+",";
	  if (type == "Num")	      return "'n', "+name+",";
	}
      if (model == MODEL_VCALL_CONV)  return "";
      if (model == MODEL_VCALL_CFREE) return "";
      if (model == MODEL_VCALL_RET)   return sfi + type;
      if (model == MODEL_VCALL_RCONV) return name;
    }
  return "*createTypeCode*unknown*";
}

void CodeGeneratorC::printProcedure (const MethodDef& mdef, const string& className)
{
  vector<ParamDef>::const_iterator pi;
  /* FIXME:
   *  - enum (choice) arguments seem broken
   */
  string mname, dname;
  
  if (className == "")
    {
      mname = makeLowerName(mdef.name);
      dname = makeLowerName(mdef.name, '-');
    }
  else
    {
      mname = makeLowerName(className) + "_" + makeLowerName(mdef.name);
      dname = makeMixedName(className) + "+" + makeLowerName(mdef.name, '-');
    }

  bool first = true;
  string ret = createTypeCode(mdef.result.type, "", MODEL_RET);
  printf("%s %s (", ret.c_str(), mname.c_str());
  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    {
      string arg = createTypeCode(pi->type, "", MODEL_ARG);
      if(!first) printf(", ");
      first = false;
      printf("%s %s", arg.c_str(), pi->name.c_str());
    }
  printf(") {\n");

  string vret = createTypeCode(mdef.result.type, "", MODEL_VCALL_RET);
  if (mdef.result.type != "void")
    printf("  %s _retval;\n", vret.c_str());

  map<string, string> cname;
  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    {
      string conv = createTypeCode (pi->type, pi->name, MODEL_VCALL_CONV);
      if (conv != "")
	{
	  cname[pi->name] = pi->name + "__c";

	  string arg = createTypeCode(pi->type, "", MODEL_ARG);
	  printf("  %s %s__c = %s;\n", arg.c_str(), pi->name.c_str(), conv.c_str());
	}
      else
	cname[pi->name] = pi->name;
    }

  printf("  ");
  if (mdef.result.type != "void")
    printf("_retval = ");
  string vcall = createTypeCode(mdef.result.type, "", MODEL_VCALL);
  printf("%s (\"%s\", ", vcall.c_str(), dname.c_str());

  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    printf("%s ", createTypeCode(pi->type, cname[pi->name], MODEL_VCALL_ARG).c_str());
  printf("0);\n");

  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    {
      string cfree = createTypeCode (pi->type, cname[pi->name], MODEL_VCALL_CFREE);
      if (cfree != "")
	printf("  %s;\n", cfree.c_str());
    }

  if (mdef.result.type != "void")
    {
      string rconv = createTypeCode (mdef.result.type, "_retval", MODEL_VCALL_RCONV);
      printf("  return %s;\n", rconv.c_str());
    }
  printf("}\n\n");
}

void CodeGeneratorC::run ()
{
  vector<SequenceDef>::const_iterator si;
  vector<RecordDef>::const_iterator ri;
  vector<EnumDef>::const_iterator ei;
  vector<ParamDef>::const_iterator pi;
  vector<ClassDef>::const_iterator ci;
  vector<MethodDef>::const_iterator mi;
 
  if (options.generateConstant)
    {
      vector<ConstantDef>::const_iterator ci;
      for (ci = parser.getConstants().begin(); ci != parser.getConstants().end(); ci++)
	{
	  string uname = makeUpperName(ci->name);
	  printf("#define %s ", uname.c_str());
	  switch (ci->type) {
	    case ConstantDef::tString: printf("\"%s\"\n", ci->str.c_str());
	      break;
	    case ConstantDef::tFloat: printf("%f\n", ci->f);
	      break;
	    case ConstantDef::tInt: printf("%d\n", ci->i);
	      break;
	  }
	}
      printf("\n");
    }
  if (options.generateTypeH)
    {
      for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	{
	  string mname = makeMixedName (si->name);
	  printf("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
	}
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  string mname = makeMixedName (ri->name);
	  printf("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
	}
      for(ei = parser.getEnums().begin(); ei != parser.getEnums().end(); ei++)
	{
	  string mname = makeMixedName (ei->name);
	  printf("\ntypedef enum {\n");
	  for (vector<EnumComponent>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
	    {
	      string ename = makeUpperName (NamespaceHelper::namespaceOf(ei->name) + ci->name);
	      printf("  %s = %d,\n", ename.c_str(), ci->value);
	    }
	  printf("} %s;\n", mname.c_str());
	}
      
      printf("\n");
      for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	{
	  string mname = makeMixedName (si->name.c_str());
	  string array = createTypeCode (si->content.type, "", MODEL_ARRAY);
	  string elements = si->content.name;
	  
	  printf("struct _%s {\n", mname.c_str());
	  printf("  guint n_%s;\n", elements.c_str ());
	  printf("  %s %s;\n", array.c_str(), elements.c_str());
	  printf("};\n");
	}
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  string mname = makeMixedName (ri->name.c_str());
	  
	  printf("struct _%s {\n", mname.c_str());
	  for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	    {
	      printf("  %s %s;\n", createTypeCode(pi->type, "", MODEL_ARG).c_str(), pi->name.c_str());
	    }
	  printf("};\n");
	}

      printf("\n");
      for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	{
	  string ret = createTypeCode (si->name, "", MODEL_RET);
	  string arg = createTypeCode (si->name, "", MODEL_ARG);
	  string element = createTypeCode (si->content.type, "", MODEL_ARG);
	  string lname = makeLowerName (si->name.c_str());
	  
	  printf("%s %s_new (void);\n", ret.c_str(), lname.c_str());
	  printf("void %s_append (%s seq, %s element);\n", lname.c_str(), arg.c_str(), element.c_str());
	  printf("%s %s_copy_shallow (%s seq);\n", ret.c_str(), lname.c_str(), arg.c_str());
	  printf("%s %s_from_seq (SfiSeq *sfi_seq);\n", ret.c_str(), lname.c_str());
	  printf("SfiSeq *%s_to_seq (%s seq);\n", lname.c_str(), arg.c_str());
	  printf("void %s_resize (%s seq, guint new_size);\n", lname.c_str(), arg.c_str());
	  printf("void %s_free (%s seq);\n", lname.c_str(), arg.c_str());
	  printf("\n");
	}
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  string ret = createTypeCode (ri->name, "", MODEL_RET);
	  string arg = createTypeCode (ri->name, "", MODEL_ARG);
	  string lname = makeLowerName (ri->name.c_str());
	  
	  printf("%s %s_new (void);\n", ret.c_str(), lname.c_str());
	  printf("%s %s_copy_shallow (%s rec);\n", ret.c_str(), lname.c_str(), arg.c_str());
	  printf("%s %s_from_rec (SfiRec *sfi_rec);\n", ret.c_str(), lname.c_str());
	  printf("SfiRec *%s_to_rec (%s rec);\n", lname.c_str(), arg.c_str());
	  printf("void %s_free (%s rec);\n", lname.c_str(), arg.c_str());
	  printf("\n");
	}
      printf("\n");
    }
  
  if (options.generateExtern)
    {
      for(ei = parser.getEnums().begin(); ei != parser.getEnums().end(); ei++)
	{
	  printf("extern SfiChoiceValues %s_values;\n", makeLowerName (ei->name).c_str());
	  if (options.generateBoxedTypes)
	    printf("extern GType %s;\n", makeGTypeName (ei->name).c_str());
	}
      
      for(ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
	printf("extern SfiRecFields %s_fields;\n",makeLowerName (ri->name).c_str());
        if (options.generateBoxedTypes)
	  printf("extern GType %s;\n", makeGTypeName (ri->name).c_str());
      }

      if (options.generateBoxedTypes)
      {
	for(si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	  printf("extern GType %s;\n", makeGTypeName (si->name).c_str());
      }
      printf("\n");
    }
  
  if (options.generateTypeC)
    {
      for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	{
	  string ret = createTypeCode (si->name, "", MODEL_RET);
	  string arg = createTypeCode (si->name, "", MODEL_ARG);
	  string element = createTypeCode (si->content.type, "", MODEL_ARG);
	  string elements = si->content.name;
	  string lname = makeLowerName (si->name.c_str());
	  string mname = makeMixedName (si->name.c_str());
	  
	  printf("%s\n", ret.c_str());
	  printf("%s_new (void)\n", lname.c_str());
	  printf("{\n");
	  printf("  return g_new0 (%s, 1);\n",mname.c_str());
	  printf("}\n\n");
	  
	  string elementCopy = createTypeCode (si->content.type, "element", MODEL_COPY);
	  printf("void\n");
	  printf("%s_append (%s seq, %s element)\n", lname.c_str(), arg.c_str(), element.c_str());
	  printf("{\n");
	  printf("  g_return_if_fail (seq != NULL);\n");
	  printf("\n");
	  printf("  seq->%s = g_realloc (seq->%s, "
		 "(seq->n_%s + 1) * sizeof (seq->%s[0]));\n",
		 elements.c_str(), elements.c_str(), elements.c_str(), elements.c_str());
	  printf("  seq->%s[seq->n_%s++] = %s;\n", elements.c_str(), elements.c_str(),
	         elementCopy.c_str());
	  printf("}\n\n");
	  
	  printf("%s\n", ret.c_str());
	  printf("%s_copy_shallow (%s seq)\n", lname.c_str(), arg.c_str());
	  printf("{\n");
	  printf("  %s seq_copy = NULL;\n", arg.c_str ());
          printf("  if (seq)\n");
          printf("    {\n");
	  printf("      guint i;\n");
	  printf("      seq_copy = %s_new ();\n", lname.c_str());
	  printf("      for (i = 0; i < seq->n_%s; i++)\n", elements.c_str());
	  printf("        %s_append (seq_copy, seq->%s[i]);\n", lname.c_str(), elements.c_str());
          printf("    }\n");
	  printf("  return seq_copy;\n");
	  printf("}\n\n");
	  
	  string elementFromValue = createTypeCode (si->content.type, "element", MODEL_FROM_VALUE);
	  printf("%s\n", ret.c_str());
	  printf("%s_from_seq (SfiSeq *sfi_seq)\n", lname.c_str());
	  printf("{\n");
	  printf("  %s seq;\n", arg.c_str());
	  printf("  guint i, length;\n");
	  printf("\n");
	  printf("  g_return_val_if_fail (sfi_seq != NULL, NULL);\n");
	  printf("\n");
	  printf("  length = sfi_seq_length (sfi_seq);\n");
	  printf("  seq = g_new0 (%s, 1);\n",mname.c_str());
	  printf("  seq->n_%s = length;\n", elements.c_str());
	  printf("  seq->%s = g_malloc (seq->n_%s * sizeof (seq->%s[0]));\n\n",
	         elements.c_str(), elements.c_str(), elements.c_str());
	  printf("  for (i = 0; i < length; i++)\n");
	  printf("  {\n");
	  printf("    GValue *element = sfi_seq_get (sfi_seq, i);\n");
	  printf("    seq->%s[i] = %s;\n", elements.c_str(), elementFromValue.c_str());
	  printf("  }\n");
	  printf("  return seq;\n");
	  printf("}\n\n");

	  string elementToValue = createTypeCode (si->content.type, "seq->" + elements + "[i]", MODEL_TO_VALUE);
	  printf("SfiSeq *\n");
	  printf("%s_to_seq (%s seq)\n", lname.c_str(), arg.c_str());
	  printf("{\n");
	  printf("  SfiSeq *sfi_seq;\n");
	  printf("  guint i;\n");
	  printf("\n");
	  printf("  g_return_val_if_fail (seq != NULL, NULL);\n");
	  printf("\n");
	  printf("  sfi_seq = sfi_seq_new ();\n");
	  printf("  for (i = 0; i < seq->n_%s; i++)\n", elements.c_str());
	  printf("  {\n");
	  printf("    GValue *element = %s;\n", elementToValue.c_str());
	  printf("    sfi_seq_append (sfi_seq, element);\n");
	  printf("    sfi_value_free (element);\n");        // FIXME: couldn't we have take_append
	  printf("  }\n");
	  printf("  return sfi_seq;\n");
	  printf("}\n\n");

	  string element_i_free = createTypeCode (si->content.type, "seq->" + elements + "[i]", MODEL_FREE);
	  printf("void\n");
	  printf("%s_resize (%s seq, guint new_size)\n", lname.c_str(), arg.c_str());
	  printf("{\n");
	  printf("  g_return_if_fail (seq != NULL);\n");
	  printf("\n");
	  if (element_i_free != "")
	    {
	      printf("  if (seq->n_%s > new_size)\n", elements.c_str());
	      printf("    {\n");
	      printf("      guint i;\n");
	      printf("      for (i = new_size; i < seq->n_%s; i++)\n", elements.c_str());
	      printf("        %s;\n", element_i_free.c_str());
	      printf("    }\n");
	    }
	  printf("\n");
	  printf("  seq->%s = g_realloc (seq->%s, new_size * sizeof (seq->%s[0]));\n",
                 elements.c_str(), elements.c_str(), elements.c_str(), elements.c_str());
	  printf("  if (new_size > seq->n_%s)\n", elements.c_str());
	  printf("    memset (&seq->%s[seq->n_%s], 0, sizeof(seq->%s[0]) * (new_size - seq->n_%s));\n",
                 elements.c_str(), elements.c_str(), elements.c_str(), elements.c_str());
	  printf("  seq->n_%s = new_size;\n", elements.c_str());
	  printf("}\n\n");

	  printf("void\n");
	  printf("%s_free (%s seq)\n", lname.c_str(), arg.c_str());
	  printf("{\n");
          printf("  if (seq)\n");
          printf("    {\n");
	  if (element_i_free != "")
	    {
	      printf("      guint i;\n");
	      printf("      for (i = 0; i < seq->n_%s; i++)\n", elements.c_str());
	      printf("        %s;\n", element_i_free.c_str());
	    }
	  printf("      g_free (seq);\n");
          printf("    }\n");
	  printf("}\n\n");
	  printf("\n");
	}
      for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  string ret = createTypeCode (ri->name, "", MODEL_RET);
	  string arg = createTypeCode (ri->name, "", MODEL_ARG);
	  string lname = makeLowerName (ri->name.c_str());
	  string mname = makeMixedName (ri->name.c_str());
	  
	  printf("%s\n", ret.c_str());
	  printf("%s_new (void)\n", lname.c_str());
	  printf("{\n");
	  printf("  %s rec = g_new0 (%s, 1);\n", arg.c_str(), mname.c_str());
	  for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	    {
	      string init =  createTypeCode(pi->type, "rec->" + pi->name, MODEL_NEW);
	      if (init != "") printf("  %s;\n",init.c_str());
	    }
	  printf("  return rec;\n");
	  printf("}\n\n");
	  
	  printf("%s\n", ret.c_str());
	  printf("%s_copy_shallow (%s rec)\n", lname.c_str(), arg.c_str());
	  printf("{\n");
	  printf("  %s rec_copy = NULL;\n", arg.c_str());
	  printf("  if (rec)\n");
	  printf("    {\n");
	  printf("      rec_copy = %s_new ();\n", lname.c_str());
	  for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	    {
	      string copy =  createTypeCode(pi->type, "rec->" + pi->name, MODEL_COPY);
	      printf("      rec_copy->%s = %s;\n", pi->name.c_str(), copy.c_str());
	    }
	  printf("    }\n");
	  printf("  return rec_copy;\n");
	  printf("}\n\n");
	  
	  printf("%s\n", ret.c_str());
	  printf("%s_from_rec (SfiRec *sfi_rec)\n", lname.c_str());
	  printf("{\n");
	  printf("  GValue *element;\n");
	  printf("  %s rec;\n", arg.c_str());
	  printf("\n");
	  printf("  g_return_val_if_fail (sfi_rec != NULL, NULL);\n");
	  printf("\n");
	  printf("  rec = g_new0 (%s, 1);\n", mname.c_str());
	  for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	    {
	      string elementFromValue = createTypeCode (pi->type, "element", MODEL_FROM_VALUE);
	      printf("  element = sfi_rec_get (sfi_rec, \"%s\");\n", pi->name.c_str());
	      printf("  rec->%s = %s;\n", pi->name.c_str(), elementFromValue.c_str());
	    }
	  printf("  return rec;\n");
	  printf("}\n\n");
	  
	  printf("SfiRec *\n");
	  printf("%s_to_rec (%s rec)\n", lname.c_str(), arg.c_str());
	  printf("{\n");
	  printf("  SfiRec *sfi_rec;\n");
	  printf("  GValue *element;\n");
	  printf("\n");
	  printf("  g_return_val_if_fail (rec != NULL, NULL);\n");
          printf("\n");
	  printf("  sfi_rec = sfi_rec_new ();\n");
	  for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	    {
	      string elementToValue = createTypeCode (pi->type, "rec->" + pi->name, MODEL_TO_VALUE);
	      printf("  element = %s;\n", elementToValue.c_str());
	      printf("  sfi_rec_set (sfi_rec, \"%s\", element);\n", pi->name.c_str());
	      printf("  sfi_value_free (element);\n");        // FIXME: couldn't we have take_set
	    }
	  printf("  return sfi_rec;\n");
	  printf("}\n\n");
	  
	  printf("void\n");
	  printf("%s_free (%s rec)\n", lname.c_str(), arg.c_str());
	  printf("{\n");
	  printf("  if (rec)\n");
	  printf("    {\n");
	  for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	    {
	      string free =  createTypeCode(pi->type, "rec->" + pi->name, MODEL_FREE);
	      if (free != "") printf("      %s;\n",free.c_str());
	    }
	  printf("      g_free (rec);\n");
	  printf("    }\n");
	  printf("}\n\n");
	  printf("\n");
	}
    }
  
  if (options.generateData)
    {
      for(ei = parser.getEnums().begin(); ei != parser.getEnums().end(); ei++)
	{
	  string name = makeLowerName (ei->name);
	  printf("static const GEnumValue %s_value[%d] = {\n",name.c_str(), ei->contents.size()+1);
	  for (vector<EnumComponent>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
	    {
	      string ename = makeUpperName (NamespaceHelper::namespaceOf(ei->name) + ci->name);
	      printf("  { %d, \"%s\", \"%s\" },\n", ci->value, ename.c_str(), ci->text.c_str());
	    }
	  printf("  { 0, NULL, NULL }\n");
	  printf("};\n");
	  printf("SfiChoiceValues %s_values = { %d, %s_value };\n", name.c_str(), ei->contents.size(), name.c_str());
	  if (options.generateBoxedTypes)
	    printf("GType %s = 0;\n", makeGTypeName (ei->name).c_str());
	  printf("\n");
	}

      if (options.generateBoxedTypes && !parser.getEnums().empty())
	{
	  printf("static void\n");
	  printf("choice2enum (const GValue *src_value,\n");
	  printf("             GValue       *dest_value)\n");
	  printf("{\n");
	  printf("  sfi_value_choice2enum (src_value, dest_value, NULL);\n");
	  printf("}\n");
	}
      
      for(ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	{
	  string name = makeLowerName (ri->name);
	  
	  printf("static GParamSpec *%s_field[%d];\n", name.c_str(), ri->contents.size());
	  printf("SfiRecFields %s_fields = { %d, %s_field };\n", name.c_str(), ri->contents.size(), name.c_str());

	  if (options.generateBoxedTypes)
	    {
	      string mname = makeMixedName (ri->name);
	      
	      printf("static void\n");
	      printf("%s_boxed2rec (const GValue *src_value, GValue *dest_value)\n", name.c_str());
	      printf("{\n");
	      printf("  sfi_value_take_rec (dest_value,\n");
	      printf("    %s_to_rec (g_value_get_boxed (src_value)));\n", name.c_str());
	      printf("}\n");
	      
	      printf("static void\n");
	      printf("%s_rec2boxed (const GValue *src_value, GValue *dest_value)\n", name.c_str());
	      printf("{\n");
	      printf("  g_value_set_boxed_take_ownership (dest_value,\n");
	      printf("    %s_from_rec (sfi_value_get_rec (src_value)));\n", name.c_str());
	      printf("}\n");
	      
	      printInfoStrings (name + "_info_strings", ri->infos);
	      printf("static SfiBoxedRecordInfo %s_boxed_info = {\n", name.c_str());
	      printf("  \"%s\",\n", mname.c_str());
	      printf("  { %d, %s_field },\n", ri->contents.size(), name.c_str());
	      printf("  (SfiBoxedToRec) %s_to_rec,\n", name.c_str());
	      printf("  (SfiBoxedFromRec) %s_from_rec,\n", name.c_str());
	      printf("  %s_boxed2rec,\n", name.c_str());
	      printf("  %s_rec2boxed,\n", name.c_str());
	      printf("  %s_info_strings\n", name.c_str());
	      printf("};\n");
	      printf("GType %s = 0;\n", makeGTypeName (ri->name).c_str());
	    }
	  printf("\n");
	}
      for(si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	{
	  string name = makeLowerName (si->name);
	  
	  printf("static GParamSpec *%s_content;\n", name.c_str());

	  if (options.generateBoxedTypes)
	    {
	      string mname = makeMixedName (si->name);
	      
	      printf("static void\n");
	      printf("%s_boxed2seq (const GValue *src_value, GValue *dest_value)\n", name.c_str());
	      printf("{\n");
	      printf("  sfi_value_take_seq (dest_value,\n");
	      printf("    %s_to_seq (g_value_get_boxed (src_value)));\n", name.c_str());
	      printf("}\n");
	      
	      printf("static void\n");
	      printf("%s_seq2boxed (const GValue *src_value, GValue *dest_value)\n", name.c_str());
	      printf("{\n");
	      printf("  g_value_set_boxed_take_ownership (dest_value,\n");
	      printf("    %s_from_seq (sfi_value_get_seq (src_value)));\n", name.c_str());
	      printf("}\n");
	      
	      printInfoStrings (name + "_info_strings", si->infos);
	      printf("static SfiBoxedSequenceInfo %s_boxed_info = {\n", name.c_str());
	      printf("  \"%s\",\n", mname.c_str());
	      printf("  NULL, /* %s_content */\n", name.c_str());
	      printf("  (SfiBoxedToSeq) %s_to_seq,\n", name.c_str());
	      printf("  (SfiBoxedFromSeq) %s_from_seq,\n", name.c_str());
	      printf("  %s_boxed2seq,\n", name.c_str());
	      printf("  %s_seq2boxed,\n", name.c_str());
	      printf("  %s_info_strings\n", name.c_str());
	      printf("};\n");
	      printf("GType %s = 0;\n", makeGTypeName (si->name).c_str());
	    }
	  printf("\n");
	}
    }

  if (options.initFunction != "")
    {
      bool first = true;
      printf("static void\n%s (void)\n", options.initFunction.c_str());
      printf("{\n");

      /*
       * It is important to follow the declaration order of the idl file here, as for
       * instance a ParamDef inside a record might come from a sequence, and a ParamDef
       * inside a Sequence might come from a record - to avoid using yet-unitialized
       * ParamDefs, we follow the getTypes() 
       */
      vector<string>::const_iterator ti;

      for(ti = parser.getTypes().begin(); ti != parser.getTypes().end(); ti++)
	{
	  if (parser.isRecord (*ti) || parser.isSequence (*ti))
	    {
	      if(!first) printf("\n");
	      first = false;
	    }
	  if (parser.isRecord (*ti))
	    {
	      const RecordDef& rdef = parser.findRecord (*ti);

	      string name = makeLowerName (rdef.name);
	      int f = 0;

	      for (pi = rdef.contents.begin(); pi != rdef.contents.end(); pi++, f++)
		{
		  if (options.generateIdlLineNumbers)
		    printf("#line %u \"%s\"\n", pi->line, parser.fileName().c_str());
		  printf("  %s_field[%d] = %s;\n", name.c_str(), f, makeParamSpec (*pi).c_str());
		}
	    }
	  if (parser.isSequence (*ti))
	    {
	      const SequenceDef& sdef = parser.findSequence (*ti);

	      string name = makeLowerName (sdef.name);
	      int f = 0;

	      if (options.generateIdlLineNumbers)
		printf("#line %u \"%s\"\n", sdef.content.line, parser.fileName().c_str());
	      printf("  %s_content = %s;\n", name.c_str(), makeParamSpec (sdef.content).c_str());
	    }
	}
      if (options.generateBoxedTypes)
      {
	for(ei = parser.getEnums().begin(); ei != parser.getEnums().end(); ei++)
	  {
	    string gname = makeGTypeName(ei->name);
	    string name = makeLowerName(ei->name);
	    string mname = makeMixedName(ei->name);

	    printf("  %s = g_enum_register_static (\"%s\", %s_value);\n", gname.c_str(),
						      mname.c_str(), name.c_str());
	    printf("  g_value_register_transform_func (SFI_TYPE_CHOICE, %s, choice2enum);\n",
						      gname.c_str());
	    printf("  g_value_register_transform_func (%s, SFI_TYPE_CHOICE,"
		   " sfi_value_enum2choice);\n", gname.c_str());
	  }
	for(ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
	  {
	    string gname = makeGTypeName(ri->name);
	    string name = makeLowerName(ri->name);

	    printf("  %s = sfi_boxed_make_record (&%s_boxed_info,\n", gname.c_str(), name.c_str());
	    printf("    (GBoxedCopyFunc) %s_copy_shallow,\n", name.c_str());
	    printf("    (GBoxedFreeFunc) %s_free);\n", name.c_str());
	  }
      	for(si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
	  {
	    string gname = makeGTypeName(si->name);
	    string name = makeLowerName(si->name);

	    printf("  %s_boxed_info.element = %s_content;\n", name.c_str(), name.c_str());
	    printf("  %s = sfi_boxed_make_sequence (&%s_boxed_info,\n", gname.c_str(), name.c_str());
	    printf("    (GBoxedCopyFunc) %s_copy_shallow,\n", name.c_str());
	    printf("    (GBoxedFreeFunc) %s_free);\n", name.c_str());
	  }
}
      printf("}\n");
    }

  if (options.generateSignalStuff)
    {
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	{
	  vector<MethodDef>::const_iterator si;

	  for (si = ci->signals.begin(); si != ci->signals.end(); si++)
	    {
	      string fullname = makeLowerName (ci->name + "::" + si->name);

	      printf("void %s_frobnicator (SignalContext *sigcontext) {\n", fullname.c_str());
	      printf("  /* TODO: do something meaningful here */\n");
	      for (pi = si->params.begin(); pi != si->params.end(); pi++)
		{
		  string arg = createTypeCode(pi->type, "", MODEL_ARG);
		  printf("  %s %s;\n", arg.c_str(), pi->name.c_str());
		}
	      printf("}\n");
	    }
	}
    }

  if (options.generateProcedures)
    {
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	{
	  for (mi = ci->methods.begin(); mi != ci->methods.end(); mi++)
	    {
	      MethodDef md;
	      md.name = mi->name;
	      md.result = mi->result;

	      ParamDef class_as_param;
	      class_as_param.name = makeLowerName(ci->name) + "_object";
	      class_as_param.type = ci->name;
	      md.params.push_back (class_as_param);

	      for(pi = mi->params.begin(); pi != mi->params.end(); pi++)
		md.params.push_back (*pi);

	      printProcedure (md, ci->name);
	    }
	}
      for (mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
	printProcedure (*mi);
    }
}

class CodeGeneratorQt : public CodeGenerator {
public:
  CodeGeneratorQt(const Parser& parser) : CodeGenerator(parser) {
  }
  void run ();
};

void CodeGeneratorQt::run ()
{
  NamespaceHelper nspace(stdout);

  if (options.generateProcedures)
    {
      vector<MethodDef>::const_iterator mi;
      for (mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
	{
	  if (mi->result.type == "void" && mi->params.empty())
	    {
	      nspace.setFromSymbol (mi->name);
	      string mname = makeLMixedName(nspace.printableForm (mi->name));
	      string dname = makeLowerName(mi->name, '-');

	      printf("void %s () {\n", mname.c_str());
	      printf("  sfi_glue_vcall_void (\"%s\", 0);\n", dname.c_str());
	      printf("}\n");
	    }
	}
    }
}

int main (int argc, char **argv)
{
  Options options;
  
  if (!options.parse (&argc, &argv))
    {
      /* invalid options */
      return 1;
    }

  if (options.doHelp)
    {
      options.printUsage ();
      return 0;
    }

  if((argc-optind) != 1)
    {
      options.printUsage ();
      return 1;
    }

  const char *inputfile = argv[1];
  
  int fd = open (inputfile, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "can't open inputfile %s (%s)\n", inputfile, strerror (errno));
      return 1;
    }
  
  Parser parser (inputfile, fd);
  if (!parser.parse())
    {
      /* parse error */
      return 1;
    }

  CodeGenerator *codeGenerator = 0;
  if (options.targetC || options.targetCore)
    codeGenerator = new CodeGeneratorC (parser);

  if (options.targetQt)
    codeGenerator = new CodeGeneratorQt (parser);

  if (!codeGenerator)
    {
      fprintf(stderr, "no target given\n");
      return 1;
    }

  printf("\n/*-------- begin %s generated code --------*/\n\n\n",argv[0]);
  codeGenerator->run ();
  printf("\n\n/*-------- end %s generated code --------*/\n\n\n",argv[0]);

  delete codeGenerator;
  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
