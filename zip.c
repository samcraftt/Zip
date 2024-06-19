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

/* Index 256, which is the first index after the ASCII dictionary entries */
#define AFTER_ASCII 256

/* allocate space for and return a new string s+t */
char *strappend_str(char *s, char *t);

/* allocate space for and return a new string s+c */
char *strappend_char(char *s, char c);

/* look for string s in the dictionary
 * return the code if found
 * return NUM_CODES if not found 
 */
unsigned int find_encoding(char *dictionary[], char *s);

/* write the code for string s to file */
void write_code(int fd, char *dictionary[], char *s);

/* compress in_file_name to out_file_name */
void compress(char *in_file_name, char *out_file_name);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: zip file\n");
        exit(1);
    }

    char *in_file_name = argv[1];
    char *out_file_name = strappend_str(in_file_name, ".zip");

    compress(in_file_name, out_file_name);

    /* have to free the memory for out_file_name since strappend_str malloc()'ed it */
    free(out_file_name);

    return 0;
}

/* allocate space for and return a new string s+t */
char *strappend_str(char *s, char *t)
{
    if (s == NULL || t == NULL)
    {
        return NULL;
    }

    // reminder: strlen() doesn't include the \0 in the length
    int new_size = strlen(s) + strlen(t) + 1;
    char *result = (char *)malloc(new_size*sizeof(char));
    strcpy(result, s);
    strcat(result, t);

    return result;
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

/* look for string s in the dictionary
 * return the code if found
 * return NUM_CODES if not found 
 */
unsigned int find_encoding(char *dictionary[], char *s)
{
    if (dictionary == NULL || s == NULL)
    {
        return NUM_CODES;
    }

    for (unsigned int i=0; i<NUM_CODES; ++i)
    {
        /* code words are added in order, so if we get to a NULL value 
         * we can stop searching */
        if (dictionary[i] == NULL)
        {
            break;
        }

        if (strcmp(dictionary[i], s) == 0)
        {
            return i;
        }
    }
    return NUM_CODES;
}

/* write the code for string s to file */
void write_code(int fd, char *dictionary[], char *s)
{
    if (dictionary == NULL || s == NULL)
    {
        return;
    }

    unsigned int code = find_encoding(dictionary, s);
    // should never call write_code() unless s is in the dictionary 
    if (code == NUM_CODES)
    {
        printf("Algorithm error!");
        exit(1);
    }

    // cast the code to an unsigned short to only use 16 bits per code word in the output file
    unsigned short actual_code = (unsigned short)code;
    if (write(fd, &actual_code, sizeof(unsigned short)) != sizeof(unsigned short))
    {
        perror("write");
        exit(1);
    }
}

/* compress in_file_name to out_file_name */
void compress(char *in_file_name, char *out_file_name)
{
    // initialize dictionary to hold first 256 chars
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
    // set the remaining elements of dictionary to NULL
    for (int i = AFTER_ASCII; i < NUM_CODES; i++)
    {
        dictionary[i] = NULL;
    }
    // some variables used in the process of compressing "in_file_name"
    int fd_in;
    int fd_out;
    char *current_string;
    char current_char;
    char *new_string;
    int str_reassigned_flag = 0;
    ssize_t successfully_read;
    unsigned int encoding;
    int index = AFTER_ASCII;
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
        perror(out_file_name);
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
    successfully_read = read(fd_in, &current_char, 1);
    if (successfully_read == -1)
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
        if (close(fd_out) < 0)
        {
            perror(out_file_name);
        }
        return;
    }
    current_string = (char *)malloc(2*sizeof(char));
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
    current_string[0] = current_char;
    current_string[1] = '\0';
    while ((successfully_read = read(fd_in, &current_char, 1)) > 0)
    {
        new_string = strappend_char(current_string, current_char);
        if (new_string == NULL)
        {
            printf("Memory error!");
            for (int i = 0; i < index; i++)
            {
                free(dictionary[i]);
            } 
            free(current_string);
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
        encoding = find_encoding(dictionary, new_string);
        if (encoding != NUM_CODES)
        {
            free(current_string);
            current_string = new_string;
            str_reassigned_flag = 1;
        }
        else
        {   
            write_code(fd_out, dictionary, current_string);
            if (index < NUM_CODES)
            {
                dictionary[index] = new_string;
                index++;
            }
            current_string[0] = current_char;
            current_string[1] = '\0';
        }
    }
    if (successfully_read == -1)
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
        if (close(fd_out) < 0)
        {
            perror(out_file_name);
        }
        return;
    }
    write_code(fd_out, dictionary, current_string);
    for (int i = 0; i < index; i++)
    {   
        free(dictionary[i]);
    }    
    free(current_string);
    if (!str_reassigned_flag)
    {
        free(new_string);
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
