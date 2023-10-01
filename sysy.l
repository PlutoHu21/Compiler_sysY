%{
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <string>
#include "sysy_yacc.h"

using namespace std;

int line_no = 1;
#define SAVE_TOKEN  yylval.name = new string(yytext, yyleng)

int stringToDec(char *s);
int stringToOct(char *s);
int stringToHex(char *s);
%}
%option noyywrap
%x COMMENT
%x SINGLELINECOMMENT

Value   ([_a-zA-Z])([_a-zA-Z0-9])*
Oct     0[0-7]*
Dec     [1-9][0-9]*
Hex     0[Xx][0-9a-fA-F]*
white		[\t\040]
white_line  [\n]

%%


"/*"		{BEGIN(COMMENT);}
<COMMENT>"*/"  {BEGIN(INITIAL);}
<COMMENT>([^*]|\n)+|.
<COMMENT><<EOF>>   {printf("Unterminated comment\n"); return 0;}
"//" {BEGIN(SINGLELINECOMMENT);}
<SINGLELINECOMMENT>.*
<SINGLELINECOMMENT>\n {BEGIN(INITIAL);}
<SINGLELINECOMMENT><<EOF>> {BEGIN(INITIAL);}


int         {yylval.token=INT;return INT;}
void        {yylval.token=VOID;return VOID;}
const       {yylval.token=CONST;return CONST;}
break       {yylval.token=BREAK;return BREAK;}
continue    {yylval.token=CONTINUE;return CONTINUE;}
if          {yylval.token=IF;return IF;}
else        {yylval.token=ELSE;return ELSE;}
return      {yylval.token=RETURN;return RETURN;}
while       {yylval.token=WHILE;return WHILE;}
"+"			{ yylval.token = ADD; return ADD; }
"-"			{ yylval.token = SUB; return SUB; }
"*"			{ yylval.token = MUL; return MUL; }
"/"			{ yylval.token = DIV; return DIV; }
"%"			{ yylval.token = MOD; return MOD; }
"="			{ yylval.token = ASSIGN; return ASSIGN; }
"=="		{ yylval.token = EQ; return EQ; }
"!="		{ yylval.token = NE; return NE; }
"<"			{ yylval.token = LT; return LT; }
">"			{ yylval.token = GT; return GT; }
"<="		{ yylval.token = LE; return LE; }
">="		{ yylval.token = GE; return GE; }
"!"			{ yylval.token = NOT; return NOT; }
"&&"		{ yylval.token = AND; return AND; }
"||"		{ yylval.token = OR; return OR; }
"["			{ yylval.token = LSQUARE; return LSQUARE; }
"]"			{ yylval.token = RSQUARE; return RSQUARE; }
"("			{ yylval.token = LPAREN; return LPAREN; }
")"			{ yylval.token = RPAREN; return RPAREN; }
"{"			{ yylval.token = LBRACE; return LBRACE; }
"}"			{ yylval.token = RBRACE; return RBRACE; }
","			{ yylval.token = COMMA; return COMMA; }
";"			{ yylval.token = SEMI; return SEMI; }

{Dec}         { yylval.token = stringToDec(yytext); return NUMBER; }
{Oct}             { yylval.token = stringToOct(yytext); return NUMBER; }
{Hex} 	{ yylval.token = stringToHex(yytext); return NUMBER; }
{Value}     {SAVE_TOKEN; return IDENT;}

{white}+		{/* ECHO; */}
{white_line}    { line_no ++;}

%%

int stringToDec(char *s)
{
    int ans=0,len=strlen(s);
    for(int i=0;i<len;i++)ans=ans*10+(s[i]^48);
    return ans;
}

int stringToOct(char *s)
{
    int ans=0,len=strlen(s);
    for(int i=1;i<len;i++)ans=ans*8+(s[i]^48);
    return ans;
}

int stringToHex(char *s)
{
    int ans=0,len=strlen(s);
    for(int i=2;i<len;i++)
    {
        if(s[i]<'A')ans=ans*16+(s[i]^48);
        else if(s[i]<'Z')ans=ans*16+10+(s[i]-'A');
        else ans=ans*16+10+(s[i]-'a');
    }
    return ans;
}
