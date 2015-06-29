//
//  bbi_code.cpp
//  BBI
//  メモリ管理と構文チェックと実行
//  Created by Yoshiyuki Ohde on 2015/06/25.
//  Copyright (c) 2015年 Yoshiyuki Ohde. All rights reserved.
//

#include "bbi.h"
#include "bbi_prot.h"

CodeSet code;   //コードセット
int startPc;    //実行開始行
int Pc=-1;      //プログラムカウンタ -1:非実行中
int baseReg;    //ベースレジスタ
int spReg;      //スタックポインタ
int maxLine;    //プログラム末尾行
vector<char*> intercode;    //変換済み内部コード格納
char* code_ptr;     //内部コード解析用ポインタ
double returnValue; //関数戻り値
bool break_Flg, return_Flg, exit_Flg; //制御用フラグ
Mymemory Dmem;  //主記憶
vector<string> strLITERAL;  //文字列リテラル格納
vector<double> nbrLITERAL;  //数値リテラル格納
bool syntaxChk_mode = false;    //構文チェックのとき真
extern vector<SymTbl> Gtable;   //大域記号表

class MyStack{
private:
    stack<double> st;
public:
    void push(double n){st.push(n);}    //スタックにプッシュ
    int size(){ return (int)st.size();}    //サイズ
    bool empty(){ return st.empty();}  //空き判定
    double pop(){   //スタックからポップした値を返す
        if(st.empty()) err_exit("stack underflow");
        double d = st.top();    //スタックの先頭の要素を取得
        st.pop();   //スタックの先頭の要素を削除
        return d;
    }
};
MyStack stk;    //オペランドスタック

/*
 * 構文チェック
 */
void syntaxChk(){
    syntaxChk_mode = true;  //構文チェックフラグをtrue
    for(Pc=1;Pc<(int)intercode.size();Pc++){    //変換済み内部コードを1行ずつ処理する
        code = firstCode(Pc);   //行先頭コードを取得
        switch (code.kind) {
            case Func: case Option: case Var:   //内部コードへの変換時にチェック済み
                break;
            case Else: case End: case Exit: //右に余計な記述がないかチェック
                code = nextCode();
                chk_EofLine();
                break;
            case If: case Elif: case While: //式値
                code = nextCode();
                (void)get_expression(0,EofLine);
                break;
            case For:   // for i=1 to 100 step 2
                code = nextCode();
                (void)get_memAdrs(code);    //制御変数アドレス
                (void)get_expression('=',0);    //初期値('='で始まり、式が続く)
                (void)get_expression(To,0); //最終値('Toで始まり、式が続く')
                //刻み値(オプション、'step'で始まり、式が続く)
                if(code.kind == Step) (void)get_expression(Step,0);
                chk_EofLine();  //stepがない場合もあるので、EofLineをget_expressionではなく、ここで確認
                break;
            case Fcall: //代入のない関数呼び出し
                fncCall_syntax(code.symNbr);
                chk_EofLine();
                (void)stk.pop();  //戻り値不要
                break;
            case Print: case Println:
                sysFncExec_syntax(code.kind);
                break;
            case Gvar: case Lvar:   //代入文
                (void)get_memAdrs(code);    //代入先変数のアドレス取得
                (void)get_expression('=',EofLine); //代入文右辺の値
                break;
            case Return:    //例) return a ? b > d   :b>dの場合、aを返却 (?はオプション)
                code = nextCode();
                if(code.kind != '?' && code.kind!=EofLine) (void)get_expression();  //戻り値を計算
                if(code.kind == '?') (void)get_expression('?',0);//?がある場合、条件文をチェック
                chk_EofLine();  //return ?の?がない場合もあるのでここでEofLineを確認
                break;
            case Break:
                code = nextCode();//?がある場合、条件文をチェック
                if(code.kind == '?') (void)get_expression('?',0);
                chk_EofLine();  //return ?の?がない場合もあるのでここでEofLineを確認
            case EofLine:   //行末
                break;
            default:
                err_exit("不正な記述です: ",kind_to_s(code.kind));
        }
    }
    syntaxChk_mode = false;
}

/*
 * 組込関数チェック
 */
