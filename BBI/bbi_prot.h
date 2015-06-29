//
//  bbi_prot.h
//  BBI
//  全関数のプロトタイプ
//
//  Created by Yoshiyuki Ohde on 2015/06/22.
//  Copyright (c) 2015年 Yoshiyuki Ohde. All rights reserved.
//

#ifndef BBI_bbi_prot_h
#define BBI_bbi_prot_h

// -------------------- bbi_pars.cpp 構文解析
void init();
void convert_to_internalCode(char *fname);
void convert();
void convert_block();
void convert_block_set();
void convert_rest();
void backPatch(int line,int n);
void push_intercode();
void set_name();
void set_aryLen();
void setCode(int cd);
int setCode(int cd, int nbr);
void setCode_rest();
void setCode_EofLine();
void setCode_End();
void optionSet();
void varDecl();
void fncDecl();
void var_namechk(const Token& tk);
bool is_localScope();
//void DBG_dump_src(char *s);
//void DBG_all_prog_disp();

// -------------------- bbi_tkn.cpp トークン処理

void initChTyp();
void fileOpen(char *fname);
void nextLine();
Token nextLine_tkn();
Token nextTkn();
bool is_ope2(char c1, char c2);
TknKind get_kind(const string& s);
Token chk_nextTkn(const Token& tk, int kind2);
void set_token_p(char *p);
int get_lineNo();
string kind_to_s(int kd);
string kind_to_s(const CodeSet& cd);

// -------------------- bbi_tbl.cpp 記号表処理

int enter(SymTbl& tb, SymKind kind);
int searchName(const string& s, int mode);
bool is_localName(const string& name, SymKind kind);
void set_startLtable();
TknKind get_kind(const string& name);
bool is_ope2(char a, char b);
string kind_to_s(int kd);
vector<SymTbl>::iterator tableP(const CodeSet& cd);

// -------------------- bbi_code.cpp メモリ管理と構文チェックと実行

void syntaxChk();
void set_startPc(int n);
void execute();
void fncExec(int fncNbr);
double get_expression(int kind1=0, int kind2=0);
void expression(int kind1, int kind2);
void expression();
void term(int n);
void factor();
void binaryExpr(TknKind op);
int opOrder(TknKind kd);
void statement();
void block();
void chk_dtTyp(const CodeSet& cd);
int get_memAdrs(const CodeSet& cd);
void fncCall_syntax(int fncNbr);
void fncCall(int fncNbr);
void sysFncExec_syntax(TknKind);
void sysFncExec(TknKind kd);
CodeSet firstCode(int n);
CodeSet nextCode();
void chk_EofLine() ;
CodeSet chk_nextCode(const CodeSet& cd, int kind2);
int endline_of_If(int line);
TknKind lookCode(int line);
void set_dtTyp(const CodeSet& cd, char typ);
int get_topAdrs(const CodeSet& cd);
void post_if_set(bool& flg);

int set_LITERAL(double d);
int set_LITERAL(const string& s);

// -------------------- bbi_misc.cpp 雑関数
string dbl_to_s(double d);
void err_exit(Tobj a="\1", Tobj b="\1", Tobj c="\1", Tobj d="\1");
string err_msg(const string& a, const string& b);
#endif
