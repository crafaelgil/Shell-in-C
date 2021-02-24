
/*----------------------------------------------------------------------------
 *  簡易版シェル
 *--------------------------------------------------------------------------*/

/*
 *  インクルードファイル
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

/*
 *  定数の定義
 */

#define BUFLEN    1024     /* コマンド用のバッファの大きさ */
#define MAXARGNUM  256     /* 最大の引数の数 */

char *lastCommand = "";
char *promptString = "Command";

char *aliases[256][2];
int aliasIndex = 0;


/*
 *  ローカルプロトタイプ宣言
 */

int parse(char [], char *[]);
void execute_command(char *[], int, char **stack, int *index, char **history, int *historyIndex);
void cd_command(char **args);
void pushd_command(char **args, char **stack, int *index);
void print_stack(char **stack, int *index);
void popd_command(char **args, char **stack, int *index);
void dirs_command(char **args, char **stack, int *index);
void history_command(char **history, int *historyIndex);
void doubleExclamation_command(int *index, char **stack, char **history,int *historyIndex);
void exclamationString_command(int *index, char **stack, char **history, char *str, int *historyIndex);
void execute_extern_command(char **args, char **history, int *historyIndex);
void alias_command(char **args);
void unalias_command(char **args);
void mkdir_command(char **args);
void prompt_command(char **args);
char* joinArguments(char **args);
char* wildCard(char **args);
char* script(char **args);

/*----------------------------------------------------------------------------
 *
 *  関数名   : main
 *
 *  作業内容 : シェルのプロンプトを実現する
 *
 *  引数     :
 *
 *  返り値   :
 *
 *  注意     :
 *
 *--------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
  char *stack[10];
  int index = 0;
  char *history[32];
  int historyIndex = 0;
  
  char command_buffer[BUFLEN]; /* コマンド用のバッファ */
  char *args[MAXARGNUM];       /* 引数へのポインタの配列 */
  int command_status;          /* コマンドの状態を表す
                                command_status = 0 : フォアグラウンドで実行
                                command_status = 1 : バックグラウンドで実行
                                command_status = 2 : シェルの終了
                                command_status = 3 : 何もしない */
  
  /*
   *  無限にループする
   */
  
  
  for(;;) {
    
    /*
     *  プロンプトを表示する
     */
    
    printf("%s : ", promptString);
    
    /*
     *  標準入力から１行を command_buffer へ読み込む
     *  入力が何もなければ改行を出力してプロンプト表示へ戻る
     */
    
    if(fgets(command_buffer, BUFLEN, stdin) == NULL) {
      printf("\n");
      continue;
    }
    
    /*
     *  入力されたバッファ内のコマンドを解析する
     *
     *  返り値はコマンドの状態
     */
    
    command_status = parse(command_buffer, args);
    
    /*
     *  終了コマンドならばプログラムを終了
     *  引数が何もなければプロンプト表示へ戻る
     */
    
    if(command_status == 2) {
      printf("done.\n");
      exit(EXIT_SUCCESS);
    } else if(command_status == 3) {
      continue;
    }
    
    /*
     *  コマンドを実行する
     */
    
    execute_command(args, command_status, stack, &index, history, &historyIndex);
  }
  
  return 0;
}

/*----------------------------------------------------------------------------
 *
 *  関数名   : parse
 *
 *  作業内容 : バッファ内のコマンドと引数を解析する
 *
 *  引数     :
 *
 *  返り値   : コマンドの状態を表す :
 *                0 : フォアグラウンドで実行
 *                1 : バックグラウンドで実行
 *                2 : シェルの終了
 *                3 : 何もしない
 *
 *  注意     :
 *
 *--------------------------------------------------------------------------*/