void sysFncExec_syntax(TknKind kd){
    switch (kd) {
        case Toint: // toint(a)
            code = nextCode(); (void)get_expression('(',')');
            stk.push(1.0);
            break;
        case Input: // input()
            code = nextCode();
            code = chk_nextCode(code, '(');
            code = chk_nextCode(code, ')');
            stk.push(1.0);
            break;
        case Print: case Println:   //print "hello", a
            do{
                code = nextCode();
                if(code.kind ==String ) code = nextCode();  //文字列出力確認
                else (void)get_expression();    //変数や定数値の式の場合
            }while(code.kind== ',');    //','ならパラメータが続く
            chk_EofLine();
            break;
    }
}

/*
 * 組み込み関数実行
 */
void sysFncExec(TknKind kd){
    double d;
    string s;
    
    switch (kd) {
        case Toint: //toint(a)
            code = nextCode();
            stk.push((int)get_expression('(',')'));
            break;
        case Input: //input()
            nextCode(); nextCode();code = nextCode();   // '(',')'をスキップ
            getline(cin,s); //標準入力から1行読み込み
            stk.push(atof(s.c_str()));  //数字に変換して格納
            break;
        case Print: case Println:   //print "hello", a, func(a,b)
            do{
                code = nextCode();
                if(code.kind == String){
                    cout << code.text; code = nextCode();   //文字列出力
                }else{
                    d = get_expression();   //関数内でExitの可能性あり
                    if(!exit_Flg) cout << d;    //数値出力
                }
            }while(code.kind == ',');
            if(kd == Println) cout << endl;  //Printlnの場合、改行
            break;
    }
}

/*
 * 開始行設定
 * bbi_pars.cppで呼び出され、main関数がある場合はmain関数がある行から実行、
 * ない場合は、1行目から実行するために設定
 */
void set_startPc(int n)
{
    startPc = n;
}

/*
 * 内部コードを実行
 */
void execute(){
    baseReg = 0;            //ベースレジスタ初期化
    spReg = Dmem.size();    //スタックポインタ初期化
    Dmem.resize(spReg+100); //主記憶領域初期確保
    break_Flg = return_Flg = exit_Flg = false;    //break,return,exitフラグの初期化
    
    Pc = startPc;
    maxLine = (int)intercode.size()-1;
    while(Pc<=maxLine && !exit_Flg){
        statement();
    }
    Pc=-1;  //非実行状態
}

/*
 *
 */
void statement(){
    CodeSet save;
    int top_line, end_line, varAdrs;
    double wkVal,endDt, stepDt;
    if(Pc>maxLine || exit_Flg) return;
    code = save = firstCode(Pc);    //行頭コードをcodeに設定し、saveに退避
    top_line = Pc;
    end_line = code.jmpAdrs;
    if(code.kind == If) end_line = endline_of_If(Pc);   //If文構造のEnd行を取得
    
    //行頭コードで処理を分ける
    switch (code.kind) {
        case If:
            if(get_expression(If,0)){// ex.) If a==1 の [a==1]式を実行し、結果を評価
                ++Pc;   //内部コードを次行へ
                block();    //if文以下のブロックを実行
                Pc = end_line + 1; //End行の次行へ
                return;
            }
            Pc = save.jmpAdrs;
            //Ifコード以降の式の評価がfalseの場合
            //elif以下
            while(lookCode(Pc)==Elif){  //先頭コードがElifの場合、elifの後続式を評価
                save = firstCode(Pc); code = nextCode();
                if(get_expression()){
                    ++Pc;   //内部コードを次行へ
                    block();        //elif以下ブロックを実行
                    Pc = end_line + 1;//End行の次行へ
                    return;
                }
                Pc = save.jmpAdrs;
            }
            //else
            if(lookCode(Pc)==Else){
                ++Pc; block(); Pc = end_line + 1;
                return;
            }
            //end
            ++Pc;
            break;
        case While:
            for(;;){
                if(!get_expression(While,0)) break;
                ++Pc; block();
                if(break_Flg||return_Flg||exit_Flg){
                    break_Flg = false; break;
                }
                Pc = top_line; code = firstCode(Pc);//While行に戻る
            }
            Pc = end_line + 1;  //While文のEnd行の次行へ
            break;
        case For:
            save = nextCode();          // ループカウンタ変数のアドレス取得
            varAdrs = get_memAdrs(save);// ex.) for i=1 to 100 step 2 の変数iのアドレス
            
            expression('=', 0);     //ループカウンタの代入式の右辺式を実行
            set_dtTyp(save,DBL_T);  //ループカウンタの変数の型を決定(すべてDBL_T)
            Dmem.set(varAdrs, stk.pop());    //ループカウンタの初期値を設定
            endDt = get_expression(To,0);   //最終値を保存
            
            //刻み値を保存
            if(code.kind == Step) stepDt = get_expression(Step,0);
            else stepDt = 1;
            
            //forループ実行
            for(;;Pc=top_line){
                if(stepDt >= 0){    //増分ループ
                    if(Dmem.get(varAdrs)>endDt) break;  //for分終了
                }else{              //減分ループ
                    if(Dmem.get(varAdrs)<endDt) break;  //for分終了
                }
                ++Pc; block();
                if(break_Flg||return_Flg||exit_Flg){
                    break_Flg=false; break; //中断
                }
                Dmem.add(varAdrs, stepDt);  //ループカウンタの値更新
            }
            Pc = end_line+1;
            break;
        case Fcall:     //代入のない関数呼び出し
            fncCall(code.symNbr);
            (void)stk.pop();    //戻り値不要のため、スタックからポップ
            ++Pc;
            break;
        case Func:      //関数定義はスキップ
            Pc = end_line+1;
            break;
        case Print: case Println:
            sysFncExec(code.kind);  //ビルトイン関数実行
            ++Pc;
            break;
        case Gvar: case Lvar:
            varAdrs = get_memAdrs(code);
            expression('=', 0);
            set_dtTyp(save, DBL_T);
            Dmem.set(varAdrs, stk.pop());
            ++Pc;
            break;
        case Return:
            wkVal = returnValue;
            code = nextCode();
            if(code.kind != '?' && code.kind!=EofLine){
                wkVal = get_expression();
            }
            post_if_set(return_Flg);    //'?'があれば処理
            if(return_Flg) returnValue = wkVal;
            if(!return_Flg) ++Pc;   //TODO elseでよくね？
            break;
        case Break:
            code = nextCode();
            post_if_set(break_Flg);    //'?'があれば処理
            if(!break_Flg) ++Pc;
            break;
        case Exit:                      //Exit処理
            code = nextCode(); exit_Flg = true;
            break;
        case Option: case Var: case EofLine:    //実行時は無視
            ++Pc;
            break;
        default:
            err_exit("不正な記述です: ",kind_to_s(code.kind));
    }
}

