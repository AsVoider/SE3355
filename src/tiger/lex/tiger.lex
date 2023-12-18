%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

%x COMMENT STR IGNORE

%%

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID         P
  *   Parser::STRING
  *   Parser::INT        P
  *   Parser::COMMA      P
  *   Parser::COLON      P
  *   Parser::SEMICOLON  P
  *   Parser::LPAREN     P
  *   Parser::RPAREN     P
  *   Parser::LBRACK     P
  *   Parser::RBRACK     P
  *   Parser::LBRACE     P
  *   Parser::RBRACE     P
  *   Parser::DOT        P
  *   Parser::PLUS       P
  *   Parser::MINUS      P
  *   Parser::TIMES      P
  *   Parser::DIVIDE     P
  *   Parser::EQ         P
  *   Parser::NEQ        P
  *   Parser::LT         P
  *   Parser::LE         P
  *   Parser::GT         P
  *   Parser::GE         P
  *   Parser::AND        P
  *   Parser::OR         P
  *   Parser::ASSIGN     P
  *   Parser::ARRAY      P
  *   Parser::IF         P
  *   Parser::THEN       P
  *   Parser::ELSE       P
  *   Parser::WHILE      P
  *   Parser::FOR        P
  *   Parser::TO         P
  *   Parser::DO         P
  *   Parser::LET        P
  *   Parser::IN         P
  *   Parser::END        P
  *   Parser::OF         P
  *   Parser::BREAK      P
  *   Parser::NIL        P
  *   Parser::FUNCTION   P
  *   Parser::VAR        P
  *   Parser::TYPE       P
  */

 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}
 /* TODO: Put your lab2 code here */

"," {adjust(); return Parser::COMMA;}
":" {adjust(); return Parser::COLON;}
";" {adjust(); return Parser::SEMICOLON;}
"(" {adjust(); return Parser::LPAREN;}
")" {adjust(); return Parser::RPAREN;}
"[" {adjust(); return Parser::LBRACK;}
"]" {adjust(); return Parser::RBRACK;}
"{" {adjust(); return Parser::LBRACE;}
"}" {adjust(); return Parser::RBRACE;}
"." {adjust(); return Parser::DOT;}
"+" {adjust(); return Parser::PLUS;}
"-" {adjust(); return Parser::MINUS;}
"*" {adjust(); return Parser::TIMES;}
"/" {adjust(); return Parser::DIVIDE;}
"=" {adjust(); return Parser::EQ;}
"<>" {adjust(); return Parser::NEQ;}
"<" {adjust(); return Parser::LT;}
"<=" {adjust(); return Parser::LE;}
">" {adjust(); return Parser::GT;}
">=" {adjust(); return Parser::GE;}
"&" {adjust(); return Parser::AND;}
"|" {adjust(); return Parser::OR;}
":=" {adjust(); return Parser::ASSIGN;}
"if" {adjust(); return Parser::IF;}
"then" {adjust(); return Parser::THEN;}
"else" {adjust(); return Parser::ELSE;}
"while" {adjust(); return Parser::WHILE;}
"for" {adjust(); return Parser::FOR;}
"to" {adjust(); return Parser::TO;}
"do" {adjust(); return Parser::DO;}
"let" {adjust(); return Parser::LET;}
"in" {adjust(); return Parser::IN;}
"end" {adjust(); return Parser::END;}
"of" {adjust(); return Parser::OF;}
"break" {adjust(); return Parser::BREAK;}
"nil" {adjust(); return Parser::NIL;}
"function" {adjust(); return Parser::FUNCTION;}
"var" {adjust(); return Parser::VAR;}
"type" {adjust(); return Parser::TYPE;}

"/*" {
  adjust();
  comment_level_ = 1;
  begin(StartCondition__::COMMENT);
}

<COMMENT> {
  "*/" {
    adjust();
    comment_level_--;
    if(comment_level_ == 0)
      begin(StartCondition__::INITIAL);
  }

  "/*" {
    adjust();
    comment_level_++;
  }

  "\n" {adjust(); errormsg_->Newline();} 

  . {adjust();}
}

\" {
  adjust();
  begin(StartCondition__::STR);
}

<STR> {
  \\([ \f\n\r\t\v]+)\\ {
    adjustStr();
  }

  \" {
    adjustStr();
    begin(StartCondition__::INITIAL);
    setMatched(string_buf_);
    string_buf_.clear();
    return Parser::STRING;
  }

  \\n {
    adjustStr();
    string_buf_ += '\n';
  }

  \\t {
    adjustStr();
    string_buf_ += "\t";
  }

  \\\^[A-Z] {
    adjustStr();
    tmp = matched();
    string_buf_ += (tmp[2] - (char) 64);
  }

  \\[0-9]{3} {
    adjustStr();
    tmp = matched();
    string_buf_ += (char) atoi(tmp.substr(1, 3).c_str());
  }

  \\\" {
    adjustStr();
    string_buf_ += "\"";
  }

  \\\\ {
    adjustStr();
    string_buf_ += "\\";
  }

  . {
    adjustStr();
    string_buf_ += matched();
  }
}

[[:alpha:]][[:alnum:]_]* {adjust(); return Parser::ID;}
[[:digit:]]+ {adjust(); return Parser::INT;}

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}