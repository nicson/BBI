//
//  bbi.h
//  BBI
//
//  Created by Yoshiyuki Ohde on 2015/06/22.
//  Copyright (c) 2015年 Yoshiyuki Ohde. All rights reserved.
//

#ifndef BBI_bbi_h
#define BBI_bbi_h

#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <fstream>
#include <sstream>

using namespace std;

/*-------------------------MACRO-*/

#define SHORT_SIZE sizeof(short int)    //(short int)型のサイズ
#define SHORT_P(p) (short int*)(p)      //(short int*)型にキャスト
#define UCHAR_P(p) (unsigned char*)(p)  //(unsigned char*)型にキャスト
#define LIN_SIZ 255                     //ソース1行の最大サイズ

/*--------------------------enum struct etc---*/
enum TknKind {
  Lparen = '(',
  Rparen = ')',
  Lbracket = '[',
  Rbracket = ']',
  Plus = '+',
  Minus = '-',
  Multi = '*',
  Divi = '/',
  Mod = '%',
  Not = '!',
  Ifsub = '?',
  Assign = '=',
  IntDivi = '\\',
  Comma = ',',
  DblQ = '"',
  Func = 150,
  Var,
  If,
  Elif,
  Else,
  For,
  To,
  Step,
  While,
  End,
  Break,
  Return,
  Option,
  Print,
  Println,
  Input,
  Toint,
  Exit,
  Equal,
  NotEq,
  Less,
  LessEq,
  Great,
  GreatEq,
  And,
  Or,
  END_KeyList,  //<-何者?
  Ident,
  IntNum,
  DblNum,
  String,
  Letter,
  Doll,
  Digit,
  Gvar,
  Lvar,
  Fcall,
  EofProg,
  EofLine,
  Others
};

/*
 * トークンの管理
 */
struct Token {
  TknKind kind;   //トークンの種類
  string text;    //トークン文字列
  double dblVal;  //数値定数のときの値
  Token() {
    kind = Others;
    text = "";
    dblVal = 0.0;
  }
  Token(TknKind k) {
    kind = k;
    text = "";
    dblVal = 0.0;
  }
  Token(TknKind k, double d) {
    kind = k;
    text = "";
    dblVal = d;
  }
  Token(TknKind k, const string& s) {
    kind = k;
    text = s;
    dblVal = 0.0;
  }
  Token(TknKind k, const string& s, double d) {
    kind = k;
    text = s;
    dblVal = d;
  }
};

/*
 * 記号表登録名の種類
 */
enum SymKind {
  noId,   //数値定数
  varId,  //変数
  fncId,  //関数
  paraId  //関数の引数
};

/*
 * 型名
 */
enum DtType {
  NON_T,  //型未定
  DBL_T   // double型
};

/*
 * 記号表構成管理
 */
struct SymTbl {
  string name;     //関数や変数の名前
  SymKind nmKind;  //記号表種類
  char dtTyp;      //変数の型(NON_T,DBL_T)
  int aryLen;      //配列長、0:単純変数
  short args;      //関数の引数個数
  int adrs;        //変数のメインメモリ番地,関数の開始行番号
  int frame;       //関数用フレームサイズ
    SymTbl(){clear();}
    void clear(){
        name="";
        nmKind=noId;
        dtTyp=NON_T;
        aryLen=0;
        args=0;
        adrs=0;
        frame=0;
    }
};


/*
 * コードの管理
 */
struct CodeSet{
    TknKind kind;       //種類
    const char* text;   //文字列リテラルのときの先頭ポインタ
    double dblVal;      //数値定数の時の値
    int symNbr;         //記号表の添字位置
    int jmpAdrs;        //とび先番号
    CodeSet(){clear();}
    CodeSet(TknKind k){
        clear();
        kind=k;
    }
    CodeSet(TknKind k,double d){
        clear();
        kind=k;
        dblVal = d;
    }
    CodeSet(TknKind k,const char* s){
        kind = k;
        text = s;
    }
    CodeSet(TknKind k, int sym, int jmp){
        kind = k;
        symNbr = sym;
        jmpAdrs = jmp;
    }
    void clear(){
        kind=Others;
        text="";
        dblVal=0.0;
        jmpAdrs=0;
        symNbr=-1;
    }
};

/*
 * 型情報付きオブジェクト
 */
struct Tobj{
    char type;  //格納型 'd':double 's':string '-':未定
    double d;   //数値
    string s;   //文字列
    Tobj(){type='-';d=0.0;s="";}
    Tobj(double dt){type='d';d=dt; s="";}
    Tobj(const string& st){type='s';s =st;}
    Tobj(const char* st){type='s';d=0.0;s=st;}
};

/*
 * メインメモリー管理クラス
 */
class Mymemory{
private:
    vector<double> mem;
public:
    void auto_resize(int n){    //256個単位で確保(再確保数抑制のため、多めに確保)
        if(n >= (int)mem.size()){
            n = (n/256+1)*256;
            mem.resize(n);
        }
    }
    void set(int adrs,double dt){mem[adrs]=dt;}     //メモリ書き込み
    void add(int adrs,double dt){mem[adrs]+=dt;}    //メモリ加算
    double get(int adrs){return mem[adrs];}         //メモリ読出
    int size(){return (int)mem.size();}             //格納サイズ
    void resize(unsigned int n){mem.resize(n);}     //サイズ確保
};















#endif