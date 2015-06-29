//
//  bbi_pars.cpp
//  BBI
//  構文解析(トークン解析を実行後、内部コードに変換)
//  Created by Yoshiyuki Ohde on 2015/06/22.
//  Copyright (c) 2015年 Yoshiyuki Ohde. All rights reserved.
//

#include "bbi.h"
#include "bbi_prot.h"

#define NO_FIX_ADRS 0  //まだ番地未定のマーク
Token token;           //現在処理中のトークン
SymTbl tmpTb;    //一時格納記号表   vector<SymTbl> Gtable,Ltableに格納
int blkNest;     //ブロックの深さ
int localAdrs;   //局所変数アドレス管理
int mainTblNbr;  // main関数があれば、その記号表位置
int loopNest;    //ループネスト
bool fncDecl_F;  //関数定義処理中なら真
bool explicit_F;  //変数宣言強制(プログラムに[option "var"]記載時)
char codebuf[LIN_SIZ + 1], *codebuf_p;  //内部コード用バッファ
extern vector<char*> intercode;  //変換済み内部コード格納(bbi_code.cppで宣言)

/*
 * 初期化
 */
void init() {
  initChTyp();  //文字種表を初期化
  mainTblNbr = -1;
  blkNest = loopNest = 0;
  fncDecl_F = explicit_F = false;
  codebuf_p = codebuf;
}

/*
 * コード変換
 */
void convert_to_internalCode(char* fname) {
  init();  //文字種表などを初期化
  //関数定義名のみ先に登録
  fileOpen(fname);
  while (token = nextLine_tkn(), token.kind != EofProg) {
    if (token.kind == Func) {
      token = nextTkn();
      set_name();           // tmpTbの名前を設定
      enter(tmpTb, fncId);  //記号表に登録
    }
  }

  //内部コードへの変換
  push_intercode();  // 0行目は不要なので埋める
  fileOpen(fname);
  token = nextLine_tkn();  //次行の先頭トークンを取得
  while (token.kind != EofProg) {
    convert();  //内部コードに変換
  }

  // main関数があればその呼び出しコードを設定
  set_startPc(1);  // 1行目から実行開始
  if (mainTblNbr != -1) {
    set_startPc((int)intercode.size());  // mainから実行開始
    // main関数呼び出しコードを追記
    setCode(Fcall, mainTblNbr);
    setCode('(');
    setCode(')');
    push_intercode();
  }
}

/*
 * 先頭だけに出現するコードを処理。残り部分はconvert_rest()で処理
 */
void convert() {
  switch (token.kind) {
    case Option:  //オプションの設定
      optionSet();
      break;
    case Var:  //変数宣言
      varDecl();
      break;
    case Func:  //関数定義
      fncDecl();
      break;
    case While:
    case For:
      ++loopNest;
      convert_block_set();
      setCode_End();
      break;
    case If:
      convert_block_set();  // if
      while (token.kind == Elif) {  // elif
        convert_block_set();
      }
      if (token.kind == Else) {  // else
        convert_block_set();
      }
      setCode_End();  // end
      break;
    case Break:
      if (loopNest <= 0) {
        err_exit("不正なbreakです。");
      }
      setCode(Break);
      token = nextTkn();
      convert_rest();  //後続する記述があるため ex. break ? a==1
      break;
    case Return:
      if (!fncDecl_F) {
        err_exit("不正なreturnです");
      }
      setCode(Return);
      token = nextTkn();
      convert_rest();
      break;
    case Exit:
      setCode(Exit);
      token = nextTkn();
      convert_rest();
      break;
    case Print:
    case Println:
      setCode(token.kind);
      token = nextTkn();
      convert_rest();
      break;
    case End:
      err_exit("不正なendです");  // endが単独で使われることはない
    default:
      convert_rest();
      break;
  }
}

/*
 * ブロック処理管理
 */
void convert_block_set() {
  int patch_line;
  patch_line =
      setCode(token.kind, NO_FIX_ADRS);  //行頭のコードを設定(For,While,IF,Else,Elif)
  token = nextTkn();
  convert_rest();   //文の残りの処理
  convert_block();  //ブロック処理
  backPatch(patch_line, get_lineNo());  // NO_FIX_ADRSを修正(end行番号)
}

/*
 * 文の残りの処理
 */