/*
 * ブロックを実行
 */
void block(){
    TknKind k;
    while (!break_Flg && !return_Flg && !exit_Flg) {
        k = lookCode(Pc);//先頭コードを取得
        if(k==Elif || k==Else || k==End){   //ブロック正常終了(ブロックの末行)
            break;
        }
        statement();
    }
}


/*
 * 先頭コードを取得
 */
CodeSet firstCode(int line){
    code_ptr = intercode[line];
    return nextCode();
}

/*
 * 次の内部コードセットを取得
 */
CodeSet nextCode(){
    TknKind kd;
    short int  jmpAdrs , tblNbr;
    if(*code_ptr == '\0') return CodeSet(EofLine);  //行末の場合
    kd = (TknKind)*UCHAR_P(code_ptr++); //コード種類を取得
    switch(kd){
        case Func:
        case While: case For: case If: case Elif: case Else:
            jmpAdrs = *SHORT_P(code_ptr); code_ptr += SHORT_SIZE;
            return CodeSet(kd,-1,jmpAdrs);
        case String:
            tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZE;
            return CodeSet(kd,strLITERAL[tblNbr].c_str());  //文字列リテラル位置
        case IntNum: case DblNum:
            tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZE;
            return CodeSet(kd,nbrLITERAL[tblNbr]);  //数値リテラル位置
        case Fcall: case Gvar: case Lvar:
            tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZE;
            return CodeSet(kd,tblNbr,-1);
        default:        //付属情報のないコード
            return CodeSet(kd);
    }
}


/*
 * expression実行し、実行結果(スタックの最上要素)を返す
 */
double get_expression(int kind1, int kind2){
    expression(kind1,kind2); return stk.pop();
}

/*
 * 確認付きexpression実行
 */
void expression(int kind1, int kind2){
    if(kind1 != 0) code = chk_nextCode(code, kind1);
    expression();
    if(kind2 != 0) code = chk_nextCode(code, kind2);
}

/*
 * expression実行
 * 例1)
 *  1+2*(3-4) 4
 *  上の例だと、[1+2*(3-4)]まで計算(演算子が続かなければその後[4]は処理しない)
 * 例2)
 *  For i=1 To 100 Step 2
 *  [For]の次コードから始まる場合、[i=1]まで処理、
 *  [To]の次コードから始まる場合、[100]まで処理、
 *  [Step]の次コードから始まる場合、[2]まで処理、
 */
