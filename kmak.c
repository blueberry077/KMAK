#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_GLOBAL_VARS   200
#define MAX_LOCAL_VARS    100
#define MAX_TASK_NAME_LEN 32
#define MAX_TASK_LINES    64
#define MAX_TASKS         64

typedef struct _KMK_VAR {
  char name[128];
  char value[256];
} KMK_Var;

typedef struct _KMK_TASK {
  char name[MAX_TASK_NAME_LEN];
  char *lines[MAX_TASK_LINES];
  int line_count;
} KMK_Task;

void usage(char *app)
{
  printf("usage: %s InputFile <Task>\n", app);
}

void trim_left(char **str);
char *start_with_word(char *line, char *word);
void ignore_comments(char *line);
char *get_variable_value(char *name);
int process_variable_substitution(char *line);
int is_task(char *task);
int parse_variable_definition(char *line);
int parse_task(char *line);
int parse_print(char *line);
int parse_cmd(char *line);
int run_task(char *name);

char *gFileContent = NULL;
KMK_Var gLocalVariables[MAX_LOCAL_VARS];
KMK_Var gGlobalVariables[MAX_GLOBAL_VARS];
KMK_Task gTasks[MAX_TASKS];
KMK_Task *gCurrentTask;
int gLocalVarsCount = 0;
int gGlobalVarsCount = 0;
int gTasksCount = 0;
int gInsideTask = 0;

int main(int argc, char **argv)
{
  if (argc < 2) {
    printf("error: No input file provided.\n");
    usage(argv[0]);
    return -1;
  }
  
  FILE *fp = fopen(argv[1], "r");
  if (!fp) {
    printf("error: Couldn't open %s\n", argv[1]);
    usage(argv[0]);
    return -1;
  }
  
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  
  gFileContent = malloc(len + 1);
  memset(gFileContent, '\0', len);
  if (!gFileContent) {
    printf("error: Not enough RAM for %s content.\n", argv[1]);
    fclose(fp);
    return -1;
  }
  fread(gFileContent, len, 1, fp);
  fclose(fp);
  
  
  char *line = strtok(gFileContent, "\r\n");
  while (line != NULL) {
    ignore_comments(line);
    if (gInsideTask) {
      if ((line[0] != ' ') && (line[0] != '\t'))
        gInsideTask = 0;
    }
    trim_left(&line);
    process_variable_substitution(line);
    if (parse_task(line)) {
      line = strtok(NULL, "\r\n");
      continue;
    } else
    if (parse_variable_definition(line)) {
    
    }
    
    if (gInsideTask && gCurrentTask) {
      gCurrentTask->lines[gCurrentTask->line_count] = line;
      gCurrentTask->line_count++;
    }
    
    line = strtok(NULL, "\r\n");
  }
  
  free(gFileContent);
  
  if (argc >= 3) {
    run_task(argv[2]);
  }
  return 0;
}

void trim_left(char **str)
{
  while (isspace(**str) && **str != '\0') {
    (*str)++;
  }
}

char *start_with_word(char *line, char *word)
{
  size_t len = strlen(word);
  if (strncmp(line, word, len))
    return NULL;
  
  return (char *)(line + len);
}

void ignore_comments(char *line)
{
  char *ptr;
  
  ptr = line;
  while (ptr = strchr(line, '#')) {
    if (ptr != line) {
      if (*(ptr-1) == '\\') {
        ptr++;
        continue;
      }
    }
    *ptr = '\0';
    return;
  }
}

char *get_variable_value(char *name)
{
  for (int i = 0; i < gGlobalVarsCount; ++i)
    if (!strcmp(gGlobalVariables[i].name, name))
      return gGlobalVariables[i].value;
  return NULL;
}

int is_task(char *task)
{
  for (int i = 0; i < gGlobalVarsCount; ++i)
    if (!strcmp(gTasks[i].name, task))
      return i;
  return -1;
}

