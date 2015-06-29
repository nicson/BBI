//
//  bbi_misc.cpp
//  BBI
//
//  Created by Yoshiyuki Ohde on 2015/06/27.
//  Copyright (c) 2015年 Yoshiyuki Ohde. All rights reserved.
//

#include "bbi.h"
#include "bbi_prot.h"

/*
 * 数値->文字列
 */
string dbl_to_s(double d){
    ostringstream ostr;
    ostr << d;
    return ostr.str();
}

/*
 * エラーメッセージを表示し、BBIインタプリタを終了する
 * 関数宣言で次のデフォルト引数を指定
 * void err_exit(Tobj a="\1", Tobj b="\1", Tobj c="\1", Tobj d="\1")
 */
void err_exit(Tobj a, Tobj b, Tobj c, Tobj d){
    Tobj ob[5];
    ob[1]=a; ob[2]=b; ob[3]=c; ob[4]=d;
    cerr << "line:" << get_lineNo() << " ERROR ";
    
    for(int i=0;i<=4 && ob[i].s!="\1";i++){
        if(ob[i].type == 'd') cout << ob[i].d;  //数値情報
        if(ob[i].type == 's') cout << ob[i].s;  //文字列情報
    }
    cout << endl;
    exit(1);
}

/*
 * エラーメッセージ生成
 */
string err_msg(const string& a, const string& b){
    if(a == "") return b + " が必要です。";
    if(b == "") return a + " が不正です。";
    return b + " が " + a + " の前に必要です。";
}












