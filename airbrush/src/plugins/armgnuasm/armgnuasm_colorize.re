/*
    armgnuasm_colorize.re
    Copyright (C) 2009 zg

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "abplugin.h"
#include "../abpairs.h"
#include "../plugins/armgnuasm/abarmgnuasm.h"

#ifdef UNICODE
typedef unsigned short UTCHAR;
#else
typedef unsigned char UTCHAR;
#endif

#define YYCTYPE unsigned long
#define YYCURSOR yycur
#define YYLIMIT yyend
#define YYMARKER yytmp
#define YYCTXMARKER yyctxtmp
#define YYFILL(n)

/*!re2c
any = [\U00000001-\U0000ffff];
O = [0-7];
D = [0-9];
L = [a-zA-Z_];
H = [a-fA-F0-9];
SIGN = [+-]?;
ESC = [\\] ([abfnrtv?'"\\] | "x" H+ | O+);
ARMCONDTHUMB = 'eq'|'ne'|'cs'|'hs'|'cc'|'lo'|'mi'|'pl'|'vs'|'vc'|'hi'|'ls'|'ge'|'lt'|'gt'|'le';
ARMCOND = ARMCONDTHUMB|'al';
THUMBREGSLO = 'r0'|'r1'|'r2'|'r3'|'r4'|'r5'|'r6'|'r7';
THUMBREGSHI = 'r8'|'r9'|'r10'|'r11'|'r12'|'r13'|'r14'|'r15'|'fp'|'ip'|'sp'|'lr'|'pc';
ARMREGSCOMMON = THUMBREGSLO|THUMBREGSHI;
ARMREGS = ARMREGSCOMMON|'cpsr' ('_'[cfsx])?;
ARMMEMORY = 'ia'|'ib'|'da'|'db'|'fd'|'ed'|'fa'|'ea';
*/

