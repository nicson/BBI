//
//  bbi_tbl.cpp
//  BBI
//  記号表処理
//  Created by Yoshiyuki Ohde on 2015/06/23.
//  Copyright (c) 2015年 Yoshiyuki Ohde. All rights reserved.
//

#include "bbi.h"
#include "bbi_prot.h"

/*
 * テーブル
 */
vector<SymTbl> Gtable;  //大域記号表(関数名、グローバル変数)
vector<SymTbl> Ltable;  //局所記号表(関数内変数、関数仮引数)
int startLtable;        //局所用の開始位置

/*
 * 記号表登録
 */
int enter(SymTbl& tb, SymKind kind){
    int n,mem_size;
    bool isLocal = is_localName(tb.name, kind);
    extern int localAdrs;   //局所変数アドレス管理
    extern Mymemory Dmem;   //主記憶(実際の変数値を格納する)
    
    //確認
    mem_size = tb.aryLen;
    if(mem_size == 0) mem_size = 1; //単純変数の場合
    if(kind!=varId && tb.name[0]=='$')  //$使用の確認
        err_exit("変数名以外で $ を使うことはできません: ",tb.name);
    tb.nmKind = kind;   //記号表種類を格納
    
    
    n = -1;     //重複確認(変数宣言の重複チェックはvar_namechk(bbi_pars.cpp)で実行)
    if(kind == fncId) n = searchName(tb.name, 'G'); //関数名の重複チェック
    if(kind == paraId) n = searchName(tb.name, 'L');//引数名の重複チェック(1関数定義内)
    if(n != -1) err_exit("名前が重複しています",tb.name);
    
    //アドレス設定
    //関数宣言時
    if(kind == fncId){
        tb.adrs = get_lineNo(); //関数開始行取得
    }else{
        //局所変数宣言時
        if(isLocal){
            tb.adrs = localAdrs;    //局所アドレスを設定
            localAdrs+=mem_size;    //局所アドレスを更新(取得した変数サイズ分ex. a=>1,a[3]=>4)
        }else{
            //大域変数宣言時
            tb.adrs = Dmem.size();  //メインメモリの次の領域添字を設定
            Dmem.resize(Dmem.size()+mem_size);  //メインメモリの領域確保
        }
    }
    
    //記号表へ登録
    if(isLocal){    //局所領域への登録
        n = (int)Ltable.size();
        Ltable.push_back(tb);
    }else{          //大域領域への登録
        n = (int)Gtable.size();
        Gtable.push_back(tb);
    }
    
    return n;   //記号表登録時の添字を返す
}

/*
 * 関数宣言時に呼び出し、
 * 局所記号表の開始位置を設定する
 */
void set_startLtable(){
    startLtable = (int)Ltable.size();
}

/*
 * 局所変数名ならtrueを返す
 */
bool is_localName(const string& name, SymKind kind){
    if(kind == paraId){ //仮引数の場合
        return true;
    }
    if(kind == varId){  //変数の場合
        if(is_localScope() && name[0] != '$'){  //関数処理中かつ変数名の頭に$がついていない場合、局所変数
            return true;
        }else{
            return false;
        }
    }
    return false;   //関数名の場合
}

/*
 * 名前検索
 */
int searchName(const string& s, int mode){
    int n;
    switch(mode){
        case 'G':   //大域記号表検索
            for(n=0;n<(int)Gtable.size();n++){
                if(Gtable[n].name == s) return n;
            }
            break;
        case 'L':    //局所記号表検索
            //呼出し時の関数定義処理中に登録された変数名のみ検索(他関数の変数名と同じでも検索されない)
            for(n=startLtable;n<(int)Ltable.size();n++){
                if(Ltable[n].name == s) return n;
            }
            break;
        case 'F':   //関数名検索
            n = searchName(s,'G');
            if(n != -1 && Gtable[n].nmKind==fncId) return n;
            break;
        case 'V':
            if(searchName(s,'F')!=-1) err_exit("関数名と重複しています",s);
            if(s[0]=='$') return searchName(s,'G'); //大域変数を検索
            if(is_localScope()) return searchName(s, 'L');  //局所領域処理中
            else return searchName(s, 'G'); //大域領域処理中
            break;
    }
    return -1;  //見つからなかった場合
}

/*
 * 反復子取得
 */
vector<SymTbl>::iterator tableP(const CodeSet& cd){
    if(cd.kind == Lvar) return Ltable.begin() + cd.symNbr;  //局所変数
    return Gtable.begin() + cd.symNbr;
}
