int parse(char buffer[],        /* バッファ */
          char *args[])         /* 引数へのポインタ配列 */
{
  int arg_index;   /* 引数用のインデックス */
  int status;   /* コマンドの状態を表す */
  
  /*
   *  変数の初期化
   */
  
  arg_index = 0;
  status = 0;
  
  /*
   *  バッファ内の最後にある改行をヌル文字へ変更
   */
  
  *(buffer + (strlen(buffer) - 1)) = '\0';
  
  /*
   *  バッファが終了を表すコマンド（"exit"）ならば
   *  コマンドの状態を表す返り値を 2 に設定してリターンする
   */
  
  if(strcmp(buffer, "exit") == 0) {
    
    status = 2;
    return status;
  }
  
  /*
   *  バッファ内の文字がなくなるまで繰り返す
   *  （ヌル文字が出てくるまで繰り返す）
   */
  
  while(*buffer != '\0') {
    
    /*
     *  空白類（空白とタブ）をヌル文字に置き換える
     *  これによってバッファ内の各引数が分割される
     */
    
    while(*buffer == ' ' || *buffer == '\t') {
      *(buffer++) = '\0';
    }
    
    /*
     * 空白の後が終端文字であればループを抜ける
     */
    
    if(*buffer == '\0') {
      break;
    }
    
    /*
     *  空白部分は読み飛ばされたはず
     *  buffer は現在は arg_index + 1 個めの引数の先頭を指している
     *
     *  引数の先頭へのポインタを引数へのポインタ配列に格納する
     */
    
    args[arg_index] = buffer;
    ++arg_index;
    
    /*
     *  引数部分を読み飛ばす
     *  （ヌル文字でも空白類でもない場合に読み進める）
     */
    
    while((*buffer != '\0') && (*buffer != ' ') && (*buffer != '\t')) {
      ++buffer;
    }
  }
  
  /*
   *  最後の引数の次にはヌルへのポインタを格納する
   */
  
  args[arg_index] = NULL;
  
  /*
   *  最後の引数をチェックして "&" ならば
   *
   *  "&" を引数から削る
   *  コマンドの状態を表す status に 1 を設定する
   *
   *  そうでなければ status に 0 を設定する
   */
  
  if(arg_index > 0 && strcmp(args[arg_index - 1], "&") == 0) {
    
    --arg_index;
//    args[arg_index] = '\0';
    args[arg_index] = NULL;
    status = 1;
    
  } else {
    
    status = 0;
    
  }
  
  /*
   *  引数が何もなかった場合
   */
  
  if(arg_index == 0) {
    status = 3;
  }
  
  /*
   *  コマンドの状態を返す
   */
  
  return status;
}

/*----------------------------------------------------------------------------
 *
 *  関数名   : execute_command
 *
 *  作業内容 : 引数として与えられたコマンドを実行する
 *             コマンドの状態がフォアグラウンドならば、コマンドを
 *             実行している子プロセスの終了を待つ
 *             バックグラウンドならば子プロセスの終了を待たずに
 *             main 関数に返る（プロンプト表示に戻る）
 *
 *  引数     :
 *
 *  返り値   :
 *
 *  注意     :
 *
 *--------------------------------------------------------------------------*/