int process_variable_substitution(char *line)
{
  int i;
  char name[128];
  char buffer[1024];
  char *src = line;
  char *dst = buffer;

  while (*src) {
    if (*src == '$' && *(src+1) == '(') {
      src += 2; // skip "$("

      i = 0;
      while (*src && *src != ')' && i < sizeof(name) - 1) {
        name[i++] = *src++;
      }
      name[i] = '\0';

      if (*src != ')') {
        printf("error: missing closing ')' in variable\n");
        return -1;
      }
      src++; // skip ')'

      char *value = get_variable_value(name);
      if (!value) {
        printf("error: undefined variable '%s'\n", name);
        return -1;
      }

      while (*value) {
        *dst++ = *value++;
      }
    } else {
      *dst++ = *src++;
    }
  }

  *dst = '\0';
  strcpy(line, buffer);
  return 0;
}


int parse_variable_definition(char *line){
  char *ptr, *name_trim_ptr;
  char *name, *value;
  
  ptr = strchr(line, '=');
  if (!ptr)
    return 0;
  
  if (gGlobalVarsCount >= MAX_GLOBAL_VARS) {
    printf("error: too many global variables.\n");
    return -1;
  }
  
  name = line;
  value = ptr + 1;
  *ptr = '\0';
  
  trim_left(&name);
  trim_left(&value);
  
  // trim right the name
  name_trim_ptr = ptr-1;
  while (isspace(*name_trim_ptr)) {
    if (name_trim_ptr <= line) {
      printf("Error: parse_variable_definition() (1)\n");
      return -1;
    }
    name_trim_ptr--;
  }
  *(name_trim_ptr + 1) = '\0';
  
  strncpy(gGlobalVariables[gGlobalVarsCount].name, name, 128);
  strncpy(gGlobalVariables[gGlobalVarsCount].value, value, 256);
  gGlobalVarsCount++;
  return 1;
}

int parse_task(char *line)
{
  char *ptr;
  char *name;
  KMK_Task *current_task = NULL;
  
  ptr = start_with_word(line, "task");
  if (!ptr)
    return 0;
  
  name = ptr;
  trim_left(&name);
  
  gCurrentTask = &gTasks[gTasksCount];
  strncpy(gCurrentTask->name, name, MAX_TASK_NAME_LEN);
  
  gInsideTask = 1;
  gTasksCount++;
  return 1;
}

int parse_print(char *line)
{
  char *ptr;
  
  ptr = start_with_word(line, "print");
  if (!ptr)
    return 0;
  
  trim_left(&ptr);
  printf("%s\n", ptr);
  return 1;
}

int run_command(const char *cmdline)
{
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  SECURITY_ATTRIBUTES sa;
  HANDLE hStdOut, hStdErr;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESTDHANDLES;

  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
  hStdErr = GetStdHandle(STD_ERROR_HANDLE);

  si.hStdOutput = hStdOut;
  si.hStdError = hStdErr;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

  if (!CreateProcessA(NULL, (LPSTR)cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
    printf("CreateProcess failed: %lu\n", GetLastError());
    return -1;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);

  DWORD exit_code = 0;
  GetExitCodeProcess(pi.hProcess, &exit_code);

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return exit_code;
}

int parse_cmd(char *line)
{
  char *ptr;
  char *command;

  ptr = start_with_word(line, "cmd");
  if (!ptr)
    return 0;

  command = ptr;
  trim_left(&command);

  if (process_variable_substitution(command) < 0) {
    printf("error: failed to process variable substitution\n");
    return -1;
  }
  printf("[CMD] %s\n", command);
  
  int res = run_command(command);
  if (res != 0) {
    printf("error: command failed with code %d\n", res);
    return -1;
  }
  return 1;
}

int run_task(char *name)
{
  int task_idx;
  KMK_Task *task;
  
  if ((task_idx = is_task(name)) < -1) {
    printf("error: task %s isn't defined.\n", name);
    return -1;
  }
  
  task = &gTasks[task_idx];
  for (int i = 0; i < task->line_count; ++i) {
    char *line = task->lines[i];
    if (parse_print(line)) {

    } else
    if (parse_cmd(line)) {
    
    }
  }
}