void convert_rest() {
  int tblNbr;
  for (;;) {
    if (token.kind == EofLine) {  //行末の場合
      break;                      //処理終了
    }
    switch (token.kind) {  //↓これらのキーワードは文の途中で出てくることはない
      case If:
      case Elif:
      case Else:
      case For:
      case While:
      case Break:
      case Func:
      case Return:
      case Exit:
      case Print:
      case Println:
      case Option:
      case Var:
      case End:
        err_exit("不正な記述です: ", token.text);
        break;
      case Ident:  //関数呼び出しor変数
        set_name();
        if ((tblNbr = searchName(tmpTb.name, 'F')) != -1) {  //関数登録あり
          if (tmpTb.name == "main") {
            err_exit("main関数の呼び出しはできません。");
          }
          setCode(Fcall, tblNbr);
          continue;
        }
        if ((tblNbr = searchName(tmpTb.name, 'V')) == -1) {  //変数登録なし
          if (explicit_F) {  //変数宣言強制の場合
            err_exit("変数宣言が必要です: ", tmpTb.name);
          }
          tblNbr = enter(tmpTb, varId);  //自動変数登録
        }
        if (is_localName(tmpTb.name, varId)) {  //局所変数の場合
          setCode(Lvar, tblNbr);
        } else {  //大域変数の場合
          setCode(Gvar, tblNbr);
        }
        continue;
      case IntNum:
      case DblNum:  //数値定数の場合
        //数値リテラル表の添字を取得しコードセット
        setCode(token.kind, set_LITERAL(token.dblVal));
        break;
      case String:
        setCode(token.kind, set_LITERAL(token.text));
        break;
      default:  // +,-,<=など
        setCode(token.kind);
    }
    token = nextTkn();
  }
  push_intercode();
  token = nextLine_tkn();
}

/*
 * ブロックの処理
 */
void convert_block() {
  TknKind k;
  ++blkNest;
  while (k = token.kind, k != Elif && k != Else && k != End &&
                             k != EofProg) {  //ブロック終端まで文を解析
    convert();
  }
  --blkNest;
}

/*
 * 関数定義
 */
void fncDecl() {
  extern vector<SymTbl> Gtable;  //大域記号表
  int tblNbr, patch_line, fncTblNbr;
  if (blkNest > 0) {  //ブロックの中での関数定義はエラー
    err_exit("関数定義の位置が不正です。");
  }
  fncDecl_F = true;   //関数定義開始フラグ
  localAdrs = 0;      //局所領域割付けカウンタ初期化
  set_startLtable();  //局所記号表開始位置
  patch_line =
      setCode(Func, NO_FIX_ADRS);  // Funcだけコード化し、後でend行番号を入れる
  token = nextTkn();
  fncTblNbr = searchName(token.text, 'F');  //関数名からテーブルを検索
  Gtable[fncTblNbr].dtTyp = DBL_T;  //関数の戻り値はDBL_T(double型)固定

  //仮引数解析
  token = nextTkn();
  token = chk_nextTkn(token, '(');  //関数名の次は'('がくるはず
  setCode('(');
  if (token.kind != ')') {  //引数がある場合、
    for (;; token = nextTkn()) {
      set_name();                     //名前をtmpTb に登録
      tblNbr = enter(tmpTb, paraId);  //記号表に登録
      setCode(Lvar, tblNbr);  //記号表番号と共に局所変数をコード化
      ++Gtable[fncTblNbr].args;  //記号表の関数の引数個数を+1
      if (token.kind != ',') {   //後続引数がない場合
        break;                   //宣言終了
      }
      setCode(',');
    }
  }
  token = chk_nextTkn(token, ')');  //次のトークンは')'のはず
  setCode(')');
  setCode_EofLine();
  convert_block();  //ブロック処理

  backPatch(patch_line, get_lineNo());     // NO_FIX_ADRSを修正
  setCode_End();
  Gtable[fncTblNbr].frame = localAdrs;  // フレームサイズを設定
    
  if (Gtable[fncTblNbr].name == "main") {  // main関数処理
    mainTblNbr = fncTblNbr;
    if (Gtable[fncTblNbr].args != 0) {
      err_exit("main関数では仮引数を指定できません。");
    }
  }
  fncDecl_F = false;  //関数処理終了
}

/*
 * line行にnをSHORT値として設定
 */
void backPatch(int line, int n) { *SHORT_P(intercode[line] + 1) = (short)n; }

/*
 * varを使う変数宣言
 */
