#pragma once

#include "operations.h"

/*
**  Public functions
*/
int op_append(const char* file, const char* local);
int op_cd(const char* dir);
int op_get(const char* file, const char* local);
int op_ls(const char* dir);
int op_mkdir(const char* dir);
int op_mv(const char* src, const char* dest);
int op_put(const char* file, const char* local);
int op_pwd();
int op_rm(const char* file);
int op_rmdir(const char* dir);
int op_quit();
