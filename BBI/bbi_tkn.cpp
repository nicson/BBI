//
//  bbi_tkn.cpp
//  BBI
//
//  Created by Yoshiyuki Ohde on 2015/06/24.
//  Copyright (c) 2015年 Yoshiyuki Ohde. All rights reserved.
//

#include "bbi.h"
#include "bbi_prot.h"


/*
 * 予約語、記号の管理
 */
struct KeyWord{
    const char* keyName;    //キーワード名
    TknKind keyKind;        //キーワードのトークンの種類
};

KeyWord KeyWdTbl[] = {
    {"func",Func},{"var",Var},
    {"if",If},{"elif",Elif},
    {"else",Else},{"for",For},
    {"to",To},{"step",Step},
    {"while",While},{"end",End},
    {"break",Break},{"return",Return},
    {"print",Print},{"println",Println},
    {"option",Option},{"input",Input},
    {"toint",Toint},{"exit",Exit},
    {"(",Lparen},{")",Rparen},
    {"[",Lbracket},{"]",Rbracket},
    {"+",Plus},{"-",Minus},
    {"*",Multi},{"/",Divi},
    {"==",Equal},{"!=",NotEq},
    {"<",Less},{"<=",LessEq},
    {">",Great},{">=",GreatEq},
    {"&&",And},{"||",Or},
    {"!",Not},{"%",Mod},
    {"?",Ifsub},{"=",Assign},
    {"\\",IntDivi},{",",Comma},
    {"\"",DblQ},
    {"@dummy",END_KeyList}
};

int srcLineno;          //ソースの行番号
TknKind ctyp[256];     //文字種表の配列
char* token_p;          //1文字取得用文字位置
bool endOfFile_F;       //ファイル終了フラグ
char buf[LIN_SIZ+5];    //ソース読込場所
ifstream fin;           //入力ストリーム
#define MAX_LINE 2000   //最大プログラム行数

/*
 * 文字種表の初期設定
 */
void initChTyp(){
    int i;
    for(i=0;i<256;i++){     //まず全ての要素にOthersを設定
        ctyp[i] = Others;
    }
    for(i='0';i<='9';i++){  //数字を設定
        ctyp[i] = Digit;
    }
    for(i='A';i<='Z';i++){  //アルファベット(大文字)
        ctyp[i] = Letter;
    }
    for(i='a';i<='z';i++){  //アルファベット(小文字)
        ctyp[i] = Letter;
    }
    ctyp['_'] = Letter; ctyp['$'] = Doll;
    ctyp['('] = Lparen; ctyp[')'] = Rparen;
    ctyp['['] = Lbracket; ctyp[']'] = Rbracket;
    ctyp['<'] = Less; ctyp['>'] = Great;
    ctyp['+'] = Plus; ctyp['-'] = Minus;
    ctyp['*'] = Multi; ctyp['/'] = Divi;
    ctyp['!'] = Not; ctyp['%'] = Mod;
    ctyp['?'] = Ifsub; ctyp['='] = Assign;
    ctyp['\\'] = IntDivi; ctyp[','] = Comma;
    ctyp['\"'] = DblQ;
    
}

/*
 * ファイルオープン
 */
void fileOpen(char* fname){
    srcLineno = 0;
    endOfFile_F = false;
    fin.open(fname);
    if(!fin){cout << fname << "をオープンできません\n";exit(1);}
}

/*
 * ファイルの次の1行を取得
 */
void nextLine(){
    string s;
    
    if(endOfFile_F) return;
    fin.getline(buf, LIN_SIZ+5);    //1行読み込み
    if(fin.eof()){  //ファイル終了
        fin.clear();        //ファイル読み込み関連フラグをクリア
        fin.close();        //ファイルに関連づけられたストリームをクリア
        endOfFile_F = true;
    }
    
    if(strlen(buf)>LIN_SIZ){
        err_exit("プログラムは1行",LIN_SIZ,"文字以内で記述してください。");
    }
    if(++srcLineno > MAX_LINE){
        err_exit("プログラムが",MAX_LINE,"を超えました。");
    }
    token_p = buf;  //トークン解析用ポインタを読み込み行頭に設定
}

/*
 * 次の行を読み、次のトークンを返す
 */
Token nextLine_tkn(){
//    nextLine();
//    return nextTkn();
    if(endOfFile_F){
        return Token(EofProg);
    }
    else{
        nextLine();
        return nextTkn();
    }
}