void varDecl() {
  setCode(Var);    //この行は非実行なのでコード変換はVarだけ
  setCode_rest();  //残りは元のまま格納
  for (;;) {
    token = nextTkn();
    var_namechk(token);  //名前検査(既に宣言済みの変数名とかぶらないかチェック)
    set_name();
    set_aryLen();             //配列なら長さも設定
    enter(tmpTb, varId);      //変数をテーブルに登録
    if (token.kind != ',') {  //変数宣言終了
      break;
    }
  }
  setCode_EofLine();
}

/*
 * 名前チェック
 */
void var_namechk(const Token& tk) {
  if (tk.kind != Ident) {
    err_exit(err_msg(tk.text, "識別子"));
  }
  if (is_localScope() && tk.text[0] == '$') {
    err_exit("関数内のvar宣言では $ 付き名前を指定できません: ", tk.text);
  }
  if (searchName(tk.text, 'V') != -1) {
    err_exit("識別子が重複しています: ", tk.text);
  }
}

/*
 * オプション設定
 */
void optionSet() {
  setCode(Option);  //この行は非実行なのでコード変換はOptionだけ
  setCode_rest();  //残りは元のまま格納
  token = nextTkn();
  if (token.kind == String && token.text == "var") {
    explicit_F = true;  //変数宣言を強制する
  } else {
    err_exit("option指定が不正です");
  }
  token = nextTkn();
  setCode_EofLine();
}

/*
 *  コード格納
 */
void setCode(int cd) { *codebuf_p++ = (char)cd; }

/*
 * コードとshort値を格納
 */
int setCode(int cd, int nbr) {
  *codebuf_p++ = (char)cd;           //コードを格納
  *SHORT_P(codebuf_p) = (short)nbr;  // SHORTとして値を格納
  codebuf_p += SHORT_SIZE;  // SHORT_SIZE分だけポインタを進める
  return get_lineNo();      // backPatch用に格納行を返す
}

/*
 * 最終格納処理
 */
void setCode_EofLine() {
  if (token.kind != EofLine) {  //トークンの種類が行終了のはず
    err_exit("不正な記述です:", token.text);
  }
  push_intercode();
  token = nextLine_tkn();  //次の行に進む
}

/*
 * 残りのテキストをそのまま格納
 */
void setCode_rest() {
  extern char* token_p;
  strcpy(codebuf_p, token_p);
  codebuf_p += strlen(token_p) + 1;
}

/*
 * endの格納処理
 */
void setCode_End() {
  if (token.kind != End) {  //トークンの種類がendのはず
    err_exit(err_msg(token.text, "end"));
  }
  setCode(End);
  token = nextTkn();
  setCode_EofLine();
}

/*
 * 変換した内部コードを格納
 */
void push_intercode() {
  int len;
  char* p;

  *codebuf_p++ = '\0';  //文字列の終端コード
  if ((len = (int)(codebuf_p - codebuf)) >= LIN_SIZ) {
    err_exit("変換後の内部コードが長すぎます。式を短くしてください。");
  }
  try {
    p = new char[len];
    memcpy(p, codebuf, len);
    intercode.push_back(p);
  } catch (bad_alloc) {
    err_exit("メモリ確保できません");
  }
  codebuf_p = codebuf;  //次の処理のために格納先先頭に位置付け
}

/*
 * tmpTbに名前を設定
 */
void set_name() {
  if (token.kind != Ident) {
    err_exit("識別子が必要です: ", token.text);
  }
  tmpTb.clear();            //クリア
  tmpTb.name = token.text;  // tokenの名前をtmpTbに設定
  token = nextTkn();        //次のTokenを取得
}

/*
 * tmpTbに配列サイズを設定
 */
void set_aryLen() {
  tmpTb.aryLen = 0;
  if (token.kind != '[') {  //配列でない
    return;
  }
  token = nextTkn();
  if (token.kind != IntNum) {
    err_exit("配列長は正の整数定数で指定してください: ", token.text);
  }
  tmpTb.aryLen = (int)token.dblVal + 1;  // var a[5]は添字0〜5が有効なので+1
  token = chk_nextTkn(nextTkn(), ']');  //次のトークンは']'のはず
  if (token.kind == '[') {
    err_exit("多次元配列は宣言できません。");
  }
}

/*
 * 関数内処理中なら真
 */
bool is_localScope() { return fncDecl_F; }