void WINAPI _export Colorize(int index,struct ColorizeParams *params)
{
  const UTCHAR *commentstart;
  const UTCHAR *line;
  int linelen,startcol;
  int lColorize=0;
  int state_data=PARSER_CLEAR;
  int *state=&state_data;
  int state_size=sizeof(state_data);
  const UTCHAR *yycur,*yyend,*yytmp=NULL,*yyctxtmp,*yytok=NULL;
  struct PairStack *hl_state=NULL;
  int hl_row; int hl_col;
  if(params->data_size>=sizeof(state_data))
  {
    state=(int *)(params->data);
    state_size=params->data_size;
  }
  Info.pGetCursor(&hl_row,&hl_col);
  for(int lno=params->startline;lno<params->endline;lno++,yytok=NULL)
  {
    startcol=(lno==params->startline)?params->startcolumn:0;
    if(((lno%Info.cachestr)==0)&&(!startcol))
      if(!Info.pAddState(params->eid,lno/Info.cachestr,state_size,(unsigned char *)state)) goto colorize_exit;
    if(lno==params->topline) lColorize=1;
    if(lColorize&&(!startcol)) Info.pDelColor(lno);
    line=(const UTCHAR*)Info.pGetLine(lno,&linelen);
    commentstart=line+startcol;
    yycur=line+startcol;
    yyend=line+linelen;
colorize_clear:
    if(yytok) if(params->callback) if(params->callback(0,lno,yytok-line,params->param)) goto colorize_exit;
    yytok=yycur;
    if(params->callback) if(params->callback(1,lno,yytok-line,params->param)) goto colorize_exit;
    if(state[0]==PARSER_COMMENT1||state[0]==PARSER_COMMENT3) goto colorize_comment1;
    if(state[0]==PARSER_STRING1||state[0]==PARSER_STRING2) goto colorize_string1;
    if(state[0]==PARSER_THUMB) goto colorize_thumb;
/*!re2c
  '.thumb' | '.thumb_func' | '.code 16'
  {
    if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_DEFINE],colors[HC_DEFINE+1]);
    state[0]=PARSER_THUMB;
    goto colorize_thumb;
  }
  "." 'h'? 'word' [ \t]+ SIGN (("0"[xX]H+)|(D+))
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_NUMBER1],colors[HC_NUMBER1+1]); goto colorize_clear; }
  '.code 32' | "." (L|D)+
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_DEFINE],colors[HC_NUMBER1+1]); goto colorize_clear; }
  "/*"
  { state[0]=PARSER_COMMENT1; commentstart=yytok; goto colorize_comment1; }
  "@"
  { commentstart=yytok; goto colorize_comment2; }
  ('add'|'sub'|'sbc'|'rsb'|'rsc'|'mul'|'mla'|'umull'|'umlal'|'smull'|'smlal') (ARMCOND)? 's'? | ('cmp'|'cmn') (ARMCOND)?
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_ARITHMETIC],colors[HC_ARM_ARITHMETIC+1]); goto colorize_clear; }
  'q' ('d')? ('add'|'sub') (ARMCOND)? | ('smul'|'smla') [wWtTbB] [tTbB] (ARMCOND) | 'smlal' [tTbB] [tTbB] (ARMCOND) |
  'clz' (ARMCOND)?
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_ARITHMETIC5],colors[HC_ARM_ARITHMETIC5+1]); goto colorize_clear; }
  ('tst'|'teq') (ARMCOND)? | ('and'|'eor'|'orr'|'bic') (ARMCOND)? 's'?
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_LOGICAL],colors[HC_ARM_LOGICAL+1]); goto colorize_clear; }
  ('mov'|'mvn') (ARMCOND)? 's'? | ('mrs'|'msr') (ARMCOND)?
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_MOVE],colors[HC_ARM_MOVE+1]); goto colorize_clear; }
  ARMREGSCOMMON [ \t]* "," [ \t]* ('lsl'|'lsr'|'asr'|'ror') [ \t]* '#' (D)+ | ARMREGSCOMMON [ \t]* "," [ \t]* 'rrx' | ARMREGSCOMMON [ \t]* "," [ \t]* ('lsl'|'lsr'|'asr'|'ror') [ \t]+ ARMREGSCOMMON
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_SHIFT],colors[HC_ARM_SHIFT+1]); goto colorize_clear; }
  ('b'|'bl'|'bx') (ARMCOND)? | 'swi' (ARMCOND)? [ \t]+ (('0x'H+)|(D+))
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_BRANCH],colors[HC_ARM_BRANCH+1]); goto colorize_clear; }
  'blx'/[ \t]+ (L|[.])(L|D)*
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_BRANCH5],colors[HC_ARM_BRANCH5+1]); goto colorize_clear; }
  'blx' (ARMCOND)? / [ \t]+ ARMREGSCOMMON
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_BRANCH5],colors[HC_ARM_BRANCH5+1]); goto colorize_clear; }
  ('ldr'|'str') (ARMCOND)? ('t'|'b'|'bt'|'sb'|'h'|'sh')? | ('ldm'|'stm') (ARMCOND)? (ARMMEMORY) | 'swp' (ARMCOND)? 'b'?
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_MEMORY],colors[HC_ARM_MEMORY+1]); goto colorize_clear; }
  'pld' | ('ldr'|'str') (ARMCOND)? 'd'
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_MEMORY5],colors[HC_ARM_MEMORY5+1]); goto colorize_clear; }
  'adr' 'l'? (ARMCOND)? | 'movl' (ARMCOND)? | ('asr'|'lsl'|'lsr'|'ror'|'rrx') (ARMCOND)? 's'? | 'push' (ARMCOND)? | 'pop' (ARMCOND)? | 'nop'
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_PSEUDO],colors[HC_ARM_PSEUDO+1]); goto colorize_clear; }
  ARMREGS
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_ARM_REGS],colors[HC_ARM_REGS+1]); goto colorize_clear; }
  L(L|D)*
  { goto colorize_clear; }
  "#" SIGN (("0"[xX]H+)|(D+))
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_NUMBER1],colors[HC_NUMBER1+1]); goto colorize_clear; }
  "-"|"+"|","|"!"
  {
    if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_KEYWORD1],colors[HC_KEYWORD1+1]);
    goto colorize_clear;
  }
  "[" {PUSH_PAIR(1)}
  "]" {POP_PAIR(1,1)}
  "{" {PUSH_PAIR(2)}
  "}" {POP_PAIR(2,2)}
  ["]
  { state[0]=PARSER_STRING1; commentstart=yytok; goto colorize_string1; }
  [ \t\v\f]+ { goto colorize_clear; }
  [\000]
  {
    if(yytok==yyend) goto colorize_end;
    goto colorize_clear;
  }
  any
  {
    goto colorize_clear;
  }
*/
colorize_comment1:
    yytok=yycur;
/*!re2c
  "*/"
  {
    if(lColorize) Info.pAddColor(lno,commentstart-line,yycur-commentstart,colors[HC_COMMENT],colors[HC_COMMENT+1]);
    state[0]=state[0]-PARSER_COMMENT1;
    goto colorize_clear;
  }
  [\000]
  { if(yytok==yyend) goto colorize_end; goto colorize_comment1; }
  any
  { goto colorize_comment1; }
*/
colorize_comment2:
    yytok=yycur;
/*!re2c
  [\000]
  {
    if(yytok==yyend)
    {
      if(lColorize) Info.pAddColor(lno,commentstart-line,yycur-commentstart,colors[HC_COMMENT],colors[HC_COMMENT+1]);
      goto colorize_end;
    }
    goto colorize_comment2;
  }
  'fixme'
  {
    if(lColorize)
    {
      Info.pAddColor(lno,commentstart-line,yytok-commentstart,colors[HC_COMMENT],colors[HC_COMMENT+1]);
      Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_FIXME],colors[HC_FIXME+1]);
      commentstart=yycur;
    }
    goto colorize_comment2;
  }
  any
  { goto colorize_comment2; }
*/
colorize_string1:
    yytok=yycur;
/*!re2c
  ESC
  { goto colorize_string1; }
  ["]
  {
    if(lColorize) Info.pAddColor(lno,commentstart-line,yycur-commentstart,colors[HC_STRING1],colors[HC_STRING1+1]);
    state[0]=state[0]-PARSER_STRING1;
    goto colorize_clear;
  }
  [\000]
  { if(yytok==yyend) goto colorize_end; goto colorize_string1; }
  any
  { goto colorize_string1; }
*/
colorize_thumb:
    yytok=yycur;
/*!re2c
  '.arm' | '.code 32'
  {
    if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_DEFINE],colors[HC_DEFINE+1]);
    state[0]=PARSER_CLEAR;
    goto colorize_clear;
  }
  "." 'h'? 'word' [ \t]+ SIGN (("0"[xX]H+)|(D+))
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_NUMBER1],colors[HC_NUMBER1+1]); goto colorize_clear; }
  '.code 16' | "." (L|D)+
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_DEFINE],colors[HC_NUMBER1+1]); goto colorize_clear; }
  "/*"
  { state[0]=PARSER_COMMENT3; commentstart=yytok; goto colorize_comment1; }
  "@"
  { commentstart=yytok; goto colorize_comment2; }
  'add' | 'adc' | 'sub' | 'sbc' | 'neg' | 'mul' | 'cmp' | 'cmn'
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_THUMB_ARITHMETIC],colors[HC_THUMB_ARITHMETIC+1]); goto colorize_clear; }
  'and' | 'eor' | 'orr' | 'bic' | 'tst'
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_THUMB_LOGICAL],colors[HC_THUMB_LOGICAL+1]); goto colorize_clear; }
  'mov' | 'mvn'
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_THUMB_MOVE],colors[HC_THUMB_MOVE+1]); goto colorize_clear; }
  'lsl' | 'lsr' | 'asr' | 'ror'
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_THUMB_SHIFT],colors[HC_THUMB_SHIFT+1]); goto colorize_clear; }
  'b' (ARMCONDTHUMB)? | 'bl' | 'bx' | 'swi' [ \t]+ (('0x'H+)|(D+))
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_THUMB_BRANCH],colors[HC_THUMB_BRANCH+1]); goto colorize_clear; }
  'blx'
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_THUMB_BRANCH5],colors[HC_THUMB_BRANCH5+1]); goto colorize_clear; }
  ('ldr'|'str') ('b'|'h'|'sh'|'sb')? | 'ldmia' | 'stmia' | 'push' | 'pop'
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_THUMB_MEMORY],colors[HC_THUMB_MEMORY+1]); goto colorize_clear; }
  'adr' 'l'? | 'movl' | 'nop'
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_THUMB_PSEUDO],colors[HC_THUMB_PSEUDO+1]); goto colorize_clear; }
  THUMBREGSLO
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_THUMB_REGS_LO],colors[HC_THUMB_REGS_LO+1]); goto colorize_clear; }
  THUMBREGSHI
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_THUMB_REGS_HI],colors[HC_THUMB_REGS_HI+1]); goto colorize_clear; }
  L(L|D)*
  { goto colorize_clear; }
  "#" SIGN (("0"[xX]H+)|(D+))
  { if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_NUMBER1],colors[HC_NUMBER1+1]); goto colorize_clear; }
  "-"|"+"|","
  {
    if(lColorize) Info.pAddColor(lno,yytok-line,yycur-yytok,colors[HC_KEYWORD1],colors[HC_KEYWORD1+1]);
    goto colorize_clear;
  }
  "[" {PUSH_PAIR(1)}
  "]" {POP_PAIR(1,1)}
  "{" {PUSH_PAIR(2)}
  "}" {POP_PAIR(2,2)}
  ["]
  { state[0]=PARSER_STRING2; commentstart=yytok; goto colorize_string1; }
  [ \t\v\f]+ { goto colorize_clear; }
  [\000]
  { if(yytok==yyend) goto colorize_end; goto colorize_clear; }
  any
  { goto colorize_clear; }
*/
colorize_end:
    if(state[0]==PARSER_COMMENT1||state[0]==PARSER_COMMENT3)
      if(lColorize) Info.pAddColor(lno,commentstart-line,yyend-commentstart,colors[HC_COMMENT],colors[HC_COMMENT+1]);
    if(state[0]==PARSER_STRING1||state[0]==PARSER_STRING2)
      if(lColorize) Info.pAddColor(lno,commentstart-line,yyend-commentstart,colors[HC_STRING1],colors[HC_STRING1+1]);
  }
colorize_exit:
  PairStackClear(params->LocalHeap,&hl_state);
}