void expression(){
    term(1);
}

/*
 * term実行
 */
void term(int n){
    TknKind op;
    if(n == 7 ) { factor(); return;}    //単項演算、定数値、変数展開、括弧内計算、toInt,inputの計算
    term(n+1);
    while(n == opOrder(code.kind)){ //強さが同じ演算子が続く
        op = code.kind;
        code = nextCode(); term(n+1);
        if(syntaxChk_mode){ //構文チェック時、演算結果をすべて1.0で格納
            stk.pop(); stk.pop(); stk.push(1.0);
        }else{
            binaryExpr(op);
        }
    }
}

/*
 * factor実行
 */
void factor(){
    TknKind kd = code.kind;
    
    //syntax check時
    if(syntaxChk_mode){
        switch (kd) {
                //Not:   a = !b の-bを計算
                //Minus: a = -b の-bを計算
                //Plus:  a = +b の-bを計算(いらなくね？)
            case Not: case Minus: case Plus:
                //(syntax checkでは1.0を格納)
                code = nextCode(); factor(); stk.pop(); stk.push(1.0);
                break;
            case Lparen:            //左括弧の場合
                expression('(',')');
                break;
            case IntNum: case DblNum:   //数値定数の場合
                stk.push(1.0); code = nextCode();
                break;
            case Gvar: case Lvar:   //変数名の場合
                (void)get_memAdrs(code);
                stk.push(1.0);
                break;
            case Toint: case Input: //ビルトイン関数toInt,inputの場合、
                sysFncExec_syntax(kd);
                break;
            case Fcall: //関数呼び出しの場合
                fncCall_syntax(code.symNbr);
                break;
            case EofLine:   //行末の場合
                err_exit("式が不正です。");
            default:
                err_exit("式誤り: ", kind_to_s(code)); // a + = などで発生
                break;
        }
        return;
    }
    
    //BBIプログラム実行時
    switch(kd){
            //Not:   a = !b の-bを計算
            //Minus: a = -b の-bを計算
            //Plus:  a = +b の-bを計算(いらなくね？)
        case Not: case Minus: case Plus:
            code = nextCode(); factor();    //!a,-a,+aのaをfactorで処理した後
            if(kd == Not) stk.push(!stk.pop()); //Not処理し、スタックにプッシュ
            if(kd == Minus) stk.push(-stk.pop());//Minus処理し、スタックにプッシュ
            break;
        case Lparen:            //左括弧の場合
            expression('(',')');
            break;
        case IntNum: case DblNum:   //数値定数の場合
            stk.push(code.dblVal); code = nextCode();
            break;
        case Gvar: case Lvar:   //変数名の場合
            chk_dtTyp(code);    //値設定済みの変数か
            stk.push(Dmem.get(get_memAdrs(code)));
            break;
        case Toint: case Input: //ビルトイン関数toInt,inputの場合、
            sysFncExec(kd);
            break;
        case Fcall: //関数呼び出しの場合
            fncCall(code.symNbr);   //コードの記号表番号を渡し、関数実行
            break;
    }
}

/*
 * 演算子の優先順位を返す
 */
int opOrder(TknKind kd){
    switch (kd) {
        case Multi: case Divi: case Mod:
        case IntDivi:               return 6;   // '*' '/' '%' '\'
        case Plus: case Minus:      return 5;   // '+' '-'
        case Less: case LessEq:
        case Great: case GreatEq:   return 4;   // '<' '<=' '>' '>='
        case Equal: case NotEq:     return 3;   // '==' '!='
        case And:                   return 2;   // &&
        case Or:                    return 1;   // ||
        default:                    return 0;   // 該当なし
    }
}

/*
 * 二項演算
 */
void binaryExpr(TknKind op){
    double d=0,d2=stk.pop(),d1=stk.pop();
    
    if((op==Divi || op==Mod || op==IntDivi )&&d2==0)
        err_exit("ゼロ除算です。");
    
    switch (op) {
        case Plus: d = d1 + d2; break;
        case Minus: d = d1 - d2; break;
        case Multi: d = d1 * d2; break;
        case Divi: d = d1 / d2; break;
        case Mod: d = (int)d1 % (int)d2; break;
        case IntDivi: d = (int)d1 / (int)d2; break;
        case Less: d = d1 < d2; break;
        case LessEq: d = d1 <= d2; break;
        case Great: d = d1 > d2; break;
        case GreatEq: d = d1>=d2; break;
        case Equal: d = d1==d2; break;
        case NotEq: d = d1!=d2; break;
        case And: d = d1&&d2; break;
        case Or: d = d1||d2; break;
    }
    stk.push(d);
}

