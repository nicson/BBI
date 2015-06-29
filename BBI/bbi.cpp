//
//  bbi.cpp
//  BBIインタプリンタのメイン処理
//
//  Created by Yoshiyuki Ohde on 2015/06/22.
//  Copyright (c) 2015年 Yoshiyuki Ohde. All rights reserved.
//
#include "bbi.h"
#include "bbi_prot.h"


int main(int argc, char* argv[]){
    if(argc==1){
        cout << "Usage: bbi filename" << endl;
    }
    convert_to_internalCode(argv[1]);  //ファイル名を渡し、内部コードに変換
    syntaxChk();    //構文チェック
    execute();      //BBIインタプリンタ実行
}