#define CH (*token_p)
#define C2 (*(token_p+1))
#define NEXT_CH()   ++token_p

/*
 * 次のトークンを返す
 */
Token nextTkn(){
    TknKind kd;
    string txt = "";
    
    //TODO 最終１行に命令が書いてあってもEofProgトークンとしてリターンされる
//    if(endOfFile_F){        //ファイル終了
//        return TknKind(EofProg);
//    }
    while (isspace(CH)) {   //空白スキップ
        NEXT_CH();
    }
    if(CH == '\0') return TknKind(EofLine); //行末
    
    switch(ctyp[CH]){
        case Doll: case Letter:     //<識別子> ::= <英字> | <"$"> |  <識別子> { <英字> | <数字> }
            txt += CH; NEXT_CH();
            while(ctyp[CH]==Letter || ctyp[CH]==Digit){
                txt += CH;
                NEXT_CH();
            }
            break;
        case Digit: //数値定数
            kd = IntNum;
            while(ctyp[CH]==Digit){ txt += CH; NEXT_CH();}
            if(CH=='.'){
                kd = DblNum;
                txt += CH;
                NEXT_CH();
            }
            while(ctyp[CH]==Digit){ txt += CH; NEXT_CH();}
            return Token(kd,txt,atof(txt.c_str())); //IntNumもdouble型で格納
        case DblQ:
            NEXT_CH();
            while (CH != '\0' && CH != '"') {
                txt += CH;
                NEXT_CH();
            }
            if(CH == '"') NEXT_CH();
            else err_exit("文字列リテラルが閉じていない。");
            return Token(String,txt);
        default:
            if(CH == '/' && C2 == '/') return Token(EofLine);  //コメント
            if(is_ope2(CH,C2)){ //2文字演算子の場合
                txt += CH;
                txt += C2;
                NEXT_CH();
                NEXT_CH();
            }else{              //1文字演算子
                txt += CH;
                NEXT_CH();
            }
    }
    kd = get_kind(txt); //トークン種別設定
    
    if(kd == Others) err_exit("不正なトークンです: ",txt);
    return Token(kd,txt);
}

/*
 * 2文字演算子なら真
 */
bool is_ope2(char c1, char c2){
    char s[] = "    ";
    if(c1 == '\0' || c2 == '\0') return false;
    s[1] = c1; s[2] = c2;
    return strstr(" ++ -- <= >= == != && || ", s) != NULL;
}

/*
 * トークン種別設定
 */
TknKind get_kind(const string& s){
    for(int i=0; KeyWdTbl[i].keyKind != END_KeyList; i++){  //予約語テーブルに該当する場合
        if(s == KeyWdTbl[i].keyName) return KeyWdTbl[i].keyKind;
    }
    if(ctyp[s[0]]==Letter || ctyp[s[0]]==Doll) return Ident;    //関数名、変数名、仮引数名
    if(ctyp[s[0]]==Digit) return DblNum;    //定数値
    return Others;  //該当種別なし
}


/*
 * 確認付きトークン取得
 */
Token chk_nextTkn(const Token& tk, int kind2){
    if(tk.kind != kind2) err_exit(err_msg(tk.text, kind_to_s(kind2)));
    return nextTkn();
}

/*
 * トークン処理ポインタ設定
 */
void set_token_p(char* p){
    token_p = p;
}

/*
 * 種別->文字列
 */
string kind_to_s(int kd){
    for(int i=0; ; i++){
        if(KeyWdTbl[i].keyKind == END_KeyList) break;
        if(KeyWdTbl[i].keyKind == kd) return KeyWdTbl[i].keyName;
    }
    return "";
}

/*
 * コード->種別文字列
 */
string kind_to_s(const CodeSet& cd){
    switch (cd.kind) {
        case Lvar: case Gvar: case Fcall: return tableP(cd)->name;  //変数名、関数名
        case IntNum: case DblNum: return dbl_to_s(cd.dblVal);   //数値定数
        case String: return string("\"") + cd.text + "\""; //文字列リテラル
        case EofLine: return "";    //行末
    }
    return kind_to_s(cd.kind);  //予約語など
}

/*
 * 読み込みor実行中の行番号
 */
int get_lineNo(){
    extern int Pc;
    return (Pc == -1) ? srcLineno : Pc; // 解析中 : 実行中
}