/*
 * 主記憶上の要素のアドレスを返す
 */
int get_memAdrs(const CodeSet& cd)
{
    int adr = 0,index,len;
    double d;
    
    adr = get_topAdrs(cd);  //主記憶の要素のアドレス(Dmemの添字)
    len = tableP(cd)->aryLen;   //配列のサイズ(0の場合単純変数)
    code = nextCode();
    if(len == 0) return adr;    //単純変数の場合、アドレスを返す
    
    //配列の場合、添字から対象要素のアドレスを特定
    d = get_expression('[',']');
    if((int)d != d) err_exit("添字は端数のない数値で指定してください。");
    if(syntaxChk_mode) return adr;
    
    index = (int)d;
    if(index<0||len<=index)// ex.)var a[5] ではaの添字は0-5の6個なのでlen=6より[len>添字]のはず
        err_exit(index," は添字範囲外です(添字範囲0-",len-1,") ");
    return adr+index;   //添字加算し返す
}

/*
 * コードが行末コードであることの確認
 */
void chk_EofLine(){
    if(code.kind != EofLine) err_exit("不正な記述: ",kind_to_s(code));
}

/*
 * コードセットのトークン種類が一致することを確認し、次のコードを返す
 */
CodeSet chk_nextCode(const CodeSet& cd, int kind2){
    if(cd.kind != kind2){
        if(kind2 == EofLine) err_exit("不正な記述です: ",kind_to_s(cd));
        if(cd.kind == EofLine) err_exit(kind_to_s(kind2)," が必要です。");
        err_exit(kind_to_s(kind2)+" が "+ kind_to_s(cd)+" の前に必要です。");
    }
    return nextCode();
}

/*
 * 型あり確認
 */
void chk_dtTyp(const CodeSet& cd){
    if(tableP(cd)->dtTyp == NON_T)
        err_exit("初期化されていない変数が使用されました。",kind_to_s(cd));
}

/*
 * 関数呼び出しチェック
 */
void fncCall_syntax(int fncNbr){
    int argCt = 0;
    
    code = nextCode(); code = chk_nextCode(code, '(');
    if(code.kind != ')'){   //引数がある場合
        for(;;code=nextCode()){
            (void)get_expression();++argCt; //引数の値を計算し、引数個数を+1
            if(code.kind!=',') break;   //','なら引数が続く
        }
    }
    code = chk_nextCode(code, ')'); // ')'のはず
    if(argCt != Gtable[fncNbr].args){
        err_exit(Gtable[fncNbr].name," 関数の引数個数が誤っています。");
    }
    stk.push(1.0);  //戻り値をスタックにプッシュ
}

/*
 * 関数呼び出し実行
 */
void fncCall(int fncNbr){
    int n, argCt=0;
    vector<double> vc;
    
    //実引数積み込み
    nextCode(); code=nextCode();//'('を捨て、１つめの引数or')'(引数無し)
    if(code.kind != ')'){   //引数ありの場合
        for(;;code=nextCode()){
            expression(); argCt++;//引数の値を計算し、引数個数を+1
            if(code.kind != ',') break; //次の引数がない場合、引数詰め込み終了
        }
    }
    code = nextCode(); // ')'をスキップ
    //引数詰め込み順序変更 [引数1,引数2 => [引数2,引数1
    for(n=0;n<argCt;n++){
        vc.push_back(stk.pop());
    }
    for(n=0;n<argCt;n++){
        stk.push(vc[n]);
    }
    
    fncExec(fncNbr);    //記号表番号を渡し、関数実行
}

/*
 * 関数実行
 */