void execute_command(char *args[],    /* 引数の配列 */
                     int command_status, /* コマンドの状態 */
                     char **stack,
                     int *index,
                     char **history,
                     int *historyIndex)
{
  int pid;      /* プロセスＩＤ */
  //int status;   /* 子プロセスの終了ステータス */
  char *commands[] = {"cd",
                      "pushd",
                      "dirs",
                      "popd",
                      "history",
                      "!!",
                      "prompt",
                      "alias",
                      "unalias",
                      "mkdir"};
  
  /*
   *  子プロセスを生成する
   *
   *  生成できたかを確認し、失敗ならばプログラムを終了する
   */
  
  /******** Your Program ********/
  pid = fork();
  if (pid == -1) {
    printf("Error\n");
    exit(EXIT_FAILURE);
  }
  else if (pid == -1) {
    fprintf(stderr, "ERROR: can't fork a child\n");
  }
  
  /*
   *  子プロセスの場合には引数として与えられたものを実行する
   *
   *  引数の配列は以下を仮定している
   *  ・第１引数には実行されるプログラムを示す文字列が格納されている
   *  ・引数の配列はヌルポインタで終了している
   */
  
  /******** Your Program ********/
  if (pid == 0) {
    
    if (strcmp(args[0], "alias") != 0 ) {
      for (int i = 0; aliases[i][1] != NULL; i++) {
        if (strcmp(args[0], aliases[i][1]) == 0) {
          args[0] = aliases[i][0];
          break;
        }
      }
    }
   
    int i = 0;
    while (strcmp(args[0],commands[i]) != 0 && i < 10) {
      i++;
    }
    
    char *command = joinArguments(args);
    
    if (i < 10) {
      history[*historyIndex] = command;
    }
    else if (memcmp(args[0],"!",1) == 0){
      history[*historyIndex] = args[0];
      args[0]++;
    }
    else {
      i = -1;
    }
    
    (*historyIndex)++;
    if (*historyIndex > 31) {
      for (int i = 0; i < 31; i++) {
        strcpy( history[i], history[i+1]);
      }
    }
    
    char *str = args[0];
    
    switch (i) {
      case 0 : return cd_command(args);break;
      case 1: return pushd_command(args, stack, index);break;
      case 2: return dirs_command(args, stack, index);break;
      case 3: return popd_command(args, stack, index);break;
      case 4: return history_command(history, historyIndex);break;
      case 5: return doubleExclamation_command(index, stack, history,historyIndex);break;
      case 6: return prompt_command(args);break;
      case 7: return alias_command(args);break;
      case 8: return unalias_command(args);break;
      case 9: return mkdir_command(args);break;
      case 10: return exclamationString_command(index, stack, history, str, historyIndex);break;
      default: return execute_extern_command(args,history,historyIndex);break;
    }
  }
  
  /*
   *  コマンドの状態がバックグラウンドなら関数を抜ける
   */
  
  /******** Your Program ********/
  
  if (command_status == 1) {
    return;
  }
  
  /*
   *  ここにくるのはコマンドの状態がフォアグラウンドの場合
   *
   *  親プロセスの場合に子プロセスの終了を待つ
   */
  
  /******** Your Program ********/
  while (pid != wait(0))
    ;
  
  return;
}

/*-- END OF FILE -----------------------------------------------------------*/

