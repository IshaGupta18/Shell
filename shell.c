#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <fcntl.h>
#include <sys/wait.h> 
#define BUFFER_SIZE 500
// To compile the file: gcc shell.c
// To run the executable: /a.out
int get_num_pipes(char **args){
    int count=0;
    int i=0;
    while (args[i]!=NULL){
        if (strcmp(args[i],"|")==0){
            count++;
        }
        i++;
    }
    return count;
}
int get_length_arg(char *arg){
    int k=0;
    while (arg[k]!='\0'){
        k++;
    }
    return k;
}
void free_args(char **args){
    int i=0;
    while (args[i]!=NULL){
        free(args[i]);
        i++;
    }
    free(args);
}
void print_args(char **args){
    int k=0;
    while (args[k]!=NULL){
        printf("%s\t",args[k]);
        fflush(NULL);
        k++;
    }
    printf("\n");
    fflush(NULL);
}
void redirection_stdout_file(char **args,int index){
    close(1);
    creat(args[index],0666);
}
void redirection_stdout_file_1(char **args,char *filename){
    close(1);
    creat(filename,0666);
}
void redirection_stderr_file(char **args,char *filename){
    close(2);
    creat(filename,0666);
}
void redirection_stdin_file(char **args,int index){
    close(0);
    open(args[index],O_RDONLY);
}
void redirect_stderr_stdout(){
    close(2);
    dup(1);
}
void redirection_stdout_file_append(char **args,int index){
    close(1);
    open(args[index], O_WRONLY | O_APPEND| O_CREAT);
}
void execute_command(char **args){
    int k=0;
    int index=0;
    char *filename;
    char **newargs;
    int length=0;
    while (args[k]!=NULL){
        k++;
        length++;
    }
    k=0;
    newargs=malloc(sizeof(char*)*length);
    int counter=0;
    while (args[k]!=NULL){
        int len = get_length_arg(args[k]);
        if (strcmp(args[k],">>")==0){
            index=k;
            k++;
            redirection_stdout_file_append(args,index+1);
        }
        else if (strcmp(args[k],">")==0){
            index=k;
            k++;
            redirection_stdout_file(args,index+1);
        }
        else if (strcmp(args[k],"<")==0){
            index=k;
            k++;
            redirection_stdin_file(args,index+1);
        }
        else if (strcmp(args[k],"2>&1")==0){
            redirect_stderr_stdout();
        }
        else if (len>=2 & args[k][0]=='1' & args[k][1]=='>'){
            int length=0;
            int x=2;
            while (args[k][x]!='\0'){
                length++;
                x++;
            }
            filename=malloc(sizeof(char)*length);
            x=2;
            while (args[k][x]!='\0'){
                filename[x-2]=args[k][x];
                x++;
            }
            redirection_stdout_file_1(args,filename);
            free(filename);
        }
        else if (len>=2 & args[k][0]=='2' & args[k][1]=='>'){
            int length=0;
            int x=2;
            while (args[k][x]!='\0'){
                length++;
                x++;
            }
            filename=malloc(sizeof(char)*length);
            x=2;
            while (args[k][x]!='\0'){
                filename[x-2]=args[k][x];
                x++;
            }
            redirection_stderr_file(args,filename);
            free(filename);
        }
        else{
            newargs[counter]=args[k];
            counter++;
        }
        k++;
    }
    newargs[counter]=NULL;
    args=newargs;
    execvp(args[0],args);
}
void piping(char **args,int num_pipes){
    int fd[2];
    pipe(fd);
    if (num_pipes==0){
        execute_command(args);
    }
    else{
        char **temp_args;
        int count_args=0;
        while(strcmp((args)[count_args],"|")!=0){
            count_args++;
        }
        temp_args = malloc(sizeof(char*)*count_args);
        for (int i=0;i<count_args;i++){
            temp_args[i]=(args)[i];
        }
        num_pipes--;
        args = (args+count_args+1);
        temp_args[count_args]=NULL;
        int pid = fork();
        if (pid==0){
            close(fd[0]);
            close(1);
            dup(fd[1]);
            close(fd[1]);
            execute_command(temp_args);
        }
        else{
            close(fd[1]);
            close(0);
            dup(fd[0]);
            close(fd[0]);
            piping(args,num_pipes);
        }
    }
}
void read_command(int fd, char **cmd,char ***args){
    char *buf = malloc(sizeof(char)*BUFFER_SIZE);
    int ret_read = read(fd,buf,BUFFER_SIZE);
    *args = malloc(sizeof(char*)*ret_read);
    int i=0;
    int arg_count=0;
    int prev_count=0;
    while (buf[i]!='\n'){
        if (buf[i]==' '){
            char *arg=malloc(sizeof(char)*(i-prev_count));
            int j=0;
            while (prev_count<i){
                arg[j]=buf[prev_count];
                prev_count++;
                j++;
            }
            prev_count=i+1;
            (*args)[arg_count]=arg;
            arg_count++;
        }
        i++;
    }
    int k=0;
    char *last_arg=malloc(sizeof(char)*(ret_read-prev_count));
    while (buf[prev_count]!='\n'){
        last_arg[k]=buf[prev_count];
        prev_count++;
        k++;
    }
    (*args)[arg_count]=last_arg;
    arg_count++;
    (*args)[arg_count]=NULL;
    *cmd = (*args)[0];

}

int main(int argc, char **argv){
    char *cmd;
    char **args;
    while (1) {
        write(1, "$", 2);
        read_command(0, &cmd, &args);
        if (strcmp(args[0],"exit")==0){
            break;
        }
        int pid = fork();
        if (pid == 0) {
            int num_pipes = get_num_pipes(args);
            piping(args,num_pipes);
        } else if (pid > 0) {
            wait(NULL);
            free(cmd);
            free_args(args);
        } else
            printf("Failed to fork\n");
    }
    return 0;
}