void fncExec(int fncNbr){
    
    //関数入り口処理1
    int save_Pc = Pc;               //現在のプログラムカウンタを保存
    int save_baseReg = baseReg;     //現在のベースレジスタを保存
    int save_spReg = spReg;         //現在のスタックポインタを保存
    char* save_code_ptr = code_ptr; //現在のコードポインタを保存
    CodeSet save_code = code;       //現在のcodeを保存
    
    //関数入り口処理2
    Pc = Gtable[fncNbr].adrs;       //関数の開始行番号を取得
    baseReg = spReg;                //現在のスタックポインタをベースレジスタに格納
    spReg += Gtable[fncNbr].frame;  //スタックポインタをフレームサイズ分加算(baseReg--spReg間のフレームを確保)
    Dmem.auto_resize(spReg);        //主記憶の有効領域確保
    returnValue=1.0;                //返却値の初期値1.0を設定
    code = firstCode(Pc);           //先頭コードを取得
    
    //引数格納処理
    nextCode(); code=nextCode();    //'Func','('をスキップ
    if(code.kind != ')'){   //引数ありの場合
        for(;;code=nextCode()){
            set_dtTyp(code, DBL_T); //代入時に型確定
            Dmem.set(get_memAdrs(code), stk.pop()); //実引数格納
            if(code.kind != ',') break; //引数終了
        }
    }
    code = nextCode(); //')'をスキップ
    
    //関数本体処理
    ++Pc; block(); return_Flg = false;
    
    //関数出口処理
    stk.push(returnValue);  //戻り値をスタックにプッシュ
    Pc = save_Pc;
    baseReg = save_baseReg;
    spReg = save_spReg;
    code_ptr = save_code_ptr;
    code = save_code;
}

/*
 * If文に対応するend位置を返す
 */
int endline_of_If(int line){
    CodeSet cd;
    char* save = code_ptr;  //コード読み取りポインタを退避
    
    cd = firstCode(line);   //If文の行の先頭コード(If)取得(code_ptrが更新される)
    for(;;){
        line = cd.jmpAdrs;  //IfコードのjmpAdrsを取得(End,Elif,Elseのいずれかの行番号)
        cd = firstCode(line);
        if(cd.kind == Elif || cd.kind == Else) continue;
        if(cd.kind == End) break;   //End行のコードなのでこの番号を返す
    }
    code_ptr = save;    //更新されたcode_ptrを退避させておいたsaveで元に戻す
    return line;
}

/*
 * line行の先頭コードのトークン種類を返す
 */
TknKind lookCode(int line){
    return (TknKind)(unsigned char)intercode[line][0];
}



/*
 * 型を決定し、配列の場合は配列をゼロで初期化
 */
void set_dtTyp(const CodeSet& cd, char typ){
    int memAdrs = get_topAdrs(cd);  //変数の先頭アドレス(Dmemの添字)を取得(配列内容の初期化に使用)
                                    //(get_topAdrs関数でLvar,Gvar別の処理を行う)
    vector<SymTbl>::iterator p = tableP(cd);    //変数テーブルの反復子を取得
    
    if(p->dtTyp != NON_T) return;   //変数の型が決定済みの場合はなにもしないで返す
    p->dtTyp = typ;     //変数の型をtypに設定
    if(p->aryLen != 0){//配列なら内容をゼロ初期化
        for(int n=0;n< p->aryLen; n++){
            Dmem.set(memAdrs+n, 0);
        }
    }
    
}

/*
 * 変数の先頭(配列の場合もその先頭)アドレス(Dmemの添字)を返す
 */
int get_topAdrs(const CodeSet& cd){
    switch (cd.kind) {
        case Gvar:
            return tableP(cd)->adrs;
        case Lvar:
            return tableP(cd)->adrs + baseReg;
        default:
            err_exit("変数名が必要です",kind_to_s(cd));
    }
    return 0;   //ここにはこない
}

/*
 * ?式　の処理
 */
void post_if_set(bool& flg){
    if(code.kind == EofLine){ flg = true; return;}  //?無しならflgを真
    if(get_expression('?',0)) flg = true;   // 条件式で処理
}

/*
 * 数値リテラルをテーブルに登録し、テーブルの添字位置を返す
 */
int set_LITERAL(double d){
    for(int n=0; n<(int)nbrLITERAL.size();n++){
        if(nbrLITERAL[n] == d) return n;    //すでに同値の数値リテラルが登録済みの場合、同じ添字位置を返す
    }
    nbrLITERAL.push_back(d);
    return (int)nbrLITERAL.size()-1; //格納した数値リテラルの添字位置を返す
}

/*
 * 文字列リテラルをテーブルに登録し、テーブルの添字位置を返す
 */
int set_LITERAL(const string& s){
    for(int n=0; n<(int)strLITERAL.size();n++){
        if(strLITERAL[n] == s) return n;    //すでに同じ文字列リテラルが登録済みの場合、同じ添字位置を返す
    }
    strLITERAL.push_back(s);
    return (int)strLITERAL.size()-1; //格納した文字列リテラルの添字位置を返す
}

