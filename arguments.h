#pragma once

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

/*
**  Flags
*/
extern int flag_use_encryption;
extern int flag_display_help;
extern int flag_gui;
extern int flag_verbose;

/*
**  Parameter values
*/
extern char param_host[1024];
extern int 	param_port;
extern char param_user[1024];
extern char param_pass[1024];
extern char param_dir[1024];

/*
**  Public functions
*/
int parseParameters(int argc, char* argv[]);
