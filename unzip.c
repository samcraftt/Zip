#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Use 16-bit code words */
#define NUM_CODES 65536

// indicates the first index after 256 ASCIIs
#define AFTER_ASCII 256

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c);

/* read the next code from fd
 * return NUM_CODES on EOF
 * return the code read otherwise
 */
unsigned int read_code(int fd);

/* uncompress in_file_name to out_file_name */
void uncompress(char *in_file_name, char *out_file_name);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: unzip file\n");
        exit(1);
    }

    char *in_file_name = argv[1];
    char *out_file_name = strdup(in_file_name);
    out_file_name[strlen(in_file_name)-4] = '\0';

    uncompress(in_file_name, out_file_name);

    free(out_file_name);

    return 0;
}

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c)
{
    if (s == NULL)
    {
        return NULL;
    }

    // reminder: strlen() doesn't include the \0 in the length
    int new_size = strlen(s) + 2;
    char *result = (char *)malloc(new_size*sizeof(char));
    strcpy(result, s);
    result[new_size-2] = c;
    result[new_size-1] = '\0';

    return result;
}

/* read the next code from fd
 * return NUM_CODES on EOF
 * return the code read otherwise
 */
unsigned int read_code(int fd)
{
    // code words are 16-bit unsigned shorts in the file
    unsigned short actual_code;
    int read_return = read(fd, &actual_code, sizeof(unsigned short));
    if (read_return == 0)
    {
        return NUM_CODES;
    }
    if (read_return != sizeof(unsigned short))
    {
       perror("read");
       exit(1);
    }
    return (unsigned int)actual_code;
}

/* uncompress in_file_name to out_file_name */
void uncompress(char *in_file_name, char *out_file_name)
{
    char *dictionary[NUM_CODES];
    for (int i = 0; i < AFTER_ASCII; i++)
    {
        char *str = (char *)malloc(2*sizeof(char));
        if (str == NULL)
        {
            printf("Memory error!");
            for (int j = 0; j < i; j++)
            {
                free(dictionary[j]);
            } 
            return;
        }
        str[0] = (char)i;
        str[1] = '\0';
        dictionary[i] = str;
    }
    for (int i = AFTER_ASCII; i < NUM_CODES; i++)
    {
        dictionary[i] = NULL;
    }
    int fd_in;
    int fd_out;
    unsigned int current_code;
    unsigned int next_code;
    char current_char;
    char *current_string; 
    char *old_string;
    int index = AFTER_ASCII; 
    int str_append_flag = 0;
    fd_in = open(in_file_name, O_RDONLY);
    if (fd_in == -1)
    {
        perror(in_file_name);
        for (int i = 0; i < index; i++)
        {
            free(dictionary[i]);
        }   
        return;
    }
    fd_out = open(out_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out == -1)
    {
        perror(in_file_name);
        for (int i = 0; i < index; i++)
        {  
            free(dictionary[i]);
        }
        if (close(fd_in) < 0)
        {
            perror(in_file_name);
        }
        return;
    }
    current_code = read_code(fd_in);
    current_char = dictionary[current_code][0];
    if (write(fd_out, &current_char, 1) < 0)
    {
        perror(out_file_name);
        for (int i = 0; i < index; i++)
        {
            free(dictionary[i]);
        }
        if (close(fd_in) < 0)
        {
            perror(in_file_name);
        }
        if (close(fd_out) < 0)
        {
            perror(out_file_name);
        }
        return;
    }
    while ((next_code = read_code(fd_in)) != NUM_CODES)
    {
        if (dictionary[next_code] == NULL)
        {
            current_string = dictionary[current_code];
            current_string = strappend_char(current_string, current_char);
            if (current_string == NULL)
            {
                printf("Memory error!");
                for (int i = 0; i < index; i++)
                {
                    free(dictionary[i]);
                }
                if (close(fd_in) < 0)
                {
                    perror(in_file_name);
                }
                if (close(fd_out) < 0)
                {
                    perror(out_file_name);
                }
                return;
            }
            str_append_flag = 1;
        }
        else
        {
            current_string = dictionary[next_code];
            str_append_flag = 0;
        }
        if (write(fd_out, current_string, strlen(current_string)) < 0)
        {
            perror(out_file_name);
            for (int i = 0; i < index; i++)
            {
                free(dictionary[i]);
            }
            if (close(fd_in) < 0)
            {
                perror(in_file_name);
            }
            if (close(fd_out) < 0)
            {
                perror(out_file_name);
            }
            if (str_append_flag)
            {
                free(current_string);
            }
            return;
        }
        current_char = current_string[0];
        old_string = dictionary[current_code];
        if (index < NUM_CODES)
        {
            dictionary[index] = strappend_char(old_string, current_char);
            if (dictionary[index] == NULL)
            {
                printf("Memory error!");
                for (int i = 0; i < index; i++)
                {
                    free(dictionary[i]);
                }
                if (close(fd_in) < 0)
                {
                    perror(in_file_name);
                }
                if (close(fd_out) < 0)
                {
                    perror(out_file_name);
                }
                if (str_append_flag)
                {   
                    free(current_string);
                }   
                return;
            }
            index++;
        }
        current_code = next_code;
    } 
    for (int i = 0; i < index; i++)
    {
        free(dictionary[i]);
    }
    if (close(fd_in) < 0)
    {
        perror(in_file_name);
    }
    if (close(fd_out) < 0)
    {
        perror(out_file_name);
    }
    if (str_append_flag)
    {        
        free(current_string);
    }
    return;
}