/*--------------------------cdコマンドに対応した関数--------------------------*/
void cd_command(char **args) {
  if (args[2] != NULL) {
    printf("Too many arguments\n");
    return;
  }
  if (args[1] == NULL) {
    if ((chdir(getenv("HOME"))) == -1) {
      fprintf(stderr, "ERROR: can't change directory\n");
    }
    return;
  }
  if ((chdir(args[1])) == -1) {
    fprintf(stderr, "ERROR: can't change directory\n");
  }
  printf("current dir: %s \n", getcwd(NULL, 0));
  return;
}
/*-------------------------pushdコマンドに対応した関数-------------------------*/
void pushd_command(char **args, char **stack, int *index) {
  if (args[1] == NULL) {
    printf("Directory name is required\n");
    return;
  }
  
  if (args[2] != NULL) {
    printf("Too many arguments\n");
    return;
  }
  
  if (args[1] == NULL) {
    return;
  }
  if (*index < 10) {
    (*index)++;
  }
  else {
    fprintf(stderr, "ERROR: directory stack is full\n");
    return;
  }
  
  if ((chdir(args[1])) == -1) {
    printf("Directory does not exist\n");
    (*index)--;
  }
  else {
    stack[*index] = getcwd(NULL, 0);
    print_stack(stack,index);
  }
  return;
}
/*-----------------------print_stackコマンドに対応した関数-----------------------*/
void print_stack(char **stack, int *index) {
  for (int i = *index; i >= 1; i--)
    printf("%s ", stack[i]);
  printf("\n");
  return;
}
/*--------------------------dirsコマンドに対応した関数--------------------------*/
void dirs_command(char **args, char **stack, int *index) {
  if (args[1] != NULL)
    fprintf(stderr, "ERROR\n");
  else {
   print_stack(stack,index);
  }
  return;
}
/*--------------------------popdコマンドに対応した関数--------------------------*/
void popd_command (char **args, char **stack, int *index) {
  if (args[1] != NULL) {
    print_stack(stack,index);
    return;
  }
  if (*index != 0) {
    if ((chdir(stack[*index-1])) == -1) {
      printf("Directory does not exist\n");
      return;
    }
  }
  else {
    if ((chdir(stack[*index])) == -1) {
      printf("Directory does not exist\n");
      return;
    }
  }
  free(stack[*index]);
  stack[*index] = NULL;
  if (*index != 0) {
    (*index)--;
  }
  print_stack(stack,index);
  return;
}
/*--------------------------historyコマンドに対応した関数--------------------------*/
void history_command(char **history, int *historyIndex) {
  printf("\n");
  for (int i = 0; i < *historyIndex - 1; i++)
    printf("%s\n", history[i]);
  printf("\n");
  return;
}
/*--------------------double exclamationコマンドに対応した関数--------------------*/
void doubleExclamation_command(int *index, char **stack, char **history, int *historyIndex) {
  lastCommand = history[*historyIndex-2];
  char *args[MAXARGNUM];       /* 引数へのポインタの配列 */
  int command_status;          /* コマンドの状態を表す
                                command_status = 0 : フォアグラウンドで実行
                                command_status = 1 : バックグラウンドで実行
                                command_status = 2 : シェルの終了
                                command_status = 3 : 何もしない */
  
  printf("Command : %s\n", lastCommand);
  command_status = parse(lastCommand, args);
  if(command_status == 2) {
    printf("done.\n");
    exit(EXIT_SUCCESS);
  } else if(command_status == 3) {
    
  }
  
  execute_command(args, command_status, stack, index, history, historyIndex);
  return;
}
/*--------------------exclamation stringコマンドに対応した関数--------------------*/
void exclamationString_command(int *index, char **stack, char **history, char *str, int *historyIndex) {
  for (int i = *historyIndex-2; i>=0; i--) {
    if (memcmp(str,history[i], strlen(str)) == 0) {
      lastCommand = history[i];
      
      char *args[MAXARGNUM];       /* 引数へのポインタの配列 */
      int command_status;          /* コマンドの状態を表す
                                    command_status = 0 : フォアグラウンドで実行
                                    command_status = 1 : バックグラウンドで実行
                                    command_status = 2 : シェルの終了
                                    command_status = 3 : 何もしない */
      
      printf("Command : %s\n", lastCommand);
      command_status = parse(lastCommand, args);
      if(command_status == 2) {
        printf("done.\n");
        exit(EXIT_SUCCESS);
      } else if(command_status == 3) {
        
      }
      
      execute_command(args, command_status, stack, index, history, historyIndex);
    
      return;
    }
  }
  printf("String not found\n");
  return;
}
/*-----------------extern command executionコマンドに対応した関数----------------*/
void execute_extern_command(char **args, char **history, int *historyIndex) {
  char *str = "";
//
//  if (args[0] != NULL && args[1] == NULL) {
//    str = script(args);
//  }
//
  if (args[1] != NULL && strcmp(args[1],"*") == 0) {
    str = wildCard(args);
  }
  else if (args[1] != NULL && strcmp(args[1], "<") == 0) {
    str = script(args);
  }
  else {
    str = joinArguments(args);
  }
  
  history[*historyIndex-1] = str;

  int pid = fork();
  if (pid == 0) {
    system(str);
    free(str);
    return;
  }

  while (pid != wait(0))
    ;
//  pid_t pid;
//  int status;
//  pid = fork();
//  if (pid == 0) { //child process
//    if (execvp(args[0], args) == -1){
//      perror("child process: execution error");
//      return;
//    }
//    exit(EXIT_FAILURE);
//  }
//  else if (pid < 0) {
//    perror("forking error");
//    return;
//  }
//  else {//parent process
//    do {
//      waitpid(pid, &status, WUNTRACED);
//    }
//    while (!WIFEXITED(status) && !WIFSIGNALED(status));
//  }
}
/*-------------join arguments from command lineコマンドに対応した関数------------*/
char* joinArguments(char **args) {
  char *str = (char *)malloc(sizeof(char) * 255);
  strcpy(str, args[0]);
  
  int i = 1;
  while (args[i] != NULL) {
    strcat(str, " ");
    strcat(str, args[i]);
    i++;
  }
  strcat(str, " ");
  return str;
}
/*----------------------change promptコマンドに対応した関数---------------------*/
void prompt_command(char **args) {
  if (args[2] != NULL) {
    printf("Too many arguments\n");
    return;
  }
  
  if (args[1] != NULL) {
    char *tempstr = (char *)malloc(sizeof(char) * 255);
    strcpy(tempstr, args[1]);
    promptString = tempstr;
    return;
  }
  else {
    promptString = "Command";
    return;
  }
}
/*--------------------------aliasコマンドに対応した関数-------------------------*/
void alias_command(char **args) {
  if (args[2] == NULL) {
    printf("Need an alias string\n");
    return;
  }
  else {
    char *str1 = (char *)malloc(sizeof(char) * 255);
    char *str2 = (char *)malloc(sizeof(char) * 255);
    strcpy(str1, args[1]);
    aliases[aliasIndex][0] = str1;
    strcpy(str2, args[2]);
    aliases[aliasIndex][1] = str2;
    aliasIndex++;
    return;
  }
}
/*-------------------------unaliasコマンドに対応した関数------------------------*/
void unalias_command(char **args) {
  if (args[1] == NULL) {
    printf("Need a command string to remove alias\n");
    return;
  }
  else {
    int i = 0;
    while (strcmp(aliases[i][0], args[1]) != 0) {
      i++;
    }
    while (aliases[i][0] != NULL) {
      aliases[i][0] = aliases[i+1][0];
      aliases[i][1] = aliases[i+1][1];
      i++;
    }
    aliases[i][0] = NULL;
    aliases[i][1] = NULL;
    return;
  }
}
/*---------------------------mkdirコマンドに対応した関数--------------------------*/
void mkdir_command(char **args) {
  if (args[2] != NULL) {
    printf("Too many arguments\n");
    return;
  }
  if (mkdir(args[1], 0777) == -1) {
    perror("Could not create directory\n");
  }
  return;
}
/*-------------------------Wild card機能に対応した関数------------------------*/
char* wildCard(char **args) {
  char *listOfFiles = (char *)malloc(sizeof(char) * 255);
  DIR *d;
  struct dirent *dir;
  d = opendir(getcwd(NULL, 0));
  if (d) {
    strcat(listOfFiles, args[0]);
    while ((dir = readdir(d)) != NULL) {
      if(dir->d_type==DT_REG && strcmp(dir->d_name, ".DS_Store") != 0){
        strcat(listOfFiles, " ");
        strcat(listOfFiles, dir->d_name);
      }
    }
    closedir(d);
  }
  int i = 2;
  while (args[i] != NULL) {
    strcat(listOfFiles, " ");
    strcat(listOfFiles, args[i]);
    i++;
  }
  
  return listOfFiles;
}
/*-------------------------script機能　に対応した関数------------------------*/
char* script(char **args) {
  
  char *script = (char *)malloc(sizeof(char) * 255);
  //if (args[1] != NULL) {
  if (args[3] != NULL) {
    printf("Too many arguments\n");
    return "";
  }
  
  FILE *file;
  //file = fopen(args[0], "r");
  file = fopen(args[2], "r");
  if (file == NULL) {
    printf("Unable to open file\n");
    return "";
  }
  
  while (fgets(script, 255, file) != NULL) {
    fputs(script, file);
  }
  
  fclose(file);

  return script;
}
